//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/RandomTexture.h
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
#include "Common.h"

namespace GFSDK
{
namespace SSAO
{

//--------------------------------------------------------------------------------
struct float4
{
    float x,y,z,w;
};

//--------------------------------------------------------------------------------
struct int16x4
{
    int16x4()
    {
    }
    int16x4(float4 F)
    {
        x = FP32ToInt16SNORM(F.x);
        y = FP32ToInt16SNORM(F.y);
        z = FP32ToInt16SNORM(F.z);
        w = FP32ToInt16SNORM(F.w);
    }
    INT16 FP32ToInt16SNORM(float f)
    {
        return (INT16)((1<<15) * f);
    }

    INT16 x,y,z,w;
};

//--------------------------------------------------------------------------------
class RandomTexture
{
public:
    RandomTexture();

    float4 GetJitter(UINT SliceIndex)
    {
        assert(SliceIndex < ARRAYSIZE(m_JittersFP32x4));
        return m_JittersFP32x4[SliceIndex % ARRAYSIZE(m_JittersFP32x4)];
    }

    ID3D11ShaderResourceView* GetSRV(UINT SliceIndex)
    {
        assert(SliceIndex < ARRAYSIZE(m_pSRVs));
        return m_pSRVs[SliceIndex];
    }

    void Create(ID3D11Device* pD3DDevice);
    void CreateTextureArray(ID3D11Device* pD3DDevice);
    void CreateTextureArraySRVs(ID3D11Device* pD3DDevice);
    void Release();
    float GetRandomNumber(UINT Index);

private:
    static const UINT MAX_MSAA_SAMPLE_COUNT = 8;
    static const UINT NUM_TEXELS = RANDOM_TEXTURE_WIDTH * RANDOM_TEXTURE_WIDTH * MAX_MSAA_SAMPLE_COUNT;

    ID3D11ShaderResourceView* m_pSRVs[MAX_MSAA_SAMPLE_COUNT];
    ID3D11Texture2D* m_pTexture;
    float4 m_JittersFP32x4[NUM_TEXELS];
    int16x4 m_JittersINT16x4[NUM_TEXELS];
};

} // namespace SSAO
} // namespace GFSDK
