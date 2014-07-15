//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src/DeinterleavedTexturing.cpp
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

#include <DXUT.h>
#include <DXUTgui.h>
#include <DXUTmisc.h>
#include <DXUTCamera.h>
#include <DXUTSettingsDlg.h>
#include <SDKmisc.h>

#include "SSAO/Renderer.h"
#include "Scene3D.h"

//--------------------------------------------------------------------------------------
// Scene description
//--------------------------------------------------------------------------------------

#define FOVY (40.0f * D3DX_PI / 180.0f)
#define ZNEAR 0.01f
#define ZFAR 500.0f

#define MEDIA_PATH L"..\\..\\DeinterleavedTexturing\\media\\"

MeshDescriptor g_MeshDesc[] =
{
    {L"Sibenik",   MEDIA_PATH L"sibenik.sdkmesh", SCENE_NO_GROUND_PLANE,  SCENE_NO_SHADING,  SCENE_USE_FIRST_PERSON_CAMERA, 0.005f },
    {L"Fence",     MEDIA_PATH L"FenceSsaoTestScene.sdkmesh",  SCENE_NO_GROUND_PLANE, SCENE_NO_SHADING,  SCENE_USE_ORBITAL_CAMERA, 0.05f },
    {L"AT-AT",     MEDIA_PATH L"AT-AT.sdkmesh",   SCENE_USE_GROUND_PLANE, SCENE_NO_SHADING,  SCENE_USE_ORBITAL_CAMERA, 0.05f },
};

//--------------------------------------------------------------------------------------
// MSAA settings
//--------------------------------------------------------------------------------------

struct MSAADescriptor
{
    WCHAR Name[32];
    int SampleCount;
};

MSAADescriptor g_MSAADesc[] =
{
    {L"1x MSAA",   1},
    {L"2x MSAA",   2},
    {L"4x MSAA",   4},
    {L"8x MSAA",   8},
    {L"",          0},
};

//--------------------------------------------------------------------------------------
// GUI constants
//--------------------------------------------------------------------------------------

#define MAX_RADIUS_MULT 3.f
#define MAX_DEPTH_THRESHOLD 10000.f

enum
{
    IDC_TOGGLEFULLSCREEN = 1,
    IDC_TOGGLEREF,
    IDC_CHANGEDEVICE,
    IDC_CHANGESCENE,
    IDC_SHOW_COLORS,
    IDC_SHOW_AO,
    IDC_RADIUS_STATIC,
    IDC_RADIUS_SLIDER,
    IDC_DEPTH_THRESHOLD_STATIC,
    IDC_DEPTH_THRESHOLD_SLIDER,
    IDC_BIAS_STATIC,
    IDC_BIAS_SLIDER,
    IDC_EXPONENT_STATIC,
    IDC_EXPONENT_SLIDER,
    IDC_DEINTERLEAVE,
    IDC_RANDOMIZE,
    IDC_BLUR_AO,
    IDC_BLUR_SHARPNESS_STATIC,
    IDC_BLUR_SHARPNESS_SLIDER,
    IDC_PER_PIXEL_AO,
    IDC_PER_SAMPLE_AO,
    IDC_1xMSAA,
    IDC_2xMSAA,
    IDC_4xMSAA,
    IDC_8xMSAA,
};

enum
{
    MSAA_MODE_1X = 0,
    MSAA_MODE_2X,
    MSAA_MODE_4X,
    MSAA_MODE_8X,
};

#define UI_RADIUS_MULT      L"Radius multiplier: "
#define UI_AO_BIAS          L"Bias: "
#define UI_POW_EXPONENT     L"Power exponent: "
#define UI_BLUR_SHARPNESS   L"Blur sharpness: "

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

CFirstPersonCamera            g_FirstPersonCamera;      // A first-person camera
CModelViewerCamera            g_OrbitalCamera;          // An orbital viewing camera
CDXUTDialogResourceManager    g_DialogResourceManager;  // manager for shared resources of dialogs
CD3DSettingsDlg               g_SettingsDlg;            // Device settings dialog
CDXUTDialog                   g_HUD;                    // dialog for standard controls
CDXUTTextHelper*              g_pTxtHelper = NULL;
UINT                          g_TextLineHeight = 15;
bool                          g_DrawUI = true;
bool                          g_BlurAO = true;
bool                          g_UseDeinterleavedTexturing = true;
bool                          g_RandomizeSamples = true;
bool                          g_PerSampleAO = true;
UINT                          g_AllocatedVideoMemoryBytes = 0;
GFSDK_SSAO_Renderer           g_AORenderer;
GFSDK_SSAO_Parameters         g_AOParams;

