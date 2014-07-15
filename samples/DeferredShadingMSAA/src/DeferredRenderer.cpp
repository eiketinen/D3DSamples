//----------------------------------------------------------------------------------
// File:        DeferredShadingMSAA\src/DeferredRenderer.cpp
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
#include "DeferredRenderer.h"

#include "Camera.h"
#include <d3dcommon.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>

#include "ObjMeshDX.h"

//--------------------------------------------------------------------------------------
// Constant buffer structure
//--------------------------------------------------------------------------------------

struct LightPassConstants
{
    D3DXMATRIX	ProjectToView4x4;
    D3DXMATRIX	ViewToWorld4x4;
};

struct ConfigConstants
{
	int			UseDiscontinuity;
	int			AdaptiveShading;
	int			SeperateEdgePass;
	int			ShowComplexPixels;
	int			UseOrenNayar;
	float	pad;
	float	pad2;
	float	pad3;
};

struct MSAAPassConstants
{
	int		SecondPass;
	float	pad;
	float	pad2;
	float	pad3;
};

//--------------------------------------------------------------------------------------
// Deferred renderer implementation
//--------------------------------------------------------------------------------------
DeferredRendererController::DeferredRendererController(CFirstPersonCamera *pCam, int iMSAACount)
	: pCamera(pCam)
	, m_iMSAACount(iMSAACount)
	, m_pDefaultInputLayout(nullptr)
	, m_pFillGBufVertexShader(nullptr)
	, m_pFillGBufPixelShader(nullptr)
	, m_pLightingVertexShader(nullptr) 
	, m_pLightingPixelShader(nullptr)
	, m_pLightingPixelShaderPerSample(nullptr)
	, m_pTextureSampler(nullptr)
	, m_pCBLighting(nullptr)
	, m_pCBConfig(nullptr)
{
	m_Approach = Discontinuity;
	m_bSeperateComplexPass = true;
	m_bUsePerSamplePixelShader = false;
}


