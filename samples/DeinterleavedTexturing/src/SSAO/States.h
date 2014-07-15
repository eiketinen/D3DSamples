//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/States.h
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
class States
{
public:
    States()
        : m_pBlendState_Disabled(NULL)
        , m_pBlendState_Multiply_PreserveAlpha(NULL)
        , m_pBlendState_Disabled_PreserveAlpha(NULL)
        , m_pDepthStencilState_Disabled(NULL)
        , m_pRasterizerState_Fullscreen_NoScissor(NULL)
        , m_pSamplerState_PointClamp(NULL)
        , m_pSamplerState_LinearClamp(NULL)
        , m_pSamplerState_PointWrap(NULL)
      {
      }

    void Create(ID3D11Device* pD3DDevice);
    void Release();

    ID3D11BlendState* GetBlendStateMultiplyPreserveAlpha()
    {
        return m_pBlendState_Multiply_PreserveAlpha;
    }
    ID3D11BlendState* GetBlendStateDisabledPreserveAlpha()
    {
        return m_pBlendState_Disabled_PreserveAlpha;
    }
    ID3D11BlendState* GetBlendStateDisabled()
    {
        return m_pBlendState_Disabled;
    }
    ID3D11DepthStencilState* GetDepthStencilStateDisabled()
    {
        return m_pDepthStencilState_Disabled;
    }
    ID3D11RasterizerState* GetRasterizerStateFullscreenNoScissor()
    {
        return m_pRasterizerState_Fullscreen_NoScissor;
    }
    ID3D11SamplerState*& GetSamplerStatePointClamp()
    {
        return m_pSamplerState_PointClamp;
    }
    ID3D11SamplerState*& GetSamplerStateLinearClamp()
    {
        return m_pSamplerState_LinearClamp;
    }
    ID3D11SamplerState*& GetSamplerStatePointWrap()
    {
        return m_pSamplerState_PointWrap;
    }

private:
    void CreateBlendStates(ID3D11Device* pD3DDevice);
    void CreateDepthStencilStates(ID3D11Device* pD3DDevice);
    void CreateRasterizerStates(ID3D11Device* pD3DDevice);
    void CreateSamplerStates(ID3D11Device* pD3DDevice);

    ID3D11BlendState* m_pBlendState_Disabled;
    ID3D11BlendState* m_pBlendState_Multiply_PreserveAlpha;
    ID3D11BlendState* m_pBlendState_Disabled_PreserveAlpha;
    ID3D11DepthStencilState* m_pDepthStencilState_Disabled;
    ID3D11RasterizerState* m_pRasterizerState_Fullscreen_NoScissor;
    ID3D11SamplerState* m_pSamplerState_PointClamp;
    ID3D11SamplerState* m_pSamplerState_LinearClamp;
    ID3D11SamplerState* m_pSamplerState_PointWrap;
};

} // namespace SSAO
} // namespace GFSDK
