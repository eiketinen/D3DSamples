//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/AppState.h
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
class AppState
{
public:
    void Save(ID3D11DeviceContext* pDeviceContext);
    void Restore(ID3D11DeviceContext* pDeviceContext);

    static void UnbindSRVs(ID3D11DeviceContext* pDeviceContext)
    {
        ID3D11ShaderResourceView* pNullSRVs[NumPSShaderResourceViews] = { NULL };
        pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(pNullSRVs), pNullSRVs);
    }

private:
    static const int NumPSShaderResourceViews = 2;
    static const int NumPSSamplers = 2;
    static const int NumPSConstantBuffers = 4;
    static const int NumRenderTargetViews = 8;

    D3D11_VIEWPORT Viewport;
    D3D11_PRIMITIVE_TOPOLOGY Topology;
    ID3D11InputLayout* pInputLayout;
    ID3D11RasterizerState* pRasterizerState;

    ID3D11VertexShader* pVS;
    ID3D11HullShader* pHS;
    ID3D11DomainShader* pDS;
    ID3D11GeometryShader* pGS;
    ID3D11PixelShader* pPS;
    ID3D11ComputeShader* pCS;

    ID3D11ShaderResourceView* pPSShaderResourceViews[NumPSShaderResourceViews];
    ID3D11SamplerState* pPSSamplers[NumPSSamplers];
    ID3D11Buffer* pPSConstantBuffers[NumPSConstantBuffers];

    ID3D11DepthStencilState* pDepthStencilState;
    UINT StencilRef;
    ID3D11BlendState* pBlendState;
    FLOAT BlendFactor[4];
    UINT SampleMask;
    ID3D11RenderTargetView* pRenderTargetViews[NumRenderTargetViews];
    ID3D11DepthStencilView* pDepthStencilView;
};

} // namespace SSAO
} // namespace GFSDK
