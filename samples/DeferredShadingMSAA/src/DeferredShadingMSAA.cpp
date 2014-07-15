//----------------------------------------------------------------------------------
// File:        DeferredShadingMSAA\src/DeferredShadingMSAA.cpp
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
#include "DirectXUtil.h"
#include "DeferredRenderer.h"

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

CFirstPersonCamera              g_Camera;                   // A model viewing camera for mesh scene
DeviceManager*                  g_DeviceManager = NULL;

static bool g_bRenderHUD        = true;

static float g_fCameraClipNear  = 0.5f;
static float g_fCameraClipFar   = 1000.0f;

// Testing
ComplexDetectApproach g_Approach = Discontinuity;
static BRDF g_bBRDF = OrenNayar;
static bool	g_bAdaptiveShading = true;
static bool	g_bDrawComplexPixels = false;
static bool	g_bSeperateComplexPass = true;
static bool g_bUsePerSamplePixelShader = false;

class AntTweakBarVisualController: public IVisualController
{
public:
	DeferredRendererController* m_pDeferredRenderer;

protected:
	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
	{
		if(uMsg == WM_KEYDOWN)
		{        
			int iKeyPressed = static_cast<int>(wParam);

			switch(iKeyPressed)
			{
			case VK_TAB:
				g_bRenderHUD = !g_bRenderHUD;
				return 0;
				break;

			case VK_SPACE:
				return 0;
				break;
			}

		}

		if(g_bRenderHUD || uMsg == WM_KEYDOWN || uMsg == WM_CHAR)
		{
			if(TwEventWin(hWnd, uMsg, wParam, lParam))
			{
				if(g_bAdaptiveShading || !g_bSeperateComplexPass)
					g_bUsePerSamplePixelShader = false;

				m_pDeferredRenderer->ChangeConfig(g_DeviceManager->GetDevice(), g_Approach, g_bBRDF, g_bAdaptiveShading, g_bDrawComplexPixels, g_bSeperateComplexPass, g_bUsePerSamplePixelShader);
				return 0; // Event has been handled by AntTweakBar
			}
		}

		return g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);
	}

	virtual void Animate(double fElapsedTimeSeconds)
	{
		g_Camera.FrameMove((float)fElapsedTimeSeconds);
	}

	void RenderText()
	{
		TwBeginText(2, 0, 0, 0);
		const unsigned int color = 0xffffc0ff;
		char msg[256];

		double averageTime = 1000.0f * g_DeviceManager->GetAverageFrameTime();
		//double fps = (averageTime > 0) ? 1.0 / averageTime : 0.0;

		sprintf_s(msg, "Average Frame Time: %f ms", averageTime);
		TwAddTextLine(msg, color, 0);

		TwEndText();
	}

	virtual void Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView*, ID3D11DepthStencilView*) 
	{ 
		if (g_bRenderHUD)
		{
			RenderText();
			TwDraw();
		}
	}

	virtual HRESULT DeviceCreated(ID3D11Device* pDevice) 
	{ 
		TwInit(TW_DIRECT3D11, pDevice);
		InitDialogs();

		static bool s_bFirstTime = true;
		if (s_bFirstTime)
		{
			s_bFirstTime = false;

			D3DXVECTOR3 eyePt = D3DXVECTOR3(-112.0f, 0.0f, 0.0f);
			D3DXVECTOR3 lookAtPt = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
			g_Camera.SetViewParams(&eyePt, &lookAtPt);
			g_Camera.SetScalers(0.005f, 100.0f);
			g_Camera.SetRotateButtons(true, false, false, false);
		}

		return S_OK;
	}

	virtual void DeviceDestroyed() 
	{ 
		TwTerminate();
	}

	virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc) 
	{
		(void)pDevice;
		TwWindowSize(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

		// Setup the camera's projection parameters    
		float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
		g_Camera.SetProjParams(D3DX_PI / 4, fAspectRatio, g_fCameraClipNear, g_fCameraClipFar);

	}

	static void TW_CALL BingoButtonCallback(void *clientData)
	{ 
		(void)clientData;
		// do something
	}

	void InitDialogs()
	{
		TwBar* bar = TwNewBar("barMain");
		TwDefine("barMain label='Settings' color='192 255 192' text=dark alpha=128 size='420 160' valueswidth=fit");
		
		
		{ // Debug rendering mode
			TwEnumVal enumModeTypeEV[] = {
				{ 0, "Coverage Mask" },
				{ 1, "Sample Discontinuity" },
			};
			TwType enumModeType = TwDefineEnum("Excellent Enum", enumModeTypeEV, sizeof(enumModeTypeEV) / sizeof(enumModeTypeEV[0]));
			TwAddVarRW(bar, "Complex Pixel Detection Approach", enumModeType, &g_Approach, "keyIncr=g keyDecr=G group=Settings");
		}
		
		{ // Debug rendering mode
			TwEnumVal enumModeTypeEV[] = {
				{ 0, "Lambertion Diffuse" },
				{ 1, "Oren Nayar Diffuse" },
			};
			TwType enumModeType = TwDefineEnum("Excellent Enum 2", enumModeTypeEV, sizeof(enumModeTypeEV) / sizeof(enumModeTypeEV[0]));
			TwAddVarRW(bar, "BRDFs", enumModeType, &g_bBRDF, "keyIncr=b keyDecr=B group=Settings");
		}
		
		TwAddVarRW(bar, "Adaptively Shade Complex Pixels", TW_TYPE_BOOLCPP, &g_bAdaptiveShading, "group=Settings");
		TwAddVarRW(bar, "Mark Complex Pixels", TW_TYPE_BOOLCPP, &g_bDrawComplexPixels, "group=Settings");
		TwAddVarRW(bar, "Seperate Complex Pixel Pass", TW_TYPE_BOOLCPP, &g_bSeperateComplexPass, "group=Settings");
		TwAddVarRW(bar, "Per-Sample Shading", TW_TYPE_BOOLCPP, &g_bUsePerSamplePixelShader, "group=Settings");
	}
};

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	(void)hInstance;
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;

	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	g_DeviceManager = new DeviceManager();
	AntTweakBarVisualController atbController;
	auto sceneController = DeferredRendererController(&g_Camera, 4);
	atbController.m_pDeferredRenderer = &sceneController;
	g_DeviceManager->AddControllerToFront(&sceneController);
	g_DeviceManager->AddControllerToFront(&atbController);

	DeviceCreationParameters deviceParams;
	deviceParams.swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	deviceParams.swapChainSampleCount = sceneController.GetMSAASampleCount();
	deviceParams.startFullscreen = false;
	deviceParams.backBufferWidth = 1280;
	deviceParams.backBufferHeight = 800;
#if defined(DEBUG) | defined(_DEBUG)
	deviceParams.createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	if(FAILED(g_DeviceManager->CreateWindowDeviceAndSwapChain(deviceParams, L"NVIDIA Graphics Library : Deferred Shading MSAA")))
	{
		MessageBox(NULL, L"Cannot initialize the D3D11 device with the requested parameters", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	g_DeviceManager->MessageLoop();
	g_DeviceManager->Shutdown();

	delete g_DeviceManager;


	return 0;
}