D3D11_VIEWPORT                g_FullViewport;
UINT                          g_BackBufferWidth = 0;
UINT                          g_BackBufferHeight = 0;
SceneRenderer                 g_pSceneRenderer;

ID3D11Texture2D*              g_ColorTexture;
ID3D11RenderTargetView*       g_ColorRTV;
ID3D11ShaderResourceView*     g_ColorSRV;
ID3D11Texture2D*              g_NormalTexture;
ID3D11RenderTargetView*       g_NormalRTV;
ID3D11ShaderResourceView*     g_NormalSRV;
ID3D11Texture2D*              g_DepthStencilTexture;
ID3D11ShaderResourceView*     g_DepthStencilSRV;
ID3D11DepthStencilView*       g_DepthStencilDSV;

SceneMesh g_SceneMeshes[ARRAYSIZE(g_MeshDesc)];

struct Scene
{
    Scene()
        : pMesh(NULL)
    {
    }
    SceneMesh *pMesh;
};
Scene g_Scenes[ARRAYSIZE(g_MeshDesc)];
bool g_UseOrbitalCamera = true;

int g_CurrentSceneId = 0;
int g_MSAACurrentSettings = MSAA_MODE_2X;
bool g_RenderTargetsDirty = false;

//--------------------------------------------------------------------------------------
class TextRenderer
{
public:
    void Init(CDXUTDialogResourceManager* pManager)
    {
        const UINT NumTextLines = 14;
        const UINT QuadWidth = 300;
        const float QuadOpacity = 0.2f;
        m_BackgroundQuad.Init(pManager);
        m_BackgroundQuad.SetLocation(0,0);
        m_BackgroundQuad.SetSize(QuadWidth, g_TextLineHeight * NumTextLines);
        m_BackgroundQuad.SetBackgroundColors(D3DCOLOR_COLORVALUE(0,0,0,QuadOpacity));
    }

    void DrawText()
    {
        g_pTxtHelper->Begin();

        g_pTxtHelper->SetInsertionPos(5, 5);
        g_pTxtHelper->SetForegroundColor(D3DXCOLOR(0.f, 0.f, 0.f, 1.f));

        g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());
        g_pTxtHelper->DrawFormattedTextLine(L"FPS: %.1f, VSync: %s" , DXUTGetFPS(), !DXUTIsVsyncEnabled() ? L"On" : L"Off");
        g_pTxtHelper->DrawFormattedTextLine(L"AO Resolution: %d x %d", g_BackBufferWidth, g_BackBufferHeight);
        g_pTxtHelper->DrawFormattedTextLine(L"Allocated Video Memory: %d MB\n", g_AORenderer.GetAllocatedVideoMemoryBytes() / (1024*1024));

        g_pTxtHelper->DrawFormattedTextLine(L"GPU Times Per Frame (ms):");
        g_pTxtHelper->DrawFormattedTextLine(L"  RenderAO: %.2f", g_AORenderer.GetGPUTimeInMS(GPU_TIME_TOTAL));
        g_pTxtHelper->DrawFormattedTextLine(L"    LinearZ: %.2f", g_AORenderer.GetGPUTimeInMS(GPU_TIME_LINEAR_Z));
        g_pTxtHelper->DrawFormattedTextLine(L"    DeinterleaveZ: %.2f", g_AORenderer.GetGPUTimeInMS(GPU_TIME_DEINTERLEAVE_Z));
        g_pTxtHelper->DrawFormattedTextLine(L"    ReconstructN: %.2f", g_AORenderer.GetGPUTimeInMS(GPU_TIME_RECONSTRUCT_NORMAL));
        g_pTxtHelper->DrawFormattedTextLine(L"    GenerateAO: %.2f", g_AORenderer.GetGPUTimeInMS(GPU_TIME_GENERATE_AO));
        g_pTxtHelper->DrawFormattedTextLine(L"    ReinterleaveAO: %.2f", g_AORenderer.GetGPUTimeInMS(GPU_TIME_REINTERLEAVE_AO));
        g_pTxtHelper->DrawFormattedTextLine(L"    BlurX: %.2f", g_AORenderer.GetGPUTimeInMS(GPU_TIME_BLURX));
        g_pTxtHelper->DrawFormattedTextLine(L"    BlurY: %.2f", g_AORenderer.GetGPUTimeInMS(GPU_TIME_BLURY));

