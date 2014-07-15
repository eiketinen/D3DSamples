//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/AppState.cpp
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

#include "AppState.h"

//--------------------------------------------------------------------------------
void GFSDK::SSAO::AppState::Save(ID3D11DeviceContext* pDeviceContext)
{
    pDeviceContext->IAGetPrimitiveTopology(&Topology);
    pDeviceContext->IAGetInputLayout(&pInputLayout);

    UINT NumViewports = 1;
    pDeviceContext->RSGetViewports(&NumViewports, &Viewport);
    pDeviceContext->RSGetState(&pRasterizerState);

    UINT NumClassInstances = 0;
    pDeviceContext->VSGetShader(&pVS, NULL, &NumClassInstances);
    pDeviceContext->HSGetShader(&pHS, NULL, &NumClassInstances);
    pDeviceContext->DSGetShader(&pDS, NULL, &NumClassInstances);
    pDeviceContext->GSGetShader(&pGS, NULL, &NumClassInstances);
    pDeviceContext->PSGetShader(&pPS, NULL, &NumClassInstances);
    pDeviceContext->CSGetShader(&pCS, NULL, &NumClassInstances);

    pDeviceContext->PSGetShaderResources(0, ARRAYSIZE(pPSShaderResourceViews), pPSShaderResourceViews);
    pDeviceContext->PSGetSamplers(0, ARRAYSIZE(pPSSamplers), pPSSamplers);
    pDeviceContext->PSGetConstantBuffers(0, ARRAYSIZE(pPSConstantBuffers), pPSConstantBuffers);

    pDeviceContext->OMGetDepthStencilState(&pDepthStencilState, &StencilRef);
    pDeviceContext->OMGetBlendState(&pBlendState, BlendFactor, &SampleMask);
    pDeviceContext->OMGetRenderTargets(ARRAYSIZE(pRenderTargetViews), pRenderTargetViews, &pDepthStencilView);
}

//--------------------------------------------------------------------------------
void GFSDK::SSAO::AppState::Restore(ID3D11DeviceContext* pDeviceContext)
{
    pDeviceContext->IASetPrimitiveTopology(Topology);
    pDeviceContext->IASetInputLayout(pInputLayout);

    pDeviceContext->RSSetViewports(1, &Viewport);
    pDeviceContext->RSSetState(pRasterizerState);

    pDeviceContext->VSSetShader(pVS, NULL, 0);
    pDeviceContext->HSSetShader(pHS, NULL, 0);
    pDeviceContext->DSSetShader(pDS, NULL, 0);
    pDeviceContext->GSSetShader(pGS, NULL, 0);
    pDeviceContext->PSSetShader(pPS, NULL, 0);
    pDeviceContext->CSSetShader(pCS, NULL, 0);

    pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(pPSShaderResourceViews), pPSShaderResourceViews);
    pDeviceContext->PSSetSamplers(0, ARRAYSIZE(pPSSamplers), pPSSamplers);
    pDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(pPSConstantBuffers), pPSConstantBuffers);

    pDeviceContext->OMSetDepthStencilState(pDepthStencilState, StencilRef);
    pDeviceContext->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
    pDeviceContext->OMSetRenderTargets(ARRAYSIZE(pRenderTargetViews), pRenderTargetViews, pDepthStencilView);

    SAFE_RELEASE(pInputLayout);
    SAFE_RELEASE(pRasterizerState);
    SAFE_RELEASE(pVS);
    SAFE_RELEASE(pHS);
    SAFE_RELEASE(pDS);
    SAFE_RELEASE(pGS);
    SAFE_RELEASE(pPS);
    SAFE_RELEASE(pCS);
    SAFE_RELEASE(pDepthStencilState);
    SAFE_RELEASE(pBlendState);
    SAFE_RELEASE(pDepthStencilView);
    SAFE_RELEASE_ARRAY(pPSShaderResourceViews);
    SAFE_RELEASE_ARRAY(pPSSamplers);
    SAFE_RELEASE_ARRAY(pPSConstantBuffers);
    SAFE_RELEASE_ARRAY(pRenderTargetViews);
}
