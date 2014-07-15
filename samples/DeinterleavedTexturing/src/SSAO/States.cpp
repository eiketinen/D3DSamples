//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/States.cpp
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

#include "States.h"

using namespace GFSDK;

//--------------------------------------------------------------------------------
void SSAO::States::CreateBlendStates(ID3D11Device* pD3DDevice)
{
    //
    // Create BlendState_Disabled
    //

    D3D11_BLEND_DESC BlendStateDesc;
    BlendStateDesc.AlphaToCoverageEnable = FALSE;
    BlendStateDesc.IndependentBlendEnable = TRUE;

    for (UINT i = 0; i < ARRAYSIZE(BlendStateDesc.RenderTarget); ++i)
    {
        BlendStateDesc.RenderTarget[i].BlendEnable = FALSE;
        BlendStateDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }

    SAFE_D3D_CALL( pD3DDevice->CreateBlendState(&BlendStateDesc, &m_pBlendState_Disabled) );

    //
    // Create BlendState_Multiply_PreserveAlpha
    //

    BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
    BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
    BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

    SAFE_D3D_CALL( pD3DDevice->CreateBlendState(&BlendStateDesc, &m_pBlendState_Multiply_PreserveAlpha) );

    //
    // Create BlendState_Disabled_PreserveAlpha
    //

    BlendStateDesc.RenderTarget[0].BlendEnable = FALSE;

    SAFE_D3D_CALL( pD3DDevice->CreateBlendState(&BlendStateDesc, &m_pBlendState_Disabled_PreserveAlpha) );
}

//--------------------------------------------------------------------------------
void SSAO::States::CreateDepthStencilStates(ID3D11Device* pD3DDevice)
{
    //
    // Create DepthStencilState_Disabled
    //

    static D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = 
    {0x0, //DepthEnable
     D3D11_DEPTH_WRITE_MASK_ZERO, //DepthWriteMask
     D3D11_COMPARISON_NEVER, //DepthFunc
     0x0, //StencilEnable
     0xFF, //StencilReadMask
     0xFF, //StencilWriteMask
     
    {D3D11_STENCIL_OP_KEEP, //StencilFailOp
     D3D11_STENCIL_OP_KEEP, //StencilDepthFailOp
     D3D11_STENCIL_OP_KEEP, //StencilPassOp
     D3D11_COMPARISON_ALWAYS  //StencilFunc
    }, //FrontFace
     
    {D3D11_STENCIL_OP_KEEP, //StencilFailOp
     D3D11_STENCIL_OP_KEEP, //StencilDepthFailOp
     D3D11_STENCIL_OP_KEEP, //StencilPassOp
     D3D11_COMPARISON_ALWAYS  //StencilFunc
    }  //BackFace
    };

    SAFE_D3D_CALL( pD3DDevice->CreateDepthStencilState(&DepthStencilDesc, &m_pDepthStencilState_Disabled) );
}

//--------------------------------------------------------------------------------
void SSAO::States::CreateRasterizerStates(ID3D11Device* pD3DDevice)
{
    //
    // Create RasterizerState_Fullscreen_NoScissor
    //

    static D3D11_RASTERIZER_DESC D3D11_RASTERIZER_DESC_0 = 
    {D3D11_FILL_SOLID, //FillMode
     D3D11_CULL_BACK, //CullMode
     0x0, //FrontCounterClockwise
     0x0/*0.000000f*/, //DepthBias
                   0.f, //DepthBiasClamp
                   0.f, //SlopeScaledDepthBias
     0x1, //DepthClipEnable
     0x0, //ScissorEnable
     0x0, //MultisampleEnable
     0x0  //AntialiasedLineEnable
    };

    SAFE_D3D_CALL( pD3DDevice->CreateRasterizerState(&D3D11_RASTERIZER_DESC_0, &m_pRasterizerState_Fullscreen_NoScissor) );
}

//--------------------------------------------------------------------------------
void SSAO::States::CreateSamplerStates(ID3D11Device* pD3DDevice)
{
    //
    // Create SamplerState_PointClamp
    //

    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[0] = 0.f;
    samplerDesc.BorderColor[1] = 0.f;
    samplerDesc.BorderColor[2] = 0.f;
    samplerDesc.BorderColor[3] = 0.f;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    SAFE_D3D_CALL( pD3DDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState_PointClamp) );

    //
    // Create SamplerState_LinearClamp
    //

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;

    SAFE_D3D_CALL( pD3DDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState_LinearClamp) );

    //
    // Create SamplerState_PointWrap
    //

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;

    SAFE_D3D_CALL( pD3DDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState_PointWrap) );
}

//--------------------------------------------------------------------------------
void SSAO::States::Create(ID3D11Device* pD3DDevice)
{
    CreateBlendStates(pD3DDevice);
    CreateDepthStencilStates(pD3DDevice);
    CreateRasterizerStates(pD3DDevice);
    CreateSamplerStates(pD3DDevice);
}

//--------------------------------------------------------------------------------
void SSAO::States::Release()
{
    SAFE_RELEASE(m_pBlendState_Disabled);
    SAFE_RELEASE(m_pBlendState_Multiply_PreserveAlpha);
    SAFE_RELEASE(m_pBlendState_Disabled_PreserveAlpha);
    SAFE_RELEASE(m_pDepthStencilState_Disabled);
    SAFE_RELEASE(m_pRasterizerState_Fullscreen_NoScissor);
    SAFE_RELEASE(m_pSamplerState_PointClamp);
    SAFE_RELEASE(m_pSamplerState_LinearClamp);
    SAFE_RELEASE(m_pSamplerState_PointWrap);
}
