//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/Renderer.cpp
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

#include "Renderer.h"

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::InitResources(ID3D11Device* pD3DDevice)
{
    m_States.Create(pD3DDevice);
    m_Shaders.Create(pD3DDevice);
    m_GPUTimers.Create(pD3DDevice, NUM_GPU_TIMERS);
    m_RandomTexture.Create(pD3DDevice);
    m_GlobalCB.Create(pD3DDevice);
    m_PerPassCB.Create(pD3DDevice);
    m_PerSampleCB.Create(pD3DDevice);
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::ReleaseResources()
{
    m_InputDepths.Release();
    m_States.Release();
    m_Shaders.Release();
    m_RTs.Release();
    m_GPUTimers.Release();
    m_RandomTexture.Release();
    m_GlobalCB.Release();
    m_PerPassCB.Release();
    m_PerSampleCB.Release();
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::DrawLinearDepth(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_LINEAR_Z);

    pDeviceContext->OMSetRenderTargets(1, &m_RTs.GetFullResViewDepthTexture()->pRTV, NULL);
    pDeviceContext->RSSetViewports(1, &m_Viewports.FullRes);

    if (m_InputDepths.SampleCount == 1)
    {
        pDeviceContext->PSSetShader(m_Shaders.GetLinearizeDepthNoMSAAPS(), NULL, 0);
    }
    else
    {
        pDeviceContext->PSSetShader(m_Shaders.GetResolveAndLinearizeDepthMSAAPS(), NULL, 0);
    }

    pDeviceContext->PSSetShaderResources(0, 1, &m_InputDepths.pDepthTextureSRV);

    pDeviceContext->Draw(3, 0);
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::DrawDeinterleavedDepth(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_DEINTERLEAVE_Z);

    pDeviceContext->RSSetViewports(1, &m_Viewports.QuarterRes);
    pDeviceContext->PSSetShader(m_Shaders.GetDeinterleaveDepthPS(), NULL, 0);
    pDeviceContext->PSSetSamplers(0, 1, &m_States.GetSamplerStatePointClamp());

    const UINT NumMRTs = 8;
    for (UINT SliceIndex = 0; SliceIndex < 16; SliceIndex += NumMRTs)
    {
        m_PerPassCB.SetOffset(SliceIndex % 4, SliceIndex / 4);
        m_PerPassCB.UpdateBuffer(pDeviceContext);

        pDeviceContext->OMSetRenderTargets(NumMRTs, &m_RTs.GetQuarterResViewDepthTextureArray()->pRTVs[SliceIndex], NULL);
        pDeviceContext->PSSetShaderResources(0, 1, &m_RTs.GetFullResViewDepthTexture()->pSRV);

        pDeviceContext->Draw(3, 0);
    }
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::DrawReconstructedNormal(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_RECONSTRUCT_NORMAL);

    pDeviceContext->OMSetRenderTargets(1, &m_RTs.GetFullResNormalTexture()->pRTV, NULL);
    pDeviceContext->RSSetViewports(1, &m_Viewports.FullRes);

    pDeviceContext->PSSetShader(m_Shaders.GetReconstructNormalPS(), NULL, 0);
    pDeviceContext->PSSetSamplers(0, 1, &m_States.GetSamplerStatePointClamp());
    pDeviceContext->PSSetShaderResources(0, 1, &m_RTs.GetFullResViewDepthTexture()->pSRV);

    pDeviceContext->Draw(3, 0);
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::DrawNonSeparableAOPS(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_GENERATE_AO);

    if (m_Options.Blur.Enable)
    {
        pDeviceContext->OMSetRenderTargets(1, &m_RTs.GetFullResAOZTexture2()->pRTV, NULL);
    }
    else
    {
        pDeviceContext->OMSetBlendState(GetOutputBlendState(), GetOutputBlendFactor(), GetMSAASampleMask());
        pDeviceContext->OMSetRenderTargets(1, &m_OutputColors.pColorRTV, NULL);
    }

    ID3D11ShaderResourceView* pSRVs[] =
    {
        m_RTs.GetFullResViewDepthTexture()->pSRV,
        m_RandomTexture.GetSRV(GetMSAASampleIndex())
    };

    ID3D11SamplerState* pSamplers[] =
    {
        pSamplers[0] = m_States.GetSamplerStatePointClamp(),
        pSamplers[1] = m_States.GetSamplerStatePointWrap()
    };

    pDeviceContext->RSSetViewports(1, &m_Viewports.FullRes);
    pDeviceContext->PSSetShader(m_Shaders.GetNonSeparableAOPS(GetEnableBlurPermutation(), GetEnableRandPermutation()), NULL, 0);
    pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(pSRVs), pSRVs);
    pDeviceContext->PSSetSamplers(0, ARRAYSIZE(pSamplers), pSamplers);
    pDeviceContext->Draw(3, 0);
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::RenderNonSeparableAO(ID3D11DeviceContext* pDeviceContext)
{
    DrawLinearDepth(pDeviceContext);
    DrawNonSeparableAOPS(pDeviceContext);

    if (m_Options.Blur.Enable)
    {
        DrawBlurXPS(pDeviceContext);
        DrawBlurYPS(pDeviceContext);
    }
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::DrawSeparableAO(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_GENERATE_AO);

    pDeviceContext->RSSetViewports(1, &m_Viewports.QuarterRes);
    pDeviceContext->PSSetSamplers(0, 1, &m_States.GetSamplerStatePointClamp());
    pDeviceContext->PSSetShader(m_Shaders.GetSeparableAOPS(GetEnableRandPermutation()), NULL, 0);

    for (UINT SliceIndex = 0; SliceIndex < 16; ++SliceIndex)
    {
        m_PerPassCB.SetOffset(SliceIndex % 4, SliceIndex / 4);
        m_PerPassCB.SetJitter(m_RandomTexture.GetJitter(GetMSAASampleIndex() * 16 + SliceIndex));
        m_PerPassCB.UpdateBuffer(pDeviceContext);

        ID3D11RenderTargetView* pOutputAOBufferRTV = m_RTs.GetQuarterResAOTextureArray()->pRTVs[SliceIndex];
        ID3D11ShaderResourceView* pQuarterResDepthSRV = m_RTs.GetQuarterResViewDepthTextureArray()->pSRVs[SliceIndex];
        ID3D11ShaderResourceView* pFullResNormalBufferSRV = m_RTs.GetFullResNormalTexture()->pSRV;

        ID3D11ShaderResourceView* pSRVs[] =
        {
            pQuarterResDepthSRV,
            pFullResNormalBufferSRV
        };

        pDeviceContext->OMSetRenderTargets(1, &pOutputAOBufferRTV, NULL);
        pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(pSRVs), pSRVs);
        pDeviceContext->Draw(3, 0);
    }
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::DrawReinterleavedAO(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_REINTERLEAVE_AO);

    ID3D11ShaderResourceView* pSRVs[] =
    {
         m_RTs.GetQuarterResAOTextureArray()->pSRV,
         m_RTs.GetFullResViewDepthTexture()->pSRV
    };

    if (m_Options.Blur.Enable)
    {
        pDeviceContext->OMSetRenderTargets(1, &m_RTs.GetFullResAOZTexture2()->pRTV, NULL);
    }
    else
    {
        pDeviceContext->OMSetBlendState(GetOutputBlendState(), GetOutputBlendFactor(), GetMSAASampleMask());
        pDeviceContext->OMSetRenderTargets(1, &m_OutputColors.pColorRTV, NULL);
    }

    pDeviceContext->RSSetViewports(1, &m_Viewports.FullRes);
    pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(pSRVs), pSRVs);
    pDeviceContext->PSSetSamplers(0, 1, &m_States.GetSamplerStatePointClamp());
    pDeviceContext->PSSetShader(m_Shaders.GetReinterleaveAOPS(GetEnableBlurPermutation()), NULL, 0);

    pDeviceContext->Draw(3, 0);
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::DrawBlurXPS(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_BLURX);

    ID3D11SamplerState* pSamplers[] =
    {
        m_States.GetSamplerStatePointClamp(),
        m_States.GetSamplerStateLinearClamp()
    };

    pDeviceContext->OMSetRenderTargets(1, &m_RTs.GetFullResAOZTexture()->pRTV, NULL);
    pDeviceContext->RSSetViewports(1, &m_Viewports.FullRes);

    pDeviceContext->PSSetShader(m_Shaders.GetBlurXPS(), NULL, 0);
    pDeviceContext->PSSetShaderResources(0, 1, &m_RTs.GetFullResAOZTexture2()->pSRV);
    pDeviceContext->PSSetSamplers(0, ARRAYSIZE(pSamplers), pSamplers);

    pDeviceContext->Draw(3, 0);
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::DrawBlurYPS(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_BLURY);

    pDeviceContext->OMSetBlendState(GetOutputBlendState(), GetOutputBlendFactor(), GetMSAASampleMask());

    pDeviceContext->OMSetRenderTargets(1, &m_OutputColors.pColorRTV, NULL);
    pDeviceContext->RSSetViewports(1, &m_Viewports.FullRes);

    pDeviceContext->PSSetShader(m_Shaders.GetBlurYPS(), NULL, 0);
    pDeviceContext->PSSetShaderResources(0, 1, &m_RTs.GetFullResAOZTexture()->pSRV);

    pDeviceContext->Draw(3, 0);
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::SetFullscreenState(ID3D11DeviceContext* pDeviceContext)
{
    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pDeviceContext->IASetInputLayout(NULL);

    pDeviceContext->RSSetState(m_States.GetRasterizerStateFullscreenNoScissor());
    pDeviceContext->RSSetViewports(1, &m_Viewports.FullRes);

    pDeviceContext->VSSetShader(m_Shaders.GetFullScreenTriangleVS(), NULL, 0);
    pDeviceContext->HSSetShader(NULL, NULL, 0);
    pDeviceContext->DSSetShader(NULL, NULL, 0);
    pDeviceContext->GSSetShader(NULL, NULL, 0);
    pDeviceContext->CSSetShader(NULL, NULL, 0);

    ID3D11Buffer* pCBs[] =
    {
        m_GlobalCB.GetCB(),
        m_PerPassCB.GetCB(),
        m_PerSampleCB.GetCB()
    };
    pDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(pCBs), pCBs);
    pDeviceContext->PSSetSamplers(0, 1, &m_States.GetSamplerStatePointClamp());

    pDeviceContext->OMSetDepthStencilState(m_States.GetDepthStencilStateDisabled(), 0x0);
    pDeviceContext->OMSetBlendState(m_States.GetBlendStateDisabled(), NULL, 0xFFFFFFFF);
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::RenderSeparableAO(ID3D11DeviceContext* pDeviceContext)
{
    DrawLinearDepth(pDeviceContext);
    DrawDeinterleavedDepth(pDeviceContext);
    DrawReconstructedNormal(pDeviceContext);
    DrawSeparableAO(pDeviceContext);
    DrawReinterleavedAO(pDeviceContext);

    if (m_Options.Blur.Enable)
    {
        DrawBlurXPS(pDeviceContext);
        DrawBlurYPS(pDeviceContext);
    }
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::Render(ID3D11DeviceContext* pDeviceContext)
{
    GPUTimer timer(&m_GPUTimers, pDeviceContext, GPU_TIME_TOTAL);

    m_GlobalCB.UpdateBuffer(pDeviceContext);

    for (UINT SampleIndex = 0; SampleIndex < GetMSAASampleCount(); ++SampleIndex)
    {
        GFSDK::SSAO::AppState::UnbindSRVs(pDeviceContext);
        SetFullscreenState(pDeviceContext);
        SetMSAASampleIndex(pDeviceContext, SampleIndex);

        if (m_Options.UseDeinterleavedTexturing)
        {
            RenderSeparableAO(pDeviceContext);
        }
        else
        {
            RenderNonSeparableAO(pDeviceContext);
        }
    }
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::SetAOResolution(UINT Width, UINT Height)
{
    if (Width  != m_RTs.GetFullWidth() ||
        Height != m_RTs.GetFullHeight())
    {
        m_RTs.Release();
        m_RTs.SetFullResolution(Width, Height);
        m_Viewports.SetFullResolution(Width, Height);
        m_GlobalCB.SetResolutionConstants(m_Viewports);
    }
}

//--------------------------------------------------------------------------------
GFSDK_SSAO_Status GFSDK_SSAO_Renderer::RenderAO(ID3D11DeviceContext* pDeviceContext,
                                                const GFSDK_SSAO_InputDepthData* pInputDepths,
                                                const GFSDK_SSAO_Parameters* pParameters,
                                                ID3D11RenderTargetView* pOutputColorRTV)
{
    GFSDK_SSAO_Status status;

    if (!pDeviceContext)
    {
        return GFSDK_SSAO_NULL_ARGUMENT;
    }

    status = SetDataFlow(pInputDepths, pParameters, pOutputColorRTV);
    if (status != GFSDK_SSAO_OK)
    {
        return status;
    }

    GFSDK::SSAO::AppState AppState;
    AppState.Save(pDeviceContext);

    m_GPUTimers.BeginFrame(pDeviceContext);

    Render(pDeviceContext);

    m_GPUTimers.EndFrame(pDeviceContext);

    AppState.Restore(pDeviceContext);

    return GFSDK_SSAO_OK;
}

//--------------------------------------------------------------------------------
GFSDK_SSAO_Status GFSDK_SSAO_Renderer::SetOutputColors(ID3D11RenderTargetView* pOutputColorRTV)
{
    if (!pOutputColorRTV)
    {
        return GFSDK_SSAO_NULL_ARGUMENT;
    }

    m_OutputColors.SetRTV(pOutputColorRTV);

    return GFSDK_SSAO_OK;
}

//--------------------------------------------------------------------------------
GFSDK_SSAO_Status GFSDK_SSAO_Renderer::SetDataFlow(const GFSDK_SSAO_InputDepthData* pDepthData,
                                                   const GFSDK_SSAO_Parameters* pParameters,
                                                   ID3D11RenderTargetView* pOutputColorRTV)
{
    GFSDK_SSAO_Status status;

    status = SetInputDepths(pDepthData);
    if (status != GFSDK_SSAO_OK)
    {
        return status;
    }

    status = SetAOParameters(pParameters);
    if (status != GFSDK_SSAO_OK)
    {
        return status;
    }

    status = SetOutputColors(pOutputColorRTV);
    if (status != GFSDK_SSAO_OK)
    {
        return status;
    }

    status = ValidateDataFlow();
    if (status != GFSDK_SSAO_OK)
    {
        return status;
    }

    return GFSDK_SSAO_OK;
}

//--------------------------------------------------------------------------------
GFSDK_SSAO_Status GFSDK_SSAO_Renderer::SetInputDepths(const GFSDK_SSAO_InputDepthData* pDepthData)
{
    GFSDK_SSAO_Status status = m_InputDepths.SetData(pDepthData);
    if (status != GFSDK_SSAO_OK)
    {
        return status;
    }

    m_GlobalCB.SetDepthData(m_InputDepths);

    SetAOResolution(UINT(m_InputDepths.Width), UINT(m_InputDepths.Height));

    return GFSDK_SSAO_OK;
}

//--------------------------------------------------------------------------------
GFSDK_SSAO_Status GFSDK_SSAO_Renderer::ValidateDataFlow()
{
    if (m_Options.Output.MSAAMode == GFSDK_SSAO_PER_SAMPLE_AO &&
        m_OutputColors.SampleCount != m_InputDepths.SampleCount)
    {
        return GFSDK_SSAO_INVALID_OUTPUT_MSAA_SAMPLE_COUNT;
    }

    return GFSDK_SSAO_OK;
}

//--------------------------------------------------------------------------------
GFSDK_SSAO_Status GFSDK_SSAO_Renderer::SetAOParameters(const GFSDK_SSAO_Parameters* pParams)
{
    if (!pParams)
    {
        return GFSDK_SSAO_NULL_ARGUMENT;
    }

    if (pParams->Blur.Enable != m_Options.Blur.Enable)
    {
        m_RTs.Release();
    }

    m_GlobalCB.SetAOParameters(pParams, m_InputDepths);
    m_Options.SetRenderOptions(pParams);

    return GFSDK_SSAO_OK;
}

//--------------------------------------------------------------------------------
GFSDK_SSAO_Status GFSDK_SSAO_Renderer::Create(ID3D11Device* pD3DDevice)
{
    if (pD3DDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
    {
        return GFSDK_SSAO_UNSUPPORTED_D3D_FEATURE_LEVEL;
    }

    if (!pD3DDevice)
    {
        return GFSDK_SSAO_NULL_ARGUMENT;
    }

    if (m_pD3DDevice != pD3DDevice)
    {
        m_pD3DDevice = pD3DDevice;
        m_pD3DDevice->AddRef();

        InitResources(pD3DDevice);
        m_RTs.SetDevice(pD3DDevice);
    }

    return GFSDK_SSAO_OK;
}

//--------------------------------------------------------------------------------
void GFSDK_SSAO_Renderer::Release()
{
    ReleaseResources();

    SAFE_RELEASE(m_pD3DDevice);
}

//--------------------------------------------------------------------------------
UINT GFSDK_SSAO_Renderer::GetAllocatedVideoMemoryBytes()
{
    // Ignore the constant buffers & random texture because they are much smaller than the render targets
    return m_RTs.GetCurrentAllocatedVideoMemoryBytes();
}

//--------------------------------------------------------------------------------
float GFSDK_SSAO_Renderer::GetGPUTimeInMS(GFSDK_SSAO_GPUTimer Timer)
{
    // If a pass occured multiple times per frame, assume that it took roughly the same GPU time for each instance
    return m_GPUTimers.GetGPUTimeInMS(Timer) * float(m_GPUTimers.GetNumInstancesPerFrame(Timer));
}
