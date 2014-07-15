//----------------------------------------------------------------------------------
// File:        SoftShadows\media/PercentageCloserSoftShadows.fxh
// SDK Version: v1.2 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
#include "Poisson.fxh"

// PRESET is defined by the app when (re)loading the fx

#if PRESET == 0
#	define USE_POISSON
#	define SEARCH_POISSON_COUNT 25
#	define SEARCH_POISSON Poisson25
#	define PCF_POISSON_COUNT 25
#	define PCF_POISSON Poisson25
#elif PRESET == 1
#	define USE_POISSON
#	define SEARCH_POISSON_COUNT 32
#	define SEARCH_POISSON Poisson32
#	define PCF_POISSON_COUNT 64
#	define PCF_POISSON Poisson64
#elif PRESET == 2
#	define USE_POISSON
#	define SEARCH_POISSON_COUNT 100
#	define SEARCH_POISSON Poisson100
#	define PCF_POISSON_COUNT 100
#	define PCF_POISSON Poisson100
#elif PRESET == 3
#	define USE_POISSON
#	define SEARCH_POISSON_COUNT 64
#	define SEARCH_POISSON Poisson64
#	define PCF_POISSON_COUNT 128
#	define PCF_POISSON Poisson128
#else
#	define BLOCKER_SEARCH_STEP_COUNT 3
#	define PCF_FILTER_STEP_COUNT 7
#endif

#ifdef USE_POISSON
#	define BLOCKER_SEARCH_COUNT SEARCH_POISSON_COUNT
#	define PCF_COUNT PCF_POISSON_COUNT
#else
#	define BLOCKER_SEARCH_DIM (BLOCKER_SEARCH_STEP_COUNT * 2 + 1)
#	define BLOCKER_SEARCH_COUNT (BLOCKER_SEARCH_DIM * BLOCKER_SEARCH_DIM)
#	define PCF_DIM (PCF_FILTER_STEP_COUNT * 2 + 1)
#	define PCF_COUNT (PCF_DIM * PCF_DIM)
#endif

// Using similar triangles from the surface point to the area light
float2 SearchRegionRadiusUV(float zWorld)
{
	return g_lightRadiusUV * (zWorld - g_lightZNear) / zWorld;
}

// Using similar triangles between the area light, the blocking plane and the surface point
float2 PenumbraRadiusUV(float zReceiver, float zBlocker)
{
	return g_lightRadiusUV * (zReceiver - zBlocker) / zBlocker;
}

// Project UV size to the near plane of the light
float2 ProjectToLightUV(float2 sizeUV, float zWorld)
{
	return sizeUV * g_lightZNear / zWorld;
}

// Derivatives of light-space depth with respect to texture coordinates
float2 DepthGradient(float2 uv, float z)
{
	float2 dz_duv = 0;

	float3 duvdist_dx = ddx(float3(uv,z));
	float3 duvdist_dy = ddy(float3(uv,z));

	dz_duv.x = duvdist_dy.y * duvdist_dx.z;
	dz_duv.x -= duvdist_dx.y * duvdist_dy.z;

	dz_duv.y = duvdist_dx.x * duvdist_dy.z;
	dz_duv.y -= duvdist_dy.x * duvdist_dx.z;

	float det = (duvdist_dx.x * duvdist_dy.y) - (duvdist_dx.y * duvdist_dy.x);
	dz_duv /= det;

	return dz_duv;
}

float BiasedZ(float z0, float2 dz_duv, float2 offset)
{
	return z0 + dot(dz_duv, offset);
}

float ZClipToZEye(float zClip)
{
	return g_lightZFar*g_lightZNear / (g_lightZFar - zClip * (g_lightZFar - g_lightZNear));   
}

