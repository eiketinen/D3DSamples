//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/Shaders.h
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
class Shaders
{
public:
    enum EnableBlurPermutation
    {
        PERMUTATION_ENABLE_BLUR_0,
        PERMUTATION_ENABLE_BLUR_1,
        PERMUTATION_ENABLE_BLUR_COUNT
    };
    enum EnableRandPermutation
    {
        PERMUTATION_USE_RANDOM_TEXTURE_0,
        PERMUTATION_USE_RANDOM_TEXTURE_1,
        PERMUTATION_USE_RANDOM_TEXTURE_COUNT,
    };

    void Create(ID3D11Device* pD3DDevice);
    void Release();

    ID3D11PixelShader* GetBlurXPS()
    {
        return m_pBlurX_PS;
    }
    ID3D11PixelShader* GetBlurYPS()
    {
        return m_pBlurY_PS;
    }
    ID3D11VertexShader* GetFullScreenTriangleVS()
    {
        return m_pFullScreenTriangle_VS;
    }
    ID3D11PixelShader* GetLinearizeDepthNoMSAAPS()
    {
        return m_pLinearizeDepthNoMSAA_PS;
    }
    ID3D11PixelShader* GetResolveAndLinearizeDepthMSAAPS()
    {
        return m_pResolveAndLinearizeDepthMSAA_PS;
    }
    ID3D11PixelShader* GetDeinterleaveDepthPS()
    {
        return m_pDeinterleaveDepth_PS;
    }
    ID3D11PixelShader* GetSeparableAOPS(EnableRandPermutation A)
    {
        return m_pSeparableAO_PS[A];
    }
    ID3D11PixelShader* GetNonSeparableAOPS(EnableBlurPermutation A, EnableRandPermutation B)
    {
        return m_pNonSeparableAO_PS[A][B];
    }
    ID3D11PixelShader* GetReinterleaveAOPS(EnableBlurPermutation A)
    {
        return m_pReinterleaveAO_PS[A];
    }
    ID3D11PixelShader* GetReconstructNormalPS()
    {
        return m_pReconstructNormal_PS;
    }

private:
    ID3D11VertexShader* m_pFullScreenTriangle_VS;
    ID3D11PixelShader* m_pLinearizeDepthNoMSAA_PS;
    ID3D11PixelShader* m_pResolveAndLinearizeDepthMSAA_PS;
    ID3D11PixelShader* m_pDeinterleaveDepth_PS;
    ID3D11PixelShader* m_pSeparableAO_PS[PERMUTATION_USE_RANDOM_TEXTURE_COUNT];
    ID3D11PixelShader* m_pNonSeparableAO_PS[PERMUTATION_ENABLE_BLUR_COUNT][PERMUTATION_USE_RANDOM_TEXTURE_COUNT];
    ID3D11PixelShader* m_pReinterleaveAO_PS[PERMUTATION_ENABLE_BLUR_COUNT];
    ID3D11PixelShader* m_pReconstructNormal_PS;
    ID3D11PixelShader* m_pBlurX_PS;
    ID3D11PixelShader* m_pBlurY_PS;
};

} // namespace SSAO
} // namespace GFSDK
