//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\assets\shaders/constants.hlsl
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

////////////////////////////////////////////////////////////////////////////////
// Constant Buffers

cbuffer cbCamera : register( b0 )
{
	row_major matrix c_projection_xform;
	row_major matrix c_world_xform_new;
	row_major matrix c_world_xform_old;
	row_major matrix c_view_xform_new;
	row_major matrix c_view_xform_old;
	float3 c_eye_pos;
	float  c_half_exposure;
	float  c_half_exposure_x_framerate;
	float  c_K;
	float  c_S;
	float  c_max_sample_tap_distance;
};

cbuffer cbObject : register( b1 )
{
	row_major matrix c_model_xform_new;
	row_major matrix c_model_xform_old;
	row_major matrix c_model_xform_normal_new;
};

////////////////////////////////////////////////////////////////////////////////
// Constant and utilities

SamplerState sampPointWrap   : register(s0);
SamplerState sampPointClamp  : register(s1);
SamplerState sampLinearClamp : register(s2);

static const float EPSILON1                 =   0.01f;
static const float EPSILON2                 =   0.001f;
static const float HALF_VELOCITY_CUTOFF     =   0.25f;
static const float SOFT_Z_EXTENT            =   0.10f;
static const float CYLINDER_CORNER_1        =   0.95f;
static const float CYLINDER_CORNER_2        =   1.05f;
static const float VARIANCE_THRESHOLD       =   1.5f;
static const float WEIGHT_CORRECTION_FACTOR =  60.0f;

static const float2 VHALF = float2(0.5f, 0.5f);
static const float2 VONE  = float2(1.0f, 1.0f);
static const float2 VTWO  = float2(2.0f, 2.0f);
static const float4 GRAY  = float4(0.5f, 0.5f, 0.5f, 1.0f);

static const int2 IVZERO  = int2(0, 0);
static const int2 IVONE   = int2(1, 1);

float2 readBiasScale(float2 v)
{
	return (v * VTWO) - VONE;
}

float2 writeBiasScale(float2 v)
{
	return (v + VONE) * VHALF;
}

float2 textureSize(Texture2D tex)
{
	uint width, height;
	tex.GetDimensions(width, height);
	return float2(width, height);
}

float cone(float magDiff, float magV)
{
    return 1.0f - abs(magDiff) / magV;
}

float cylinder(float magDiff, float magV)
{
    return 1.0 - smoothstep(CYLINDER_CORNER_1 * magV,
                            CYLINDER_CORNER_2 * magV,
                            abs(magDiff));
}

float softDepthCompare(float za, float zb)
{
    return clamp((1.0 - (za - zb) / SOFT_Z_EXTENT), 0.0, 1.0);
}