// Returns accumulated blocker depth in the search region, as well as the number of found blockers.
// Blockers are defined as shadow-map samples between the surface point and the light.
void FindBlocker(out float accumBlockerDepth, 
	out float numBlockers,
	Texture2D<float> g_shadowMap,
	float2 uv,
	float z0,
	float2 dz_duv,
	float2 searchRegionRadiusUV)
{
	accumBlockerDepth = 0;
	numBlockers = 0;

#ifdef USE_POISSON
	for (int i = 0; i < SEARCH_POISSON_COUNT; ++i)
	{
		float2 offset = SEARCH_POISSON[i] * searchRegionRadiusUV;
		float shadowMapDepth = g_shadowMap.SampleLevel(PointSampler, uv + offset, 0);
		float z = BiasedZ(z0, dz_duv, offset);
		if (shadowMapDepth < z)
		{
			accumBlockerDepth += shadowMapDepth;
			numBlockers++;
		}
	}
#else
	float2 stepUV = searchRegionRadiusUV / BLOCKER_SEARCH_STEP_COUNT;
	for(float x = -BLOCKER_SEARCH_STEP_COUNT; x <= BLOCKER_SEARCH_STEP_COUNT; ++x)
		for(float y = -BLOCKER_SEARCH_STEP_COUNT; y <= BLOCKER_SEARCH_STEP_COUNT; ++y)
		{
			float2 offset = float2(x, y) * stepUV;
			float shadowMapDepth = g_shadowMap.SampleLevel(PointSampler, uv + offset, 0);
			float z = BiasedZ(z0, dz_duv, offset);
			if (shadowMapDepth < z)
			{
				accumBlockerDepth += shadowMapDepth;
				numBlockers++;
			}
		}
#endif
}

// Performs PCF filtering on the shadow map using multiple taps in the filter region.
float PCF_Filter(float2 uv, float z0, float2 dz_duv, float2 filterRadiusUV)
{
	float sum = 0;

#ifdef USE_POISSON
	for (int i = 0; i < PCF_POISSON_COUNT; ++i)
	{
		float2 offset = PCF_POISSON[i] * filterRadiusUV;
		float z = BiasedZ(z0, dz_duv, offset);
		sum += g_shadowMap.SampleCmpLevelZero(PCF_Sampler, uv + offset, z);
	}	
#else
	float2 stepUV = filterRadiusUV / PCF_FILTER_STEP_COUNT;
	for (float x = -PCF_FILTER_STEP_COUNT; x <= PCF_FILTER_STEP_COUNT; ++x)
	{
		for (float y = -PCF_FILTER_STEP_COUNT; y <= PCF_FILTER_STEP_COUNT; ++y)
		{
			float2 offset = float2(x, y) * stepUV;
			float z = BiasedZ(z0, dz_duv, offset);
			sum += g_shadowMap.SampleCmpLevelZero(PCF_Sampler, uv + offset, z);
		}
	}
#endif

	return sum / PCF_COUNT;
}

float PCSS_Shadow(float2 uv, float z, float2 dz_duv, float zEye)
{
	// ------------------------
	// STEP 1: blocker search
	// ------------------------
	float accumBlockerDepth = 0;
	float numBlockers = 0;
	float2 searchRegionRadiusUV = SearchRegionRadiusUV(zEye);
	FindBlocker(accumBlockerDepth, numBlockers, g_shadowMap, uv, z, dz_duv, searchRegionRadiusUV);

	// Early out if not in the penumbra
	if (numBlockers == 0)
		return 1.0;
	else if (numBlockers == BLOCKER_SEARCH_COUNT)
		return 0.0;

	// ------------------------
	// STEP 2: penumbra size
	// ------------------------
	float avgBlockerDepth = accumBlockerDepth / numBlockers;
	float avgBlockerDepthWorld = ZClipToZEye(avgBlockerDepth);
	float2 penumbraRadiusUV = PenumbraRadiusUV(zEye, avgBlockerDepthWorld);
	float2 filterRadiusUV = ProjectToLightUV(penumbraRadiusUV, zEye);

	// ------------------------
	// STEP 3: filtering
	// ------------------------
	return PCF_Filter(uv, z, dz_duv, filterRadiusUV);
}

float PCF_Shadow(float2 uv, float z, float2 dz_duv, float zEye)
{
	// Do a blocker search to enable early out
	float accumBlockerDepth = 0;
	float numBlockers = 0;
	float2 searchRegionRadiusUV = SearchRegionRadiusUV(zEye);
	FindBlocker(accumBlockerDepth, numBlockers, g_shadowMap, uv, z, dz_duv, searchRegionRadiusUV);

	// Early out if no blockers
	if (numBlockers == 0)
		return 1.0;
	
	float2 filterRadiusUV = 0.1 * g_lightRadiusUV;
	return PCF_Filter(uv, z, dz_duv, filterRadiusUV);
}