        g_pTxtHelper->End();
    }

    void OnRender(float fElapsedTime)
    {
        m_BackgroundQuad.OnRender(fElapsedTime);

        DrawText();
    }

private:
    CDXUTDialog m_BackgroundQuad;
};
TextRenderer g_TextRenderer;

//--------------------------------------------------------------------------------------
void InitAOParams(GFSDK_SSAO_Parameters &AOParams)
{
    AOParams = GFSDK_SSAO_Parameters();
    AOParams.Radius = 2.f;
    AOParams.Bias = 0.2f;
    AOParams.PowerExponent = 2.f;
    AOParams.Blur.Sharpness = 8.f;
    AOParams.Output.BlendMode = GFSDK_SSAO_OVERWRITE_RGB;
    AOParams.Output.MSAAMode = GFSDK_SSAO_PER_SAMPLE_AO;
}

//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
void ReleaseRenderTargets()
{
    SAFE_RELEASE(g_ColorTexture);
    SAFE_RELEASE(g_ColorRTV);
    SAFE_RELEASE(g_ColorSRV);

    SAFE_RELEASE(g_NormalTexture);
    SAFE_RELEASE(g_NormalRTV);
    SAFE_RELEASE(g_NormalSRV);
}

//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
void CreateRenderTargets(ID3D11Device* pd3dDevice)
{
    ReleaseRenderTargets();

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width              = g_BackBufferWidth;
    texDesc.Height             = g_BackBufferHeight;
    texDesc.ArraySize          = 1;
    texDesc.MiscFlags          = 0;
    texDesc.MipLevels          = 1;
    texDesc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.Usage              = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags     = NULL;

    // Allocate MSAA color buffer
    texDesc.SampleDesc.Count   = g_MSAADesc[g_MSAACurrentSettings].SampleCount;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;

    pd3dDevice->CreateTexture2D(&texDesc, NULL, &g_ColorTexture);
    pd3dDevice->CreateRenderTargetView(g_ColorTexture, NULL, &g_ColorRTV);
    pd3dDevice->CreateShaderResourceView(g_ColorTexture, NULL, &g_ColorSRV);

    // Allocate MSAA normal buffer
    texDesc.Format             = DXGI_FORMAT_R11G11B10_FLOAT;

    pd3dDevice->CreateTexture2D(&texDesc, NULL, &g_NormalTexture);
    pd3dDevice->CreateRenderTargetView(g_NormalTexture, NULL, &g_NormalRTV);
    pd3dDevice->CreateShaderResourceView(g_NormalTexture, NULL, &g_NormalSRV);
}

//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
void ReleaseDepthBuffer()
{
    SAFE_RELEASE(g_DepthStencilTexture);
    SAFE_RELEASE(g_DepthStencilSRV);
    SAFE_RELEASE(g_DepthStencilDSV);
}

//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
void CreateDepthBuffer(ID3D11Device* pd3dDevice)
{
    ReleaseDepthBuffer();

    // Create a hardware-depth texture that can be fetched from a shader.
    // To do so, use a TYPELESS format and set the D3D11_BIND_SHADER_RESOURCE flag.
    // D3D10.0 did not allow creating such a depth texture with SampleCount > 1.
    // This is now possible since D3D10.1.
    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.ArraySize          = 1;
    texDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags     = NULL;
    texDesc.Width              = g_BackBufferWidth;
    texDesc.Height             = g_BackBufferHeight;
    texDesc.MipLevels          = 1;
    texDesc.MiscFlags          = NULL;
    texDesc.SampleDesc.Count   = g_MSAADesc[g_MSAACurrentSettings].SampleCount;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage              = D3D11_USAGE_DEFAULT;
    texDesc.Format             = DXGI_FORMAT_R24G8_TYPELESS;
    pd3dDevice->CreateTexture2D(&texDesc, NULL, &g_DepthStencilTexture);

    // Create a depth-stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    if (g_MSAADesc[g_MSAACurrentSettings].SampleCount > 1)
    {
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    }
    dsvDesc.Texture2D.MipSlice = 0;
    dsvDesc.Flags = 0; // new in D3D11
    pd3dDevice->CreateDepthStencilView(g_DepthStencilTexture, &dsvDesc, &g_DepthStencilDSV);

    // Create a shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    if (g_MSAADesc[g_MSAACurrentSettings].SampleCount > 1)
    {
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
    }
    pd3dDevice->CreateShaderResourceView(g_DepthStencilTexture, &srvDesc, &g_DepthStencilSRV);
}

