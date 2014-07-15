//----------------------------------------------------------------------------------
// File:        SoftShadows\src/SimpleTexture2D.cpp
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
#include "stdafx.h"
#include "SimpleTexture2D.h"

////////////////////////////////////////////////////////////////////////////////
// UtilCreateTexture2DSRV()
////////////////////////////////////////////////////////////////////////////////
inline ID3D11ShaderResourceView *UtilCreateTexture2DSRV(ID3D11Device *pd3dDevice,ID3D11Texture2D *pTex)
{
    HRESULT hr;
    ID3D11ShaderResourceView *pSRV = NULL;

    D3D11_TEXTURE2D_DESC TexDesc;
    ((ID3D11Texture2D*)pTex)->GetDesc(&TexDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    ::ZeroMemory(&SRVDesc,sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    SRVDesc.Texture2D.MipLevels = TexDesc.MipLevels;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Format = TexDesc.Format;

    hr = pd3dDevice->CreateShaderResourceView(pTex,&SRVDesc,&pSRV);

    return pSRV;
}

////////////////////////////////////////////////////////////////////////////////
// MakeTexture2D()
////////////////////////////////////////////////////////////////////////////////
inline HRESULT MakeTexture2D(ID3D11Device *pd3dDevice,DXGI_FORMAT format,UINT width,UINT height, UINT msaaCount, ID3D11Texture2D **ppTex2D,ID3D11RenderTargetView **ppRTV, ID3D11ShaderResourceView **ppSRV,ID3D11DepthStencilView **ppDSV)
{
    HRESULT hr = S_OK;
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc,sizeof(D3D11_TEXTURE2D_DESC));
    desc.ArraySize = 1;
    if (ppRTV) desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    if (ppSRV) desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (ppDSV) desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    desc.Format = format;
    desc.Width = width;
    desc.Height = height;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.SampleDesc.Count = msaaCount;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels = 1;
    V_RETURN(pd3dDevice->CreateTexture2D(&desc,NULL,ppTex2D));

    DXGI_FORMAT srvFormat;
    DXGI_FORMAT rtvFormat;
    DXGI_FORMAT dsvFormat;
    switch (desc.Format)
    {
    case DXGI_FORMAT_R24G8_TYPELESS:
        srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        rtvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;

    case DXGI_FORMAT_R32_TYPELESS:
        srvFormat = DXGI_FORMAT_R32_FLOAT;
        rtvFormat = DXGI_FORMAT_R32_FLOAT;
        dsvFormat = DXGI_FORMAT_D32_FLOAT;
        break;

    default:
        srvFormat = desc.Format;
        rtvFormat = desc.Format;
        dsvFormat = desc.Format;
    }

    if (ppSRV)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        ZeroMemory(&SRVDesc,sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
        SRVDesc.Format = srvFormat;
        if (msaaCount > 1)
        {
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = 1;
        }

        V_RETURN(pd3dDevice->CreateShaderResourceView(*ppTex2D,&SRVDesc,ppSRV));
    }

    if (ppRTV)
    {
        D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
        ZeroMemory(&RTVDesc,sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
        RTVDesc.Format = rtvFormat;
        if (msaaCount > 1)
            RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        else
            RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        V_RETURN(pd3dDevice->CreateRenderTargetView(*ppTex2D,&RTVDesc,ppRTV));
    }

    if (ppDSV)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
        ZeroMemory(&DSVDesc,sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
        DSVDesc.Format = dsvFormat;
        if (msaaCount > 1)
            DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        else
            DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;        

        V_RETURN(pd3dDevice->CreateDepthStencilView(*ppTex2D,&DSVDesc,ppDSV));
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// LoadTexture2D()
////////////////////////////////////////////////////////////////////////////////
inline HRESULT LoadTexture2D(ID3D11Device *device, const wchar_t *fileName, ID3D11Texture2D **ppTex2D,ID3D11RenderTargetView **ppRTV, ID3D11ShaderResourceView **ppSRV,ID3D11DepthStencilView **ppDSV)
{
    HRESULT hr = S_OK;
    wchar_t str[MAX_PATH];
    V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, fileName));
    D3DX11_IMAGE_LOAD_INFO loadInfo;
    if (ppRTV) loadInfo.BindFlags |= D3D11_BIND_RENDER_TARGET;
    if (ppSRV) loadInfo.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (ppDSV) loadInfo.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    V_RETURN(D3DX11CreateTextureFromFile(device, str, &loadInfo, nullptr, (ID3D11Resource **)ppTex2D, &hr));
    
    if (ppRTV)
    {
        V_RETURN(device->CreateRenderTargetView(*ppTex2D, nullptr, ppRTV));
    }

    if (ppSRV)
    {
        V_RETURN(device->CreateShaderResourceView(*ppTex2D, nullptr, ppSRV));
    }    

    if (ppDSV)
    {
        V_RETURN(device->CreateDepthStencilView(*ppTex2D, nullptr, ppDSV));
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// SimpleTexture2D::SimpleTexture2D()
////////////////////////////////////////////////////////////////////////////////
SimpleTexture2D::SimpleTexture2D(
    ID3D11Device *device,
    const char *name,
    DXGI_FORMAT format,
    UINT width,
    UINT height,
    UINT msaaCount,
    int viewFlags)
    : m_tex2D()
    , m_dsv()
    , m_srv()
    , m_rtv()
    , m_format(format)
    , m_width(width)
    , m_height(height)
    , m_msaaCount(msaaCount)
    , m_hr(S_OK)
{
    ID3D11Texture2D *tex2D = nullptr;
    ID3D11RenderTargetView *rtv = nullptr;
    ID3D11ShaderResourceView *srv = nullptr;
    ID3D11DepthStencilView *dsv = nullptr;
    m_hr = MakeTexture2D(
        device,
        m_format,
        m_width,
        m_height,
        m_msaaCount,
        &tex2D,
        0 != (viewFlags & RenderTargetView) ? &rtv : nullptr,
        0 != (viewFlags & ShaderResourceView) ? &srv : nullptr,
        0 != (viewFlags & DepthStencilView) ? &dsv : nullptr);

    if (tex2D != nullptr)
    {
        DXUT_SetDebugName(tex2D, name);
        m_tex2D.reset(tex2D);
    }

    if (dsv != nullptr)
    {
        DXUT_SetDebugName(dsv, name);
        m_dsv.reset(dsv);
    }

    if (srv != nullptr)
    {
        DXUT_SetDebugName(srv, name);
        m_srv.reset(srv);
    }
        
    if (rtv != nullptr)
    {
        DXUT_SetDebugName(rtv, name);
        m_rtv.reset(rtv);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SimpleTexture2D::SimpleTexture2D()
////////////////////////////////////////////////////////////////////////////////
SimpleTexture2D::SimpleTexture2D(
    ID3D11Device *device,
    const char *name,
    const wchar_t *fileName,
    int viewFlags)
    : m_tex2D()
    , m_dsv()
    , m_srv()
    , m_rtv()
    , m_format(DXGI_FORMAT_UNKNOWN)
    , m_width(0)
    , m_height(0)
    , m_msaaCount(1)
    , m_hr(S_OK)
{
    
    ID3D11Texture2D *tex2D = nullptr;
    ID3D11RenderTargetView *rtv = nullptr;
    ID3D11ShaderResourceView *srv = nullptr;
    ID3D11DepthStencilView *dsv = nullptr;

    m_hr = LoadTexture2D(
        device,
        fileName,
        &tex2D,
        0 != (viewFlags & RenderTargetView) ? &rtv : nullptr,
        0 != (viewFlags & ShaderResourceView) ? &srv : nullptr,
        0 != (viewFlags & DepthStencilView) ? &dsv : nullptr);

    if (tex2D != nullptr)
    {
        DXUT_SetDebugName(tex2D, name);
        m_tex2D.reset(tex2D);
    }

    if (dsv != nullptr)
    {
        DXUT_SetDebugName(dsv, name);
        m_dsv.reset(dsv);
    }

    if (srv != nullptr)
    {
        DXUT_SetDebugName(srv, name);
        m_srv.reset(srv);
    }
        
    if (rtv != nullptr)
    {
        DXUT_SetDebugName(rtv, name);
        m_rtv.reset(rtv);
    }
}