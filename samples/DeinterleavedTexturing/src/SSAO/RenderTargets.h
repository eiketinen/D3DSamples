//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/RenderTargets.h
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
class RTTexture
{
public:
    ID3D11Texture2D* pTexture;
    ID3D11RenderTargetView* pRTV;
    ID3D11ShaderResourceView* pSRV;
    UINT m_AllocatedSizeInBytes;

    RTTexture()
        : pTexture(NULL)
        , pRTV(NULL)
        , pSRV(NULL)
        , m_AllocatedSizeInBytes(0)
    {
    }

    bool IsAllocated() const
    {
        return (m_AllocatedSizeInBytes != 0);
    }

    UINT GetAllocatedSizeInBytes()
    {
        return m_AllocatedSizeInBytes;
    }

    void CreateOnce(ID3D11Device* pDevice, UINT Width, UINT Height, DXGI_FORMAT Format, UINT ArraySize=1);
    void SafeRelease();

    static UINT FormatSizeInBytes(DXGI_FORMAT Format);
};

//--------------------------------------------------------------------------------
template<UINT ARRAY_SIZE>
class RTTextureArray : public RTTexture
{
public:
    ID3D11RenderTargetView* pRTVs[ARRAY_SIZE];
    ID3D11ShaderResourceView* pSRVs[ARRAY_SIZE];

    RTTextureArray()
        : RTTexture()
    {
        ZERO_ARRAY(pRTVs);
        ZERO_ARRAY(pSRVs);
    }

    void CreateOnce(ID3D11Device* pDevice, UINT Width, UINT Height, DXGI_FORMAT Format);
    void SafeRelease();
};

//--------------------------------------------------------------------------------
template<UINT ARRAY_SIZE>
void SSAO::RTTextureArray<ARRAY_SIZE>::CreateOnce(ID3D11Device* pDevice, UINT Width, UINT Height, DXGI_FORMAT Format)
{
    if (!pTexture)
    {
        RTTexture::CreateOnce(pDevice, Width, Height, Format, ARRAY_SIZE);

        if (!pTexture)
        {
            assert(0);
            return;
        }

        for (UINT SliceId = 0; SliceId < ARRAYSIZE(pRTVs); ++SliceId)
        {
            D3D11_RENDER_TARGET_VIEW_DESC desc;
            desc.Format = Format;
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MipSlice = 0;
            desc.Texture2DArray.FirstArraySlice = SliceId;
            desc.Texture2DArray.ArraySize = 1;
            SAFE_D3D_CALL( pDevice->CreateRenderTargetView(pTexture, &desc, &pRTVs[SliceId]) );
        }

        for (UINT SliceId = 0; SliceId < ARRAYSIZE(pSRVs); ++SliceId)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            desc.Format = Format;
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MostDetailedMip = 0;
            desc.Texture2DArray.MipLevels = 1;
            desc.Texture2DArray.ArraySize = 1;
            desc.Texture2DArray.FirstArraySlice = SliceId;
            SAFE_D3D_CALL( pDevice->CreateShaderResourceView(pTexture, &desc, &pSRVs[SliceId]) );
        }
    }
}

//--------------------------------------------------------------------------------
template<UINT ARRAY_SIZE>
void SSAO::RTTextureArray<ARRAY_SIZE>::SafeRelease()
{
    RTTexture::SafeRelease();

    SAFE_RELEASE_ARRAY(pRTVs);
    SAFE_RELEASE_ARRAY(pSRVs);
}

//--------------------------------------------------------------------------------
class RenderTargets
{
public:
    RenderTargets() :
          m_pDevice(NULL)
        , m_FullWidth(0)
        , m_FullHeight(0)
    {
    }

    void Release()
    {
        m_FullResAOZTexture.SafeRelease();
        m_FullResAOZTexture2.SafeRelease();
        m_FullResNormalTexture.SafeRelease();
        m_FullResViewDepthTexture.SafeRelease();
        m_QuarterResAOTextureArray.SafeRelease();
        m_QuarterResViewDepthTextureArray.SafeRelease();
    }

    void SetDevice(ID3D11Device* pDevice)
    {
        m_pDevice = pDevice;
    }

    void SetFullResolution(UINT Width, UINT Height)
    {
        m_FullWidth = Width;
        m_FullHeight = Height;
    }

    UINT GetFullWidth()
    {
        return m_FullWidth;
    }

    UINT GetFullHeight()
    {
        return m_FullHeight;
    }

    const RTTexture* GetFullResAOZTexture()
    {
        m_FullResAOZTexture.CreateOnce(m_pDevice, m_FullWidth, m_FullHeight, DXGI_FORMAT_R16G16_FLOAT);
        return &m_FullResAOZTexture;
    }

    const RTTexture* GetFullResAOZTexture2()
    {
        m_FullResAOZTexture2.CreateOnce(m_pDevice, m_FullWidth, m_FullHeight, DXGI_FORMAT_R16G16_FLOAT);
        return &m_FullResAOZTexture2;
    }

    const RTTexture* GetFullResViewDepthTexture()
    {
        m_FullResViewDepthTexture.CreateOnce(m_pDevice, m_FullWidth, m_FullHeight, DXGI_FORMAT_R32_FLOAT);
        return &m_FullResViewDepthTexture;
    }

    const RTTextureArray<16>* GetQuarterResViewDepthTextureArray()
    {
        m_QuarterResViewDepthTextureArray.CreateOnce(m_pDevice, iDivUp(m_FullWidth,4), iDivUp(m_FullHeight,4), DXGI_FORMAT_R32_FLOAT);
        return &m_QuarterResViewDepthTextureArray;
    }

    const RTTextureArray<16>* GetQuarterResAOTextureArray()
    {
        m_QuarterResAOTextureArray.CreateOnce(m_pDevice, iDivUp(m_FullWidth,4), iDivUp(m_FullHeight,4), DXGI_FORMAT_R8_UNORM);
        return &m_QuarterResAOTextureArray;
    }

    const RTTexture* GetFullResNormalTexture()
    {
        m_FullResNormalTexture.CreateOnce(m_pDevice, m_FullWidth, m_FullHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
        return &m_FullResNormalTexture;
    }

    UINT GetCurrentAllocatedVideoMemoryBytes()
    {
        return m_FullResAOZTexture.GetAllocatedSizeInBytes() +
               m_FullResAOZTexture2.GetAllocatedSizeInBytes() +
               m_FullResNormalTexture.GetAllocatedSizeInBytes() +
               m_FullResViewDepthTexture.GetAllocatedSizeInBytes() +
               m_QuarterResAOTextureArray.GetAllocatedSizeInBytes() +
               m_QuarterResViewDepthTextureArray.GetAllocatedSizeInBytes();
    }

private:
    ID3D11Device* m_pDevice;
    UINT m_FullWidth;
    UINT m_FullHeight;
    RTTexture m_FullResAOZTexture;
    RTTexture m_FullResAOZTexture2;
    RTTexture m_FullResNormalTexture;
    RTTexture m_FullResViewDepthTexture;
    RTTextureArray<16> m_QuarterResAOTextureArray;
    RTTextureArray<16> m_QuarterResViewDepthTextureArray;
};

} // namespace SSAO
} // namespace GFSDK