//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
void ResizeScreenSizedBuffers(ID3D11Device* pd3dDevice)
{
    CreateRenderTargets(pd3dDevice);
    CreateDepthBuffer(pd3dDevice);
}

//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
    assert(pDeviceSettings->ver == DXUT_D3D11_DEVICE);

    // Do not allocate any depth buffer in DXUT
    pDeviceSettings->d3d11.AutoCreateDepthStencil = false;

    // For the first device created if it is a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if (s_bFirstTime)
    {
        s_bFirstTime = false;
        if ((DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
            (DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
             pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE))
        {
            DXUTDisplaySwitchingToREFWarning(pDeviceSettings->ver);
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
    return true;
}

//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
void LoadScenes(ID3D11Device* pd3dDevice)
{
    int SceneId = 0;

    // Load the sdkmesh data files
    for (int i = 0; i < ARRAYSIZE(g_MeshDesc); ++i)
    {
        if (FAILED(g_SceneMeshes[i].OnCreateDevice(pd3dDevice, g_MeshDesc[i])))
        {
            MessageBox(NULL, L"Unable to create mesh", L"ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
            PostQuitMessage(0);
        }
        g_Scenes[SceneId++].pMesh = &g_SceneMeshes[i];
    }
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    HRESULT hr;

    DXUTTRACE(L"OnD3D11CreateDevice called\n");

    SetCursor(LoadCursor(0, IDC_ARROW));

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext(); // does not addref
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext) );
    V_RETURN( g_SettingsDlg.OnD3D11CreateDevice(pd3dDevice) );
    g_pTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, g_TextLineHeight);

    // Setup orbital camera
    D3DXVECTOR3 vecEye(0.0f, 2.0f, 0.0f);
    D3DXVECTOR3 vecAt (0.0f, 0.0f, 0.0f);
    g_OrbitalCamera.SetViewParams(&vecEye, &vecAt);
    g_OrbitalCamera.SetRadius(1.5f, 0.01f);

    // Setup first-person camera
    D3DXVECTOR3 sibenikVecEye(0.0960150138f, 0.0273544509f, -0.0185411610f);
    D3DXVECTOR3 sibenikVecAt (-0.623801112f, -0.649074197f, -0.174454257f);
    g_FirstPersonCamera.SetViewParams(&sibenikVecEye, &sibenikVecAt);
    g_FirstPersonCamera.SetEnablePositionMovement(1);
    g_FirstPersonCamera.SetScalers(0.001f, 0.05f);

    // Load Scene3D.fx
    g_pSceneRenderer.OnCreateDevice(pd3dDevice);

    // Load meshes and bin files
    LoadScenes(pd3dDevice);

    GFSDK_SSAO_Status status;
    status = g_AORenderer.Create(pd3dDevice);
    assert(status == GFSDK_SSAO_OK);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// SwapChain has changed and may have new attributes such as size.
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    HRESULT hr;

    DXUTTRACE(L"OnD3D11ResizedSwapChain called\n");

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc) );
    V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc) );

    g_BackBufferWidth   = pBackBufferSurfaceDesc->Width;
    g_BackBufferHeight  = pBackBufferSurfaceDesc->Height;

    g_FullViewport.TopLeftX = 0.f;
    g_FullViewport.TopLeftY = 0.f;
    g_FullViewport.MinDepth = 0.f;
    g_FullViewport.MaxDepth = 1.f;
    g_FullViewport.Width    = (FLOAT)g_BackBufferWidth;
    g_FullViewport.Height   = (FLOAT)g_BackBufferHeight;

    // Setup the camera's projection parameters
    float AspectRatio  = (float)g_BackBufferWidth / (float)g_BackBufferHeight;
    g_OrbitalCamera.SetProjParams (FOVY, AspectRatio, ZNEAR, ZFAR);
    g_OrbitalCamera.SetWindow     (g_BackBufferWidth, g_BackBufferHeight);
    g_OrbitalCamera.SetButtonMasks(MOUSE_LEFT_BUTTON, MOUSE_WHEEL, 0);

    g_FirstPersonCamera.SetProjParams (FOVY, AspectRatio, ZNEAR, ZFAR);
    g_FirstPersonCamera.SetRotateButtons( 1, 1, 1 );

    UINT HudWidth = 256;
    float HudOpacity = 0.32f;
    g_HUD.SetLocation(g_BackBufferWidth - HudWidth, 0);
    g_HUD.SetSize    (HudWidth, g_BackBufferHeight);
    g_HUD.SetBackgroundColors(D3DCOLOR_COLORVALUE(0,0,0,HudOpacity));

    // Allocate our own screen-sized buffers, as the SwapChain only contains a non-MSAA color buffer.
    ResizeScreenSizedBuffers(pd3dDevice);

    return hr;
}