HRESULT DeferredRendererController::DeviceCreated(ID3D11Device* pDevice)
{
	HRESULT hr;

	// Load meshes
	ObjMeshDX* pMesh = new ObjMeshDX;
	pMesh->Create(pDevice, L"../../DeferredShadingMSAA/media/sponza/sponza.obj");
	m_Meshes.push_back(pMesh);
	
	// Compile the shaders to a model based on the feature level we acquired
    LPCSTR pszVSModel = "vs_5_0";
    LPCSTR pszPSModel = "ps_5_0";

	if(pDevice->GetFeatureLevel() != D3D_FEATURE_LEVEL_11_0)
		::MessageBoxA(NULL, "This sample requires D3D_FEATURE_LEVEL_11_0 to run.", "Error", MB_OK | MB_ICONERROR);

	// Init vertex shader
	ID3DBlob* pShaderBuffer = nullptr;
	DirectXUtil::CompileFromFile(L"../../DeferredShadingMSAA/src/shaders/FillGBuffer.hlsl", "VS", pszVSModel, &pShaderBuffer);
	V_RETURN(pDevice->CreateVertexShader(pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), NULL, &m_pFillGBufVertexShader));

	// Init input layout
	V_RETURN(pDevice->CreateInputLayout(ObjMeshDX::layout_Position_Normal_Texture, ObjMeshDX::numElements_layout_Position_Normal_Texture, pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), &m_pDefaultInputLayout));
	pShaderBuffer->Release();
	
	DirectXUtil::CompileFromFile(L"../../DeferredShadingMSAA/src/shaders/ComplexMask.hlsl", "QuadVS", pszVSModel, &pShaderBuffer);
	V_RETURN(pDevice->CreateVertexShader(pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), NULL, &m_pComplexMaskVertexShader));
	pShaderBuffer->Release();

	DirectXUtil::CompileFromFile(L"../../DeferredShadingMSAA/src/shaders/Lighting.hlsl", "QuadVS", pszVSModel, &pShaderBuffer);
	V_RETURN(pDevice->CreateVertexShader(pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), NULL, &m_pLightingVertexShader));
	pShaderBuffer->Release();
	

	// Init pixel shader
	DirectXUtil::CompileFromFile(L"../../DeferredShadingMSAA/src/shaders/FillGBuffer.hlsl", "PS", pszPSModel, &pShaderBuffer);
	V_RETURN(pDevice->CreatePixelShader(pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), NULL, &m_pFillGBufPixelShader));
	pShaderBuffer->Release();
	
	DirectXUtil::CompileFromFile(L"../../DeferredShadingMSAA/src/shaders/ComplexMask.hlsl", "PS", pszPSModel, &pShaderBuffer);
	V_RETURN(pDevice->CreatePixelShader(pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), NULL, &m_pComplexMaskPixelShader));
	pShaderBuffer->Release();
	
	DirectXUtil::CompileFromFile(L"../../DeferredShadingMSAA/src/shaders/Lighting.hlsl", "PS", pszPSModel, &pShaderBuffer);
	V_RETURN(pDevice->CreatePixelShader(pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), NULL, &m_pLightingPixelShader));
	pShaderBuffer->Release();
	
	DirectXUtil::CompileFromFile(L"../../DeferredShadingMSAA/src/shaders/Lighting.hlsl", "PS_PerSample", pszPSModel, &pShaderBuffer);
	V_RETURN(pDevice->CreatePixelShader(pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), NULL, &m_pLightingPixelShaderPerSample));
	pShaderBuffer->Release();

    // Create the constant buffers
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.ByteWidth = sizeof(LightPassConstants);
		V_RETURN(pDevice->CreateBuffer(&bd, NULL, &m_pCBLighting));
	}
	
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.ByteWidth = sizeof(MSAAPassConstants);
		V_RETURN(pDevice->CreateBuffer(&bd, NULL, &m_pCBMSAAPass));
	}

	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.ByteWidth = sizeof(ConfigConstants);

		ConfigConstants configConsts = { m_Approach == Discontinuity ? 1 : 0, 1, m_bSeperateComplexPass ? 1 : 0, 0, 1 };
		D3D11_SUBRESOURCE_DATA rsData;
		ZeroMemory(&rsData, sizeof(D3D11_SUBRESOURCE_DATA));
		rsData.pSysMem = &configConsts;
		V_RETURN(pDevice->CreateBuffer(&bd, &rsData, &m_pCBConfig));;
	}

	// Create Depth Stencil State
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

		dsDesc.DepthEnable = false;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		
		dsDesc.StencilEnable = true;
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace = dsDesc.FrontFace;
		V_RETURN(pDevice->CreateDepthStencilState(&dsDesc, &m_pWriteStencilDSState));
	}
	
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		
		dsDesc.DepthEnable = false;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

		dsDesc.StencilEnable = true;
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = 0;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		dsDesc.BackFace = dsDesc.FrontFace;
		V_RETURN(pDevice->CreateDepthStencilState(&dsDesc, &m_pTestStencilDSState));
	}
	

	// Create Samplers
	{
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
		sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.MaxAnisotropy = 16;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = FLT_MAX;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		V_RETURN(pDevice->CreateSamplerState(&sampDesc, &m_pTextureSampler));
	}

	{
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.MaxAnisotropy = 0;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = FLT_MAX;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		V_RETURN(pDevice->CreateSamplerState(&sampDesc, &m_pPointSampler));
	}

	return S_OK;
}

