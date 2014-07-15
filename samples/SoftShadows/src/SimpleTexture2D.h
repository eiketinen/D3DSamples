//----------------------------------------------------------------------------------
// File:        SoftShadows\src/SimpleTexture2D.h
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

class SimpleTexture2D
{
public:
    
    enum ViewFlags
    {
        RenderTargetView   = 1,
        ShaderResourceView = 2,
        DepthStencilView   = 4
    };

    SimpleTexture2D(
        ID3D11Device *device,
        const char *name,
        DXGI_FORMAT format,
        UINT width,
        UINT height,
        UINT msaaCount,
        int viewFlags = RenderTargetView | ShaderResourceView);

    SimpleTexture2D(
        ID3D11Device *device,
        const char *name,
        const wchar_t *fileName,
        int viewFlags = ShaderResourceView);

    ID3D11Texture2D *getTexture() { return m_tex2D.get(); }
    ID3D11DepthStencilView *getDepthStencilView() { return m_dsv.get(); }
    ID3D11ShaderResourceView *getShaderResourceView() { return m_srv.get(); }
    ID3D11RenderTargetView *getRenderTargetView() { return m_rtv.get(); }

    DXGI_FORMAT getFormat() const { return m_format; }

    UINT getWidth() const { return m_width; }
    UINT getHeight() const { return m_height; }
    UINT getMsaaCount() const { return m_msaaCount; }

    bool isValid() const { return SUCCEEDED(m_hr); }
    HRESULT errorCode() const { return m_hr; }

private:

    // Noncopyable
    SimpleTexture2D(const SimpleTexture2D &);
    SimpleTexture2D &operator =(const SimpleTexture2D &);

    unique_ref_ptr<ID3D11Texture2D>::type          m_tex2D;
    unique_ref_ptr<ID3D11DepthStencilView>::type   m_dsv;
    unique_ref_ptr<ID3D11ShaderResourceView>::type m_srv;
    unique_ref_ptr<ID3D11RenderTargetView>::type   m_rtv;

    DXGI_FORMAT m_format;

    UINT m_width;
    UINT m_height;
    UINT m_msaaCount;

    HRESULT m_hr;
};