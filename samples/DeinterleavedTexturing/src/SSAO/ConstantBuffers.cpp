//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/ConstantBuffers.cpp
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

#include "ConstantBuffers.h"
#include "MatrixView.h"

#ifndef USE_MAP_DISCARD
#define USE_MAP_DISCARD 1
#endif

using namespace GFSDK;

//--------------------------------------------------------------------------------
void SSAO::BaseConstantBuffer::Create(ID3D11Device* pD3DDevice)
{
#if USE_MAP_DISCARD
    D3D11_BUFFER_DESC desc = 
    {
         m_ByteWidth, //ByteWidth
         D3D11_USAGE_DYNAMIC, //Usage
         D3D11_BIND_CONSTANT_BUFFER, //BindFlags
         D3D11_CPU_ACCESS_WRITE, //CPUAccessFlags
         0  //MiscFlags
    };
#else
    D3D11_BUFFER_DESC desc = 
    {
         m_ByteWidth, //ByteWidth
         D3D11_USAGE_DEFAULT, //Usage
         D3D11_BIND_CONSTANT_BUFFER, //BindFlags
         0, //CPUAccessFlags
         0  //MiscFlags
    };
#endif

    // The D3D11 runtime requires constant buffer sizes to be multiple of 16 bytes
    assert(desc.ByteWidth % 16 == 0);

    SAFE_D3D_CALL( pD3DDevice->CreateBuffer(&desc, NULL, &m_pConstantBuffer) );
}

//--------------------------------------------------------------------------------
void SSAO::BaseConstantBuffer::Release()
{
    SAFE_RELEASE(m_pConstantBuffer);
}

//--------------------------------------------------------------------------------
void SSAO::BaseConstantBuffer::UpdateCB(ID3D11DeviceContext* pDeviceContext, void* pData)
{
#if USE_MAP_DISCARD
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if (pDeviceContext->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) == S_OK)
    {
        memcpy(MappedResource.pData, pData, m_ByteWidth);
        pDeviceContext->Unmap(m_pConstantBuffer, 0);
    }
#else
    pDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, pData, 0, 0);
#endif
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::SetBlurConstants(const GFSDK_SSAO_BlurParameters* pParams, const SSAO::InputDepthInfo& InputDepth)
{
    float BaseSharpness = max(pParams->Sharpness, 0.f);
    BaseSharpness /= InputDepth.MetersToViewSpaceUnits;
    m_Data.BlurSharpness = BaseSharpness;
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::SetAORadiusConstants(const GFSDK_SSAO_Parameters* pParams, const SSAO::InputDepthInfo& InputDepth)
{
    const float RadiusInMeters = max(pParams->Radius, EPSILON);
    const float R = RadiusInMeters * InputDepth.MetersToViewSpaceUnits;
    m_Data.R2 = R * R;
    m_Data.NegInvR2 = -1.f / m_Data.R2;

    const float TanHalfFovy = InputDepth.ProjectionMatrixInfo.GetTanHalfFovy();
    m_Data.RadiusToScreen = R * 0.5f / TanHalfFovy * float(InputDepth.Height);
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::SetAOParameters(const GFSDK_SSAO_Parameters* pParams, const SSAO::InputDepthInfo& InputDepth)
{
    SetAORadiusConstants(pParams, InputDepth);
    SetBlurConstants(&pParams->Blur, InputDepth);

    m_Data.PowExponent = max(pParams->PowerExponent, 0.f);
    m_Data.NDotVBias = clamp(pParams->Bias, 0.f, 1.f);
    m_Data.AOMultiplier = 1.f / (1.f - m_Data.NDotVBias);
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::SetDepthData(const SSAO::InputDepthInfo& InputDepth)
{
    SetDepthLinearizationConstants(InputDepth);
    SetViewportConstants();
    SetProjectionConstants(InputDepth);
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::SetResolutionConstants(const SSAO::Viewports &Viewports)
{
    m_Data.InvFullResolution[0] = 1.f / Viewports.FullRes.Width;
    m_Data.InvFullResolution[1] = 1.f / Viewports.FullRes.Height;
    m_Data.InvQuarterResolution[0] = 1.f / Viewports.QuarterRes.Width;
    m_Data.InvQuarterResolution[1] = 1.f / Viewports.QuarterRes.Height;
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::SetProjectionConstants(const SSAO::InputDepthInfo& InputDepth)
{
    const float TanHalfFovy   = InputDepth.ProjectionMatrixInfo.GetTanHalfFovy();
    const float InvFocalLenX  = TanHalfFovy * (float(InputDepth.Width) / float(InputDepth.Height));
    const float InvFocalLenY  = TanHalfFovy;
    m_Data.UVToViewA[0] =  2.f * InvFocalLenX;
    m_Data.UVToViewA[1] = -2.f * InvFocalLenY;
    m_Data.UVToViewB[0] = -1.f * InvFocalLenX;
    m_Data.UVToViewB[1] =  1.f * InvFocalLenY;
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::SetViewportConstants()
{
    // In Shaders/Src/LinearizeDepth_Common.hlsl:
    // float NormalizedDepth = saturate(g_InverseDepthRangeA * HardwareDepth + g_InverseDepthRangeB);

    m_Data.InverseDepthRangeA = 1.f;
    m_Data.InverseDepthRangeB = 0.f;
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::SetDepthLinearizationConstants(const SSAO::InputDepthInfo& InputDepth)
{
    // In Shaders/Src/LinearizeDepth_Common.hlsl:
    // float ViewDepth = 1.0 / (NormalizedDepth * g_LinearizeDepthA + g_LinearizeDepthB);

    // Inverse projection from [0,1] to [ZNear,ZFar]
    // W = 1 / [(1/ZFar - 1/ZNear) * Z + 1/ZNear]
    const float ZNear = InputDepth.ProjectionMatrixInfo.GetZNear();
    const float ZFar  = InputDepth.ProjectionMatrixInfo.GetZFar();
    m_Data.LinearizeDepthA = 1.f/ZFar - 1.f/ZNear;
    m_Data.LinearizeDepthB = 1.f/ZNear;

    // For reversed infinite projections (ZNear = +inf)
    // Clamp this constant to avoid dividing by 0.f in LinearizeDepth_Common.hlsl
    m_Data.LinearizeDepthB = max(m_Data.LinearizeDepthB, EPSILON);
}

//--------------------------------------------------------------------------------
void SSAO::GlobalConstantBuffer::UpdateBuffer(ID3D11DeviceContext* pDeviceContext)
{
    UpdateCB(pDeviceContext, &m_Data);
}