void DeferredRendererController::Render(ID3D11Device*, ID3D11DeviceContext* pDeviceContext, ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV) 
{ 
	if(!pCamera) return;

	D3DXVECTOR3 viewForward = *pCamera->GetWorldAhead();
	D3DXMATRIX viewMatrix = *pCamera->GetViewMatrix();
	D3DXMATRIX projMatrix = *pCamera->GetProjMatrix();
	D3DXMATRIX worldMatrix;
	D3DXMatrixScaling(&worldMatrix, 0.1f, 0.1f, 0.1f);
	D3DXMATRIX worldViewProjMatrix = worldMatrix * viewMatrix * projMatrix;

	// Clear the back buffer
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pDeviceContext->ClearRenderTargetView(pRTV, clearColor);
	pDeviceContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);

	{
		// Fill the G-Buffer
		ID3D11RenderTargetView* pGBufRTV[] = { m_TexGBuffer.GetRenderTargetView(), m_TexGBuffer2.GetRenderTargetView(), m_TexCoverageMask.GetRenderTargetView() };
		pDeviceContext->ClearRenderTargetView(pGBufRTV[0], clearColor);
		pDeviceContext->ClearRenderTargetView(pGBufRTV[1], clearColor);
		pDeviceContext->ClearRenderTargetView(pGBufRTV[2], clearColor);

		pDeviceContext->OMSetRenderTargets(3, pGBufRTV, pDSV);
		pDeviceContext->OMSetDepthStencilState(NULL, 0);

		pDeviceContext->IASetInputLayout(m_pDefaultInputLayout);
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pDeviceContext->VSSetShader(m_pFillGBufVertexShader, NULL, 0);
		pDeviceContext->PSSetShader(m_pFillGBufPixelShader, NULL, 0);

		pDeviceContext->PSSetSamplers(0, 1, &m_pTextureSampler);

        for(UINT32 i = 0; i < m_Meshes.size(); i++)
			m_Meshes[i]->Draw(pDeviceContext, worldMatrix, viewMatrix, worldViewProjMatrix, 0);

		ID3D11RenderTargetView* pClearRTV[] = { NULL, NULL, NULL };
		pDeviceContext->OMSetRenderTargets(3, pClearRTV, NULL);
	}

	// Resolve GBuffer
	if(m_Approach == CoverageMask)
		pDeviceContext->ResolveSubresource((ID3D11Resource*)m_TexGBufResolved2.GetTexture(), 0, (ID3D11Resource*)m_TexGBuffer2.GetTexture(), 0, DXGI_FORMAT_R16G16B16A16_FLOAT);

	if(m_bSeperateComplexPass)
	{
		pDeviceContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);

		pDeviceContext->OMSetDepthStencilState(m_pWriteStencilDSState, 1);
		pDeviceContext->OMSetRenderTargets(1, &pRTV, pDSV);
		
		pDeviceContext->VSSetShader(m_pComplexMaskVertexShader, NULL, 0);
		pDeviceContext->PSSetShader(m_pComplexMaskPixelShader, NULL, 0);
		
		ID3D11Buffer* pConstBuf[] = { m_pCBConfig };
		pDeviceContext->PSSetConstantBuffers(1, 1, pConstBuf);

		ID3D11ShaderResourceView* pGBufSRV[] = { m_TexGBuffer.GetShaderResourceView(), m_TexGBuffer2.GetShaderResourceView(), m_TexGBufResolved2.GetShaderResourceView() };
		pDeviceContext->PSSetShaderResources(0, 3, pGBufSRV);
		pDeviceContext->PSSetSamplers(0, 1, &m_pPointSampler);

		pDeviceContext->Draw(3, 0);
		
		ID3D11RenderTargetView* pClearRTV[] = { NULL };
		pDeviceContext->OMSetRenderTargets(1, pClearRTV, NULL);

		ID3D11ShaderResourceView* pClearRSV[] = { NULL, NULL, NULL };
		pDeviceContext->PSSetShaderResources(0, 3, pClearRSV);
	}

	{
		// Calculate lighting
		pDeviceContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

		if(m_bSeperateComplexPass)
		{
			pDeviceContext->OMSetDepthStencilState(m_pTestStencilDSState, 1);
		}

		pDeviceContext->OMSetRenderTargets(1, &pRTV, pDSV);
		pDeviceContext->VSSetShader(m_pLightingVertexShader, NULL, 0);
		pDeviceContext->PSSetShader(m_pLightingPixelShader, NULL, 0);

		// Set up lighting pass matrices
		{
			D3D11_MAPPED_SUBRESOURCE MappedResource;
			pDeviceContext->Map(m_pCBLighting, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			LightPassConstants* pLightPassConsts = (LightPassConstants*)MappedResource.pData;

			D3DXMATRIX ProjToView;
			D3DXMatrixInverse(&ProjToView, NULL, pCamera->GetProjMatrix());
			D3DXMatrixTranspose(&pLightPassConsts->ProjectToView4x4, &ProjToView);
		
			D3DXMATRIX ViewToWorld;
			D3DXMatrixInverse(&ViewToWorld, NULL, pCamera->GetViewMatrix());
			D3DXMatrixTranspose(&pLightPassConsts->ViewToWorld4x4, &ViewToWorld);

			pDeviceContext->Unmap(m_pCBLighting, 0);
		}

		// Set up msaa pass
		{
			D3D11_MAPPED_SUBRESOURCE MappedResource;
			pDeviceContext->Map(m_pCBMSAAPass, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			MSAAPassConstants* pMSAAConsts = (MSAAPassConstants*)MappedResource.pData;

			pMSAAConsts->SecondPass = 0;

			pDeviceContext->Unmap(m_pCBMSAAPass, 0);
		}
		
		ID3D11Buffer* pConstBuf[] = { m_pCBLighting, m_pCBConfig, m_pCBMSAAPass };
		pDeviceContext->PSSetConstantBuffers(0, 3, pConstBuf);

		ID3D11ShaderResourceView* pGBufSRV[] = {
			m_TexGBuffer.GetShaderResourceView(),
			m_TexGBuffer2.GetShaderResourceView(),
			m_TexGBufResolved2.GetShaderResourceView(),
			m_TexCoverageMask.GetShaderResourceView(),
		};
		pDeviceContext->PSSetShaderResources(0, 4, pGBufSRV);
		pDeviceContext->PSSetSamplers(0, 1, &m_pPointSampler);

		pDeviceContext->Draw(3, 0);
		
		if(m_bSeperateComplexPass)
		{
			pDeviceContext->OMSetDepthStencilState(m_pTestStencilDSState, 0);

			// Set up msaa pass
			{
				D3D11_MAPPED_SUBRESOURCE MappedResource;
				pDeviceContext->Map(m_pCBMSAAPass, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
				MSAAPassConstants* pMSAAConsts = (MSAAPassConstants*)MappedResource.pData;

				pMSAAConsts->SecondPass = 1;

				pDeviceContext->Unmap(m_pCBMSAAPass, 0);
			}
		
			if(m_bUsePerSamplePixelShader)
				pDeviceContext->PSSetShader(m_pLightingPixelShaderPerSample, NULL, 0);
			
			ID3D11Buffer* pConstBuf[] = { m_pCBLighting, m_pCBConfig, m_pCBMSAAPass };
			pDeviceContext->PSSetConstantBuffers(0, 3, pConstBuf);

			pDeviceContext->Draw(3, 0);
		}
		
		ID3D11RenderTargetView* pClearRTV[] = { NULL };
		pDeviceContext->OMSetRenderTargets(1, pClearRTV, NULL);

		ID3D11ShaderResourceView* pClearRSV[] = { NULL, NULL, NULL, NULL };
		pDeviceContext->PSSetShaderResources(0, 4, pClearRSV);
	}

}

void DeferredRendererController::BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc) 
{     
	(void)pDevice;
	(void)pBackBufferSurfaceDesc;

	auto width = pBackBufferSurfaceDesc->Width;
	auto height = pBackBufferSurfaceDesc->Height;

	// Init GBuffer RTV
	m_TexGBuffer.Initialize(pDevice, DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, m_iMSAACount, true, true, false);
	m_TexGBuffer2.Initialize(pDevice, DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, m_iMSAACount, true, true, false);
	m_TexCoverageMask.Initialize(pDevice, DXGI_FORMAT_R16_UINT, width, height, m_iMSAACount, true, true, false);
	m_TexGBufResolved2.Initialize(pDevice, DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, true, false, false);
}

void DeferredRendererController::ChangeConfig(ID3D11Device* pDevice, const ComplexDetectApproach approach, const BRDF brdf, const bool bAdaptiveShading, const bool bShowComplexPixels, const bool bSeperatePass, const bool bPerSampleShader)
{
	m_Approach = approach;
	m_bSeperateComplexPass = bSeperatePass;
	m_bUsePerSamplePixelShader = bPerSampleShader;

	ID3D11DeviceContext* pDeviceContext = NULL;
	pDevice->GetImmediateContext(&pDeviceContext);

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	pDeviceContext->Map(m_pCBConfig, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	ConfigConstants* pConfigConsts = (ConfigConstants*)MappedResource.pData;
	
	pConfigConsts->UseDiscontinuity = m_Approach == Discontinuity ? 1 : 0;
	pConfigConsts->AdaptiveShading = bAdaptiveShading ? 1 : 0;
	pConfigConsts->SeperateEdgePass = m_bSeperateComplexPass ? 1 : 0;
	pConfigConsts->ShowComplexPixels = bShowComplexPixels ? 1 : 0;
	pConfigConsts->UseOrenNayar = brdf == OrenNayar ? 1 : 0;

	pDeviceContext->Unmap(m_pCBConfig, 0);
}

void DeferredRendererController::DeviceDestroyed() 
{
	SAFE_RELEASE(m_pDefaultInputLayout);
	SAFE_RELEASE(m_pFillGBufVertexShader);
	SAFE_RELEASE(m_pFillGBufPixelShader);
	
	SAFE_RELEASE(m_pComplexMaskVertexShader);
	SAFE_RELEASE(m_pComplexMaskPixelShader);
	
	SAFE_RELEASE(m_pLightingVertexShader);
	SAFE_RELEASE(m_pLightingPixelShader);
	SAFE_RELEASE(m_pLightingPixelShaderPerSample);
	
	SAFE_RELEASE(m_pCBLighting);
	SAFE_RELEASE(m_pCBMSAAPass);
	SAFE_RELEASE(m_pCBConfig);
	
	SAFE_RELEASE(m_pTextureSampler);
	SAFE_RELEASE(m_pPointSampler);
	SAFE_RELEASE(m_pWriteStencilDSState);
	SAFE_RELEASE(m_pTestStencilDSState);
	
	m_TexGBuffer.Release();
	m_TexGBuffer2.Release();
	m_TexGBufResolved2.Release();
	m_TexCoverageMask.Release();
}

