//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/ConstantBuffers.h
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
#include <D3D11.h>
#include "Viewports.h"
#include "RandomTexture.h"
#include "InputDepthInfo.h"

// Notes:
// * The structs below must match the layouts from shaders/src/ConstantBuffers.hlsl
// * The struct sizes must be a multiple of 16 bytes (d3d11 requirement).

namespace GFSDK
{
namespace SSAO
{

//--------------------------------------------------------------------------------
class BaseConstantBuffer
{
public:
    BaseConstantBuffer(UINT ByteWidth)
        : m_ByteWidth(ByteWidth)
        , m_pConstantBuffer(NULL)
    {
    }

    void Create(ID3D11Device* pD3DDevice);
    void Release();
    void UpdateCB(ID3D11DeviceContext* pDeviceContext, void* pData);

    ID3D11Buffer* GetCB()
    {
        return m_pConstantBuffer;
    }

protected:
    UINT m_ByteWidth;
    ID3D11Buffer *m_pConstantBuffer;
};

//--------------------------------------------------------------------------------
class GlobalConstantBuffer : public BaseConstantBuffer
{
public:
    GlobalConstantBuffer()
        : BaseConstantBuffer(sizeof(m_Data))
    {
        ZERO_STRUCT(m_Data);
    }

    void SetAOParameters(const GFSDK_SSAO_Parameters* pParams, const SSAO::InputDepthInfo& InputDepth);
    void SetDepthData(const SSAO::InputDepthInfo& InputDepth);
    void SetResolutionConstants(const SSAO::Viewports &Viewports);
    void UpdateBuffer(ID3D11DeviceContext* pDeviceContext);

private:
    struct
    {
        float InvQuarterResolution[2];
        float InvFullResolution[2];

        float UVToViewA[2];
        float UVToViewB[2];

        float RadiusToScreen;
        float R2;
        float NegInvR2;
        float NDotVBias;

        float LinearizeDepthA;
        float LinearizeDepthB;
        float InverseDepthRangeA;
        float InverseDepthRangeB;

        float AOMultiplier;
        float PowExponent;
        float BlurSharpness;
        float Pad;
    } m_Data;

    void SetDepthLinearizationConstants(const SSAO::InputDepthInfo& InputDepth);
    void SetProjectionConstants(const SSAO::InputDepthInfo& InputDepth);
    void SetViewportConstants();

    void SetBlurConstants(const GFSDK_SSAO_BlurParameters* pParams, const SSAO::InputDepthInfo& InputDepth);
    void SetAORadiusConstants(const GFSDK_SSAO_Parameters* pParams, const SSAO::InputDepthInfo& InputDepth);
};

//--------------------------------------------------------------------------------
class PerPassConstantBuffer : public BaseConstantBuffer
{
public:
    PerPassConstantBuffer() : BaseConstantBuffer(sizeof(m_Data)) 
    {
        ZERO_STRUCT(m_Data);
    }

    void UpdateBuffer(ID3D11DeviceContext* pDeviceContext)
    {
        UpdateCB(pDeviceContext, &m_Data);
    }

    void SetOffset(UINT OffsetX, UINT OffsetY)
    {
        m_Data.Float2Offset[0] = float(OffsetX) + 0.5f;
        m_Data.Float2Offset[1] = float(OffsetY) + 0.5f;
    }

    void SetJitter(float4 Jitter)
    {
        m_Data.Jitter = Jitter;
    }

private:
    struct
    {
        float4 Jitter;
        float Float2Offset[2];
        float Pad[2];
    } m_Data;
};

//--------------------------------------------------------------------------------
class PerSampleConstantBuffer : public BaseConstantBuffer
{
public:
    PerSampleConstantBuffer() : BaseConstantBuffer(sizeof(m_Data))
    {
        ZERO_STRUCT(m_Data);
    }

    void UpdateBuffer(ID3D11DeviceContext* pDeviceContext)
    {
        UpdateCB(pDeviceContext, &m_Data);
    }

    void SetSampleIndex(UINT SampleIndex)
    {
        m_Data.SampleIndex = INT(SampleIndex);
    }

    UINT GetSampleIndex()
    {
        return m_Data.SampleIndex;
    }

private:
    struct
    {
        INT SampleIndex;
        INT Pad[3];
    } m_Data;
};

} // namespace SSAO
} // namespace GFSDK
