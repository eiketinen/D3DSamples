//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\assets\shaders/ps_gather.hlsl
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

#include "constants.hlsl"

////////////////////////////////////////////////////////////////////////////////
// Resources

Texture2D texColor       : register(t0);
Texture2D texDepth       : register(t1);
Texture2D texVelocity    : register(t2);
Texture2D texNeighborMax : register(t3);
Texture2D texRandom      : register(t4);

////////////////////////////////////////////////////////////////////////////////
// IO Structures

struct VS_OUTPUT
{
	float4 P  : SV_POSITION;
	float2 TC : TEXCOORD0;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader

float getDepth(float2 UV)
{
	// Negative since the paper says depth are negative
	return -(texDepth.SampleLevel(sampPointClamp, UV, 0).r);
}

float pseudoRandom(float2 UV)
{
	// Scale up the texcoord and wrap to tile the random numbers across the target
	return texRandom.SampleLevel(sampPointWrap, UV * c_K, 0).r - 0.5f;
}

float4 main(VS_OUTPUT input) : SV_Target0
{
	float2 texDim = textureSize(texColor);

	// X (position of current fragment)
	float2 X = input.TC;

	// Sample from the color buffer at X
	float4 CX = texColor.SampleLevel(sampLinearClamp, X, 0);

	// Sample from the NeighborMax buffer at X; holds dominant half-velocity for the current fragment's neighborhood tile
	float2 NX = readBiasScale(texNeighborMax.Sample(sampPointClamp, X).xy);
	float NXLength = length(NX);

	// Weighting, correcting, and clamping half-velocity
	float TempNX = NXLength * c_half_exposure;
	bool FlagNX = (TempNX >= EPSILON1);
	TempNX = clamp(TempNX, 0.1f, c_K);

	// If the velocities are too short, we simply show the color texel and exit
	if (TempNX < HALF_VELOCITY_CUTOFF)
	{
		return CX;
	}

	// Weighting, correcting, and clamping half-velocity
	if (FlagNX)
	{
		NX *= (TempNX / NXLength);
		NXLength = length(NX);
	}

	// Sample from the primary half-velocity buffer at X
	float2 VX = readBiasScale(texVelocity.Sample(sampPointClamp, X).xy);
	float VXLength = length(VX);

	// Weighting, correcting, and clamping half-velocity
	float TempVX = VXLength * c_half_exposure;
	bool FlagVX = (TempVX >= EPSILON1);
	TempVX = clamp(TempVX, 0.1f, c_K);
	if (FlagVX)
	{
		VX *= (TempVX / VXLength);
		VXLength = length(VX);
	}

	// Random value in [-0.5, 0.5]
	float R = pseudoRandom(X);

	// Sample from the depth buffer at X
	float ZX = getDepth(X);

	// If VX (for current fragment) is too small, then we use NX (for current tile)
	float2 CorrectedVX = (VXLength < VARIANCE_THRESHOLD) ? normalize(NX) : normalize(VX);

	// Weight value (suggested by the article authors' implementation)
	float Weight = c_S / WEIGHT_CORRECTION_FACTOR / TempVX;

	// Cumulative sum (initialized to the current fragment, since we skip it in the loop)
	float3 Sum = CX.xyz * Weight;

	// Index for same fragment
	int SelfIndex = (c_S - 1) / 2;

	// Adjusted constants for texture size
	float max_sample_tap_distance = c_max_sample_tap_distance / texDim.x;
	float2 half_texel = VHALF / texDim.x;

	// Iterate once for each reconstruction sample tap
	for (int i = 0; i < c_S; ++i)
	{
		// Skip the same fragment
		if (i == SelfIndex) { continue; }

		// T (distance between current fragment and sample tap)
		// NOTE: we are not sampling adjacent ones; we are extending our taps
		//       a little further
		float lerp_amount = (float(i) + R + 1.0f) / (c_S + 1.0f);
		float T = lerp(-max_sample_tap_distance, max_sample_tap_distance, lerp_amount);

		// The authors' implementation suggests alternating between the
		// corrected velocity and the neighborhood's
		float2 Switch = ((i & 1) == 1) ? CorrectedVX : NX;

		// Y (position of current sample tap)
		float2 Y = float2(X + float2(Switch * T + half_texel));

		// Sample from the primary half-velocity buffer at Y
		float2 VY = readBiasScale(texVelocity.SampleLevel(sampPointClamp, Y, 0).xy);
		float VYLength = length(VY);

		// Weighting, correcting and clamping half-velocity
		float TempVY = VYLength * c_half_exposure;
		bool FlagVY = (TempVY >= EPSILON1);
		TempVY = clamp(TempVY, 0.1f, c_K);
		if (FlagVY)
		{
			VY *= (TempVY / VYLength);
			VYLength = length(VY);
		}

		// Sample from the depth buffer at Y
		float ZY = getDepth(Y);

		// alpha = foreground contribution + background contribution + 
		//         blur of both foreground and background
		float alphaY = softDepthCompare(ZX, ZY) * cone(T, TempVY) +
					   softDepthCompare(ZY, ZX) * cone(T, TempVX) +
					   cylinder(T, TempVY) * cylinder(T, TempVX) * 2.0;

		// Applying to weight and weighted sum
		Weight += alphaY;
		Sum += (alphaY * texColor.SampleLevel(sampLinearClamp, Y, 0).xyz);
	}

	return float4(Sum / Weight, 1);
}
