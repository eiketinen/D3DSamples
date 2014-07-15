//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/Renderer.h
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
#include "AppState.h"
#include "ConstantBuffers.h"
#include "InputDepthInfo.h"
#include "OutputColorInfo.h"
#include "ProjectionMatrixInfo.h"
#include "RandomTexture.h"
#include "RenderOptions.h"
#include "RenderTargets.h"
#include "Shaders.h"
#include "States.h"
#include "GPUTimers.h"

//--------------------------------------------------------------------------------
class GFSDK_SSAO_Renderer
{
public:
    GFSDK_SSAO_Renderer()
        : m_pD3DDevice(NULL)
    {
    }

    GFSDK_SSAO_Status Create(ID3D11Device* pD3DDevice);

    void Release();

    GFSDK_SSAO_Status RenderAO(ID3D11DeviceContext* pDeviceContext,
                               const GFSDK_SSAO_InputDepthData* pDepthData,
                               const GFSDK_SSAO_Parameters* pParameters,
                               ID3D11RenderTargetView* pOutputColorRTV);

    UINT GetAllocatedVideoMemoryBytes();

    float GetGPUTimeInMS(GFSDK_SSAO_GPUTimer Timer);

private:
    GFSDK::SSAO::Shaders::EnableBlurPermutation GetEnableBlurPermutation()
    {
        return (m_Options.Blur.Enable) ? GFSDK::SSAO::Shaders::PERMUTATION_ENABLE_BLUR_1 :
                                         GFSDK::SSAO::Shaders::PERMUTATION_ENABLE_BLUR_0;
    }
    GFSDK::SSAO::Shaders::EnableRandPermutation GetEnableRandPermutation()
    {
        return (m_Options.RandomizeSamples) ? GFSDK::SSAO::Shaders::PERMUTATION_USE_RANDOM_TEXTURE_1:
                                              GFSDK::SSAO::Shaders::PERMUTATION_USE_RANDOM_TEXTURE_0;
    }

    void SetMSAASampleIndex(ID3D11DeviceContext* pDeviceContext, UINT MSAASampleIndex)
    {
        m_PerSampleCB.SetSampleIndex(MSAASampleIndex);
        m_PerSampleCB.UpdateBuffer(pDeviceContext);
    }
    UINT GetMSAASampleIndex()
    {
        return m_PerSampleCB.GetSampleIndex();
    }
    UINT GetMSAASampleMask()
    {
        return (m_Options.Output.MSAAMode == GFSDK_SSAO_PER_SAMPLE_AO) ? (1 << GetMSAASampleIndex()) : 0xFFFFFFFF;
    }
    UINT GetMSAASampleCount()
    {
        return (m_Options.Output.MSAAMode == GFSDK_SSAO_PER_SAMPLE_AO) ? m_OutputColors.SampleCount : 1;
    }

    ID3D11BlendState* GetOutputBlendState()
    {
        return (m_Options.Output.BlendMode == GFSDK_SSAO_OVERWRITE_RGB) ? m_States.GetBlendStateDisabledPreserveAlpha() :
                                                                          m_States.GetBlendStateMultiplyPreserveAlpha();
    }
    const FLOAT* GetOutputBlendFactor()
    {
        return NULL;
    }

    void InitResources(ID3D11Device* pD3DDevice);
    void ReleaseResources();

    void SetFullscreenState(ID3D11DeviceContext* pDeviceContext);
    void SetAOResolution(UINT Width, UINT Height);

    GFSDK_SSAO_Status SetDataFlow(const GFSDK_SSAO_InputDepthData* pDepthData,
                                  const GFSDK_SSAO_Parameters* pParameters,
                                  ID3D11RenderTargetView* pOutputColorRTV);

    GFSDK_SSAO_Status SetInputDepths(const GFSDK_SSAO_InputDepthData* pDepthData);
    GFSDK_SSAO_Status SetAOParameters(const GFSDK_SSAO_Parameters* pParameters);
    GFSDK_SSAO_Status SetOutputColors(ID3D11RenderTargetView* pOutputColorRTV);
    GFSDK_SSAO_Status ValidateDataFlow();

    void DrawLinearDepth(ID3D11DeviceContext* pDeviceContext);
    void DrawDeinterleavedDepth(ID3D11DeviceContext* pDeviceContext);
    void DrawReconstructedNormal(ID3D11DeviceContext* pDeviceContext);
    void DrawSeparableAO(ID3D11DeviceContext* pDeviceContext);
    void DrawReinterleavedAO(ID3D11DeviceContext* pDeviceContext);
    void DrawNonSeparableAOPS(ID3D11DeviceContext* pDeviceContext);

    void Render(ID3D11DeviceContext* pDeviceContext);
    void RenderSeparableAO(ID3D11DeviceContext* pDeviceContext);
    void RenderNonSeparableAO(ID3D11DeviceContext* pDeviceContext);

    void DrawBlurXPS(ID3D11DeviceContext* pDeviceContext);
    void DrawBlurYPS(ID3D11DeviceContext* pDeviceContext);

    ID3D11Device* m_pD3DDevice;
    GFSDK::SSAO::GlobalConstantBuffer m_GlobalCB;
    GFSDK::SSAO::PerPassConstantBuffer m_PerPassCB;
    GFSDK::SSAO::PerSampleConstantBuffer m_PerSampleCB;
    GFSDK::SSAO::RandomTexture m_RandomTexture;
    GFSDK::SSAO::RenderOptions m_Options;
    GFSDK::SSAO::InputDepthInfo m_InputDepths;
    GFSDK::SSAO::OutputColorInfo m_OutputColors;
    GFSDK::SSAO::RenderTargets m_RTs;
    GFSDK::SSAO::Shaders m_Shaders;
    GFSDK::SSAO::States m_States;
    GFSDK::SSAO::Viewports m_Viewports;
    GPUTimers m_GPUTimers;
};
