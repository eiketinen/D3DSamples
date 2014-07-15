//----------------------------------------------------------------------------------
// File:        ComputeFilter\src/sat.cpp
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
#include "common_util.h"
#include "sat.h"
#include "perftracker.h"

#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

namespace SAT
{
    #define BLOCK_COUNT(x, y) (((x)-1)/(y)+1)

    const int WARP_SIZE = 32;
    const int GROUP_SIZE = 16;
    const int SAT_DOWNSAMPLE_AMOUNT = 1; // 2^(n) amount to scale the SAT texture down by

    ////////////////////////////////////////////////////////////////////////////////

    ID3D11ComputeShader * g_pTranslateInputCS = nullptr;
    ID3D11ComputeShader * g_pSumSegmentsCS = nullptr;
    ID3D11ComputeShader * g_pOffsetSegmentsCS = nullptr;

    ID3D11Texture2D *           g_pSAT_Tex = nullptr;
    ID3D11UnorderedAccessView * g_pSAT_UAV = nullptr;
    ID3D11ShaderResourceView *  g_pSAT_SRV = nullptr;

    ID3D11Texture2D *           g_pWork1_Tex = nullptr;
    ID3D11UnorderedAccessView * g_pWork1_UAV = nullptr;
    ID3D11ShaderResourceView *  g_pWork1_SRV = nullptr;

    ID3D11Texture2D *           g_pWork2_Tex = nullptr;
    ID3D11UnorderedAccessView * g_pWork2_UAV = nullptr;
    ID3D11ShaderResourceView *  g_pWork2_SRV = nullptr;

    ID3D11SamplerState *        g_pSampler = nullptr;

    unsigned int g_satWidth;
    unsigned int g_satHeight;

    ////////////////////////////////////////////////////////////////////////////////

    void Initialize(ID3D11Device * d3dDevice, int width, int height )
    {
        SAT::Cleanup();

        g_satWidth = width >> SAT_DOWNSAMPLE_AMOUNT;
        g_satHeight = height >> SAT_DOWNSAMPLE_AMOUNT;

        //
        // Shaders
        //
        {
            void * bytecode;
            UINT bytecode_length;

            get_shader_resource(L"CS_SAT_TRANSLATE", &bytecode, bytecode_length);
            d3dDevice->CreateComputeShader(bytecode, bytecode_length, NULL, &g_pTranslateInputCS);

            get_shader_resource(L"CS_SAT_SUM", &bytecode, bytecode_length);
            d3dDevice->CreateComputeShader(bytecode, bytecode_length, NULL, &g_pSumSegmentsCS);

            get_shader_resource(L"CS_SAT_OFFSET", &bytecode, bytecode_length);
            d3dDevice->CreateComputeShader(bytecode, bytecode_length, NULL, &g_pOffsetSegmentsCS);
        }

        //
        // Buffers
        //
        {
            D3D11_TEXTURE2D_DESC texDesc;
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            texDesc.SampleDesc.Count = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = 0;
            texDesc.Width  = g_satWidth;
            texDesc.Height = g_satHeight;
            d3dDevice->CreateTexture2D(&texDesc, NULL, &g_pSAT_Tex);
            texDesc.Width  = max(g_satWidth,g_satHeight);
            texDesc.Height = max(g_satWidth,g_satHeight);
            d3dDevice->CreateTexture2D(&texDesc, NULL, &g_pWork1_Tex);
            d3dDevice->CreateTexture2D(&texDesc, NULL, &g_pWork2_Tex);

            uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;
            d3dDevice->CreateUnorderedAccessView(g_pSAT_Tex, &uavDesc, &g_pSAT_UAV);
            d3dDevice->CreateUnorderedAccessView(g_pWork1_Tex, &uavDesc, &g_pWork1_UAV);
            d3dDevice->CreateUnorderedAccessView(g_pWork2_Tex, &uavDesc, &g_pWork2_UAV);

            srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;
            d3dDevice->CreateShaderResourceView(g_pSAT_Tex, &srvDesc, &g_pSAT_SRV);
            d3dDevice->CreateShaderResourceView(g_pWork1_Tex, &srvDesc, &g_pWork1_SRV);
            d3dDevice->CreateShaderResourceView(g_pWork2_Tex, &srvDesc, &g_pWork2_SRV);            
        }

        //
        // Sampler
        //
        {
            D3D11_SAMPLER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.MipLODBias = 0.0f;
            desc.MaxAnisotropy = 1;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f;
            desc.MinLOD = 0;
            desc.MaxLOD = D3D11_FLOAT32_MAX;
            d3dDevice->CreateSamplerState(&desc, &g_pSampler);
        }
    }