//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
    SceneMesh *pSceneMesh = g_Scenes[g_CurrentSceneId].pMesh;
    g_UseOrbitalCamera = pSceneMesh && pSceneMesh->UseOrbitalCamera();

    if (g_UseOrbitalCamera)
    {
        g_OrbitalCamera.FrameMove(fElapsedTime);
    }
    else
    {
        g_FirstPersonCamera.FrameMove(fElapsedTime);
    }
}

//--------------------------------------------------------------------------------------
void RenderAOFromMesh(ID3D11Device* pd3dDevice, 
                      ID3D11DeviceContext* pd3dImmediateContext,
                      ID3D11RenderTargetView* pBackBufferRTV,
                      SceneMesh *pMesh)
{
    UINT SampleCount = g_MSAADesc[g_MSAACurrentSettings].SampleCount;
    SceneViewInfo ViewInfo;

    pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dImmediateContext->RSSetViewports(1, &g_FullViewport);

    //--------------------------------------------------------------------------------------
    // Clear render target and depth buffer
    //--------------------------------------------------------------------------------------
    float BgColor[4] = { 1.0f, 1.0f, 1.0f };
    pd3dImmediateContext->ClearRenderTargetView(g_ColorRTV, BgColor);
    pd3dImmediateContext->ClearDepthStencilView(g_DepthStencilDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

    //--------------------------------------------------------------------------------------
    // Render color and depth with the Scene3D class
    //--------------------------------------------------------------------------------------
    pd3dImmediateContext->OMSetRenderTargets(1, &g_ColorRTV, g_DepthStencilDSV);

    if (g_UseOrbitalCamera)
    {
        ViewInfo.WorldViewMatrix = (*g_OrbitalCamera.GetWorldMatrix()) * (*g_OrbitalCamera.GetViewMatrix());
        ViewInfo.ProjectionMatrix = *g_OrbitalCamera.GetProjMatrix();
    }
    else
    {
        D3DXMATRIX WorldMatrix;
        D3DXMatrixRotationX(&WorldMatrix, -D3DX_PI * 0.5f);
        ViewInfo.WorldViewMatrix = WorldMatrix * (*g_FirstPersonCamera.GetViewMatrix());
        ViewInfo.ProjectionMatrix = *g_FirstPersonCamera.GetProjMatrix();
    }

    g_pSceneRenderer.OnFrameRender(&ViewInfo, pMesh);

    //--------------------------------------------------------------------------------------
    // Render the SSAO
    //--------------------------------------------------------------------------------------

    GFSDK_SSAO_InputDepthData InputDepthData;
    InputDepthData.pFullResDepthTextureSRV = g_DepthStencilSRV;
    InputDepthData.pProjectionMatrix = (CONST FLOAT*)ViewInfo.ProjectionMatrix;
    InputDepthData.ProjectionMatrixLayout = GFSDK_SSAO_ROW_MAJOR_ORDER;
    InputDepthData.MetersToViewSpaceUnits = pMesh->GetSceneScale();

    GFSDK_SSAO_Status status;
    status = g_AORenderer.RenderAO(pd3dImmediateContext, &InputDepthData, &g_AOParams, g_ColorRTV);
    assert(status == GFSDK_SSAO_OK);

    //--------------------------------------------------------------------------------------
    // Copy/resolve colors to the 1xAA backbuffer
    //--------------------------------------------------------------------------------------

    pd3dImmediateContext->OMSetRenderTargets(1, &pBackBufferRTV, NULL);

    ID3D11Texture2D* pBackBufferTexture;
    pBackBufferRTV->GetResource((ID3D11Resource**)&pBackBufferTexture);

    if (SampleCount > 1)
    {
        pd3dImmediateContext->ResolveSubresource(pBackBufferTexture, 0, g_ColorTexture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    }
    else
    {
        pd3dImmediateContext->CopyResource(pBackBufferTexture, g_ColorTexture);
    }

    SAFE_RELEASE(pBackBufferTexture);
}

//--------------------------------------------------------------------------------------
// Callback function that renders the frame.  This function sets up the rendering 
// matrices and renders the scene and UI.
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, 
                                 double fTime, float fElapsedTime, void* pUserContext)
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if (g_SettingsDlg.IsActive())
    {
        g_SettingsDlg.OnRender(fElapsedTime);
        return;
    }

    // Reallocate the render targets and depth buffer if the MSAA mode has been changed in the GUI.
    if (g_RenderTargetsDirty)
    {
        g_RenderTargetsDirty = false;
        ResizeScreenSizedBuffers(pd3dDevice);
    }

    SceneMesh *pMesh = g_Scenes[g_CurrentSceneId].pMesh;

    g_AOParams.UseDeinterleavedTexturing = g_UseDeinterleavedTexturing;
    g_AOParams.RandomizeSamples = g_RandomizeSamples;
    g_AOParams.Blur.Enable = g_BlurAO;

    ID3D11RenderTargetView* pBackBufferRTV = DXUTGetD3D11RenderTargetView(); // does not addref

    if (pMesh)
    {
        RenderAOFromMesh(pd3dDevice, 
                         pd3dImmediateContext,
                         pBackBufferRTV,
                         pMesh);
    }

    //--------------------------------------------------------------------------------------
    // Render the GUI
    //--------------------------------------------------------------------------------------
    if (g_DrawUI)
    {
        g_HUD.OnRender(fElapsedTime);
        g_TextRenderer.OnRender(fElapsedTime);
    }
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
    DXUTTRACE(L"OnD3D11ReleasingSwapChain called\n");

    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles.
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
    DXUTTRACE(L"OnD3D11DestroyDevice called\n");

    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_SettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE(g_pTxtHelper);

    g_pSceneRenderer.OnDestroyDevice();

    for (int i = 0; i < ARRAYSIZE(g_MeshDesc); i++)
    {
        g_SceneMeshes[i].OnDestroyDevice();
    }

    ReleaseRenderTargets();
    ReleaseDepthBuffer();

    g_AORenderer.Release();
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
                          bool* pbNoFurtherProcessing, void* pUserContext)
{
    // Always allow dialog resource manager calls to handle global messages
    // so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
    if (*pbNoFurtherProcessing)
    {
        return 0;
    }

    if (g_SettingsDlg.IsActive())
    {
        g_SettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
    if (*pbNoFurtherProcessing)
    {
        return 0;
    }

    // Pass all windows messages to camera so it can respond to user input
    if (g_UseOrbitalCamera)
    {
        g_OrbitalCamera.HandleMessages(hWnd, uMsg, wParam, lParam);
    }
    else
    {
        g_FirstPersonCamera.HandleMessages(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
    switch (nControlID)
    {
        case IDC_TOGGLEFULLSCREEN:   DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:          DXUTToggleREF();        break;
           
        case IDC_CHANGEDEVICE:
        {
            g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive()); 
            break;
        }
        case IDC_CHANGESCENE:
        {
            CDXUTComboBox* pComboBox = (CDXUTComboBox*) pControl;
            g_CurrentSceneId = pComboBox->GetSelectedIndex();
            break;
        }
        case IDC_BLUR_AO:
        {
            g_BlurAO = g_HUD.GetCheckBox(nControlID)->GetChecked();
            break;
        }
        case IDC_DEINTERLEAVE:
        {
            g_UseDeinterleavedTexturing = g_HUD.GetCheckBox(nControlID)->GetChecked();
            break;
        }
        case IDC_RANDOMIZE:
        {
            g_RandomizeSamples = g_HUD.GetCheckBox(nControlID)->GetChecked();
            break;
        }
        case IDC_RADIUS_SLIDER:
        {
            g_AOParams.Radius = (float) g_HUD.GetSlider(IDC_RADIUS_SLIDER)->GetValue() * MAX_RADIUS_MULT / 100.0f;

            WCHAR sz[100];
            StringCchPrintf(sz, 100, UI_RADIUS_MULT L"%0.2f", g_AOParams.Radius); 
            g_HUD.GetStatic(IDC_RADIUS_STATIC)->SetText(sz);

            break;
        }
        case IDC_BIAS_SLIDER:
        {
            g_AOParams.Bias = (float) g_HUD.GetSlider(IDC_BIAS_SLIDER)->GetValue() / 1000.f;

            WCHAR sz[100];
            StringCchPrintf(sz, 100, UI_AO_BIAS L"%g", g_AOParams.Bias); 
            g_HUD.GetStatic(IDC_BIAS_STATIC)->SetText(sz);

            break;
        }
        case IDC_EXPONENT_SLIDER: 
        {
            g_AOParams.PowerExponent = (float)g_HUD.GetSlider(IDC_EXPONENT_SLIDER)->GetValue() / 100.0f;

            WCHAR sz[100];
            StringCchPrintf(sz, 100, UI_POW_EXPONENT L"%0.2f", g_AOParams.PowerExponent);
            g_HUD.GetStatic(IDC_EXPONENT_STATIC)->SetText(sz);

            break;
        }
        case IDC_BLUR_SHARPNESS_SLIDER: 
        {
            g_AOParams.Blur.Sharpness = (float)g_HUD.GetSlider(IDC_BLUR_SHARPNESS_SLIDER)->GetValue() / 100.0f;

            WCHAR sz[100];
            StringCchPrintf(sz, 100, UI_BLUR_SHARPNESS L"%0.2f", g_AOParams.Blur.Sharpness);
            g_HUD.GetStatic(IDC_BLUR_SHARPNESS_STATIC)->SetText(sz);

            break;
        }
        case IDC_PER_PIXEL_AO:
        {
            g_AOParams.Output.MSAAMode = GFSDK_SSAO_PER_PIXEL_AO;
            break;
        }
        case IDC_PER_SAMPLE_AO:
        {
            g_AOParams.Output.MSAAMode = GFSDK_SSAO_PER_SAMPLE_AO;
            break;
        }
        case IDC_1xMSAA:
        case IDC_2xMSAA:
        case IDC_4xMSAA:
        case IDC_8xMSAA:
        {
            g_RenderTargetsDirty = true;
            g_MSAACurrentSettings = nControlID - IDC_1xMSAA;
            assert(g_MSAACurrentSettings >= MSAA_MODE_1X);
            assert(g_MSAACurrentSettings <= MSAA_MODE_8X);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
    if (bKeyDown)
    {
        switch (nChar)
        {
            case 'U':
                g_DrawUI = !g_DrawUI;
                break;
        }
    }
}

//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, 
                       bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, 
                       int xPos, int yPos, void* pUserContext)
{
}

//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
    return true;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void InitGUI()
{
    // Initialize dialogs
    g_SettingsDlg.Init(&g_DialogResourceManager);
    g_HUD.Init(&g_DialogResourceManager);
    g_HUD.SetCallback(OnGUIEvent);
    g_TextRenderer.Init(&g_DialogResourceManager);
    
    int iY = 10; 
    g_HUD.AddButton  (IDC_TOGGLEFULLSCREEN,   L"Toggle full screen" ,   35, iY, 160, 22);
    g_HUD.AddButton  (IDC_TOGGLEREF,          L"Toggle REF (F3)"    ,   35, iY += 24, 160, 22, VK_F3);
    g_HUD.AddButton  (IDC_CHANGEDEVICE,       L"Change device (F2)" ,   35, iY += 24, 160, 22, VK_F2);
    iY += 20;

    g_HUD.AddCheckBox(IDC_DEINTERLEAVE,     L"Deinterleaved Texturing", 35, iY += 28, 125, 22, g_UseDeinterleavedTexturing);
    g_HUD.AddCheckBox(IDC_RANDOMIZE,        L"Randomize Samples",       35, iY += 28, 125, 22, g_RandomizeSamples);
    g_HUD.AddCheckBox(IDC_BLUR_AO,          L"Blur AO",                 35, iY += 28, 125, 22, g_BlurAO);
    iY += 24;

    CDXUTComboBox *pComboBox;
    g_HUD.AddComboBox(IDC_CHANGESCENE,    35, iY += 24, 160, 22, 'M', false, &pComboBox);
    for (int i = 0; i < ARRAYSIZE(g_MeshDesc); i++)
    {
        pComboBox->AddItem(g_MeshDesc[i].Name, NULL);
    }
    iY += 24;

    WCHAR sz[100];
    int dy = 20;
    StringCchPrintf(sz, 100, UI_RADIUS_MULT L"%0.2f", g_AOParams.Radius); 
    g_HUD.AddStatic(IDC_RADIUS_STATIC, sz, 35, iY += dy, 125, 22);
    g_HUD.AddSlider(IDC_RADIUS_SLIDER, 50, iY += dy, 100, 22, 0, 100, int(g_AOParams.Radius / MAX_RADIUS_MULT * 100));

    StringCchPrintf(sz, 100, UI_AO_BIAS L"%g", g_AOParams.Bias); 
    g_HUD.AddStatic(IDC_BIAS_STATIC, sz, 35, iY += dy, 125, 22);
    g_HUD.AddSlider(IDC_BIAS_SLIDER, 50, iY += dy, 100, 22, 0, 500, int(g_AOParams.Bias * 1000));

    StringCchPrintf(sz, 100, UI_POW_EXPONENT L"%0.2f", g_AOParams.PowerExponent); 
    g_HUD.AddStatic(IDC_EXPONENT_STATIC, sz, 35, iY += dy, 125, 22);
    g_HUD.AddSlider(IDC_EXPONENT_SLIDER, 50, iY += dy, 100, 22, 0, 400, (int)(100.0f*g_AOParams.PowerExponent));

    StringCchPrintf(sz, 100, UI_BLUR_SHARPNESS L"%0.2f", g_AOParams.Blur.Sharpness); 
    g_HUD.AddStatic(IDC_BLUR_SHARPNESS_STATIC, sz, 35, iY += dy, 125, 22);
    g_HUD.AddSlider(IDC_BLUR_SHARPNESS_SLIDER, 50, iY += dy, 100, 22, 0, 1600, (int)(100.0f*g_AOParams.Blur.Sharpness));

    UINT ButtonGroup = 0;
    iY += 24;
    g_HUD.AddRadioButton( IDC_1xMSAA,       ButtonGroup, L"1X MSAA",                35, iY += 24, 125, 22,  (g_MSAACurrentSettings == MSAA_MODE_1X) );
    g_HUD.AddRadioButton( IDC_2xMSAA,       ButtonGroup, L"2X MSAA",                35, iY += 24, 125, 22,  (g_MSAACurrentSettings == MSAA_MODE_2X) );
    g_HUD.AddRadioButton( IDC_4xMSAA,       ButtonGroup, L"4X MSAA",                35, iY += 24, 125, 22,  (g_MSAACurrentSettings == MSAA_MODE_4X) );
    g_HUD.AddRadioButton( IDC_8xMSAA,       ButtonGroup, L"8X MSAA",                35, iY += 24, 125, 22,  (g_MSAACurrentSettings == MSAA_MODE_8X) );

    ++ButtonGroup;
    iY += 24;
    g_HUD.AddRadioButton( IDC_PER_PIXEL_AO,   ButtonGroup, L"PER_PIXEL_AO",         35, iY += 24, 125, 22,  (g_AOParams.Output.MSAAMode == GFSDK_SSAO_PER_PIXEL_AO) );
    g_HUD.AddRadioButton( IDC_PER_SAMPLE_AO,  ButtonGroup, L"PER_SAMPLE_AO",        35, iY += 24, 125, 22,  (g_AOParams.Output.MSAAMode == GFSDK_SSAO_PER_SAMPLE_AO) );
}

//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HRESULT hr;
    V_RETURN(DXUTSetMediaSearchPath(MEDIA_PATH));

    // Enable run-time memory check for debug builds.
#if defined(DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove(OnFrameMove);
    DXUTSetCallbackKeyboard(OnKeyboard);
    DXUTSetCallbackMouse(OnMouse);
    DXUTSetCallbackMsgProc(MsgProc);
    DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
    DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

    // Set the D3D11 DXUT callbacks
    DXUTSetCallbackD3D11DeviceAcceptable  (IsD3D11DeviceAcceptable);
    DXUTSetCallbackD3D11DeviceCreated     (OnD3D11CreateDevice);
    DXUTSetCallbackD3D11SwapChainResized  (OnD3D11ResizedSwapChain);
    DXUTSetCallbackD3D11FrameRender       (OnD3D11FrameRender);
    DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
    DXUTSetCallbackD3D11DeviceDestroyed   (OnD3D11DestroyDevice);

    // Perform any application-level initialization here
    InitAOParams(g_AOParams);
    InitGUI();

    UINT Width = 1920;
    UINT Height = 1200;

    DXUTInit(true, true, NULL); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
    DXUTSetIsInGammaCorrectMode(false); // Do not use a SRGB back buffer for this sample
    DXUTCreateWindow(L"Deinterleaved Texturing");
    DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, Width, Height);
    //DXUTToggleFullScreen();

    DXUTMainLoop(); // Enter into the DXUT render loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}
