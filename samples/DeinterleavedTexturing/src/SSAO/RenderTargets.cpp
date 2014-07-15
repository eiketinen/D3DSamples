//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/RenderTargets.cpp
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

#include "RenderTargets.h"

//--------------------------------------------------------------------------------
UINT GFSDK::SSAO::RTTexture::FormatSizeInBytes(DXGI_FORMAT Format)
{
    UINT NumBytes = 0;
    switch (Format)
    {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            NumBytes = 8;
            break;
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R11G11B10_FLOAT:
            NumBytes = 4;
            break;
        case DXGI_FORMAT_R16_FLOAT:
            NumBytes = 2;
            break;
        case DXGI_FORMAT_R8_UNORM:
            NumBytes = 1;
            break;
        default:
            assert(0);
            break;
    }
    return NumBytes;
}

//--------------------------------------------------------------------------------
void GFSDK::SSAO::RTTexture::CreateOnce(ID3D11Device* pDevice, UINT Width, UINT Height, DXGI_FORMAT Format, UINT ArraySize)
{
    if (!pTexture)
    {
        D3D11_TEXTURE2D_DESC desc;
        desc.Width              = Width;
        desc.Height             = Height;
        desc.Format             = Format;
        desc.MipLevels          = 1;
        desc.ArraySize          = ArraySize;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage              = D3D11_USAGE_DEFAULT;
        desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags     = 0;
        desc.MiscFlags          = 0;

        HRESULT hr = pDevice->CreateTexture2D( &desc, NULL, &pTexture );
        if (hr != S_OK)
        {
            assert(0);
            return;
        }

        hr = pDevice->CreateShaderResourceView( pTexture, NULL, &pSRV );
        if (hr != S_OK)
        {
            assert(0);
            return;
        }

        hr = pDevice->CreateRenderTargetView( pTexture, NULL, &pRTV );
        if (hr != S_OK)
        {
            assert(0);
            return;
        }

        m_AllocatedSizeInBytes = Width * Height * ArraySize * FormatSizeInBytes(Format);
    }
}

//--------------------------------------------------------------------------------
void GFSDK::SSAO::RTTexture::SafeRelease()
{
    SAFE_RELEASE(pTexture);
    SAFE_RELEASE(pRTV);
    SAFE_RELEASE(pSRV);

    m_AllocatedSizeInBytes = 0;
}
