//----------------------------------------------------------------------------------
// File:        DeferredShadingMSAA\src\shaders/Lighting.hlsl
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
#include "Common.hlsli"

//--------------------------------------------------------------------------------------
// Texture Variables
//--------------------------------------------------------------------------------------
Texture2DMS<float4, SAMPLE_COUNT> texGBufferMS1 : register(t0);
Texture2DMS<float4, SAMPLE_COUNT> texGBufferMS2 : register(t1);
Texture2D texGBuffer2 : register(t2);
Texture2DMS<uint, SAMPLE_COUNT> texCoverageMS : register(t3);

SamplerState pointSampler : register( s0 );


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer LightPassBuffer : register(b0)
{
	matrix	ProjToView4x4;
	matrix	ViewToWorld4x4;
};

cbuffer MSAAPassBuffer : register(b2)
{
	int		SecondPass;
	float3	pad3;
};

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct V2P
{
	float4 pos : SV_POSITION;
	float4 posProj : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
V2P QuadVS(uint id : SV_VertexID)
{
    V2P result;
    result.uv = float2((id << 1) & 2, id & 2);
    result.pos = float4(result.uv * float2(2,-2) + float2(-1,1), 0, 1);
	result.posProj = result.pos;
    return result;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------


float3 LambertDiffuse(
    float3 lightNormal,
    float3 surfaceNormal,
    float3 lightColor,
    float3 pixelColor
    )
{
	const float INV_PI = 0.31830988618379069122f;

    // compute amount of contribution per light
    float diffuseAmount = saturate(dot(lightNormal, surfaceNormal));
    float3 diffuse = (diffuseAmount + 0.1f) * lightColor * pixelColor * INV_PI;
    return diffuse;
}

float SinTheta2(const float val) { return max(0.0f, 1.0f - val * val); }
float SinTheta(const float val) { return sqrt(SinTheta2(val)); }

float3 OrenNayarDiffuse(
    float3 lightNormal,
    float3 viewNormal,
    float3 surfaceNormal,
    float3 lightColor,
    float3 pixelColor,
	float sigma
    )
{
	const float INV_PI = 0.31830988618379069122f;

	sigma = sigma / 180.0f * 3.1415192653f;
	float sigmaSqr = sigma * sigma;
	float A = 1.0f - (sigmaSqr / (2.0f * (sigmaSqr + 0.33f)));
	float B = 0.45f * sigmaSqr / (sigmaSqr + 0.09f);
	
	float fDotLt = saturate(dot(lightNormal, surfaceNormal));
	float fDotVw = saturate(dot(viewNormal, surfaceNormal));
	
	float fSinThetaI = SinTheta(fDotLt);
	float fSinThetaO = SinTheta(fDotVw);

	float fMaxCos = saturate(dot(normalize(lightNormal - surfaceNormal * fDotLt), normalize(viewNormal - surfaceNormal * fDotVw)));

	float fSinAlpha, fTanBeta;
	if(abs(fDotLt) > abs(fDotVw))
	{
		fSinAlpha = fSinThetaO;
		fTanBeta = fSinThetaI / clamp(abs(fDotLt), 0.01f, 1.0f);
	}
	else
	{
		fSinAlpha = fSinThetaI;
		fTanBeta = fSinThetaO / clamp(abs(fDotVw), 0.01f, 1.0f);
	}

    return pixelColor * lightColor * INV_PI * (A + B * fMaxCos * fSinAlpha * fTanBeta) * (fDotLt + 0.1f);
}

float3 BlinnPhoneSpecular(
    float3 toEye,
    float3 lightNormal,
    float3 surfaceNormal,
    float3 materialSpecularColor,
    float materialSpecularPower,
    float lightSpecularIntensity,
    float3 lightColor
    )
{
    // compute specular contribution
    float3 vHalf = normalize(lightNormal + toEye);
    float specularAmount = saturate(dot(surfaceNormal, vHalf));
    specularAmount = pow(specularAmount, max(materialSpecularPower,0.0001f)) * lightSpecularIntensity;
    float3 specular = materialSpecularColor * lightColor * specularAmount;
    
    return specular;
}

float3 DepthToPosition(float iDepth, float4 iPosProj, matrix mProjInv, float fFarClip)
{
	float3 vPosView = mul(iPosProj, mProjInv).xyz;
	float3 vViewRay = float3(vPosView.xy * (fFarClip / vPosView.z), fFarClip);
	float3 vPosition = vViewRay * iDepth;

	return vPosition;
}

float2 ProjToScreen(float4 iCoord)
{
	float2 oCoord = iCoord.xy / iCoord.w;
	return 0.5f * (float2(oCoord.x, -oCoord.y) + 1);
}

float4 ScreenToProj(float2 iCoord)
{
	return float4(2.0f * float2(iCoord.x, 1.0f - iCoord.y) - 1, 0.0f, 1.0f);
}

void PerformFragmentAnalysis4(in uint4 inputCoverage[1], out uint sampleCount, out uint fragmentCount, out uint4 sampleWeights[1])
{
	uint4 coverage = inputCoverage[0];

	sampleWeights[0] = uint4( 1, 1, 1, 1 );

	// Kill the same primitive IDs in the pixel
	if( coverage.x == coverage.y ) { coverage.y = 0; sampleWeights[0].x += 1; sampleWeights[0].y = 0; }
	if( coverage.x == coverage.z ) { coverage.z = 0; sampleWeights[0].x += 1; sampleWeights[0].z = 0; }
	if( coverage.x == coverage.w ) { coverage.w = 0; sampleWeights[0].x += 1; sampleWeights[0].w = 0; }
	if( coverage.y == coverage.z ) { coverage.z = 0; sampleWeights[0].y += 1; sampleWeights[0].z = 0; }
	if( coverage.y == coverage.w ) { coverage.w = 0; sampleWeights[0].y += 1; sampleWeights[0].w = 0; }
	if( coverage.z == coverage.w ) { coverage.w = 0; sampleWeights[0].z += 1; sampleWeights[0].w = 0; }

	sampleWeights[0].x = coverage.x > 0 ? sampleWeights[0].x : 0;
	sampleWeights[0].y = coverage.y > 0 ? sampleWeights[0].y : 0;
	sampleWeights[0].z = coverage.z > 0 ? sampleWeights[0].z : 0;
	sampleWeights[0].w = coverage.w > 0 ? sampleWeights[0].w : 0;

	sampleCount = (sampleWeights[0].x > 0 ? 1 : 0) + (sampleWeights[0].y > 0 ? 1 : 0) + (sampleWeights[0].z > 0 ? 1 : 0) + (sampleWeights[0].w > 0 ? 1 : 0);
	fragmentCount = sampleWeights[0].x + sampleWeights[0].y + sampleWeights[0].z + sampleWeights[0].w;
}


float4 PS(V2P pixel) : SV_Target
{
	// Complex pixel detection
	bool bComplexPixel = false;
	if(SeperateEdgePass)
	{
		if(SecondPass)
			bComplexPixel = true;
	}
	else
	{
		if(UseDiscontinuity)
		{
			float4 gBuf1 = texGBufferMS1.Load(pixel.pos.xy, 0);
			float4 gBuf2 = texGBufferMS2.Load(pixel.pos.xy, 0);
		
			float3 normal = gBuf1.xyz;
			float depth = gBuf1.w;
			float3 albedo = gBuf2.xyz;

			[unroll]
			for(int i = 1; i < SAMPLE_COUNT; i++)
			{
				float4 nextGBuf1 = texGBufferMS1.Load(pixel.pos.xy, i);
				float4 nextGBuf2 = texGBufferMS2.Load(pixel.pos.xy, i);
				
				float3 nextNormal = nextGBuf1.xyz;
				float nextDepth = nextGBuf1.w;
				float3 nextAlbedo = nextGBuf2.xyz;

				[branch]
				if(abs(depth - nextDepth) > 0.1f || abs(dot(abs(normal - nextNormal), float3(1, 1, 1))) > 0.1f || abs(dot(albedo - nextAlbedo, float3(1, 1, 1))) > 0.1f)
				{
					bComplexPixel = true;
					break;
				}
			}
		}
		else
			bComplexPixel = texGBuffer2.Sample(pointSampler, pixel.uv).a > 0.0f;
	}

	float3 diffuse = 0;
	float3 spec = 0;
	int sampleCount;
	int fragmentCount;
	
	if(!bComplexPixel)
	{
		sampleCount = 1;
		fragmentCount = 1;

		// Read G-Buffer
		float4 gBuf = texGBufferMS1.Load(pixel.pos.xy, 0);
		float3 mtlColor = texGBufferMS2.Load(pixel.pos.xy, 0).rgb;
		// Get the normal
		float3 worldNormal = gBuf.xyz;
		// Calculate position
		float  viewLinearDepth = gBuf.w;
		float3 viewPos = DepthToPosition(viewLinearDepth, pixel.posProj, ProjToView4x4, 1000.0f);
		float3 worldPos = mul(float4(viewPos, 1.0f), ViewToWorld4x4).xyz;
		// Calculate to eye vector
		float3 eyeWorldPos = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), ViewToWorld4x4).xyz;
		float3 toEyeVector = normalize(eyeWorldPos - worldPos);
		
		if(UseOrenNayar)
			diffuse += length(worldNormal) > 0.0f ? OrenNayarDiffuse(normalize(float3(-1, 1, 1)), toEyeVector, worldNormal, float3(3.0f, 3.0f, 3.0f), mtlColor, 15.0f) : float3(0.2f, 0.5f, 1.0f);
		else
			diffuse += length(worldNormal) > 0.0f ? LambertDiffuse(normalize(float3(-1, 1, 1)), worldNormal, float3(3.0f, 3.0f, 3.0f), mtlColor) : float3(0.2f, 0.5f, 1.0f);
	}
	else
	{
		// Coverage analysis
		// Compute coverage matrix
		sampleCount = SAMPLE_COUNT;
		fragmentCount = SAMPLE_COUNT;

		uint4 coverage[(SAMPLE_COUNT - 1) / SAMPLE_COUNT + 1];
		uint4 sampleWeights[(SAMPLE_COUNT - 1) / SAMPLE_COUNT + 1] = { uint4(1, 1, 1, 1) };

		if(AdaptiveShading)
		{
			[unroll]
			for( int i = 0; i < SAMPLE_COUNT; i ++ )
			{
				coverage[i / SAMPLE_COUNT][i % SAMPLE_COUNT] = texCoverageMS.Load( pixel.pos.xy, i );
			}

			PerformFragmentAnalysis4(coverage, sampleCount, fragmentCount, sampleWeights);
		}

		int sampleIdx = 0;
		[unroll]
		for(int i = 0; i < sampleCount; i++)
		{
			
			if(AdaptiveShading)
			{
				while(sampleWeights[sampleIdx / SAMPLE_COUNT][sampleIdx % SAMPLE_COUNT] == 0)
				{
					sampleIdx++;
				}
			}

			// Read G-Buffer
			float4 gBuf = texGBufferMS1.Load(pixel.pos.xy, sampleIdx);
			float3 mtlColor = texGBufferMS2.Load(pixel.pos.xy, sampleIdx).rgb;
			// Get the normal
			float3 worldNormal = gBuf.xyz;
			// Calculate position
			float  viewLinearDepth = gBuf.w;
			float3 viewPos = DepthToPosition(viewLinearDepth, pixel.posProj, ProjToView4x4, 1000.0f);
			float3 worldPos = mul(float4(viewPos, 1.0f), ViewToWorld4x4).xyz;
			// Calculate to eye vector
			float3 eyeWorldPos = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), ViewToWorld4x4).xyz;
			float3 toEyeVector = normalize(eyeWorldPos - worldPos);
		
			if(UseOrenNayar)
				diffuse += (length(worldNormal) > 0.0f ? OrenNayarDiffuse(normalize(float3(-1, 1, 1)), toEyeVector, worldNormal, float3(3.0f, 3.0f, 3.0f), mtlColor, 15.0f) : float3(0.2f, 0.5f, 1.0f)) * sampleWeights[sampleIdx / 4][sampleIdx % 4] ;
			else
				diffuse += (length(worldNormal) > 0.0f ? LambertDiffuse(normalize(float3(-1, 1, 1)), worldNormal, float3(3.0f, 3.0f, 3.0f), mtlColor) : float3(0.2f, 0.5f, 1.0f)) * sampleWeights[sampleIdx / 4][sampleIdx % 4];
			
			sampleIdx++;
			//spec += 1.0 - exp2(-BlinnPhoneSpecular(toEyeVector, normalize(float3(1,1,0)), worldNormal, float3(1.0f, 1.0f, 1.0f),15.0f, 100.0f, float3(0.5f, 0.5f, 0.5f))); // Linearly resolve high light
		}
	}
	
	if(bComplexPixel && ShowComplexPixels)
	{
		if(sampleCount == 2)
			return float4(0.0f, 1.0f, 0.0f, 1.0f);
		if(sampleCount == 3)
			return float4(0.0f, 0.0f, 1.0f, 1.0f);
		if(sampleCount == 4)
			return float4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	
	return float4((diffuse + spec) / fragmentCount, 1.0f);
}

