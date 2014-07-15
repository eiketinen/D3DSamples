//----------------------------------------------------------------------------------
// File:        DeferredShadingMSAA\src/DirectXUtil.h
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

#include "Camera.h"
#include "AntTweakBar.h"
#include "DeviceManager.h"
//#include "resource.h"
#include "strsafe.h"
#include <string>

#include <d3dcommon.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>

#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if(FAILED(hr)) { return hr; } }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)      { if (p) { delete p; (p)=NULL; } }
#endif

namespace DirectXUtil
{
	inline HRESULT CompileFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, const D3D10_SHADER_MACRO* pDefines = NULL)
	{
		HRESULT hr = S_OK;

		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows 
		// the shaders to be optimized and to run exactly the way they will run in 
		// the release configuration of this program.
		dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

		ID3DBlob* pErrorBlob;
		hr = D3DX11CompileFromFile(szFileName, pDefines, NULL, szEntryPoint, szShaderModel, 
			dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
		if(FAILED(hr))
		{
			if(pErrorBlob != NULL)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			SAFE_RELEASE(pErrorBlob);
			return hr;
		}
		SAFE_RELEASE(pErrorBlob);

		return S_OK;
	}

	inline ID3D11ShaderResourceView* CreateTexture2DSRV(ID3D11Device *pd3dDevice, ID3D11Texture2D* pTex)
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

		hr = pd3dDevice->CreateShaderResourceView(pTex, &SRVDesc, &pSRV);

		return pSRV;
	}

	inline HRESULT InitTexture2D(ID3D11Device *pd3dDevice,DXGI_FORMAT format,UINT width,UINT height, UINT msaaCount, ID3D11Texture2D **ppTex2D, ID3D11ShaderResourceView **ppSRV, ID3D11RenderTargetView **ppRTV, ID3D11DepthStencilView **ppDSV)
	{
		HRESULT hr = S_OK;
		D3D11_TEXTURE2D_DESC Desc;
		ZeroMemory(&Desc,sizeof(D3D11_TEXTURE2D_DESC));
		Desc.ArraySize = 1;
		if(ppRTV) Desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if(ppSRV) Desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if(ppDSV) Desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		Desc.Format = format;
		Desc.Width = width;
		Desc.Height = height;
		Desc.Usage = D3D11_USAGE_DEFAULT;
		Desc.SampleDesc.Count = msaaCount;
		Desc.SampleDesc.Quality = 0;
		Desc.MipLevels = 1;
		V_RETURN(pd3dDevice->CreateTexture2D(&Desc,NULL,ppTex2D));

		DXGI_FORMAT srvFormat = Desc.Format;
		DXGI_FORMAT rtvFormat = Desc.Format;
		DXGI_FORMAT dsvFormat = Desc.Format;
		if(format == DXGI_FORMAT_R24G8_TYPELESS)
		{
			srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			rtvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		}

		if(ppSRV)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			ZeroMemory(&SRVDesc,sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			SRVDesc.Format = srvFormat;
			if(msaaCount > 1)
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

		if(ppRTV)
		{
			D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
			ZeroMemory(&RTVDesc,sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
			RTVDesc.Format = rtvFormat;
			if(msaaCount > 1)
				RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
			else
				RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			V_RETURN(pd3dDevice->CreateRenderTargetView(*ppTex2D,&RTVDesc,ppRTV));
		}

		if(ppDSV)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
			ZeroMemory(&DSVDesc,sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
			DSVDesc.Format = dsvFormat;
			if(msaaCount > 1)
				DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
			else
				DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;		

			V_RETURN(pd3dDevice->CreateDepthStencilView(*ppTex2D,&DSVDesc,ppDSV));
		}

		return hr;
	}
}

class SimpleTexture
{
public:
	ID3D11Texture2D*			m_pTex2D;
	ID3D11ShaderResourceView*	m_pSRV;
	ID3D11RenderTargetView*		m_pRTV;
	ID3D11DepthStencilView*		m_pDSV;
	
	SimpleTexture() : m_pTex2D(NULL), m_pDSV(NULL), m_pSRV(NULL), m_pRTV(NULL) {}

	HRESULT Initialize(ID3D11Device *pd3dDevice, DXGI_FORMAT format,UINT width,UINT height, UINT msaaCount, bool bSRV = true, bool bRTV = true, bool bDSV = false)
	{
		HRESULT hr;

		Release();

		V_RETURN(DirectXUtil::InitTexture2D(pd3dDevice,
						format,
						width,
						height,
						msaaCount,
						&this->m_pTex2D,
						bSRV ? &this->m_pSRV : NULL,
						bRTV ? &this->m_pRTV : NULL,
						bDSV ? &this->m_pDSV : NULL));

		return S_OK;
	}


	
	ID3D11Texture2D* GetTexture() const
	{
		return m_pTex2D;
	}
	ID3D11ShaderResourceView* GetShaderResourceView() const
	{
		return m_pSRV;
	}
	ID3D11RenderTargetView* GetRenderTargetView() const
	{
		return m_pRTV;
	}
	ID3D11DepthStencilView* GetDepthStencilView() const
	{
		return m_pDSV;
	}


	void Release()
	{
		SAFE_RELEASE(m_pTex2D);
		SAFE_RELEASE(m_pDSV);
		SAFE_RELEASE(m_pSRV);
		SAFE_RELEASE(m_pRTV);
	}
};

