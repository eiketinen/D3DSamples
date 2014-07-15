//----------------------------------------------------------------------------------
// File:        DeferredShadingMSAA\src/DeferredRenderer.h
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

#include "DeviceManager.h"
#include "DirectXUtil.h"
#include <vector>

class CFirstPersonCamera;
class ObjMeshDX;

enum ComplexDetectApproach
{
	CoverageMask = 0, Discontinuity
};
	
enum BRDF
{
	Lambert = 0, OrenNayar
};

class DeferredRendererController : public IVisualController
{
public:
	DeferredRendererController(CFirstPersonCamera *pCam, int iMSAACount = 1);
	virtual void Render(ID3D11Device*, ID3D11DeviceContext* pDeviceContext, ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV);
	virtual HRESULT DeviceCreated(ID3D11Device* pDevice);
	virtual void DeviceDestroyed();
	virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);

	void ChangeConfig(ID3D11Device* pDevice, const ComplexDetectApproach approach, const BRDF brdf, const bool bAdaptiveShading, const bool bShowComplexPixels, const bool bSeperatePass, const bool bPerSampleShader);
	int GetMSAASampleCount() const { return m_iMSAACount; }
	

protected:
	CFirstPersonCamera *pCamera;
	std::vector<ObjMeshDX*> m_Meshes;

	int					m_iMSAACount;
	ComplexDetectApproach m_Approach;
	bool				m_bSeperateComplexPass;
	bool				m_bUsePerSamplePixelShader;

	// GBuffer resources
	ID3D11InputLayout*	m_pDefaultInputLayout;
	ID3D11VertexShader* m_pFillGBufVertexShader;
	ID3D11PixelShader*	m_pFillGBufPixelShader;
	
	// Mask Generation
	ID3D11VertexShader* m_pComplexMaskVertexShader;
	ID3D11PixelShader*	m_pComplexMaskPixelShader;

	// Lighting resources
	ID3D11VertexShader* m_pLightingVertexShader;
	ID3D11PixelShader*	m_pLightingPixelShader;
	ID3D11PixelShader*	m_pLightingPixelShaderPerSample;
	
	ID3D11Buffer*		m_pCBLighting;
	ID3D11Buffer*		m_pCBMSAAPass;
	ID3D11Buffer*		m_pCBConfig;
	
	// Textures
	SimpleTexture		m_TexGBuffer;
	SimpleTexture		m_TexGBuffer2;
	SimpleTexture		m_TexGBufResolved2;
	SimpleTexture		m_TexCoverageMask;
	
	ID3D11SamplerState*	m_pTextureSampler;
	ID3D11SamplerState*	m_pPointSampler;
	ID3D11DepthStencilState* m_pWriteStencilDSState;
	ID3D11DepthStencilState* m_pTestStencilDSState;
};