//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/InputDepthInfo.cpp
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

#include "InputDepthInfo.h"
#include "ProjectionMatrixInfo.h"
#include "TextureUtil.h"

//--------------------------------------------------------------------------------
GFSDK_SSAO_Status GFSDK::SSAO::InputDepthInfo::SetData(const GFSDK_SSAO_InputDepthData* pDepthData)
{
    if (!pDepthData ||
        !pDepthData->pFullResDepthTextureSRV ||
        !pDepthData->pProjectionMatrix)
    {
        return GFSDK_SSAO_NULL_ARGUMENT;
    }

    D3D11_TEXTURE2D_DESC TextureDesc;
    GFSDK::SSAO::TextureUtil::GetDesc(pDepthData->pFullResDepthTextureSRV, &TextureDesc);

    if (!GFSDK::SSAO::ProjectionMatrixInfo::IsValid(pDepthData))
    {
        return GFSDK_SSAO_INVALID_PROJECTION_MATRIX;
    }

    Release();
    pDepthTextureSRV = pDepthData->pFullResDepthTextureSRV;
    pDepthTextureSRV->AddRef();

    Width = TextureDesc.Width;
    Height = TextureDesc.Height;
    SampleCount = TextureDesc.SampleDesc.Count;

    ProjectionMatrixInfo = SSAO::ProjectionMatrixInfo(pDepthData);
    MetersToViewSpaceUnits = max(pDepthData->MetersToViewSpaceUnits, 0.f);

    return GFSDK_SSAO_OK;
}

//--------------------------------------------------------------------------------
void GFSDK::SSAO::InputDepthInfo::Release()
{
    SAFE_RELEASE(pDepthTextureSRV);
}