float4 PS_PerSample(V2P		pixel,
					uint	i : SV_SampleIndex) : SV_Target
{
	float3 diffuse = 0;
	float3 spec = 0;
	
	// Read G-Buffer
	float4 gBuf = texGBufferMS1.Load(pixel.pos.xy, i);
	float3 mtlColor = texGBufferMS2.Load(pixel.pos.xy, i).rgb;
	// Get the normal
	float3 worldNormal = gBuf.xyz;
	// Calculate position
	float  viewLinearDepth = gBuf.w;
	float3 viewPos = DepthToPosition(viewLinearDepth, pixel.posProj, ProjToView4x4, 1000.0f);
	float3 worldPos = mul(float4(viewPos, 1.0f), ViewToWorld4x4).xyz;
	// Calculate to eye vector
	float3 eyeWorldPos = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), ViewToWorld4x4).xyz;
	float3 toEyeVector = normalize(eyeWorldPos - worldPos);
		
	if(UseOrenNayar)
		diffuse += length(worldNormal) > 0.0f ? OrenNayarDiffuse(normalize(float3(-1, 1, 1)), toEyeVector, worldNormal, float3(3.0f, 3.0f, 3.0f), mtlColor, 15.0f) : float3(0.2f, 0.5f, 1.0f);
	else
		diffuse += length(worldNormal) > 0.0f ? LambertDiffuse(normalize(float3(-1, 1, 1)), worldNormal, float3(3.0f, 3.0f, 3.0f), mtlColor) : float3(0.2f, 0.5f, 1.0f);
	
	if(ShowComplexPixels)
		return float4(1.0f, 0.0f, 0.0f, 1.0f);

	return float4(diffuse + spec, 1.0f);
}