    void Cleanup()
    {
        SAFE_RELEASE(g_pTranslateInputCS);
        SAFE_RELEASE(g_pSumSegmentsCS);
        SAFE_RELEASE(g_pOffsetSegmentsCS);

        SAFE_RELEASE(g_pWork2_Tex);
        SAFE_RELEASE(g_pWork2_UAV);
        SAFE_RELEASE(g_pWork2_SRV);

        SAFE_RELEASE(g_pWork1_Tex);
        SAFE_RELEASE(g_pWork1_SRV);
        SAFE_RELEASE(g_pWork1_UAV);

        SAFE_RELEASE(g_pSAT_Tex);
        SAFE_RELEASE(g_pSAT_SRV);
        SAFE_RELEASE(g_pSAT_UAV);

        SAFE_RELEASE(g_pSampler);
    }

    void GenerateSAT(ID3D11DeviceContext * ctx, ID3D11ShaderResourceView * pInputSRV)
    {
        PERF_EVENT_SCOPED(ctx, "DoF > SAT");

        void * pNULL = NULL;

        // Copy/translate data
        PERF_EVENT_BEGIN(ctx, "DoF > SAT > Translate");
        ctx->CSSetShaderResources(0, 1, &pInputSRV);
        ctx->CSSetSamplers(0, 1, &g_pSampler);
        ctx->CSSetUnorderedAccessViews(0, 1, &g_pSAT_UAV, NULL);
        ctx->CSSetShader(g_pTranslateInputCS, NULL, NULL);
        ctx->Dispatch(BLOCK_COUNT(g_satWidth, WARP_SIZE), BLOCK_COUNT(g_satHeight, GROUP_SIZE), 1);
        ctx->CSSetShaderResources(0, 1, (ID3D11ShaderResourceView **)&pNULL);
        ctx->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView **)&pNULL, NULL);
        PERF_EVENT_END(ctx);

        // Sum groups in rows
        PERF_EVENT_BEGIN(ctx, "DoF > SAT > Sum");
        ctx->CSSetShaderResources(0, 1, &g_pSAT_SRV);
        ctx->CSSetUnorderedAccessViews(0, 1, &g_pWork1_UAV, NULL);
        ctx->CSSetShader(g_pSumSegmentsCS, NULL, NULL);
        ctx->Dispatch(BLOCK_COUNT(g_satWidth, WARP_SIZE), BLOCK_COUNT(g_satHeight, GROUP_SIZE), 1);
        ctx->CSSetShaderResources(0, 1, (ID3D11ShaderResourceView **)&pNULL);
        ctx->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView **)&pNULL, NULL);
        PERF_EVENT_END(ctx);

        // Offset row groups and transpose
        PERF_EVENT_BEGIN(ctx, "DoF > SAT > Offset");
        ctx->CSSetShaderResources(0, 1, &g_pWork1_SRV);
        ctx->CSSetUnorderedAccessViews(0, 1, &g_pWork2_UAV, NULL);
        ctx->CSSetShader(g_pOffsetSegmentsCS, NULL, NULL);
        ctx->Dispatch(BLOCK_COUNT(g_satWidth, WARP_SIZE), BLOCK_COUNT(g_satHeight, GROUP_SIZE), 1);
        ctx->CSSetShaderResources(0, 1, (ID3D11ShaderResourceView **)&pNULL);
        ctx->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView **)&pNULL, NULL);
        PERF_EVENT_END(ctx);

        // Sum groups in (transposed) columns
        PERF_EVENT_BEGIN(ctx, "DoF > SAT > Sum");
        ctx->CSSetShaderResources(0, 1, &g_pWork2_SRV);
        ctx->CSSetUnorderedAccessViews(0, 1, &g_pWork1_UAV, NULL);
        ctx->CSSetShader(g_pSumSegmentsCS, NULL, NULL);
        ctx->Dispatch(BLOCK_COUNT(g_satHeight, WARP_SIZE), BLOCK_COUNT(g_satWidth, GROUP_SIZE), 1);
        ctx->CSSetShaderResources(0, 1, (ID3D11ShaderResourceView **)&pNULL);
        ctx->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView **)&pNULL, NULL);
        PERF_EVENT_END(ctx);

        // Offset (transposed) column groups and transpose
        PERF_EVENT_BEGIN(ctx, "DoF > SAT > Offset");
        ctx->CSSetShaderResources(0, 1, &g_pWork1_SRV);
        ctx->CSSetUnorderedAccessViews(0, 1, &g_pSAT_UAV, NULL);
        ctx->CSSetShader(g_pOffsetSegmentsCS, NULL, NULL);
        ctx->Dispatch(BLOCK_COUNT(g_satHeight, WARP_SIZE), BLOCK_COUNT(g_satWidth, GROUP_SIZE), 1);
        ctx->CSSetShaderResources(0, 1, (ID3D11ShaderResourceView **)&pNULL);
        ctx->CSSetUnorderedAccessViews(0, 1, (ID3D11UnorderedAccessView **)&pNULL, NULL);
        PERF_EVENT_END(ctx);
    }

    ID3D11ShaderResourceView * GetSAT()
    {
        return g_pSAT_SRV;
    }
};