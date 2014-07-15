//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/Types.h
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

#pragma once
#pragma pack(push,8) // Make sure we have consistent structure packings

#include <d3d11.h>

/*====================================================================================================
   Enums.
====================================================================================================*/

enum GFSDK_SSAO_Status
{
    GFSDK_SSAO_OK,                                          // Success
    GFSDK_SSAO_VERSION_MISMATCH,                            // The header version number does not match the DLL version number
    GFSDK_SSAO_NULL_ARGUMENT,                               // One of the required argument pointers is NULL
    GFSDK_SSAO_INVALID_PROJECTION_MATRIX,                   // The projection matrix is not valid (off-centered?)
    GFSDK_SSAO_INVALID_WORLD_TO_VIEW_MATRIX,                // The world-to-view matrix is not valid (transposing it may help)
    GFSDK_SSAO_INVALID_DEPTH_TEXTURE,                       // The depth texture is not valid (MSAA view-depth textures are not supported)
    GFSDK_SSAO_INVALID_NORMAL_TEXTURE_RESOLUTION,           // The normal-texture resolution does not match the depth-texture resolution
    GFSDK_SSAO_INVALID_NORMAL_TEXTURE_SAMPLE_COUNT,         // The normal-texture sample count does not match the depth-texture sample count
    GFSDK_SSAO_INVALID_OUTPUT_MSAA_SAMPLE_COUNT,            // MSAASampleCount(OutputColorRT) != MSAASampleCount(InputDepthTexture)
    GFSDK_SSAO_UNSUPPORTED_D3D_FEATURE_LEVEL,               // The current D3D11 feature level is lower than 11_0
    GFSDK_SSAO_MEMORY_ALLOCATION_FAILED,                    // Failed to allocate memory on the heap
};

enum GFSDK_SSAO_BlendMode
{
    GFSDK_SSAO_OVERWRITE_RGB,                               // Overwrite the destination RGB with the AO, preserving alpha
    GFSDK_SSAO_MULTIPLY_RGB,                                // Multiply the AO over the destination RGB, preserving alpha
};

enum GFSDK_SSAO_MSAAMode
{
    GFSDK_SSAO_PER_PIXEL_AO,                                // Render one AO value per output pixel (recommended)
    GFSDK_SSAO_PER_SAMPLE_AO,                               // Render one AO value per output MSAA sample (slower)
};

enum GFSDK_SSAO_MatrixLayout
{
    GFSDK_SSAO_ROW_MAJOR_ORDER,                             // The matrix is stored as Row[0],Row[1],Row[2],Row[3]
    GFSDK_SSAO_COLUMN_MAJOR_ORDER,                          // The matrix is stored as Col[0],Col[1],Col[2],Col[3]
};

enum GFSDK_SSAO_GPUTimer
{
    GPU_TIME_LINEAR_Z,
    GPU_TIME_DEINTERLEAVE_Z,
    GPU_TIME_RECONSTRUCT_NORMAL,
    GPU_TIME_GENERATE_AO,
    GPU_TIME_REINTERLEAVE_AO,
    GPU_TIME_BLURX,
    GPU_TIME_BLURY,
    GPU_TIME_TOTAL,
    NUM_GPU_TIMERS
};

/*====================================================================================================
   Input data.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// Input depth data.
//
// Requirements:
//    * View-space depths (linear) are required to be non-multisample.
//    * Hardware depths (non-linear) can be multisample or non-multisample.
//    * The projection matrix must have the following form, with |P23| == 1.f:
//       { P00, 0.f, 0.f, 0.f }
//       { 0.f, P11, 0.f, 0.f }
//       { 0.f, 0.f, P22, P23 }
//       { 0.f, 0.f, P32, 0.f }
//
// Remark:
//    * MetersToViewSpaceUnits is used to convert the AO radius parameter from meters to view-space units,
//      as well as to convert the blur sharpness parameter from inverse meters to inverse view-space units.
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_InputDepthData
{
    ID3D11ShaderResourceView*   pFullResDepthTextureSRV;    // Full-resolution hardware depth texture
    const FLOAT*                pProjectionMatrix;          // 4x4 perspective matrix from the depth generation pass
    GFSDK_SSAO_MatrixLayout     ProjectionMatrixLayout;     // Memory layout of the projection matrix
    FLOAT                       MetersToViewSpaceUnits;     // DistanceInViewSpaceUnits = MetersToViewSpaceUnits * DistanceInMeters

    GFSDK_SSAO_InputDepthData()
        : pFullResDepthTextureSRV(NULL)
        , pProjectionMatrix(NULL)
        , ProjectionMatrixLayout(GFSDK_SSAO_ROW_MAJOR_ORDER)
        , MetersToViewSpaceUnits(1.f)
     {
     }
};

/*====================================================================================================
   Parameters.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_BlurParameters
{
    BOOL                            Enable;                 // To blur the AO with an edge-preserving blur
    FLOAT                           Sharpness;              // The higher, the more the blur preserves edges // 0.0~16.0

    GFSDK_SSAO_BlurParameters()
        : Enable(TRUE)
        , Sharpness(4.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_OutputParameters
{
    GFSDK_SSAO_BlendMode            BlendMode;              // Blend mode used to composite the AO to the output render target
    GFSDK_SSAO_MSAAMode             MSAAMode;               // Relevant only if the input and output textures are multisample

    GFSDK_SSAO_OutputParameters()
        : BlendMode(GFSDK_SSAO_OVERWRITE_RGB)
        , MSAAMode(GFSDK_SSAO_PER_PIXEL_AO)
    {
    }
};

//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_Parameters
{
    BOOL                            UseDeinterleavedTexturing;  // To render the AO with separate draw calls per sampling pattern
    BOOL                            RandomizeSamples;           // To enable the per-pixel randomization of the sample coordinates
    FLOAT                           Radius;                     // The AO radius in meters
    FLOAT                           Bias;                       // To hide low-tessellation artifacts // 0.0~1.0
    FLOAT                           PowerExponent;              // The final AO output is pow(AO, powerExponent)
    GFSDK_SSAO_BlurParameters       Blur;                       // Optional AO blur, to blur the AO before compositing it
    GFSDK_SSAO_OutputParameters     Output;                     // To composite the AO with the output render target

    GFSDK_SSAO_Parameters()
        : UseDeinterleavedTexturing(TRUE)
        , RandomizeSamples(TRUE)
        , Radius(1.f)
        , Bias(0.1f)
        , PowerExponent(2.f)
    {
    }
};

#pragma pack(pop)
