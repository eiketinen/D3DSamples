//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/Shaders.cpp
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

#include "Shaders.h"
#include "Shaders/Bin.h"

using namespace GFSDK;

//--------------------------------------------------------------------------------
#define CREATE_VERTEX_SHADER(X)\
    SAFE_D3D_CALL( pD3DDevice->CreateVertexShader(g_##X##_VS, sizeof(g_##X##_VS), NULL, &m_p##X##_VS) );

//--------------------------------------------------------------------------------
#define CREATE_PIXEL_SHADER(X)\
    SAFE_D3D_CALL( pD3DDevice->CreatePixelShader(g_##X##_PS, sizeof(g_##X##_PS), NULL, &m_p##X##_PS) );

//--------------------------------------------------------------------------------
#define CREATE_PIXEL_SHADER_PERMUTATION(X,Y)\
    SAFE_D3D_CALL( pD3DDevice->CreatePixelShader(g_##X##_##Y##_PS, sizeof(g_##X##_##Y##_PS), NULL, &m_p##X##_PS[PERMUTATION##_##Y]) );

//--------------------------------------------------------------------------------
#define CREATE_PIXEL_SHADER_PERMUTATION2(X,Y,Z)\
    SAFE_D3D_CALL( pD3DDevice->CreatePixelShader(g_##X##_##Y##_##Z##_PS, sizeof(g_##X##_##Y##_##Z##_PS), NULL, &m_p##X##_PS[PERMUTATION##_##Y][PERMUTATION##_##Z]) );

//--------------------------------------------------------------------------------
void SSAO::Shaders::Create(ID3D11Device* pD3DDevice)
{
    CREATE_VERTEX_SHADER(FullScreenTriangle);
    CREATE_PIXEL_SHADER(LinearizeDepthNoMSAA);
    CREATE_PIXEL_SHADER(ResolveAndLinearizeDepthMSAA);
    CREATE_PIXEL_SHADER(DeinterleaveDepth);
    CREATE_PIXEL_SHADER(ReconstructNormal);
    CREATE_PIXEL_SHADER_PERMUTATION(SeparableAO, USE_RANDOM_TEXTURE_0);
    CREATE_PIXEL_SHADER_PERMUTATION(SeparableAO, USE_RANDOM_TEXTURE_1);
    CREATE_PIXEL_SHADER_PERMUTATION2(NonSeparableAO, ENABLE_BLUR_0, USE_RANDOM_TEXTURE_0);
    CREATE_PIXEL_SHADER_PERMUTATION2(NonSeparableAO, ENABLE_BLUR_0, USE_RANDOM_TEXTURE_1);
    CREATE_PIXEL_SHADER_PERMUTATION2(NonSeparableAO, ENABLE_BLUR_1, USE_RANDOM_TEXTURE_0);
    CREATE_PIXEL_SHADER_PERMUTATION2(NonSeparableAO, ENABLE_BLUR_1, USE_RANDOM_TEXTURE_1);
    CREATE_PIXEL_SHADER_PERMUTATION(ReinterleaveAO, ENABLE_BLUR_0);
    CREATE_PIXEL_SHADER_PERMUTATION(ReinterleaveAO, ENABLE_BLUR_1);
    CREATE_PIXEL_SHADER(BlurX);
    CREATE_PIXEL_SHADER(BlurY);
}

//--------------------------------------------------------------------------------
void SSAO::Shaders::Release()
{
    SAFE_RELEASE(m_pFullScreenTriangle_VS);
    SAFE_RELEASE(m_pLinearizeDepthNoMSAA_PS);
    SAFE_RELEASE(m_pResolveAndLinearizeDepthMSAA_PS);
    SAFE_RELEASE(m_pDeinterleaveDepth_PS);
    SAFE_RELEASE(m_pReconstructNormal_PS);
    SAFE_RELEASE_ARRAY(m_pSeparableAO_PS);
    SAFE_RELEASE_ARRAY_2D(m_pNonSeparableAO_PS);
    SAFE_RELEASE_ARRAY(m_pReinterleaveAO_PS);
    SAFE_RELEASE(m_pBlurX_PS);
    SAFE_RELEASE(m_pBlurY_PS);
}
