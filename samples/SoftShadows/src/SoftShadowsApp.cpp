//----------------------------------------------------------------------------------
// File:        SoftShadows\src/SoftShadowsApp.cpp
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
#include "SoftShadowsApp.h"

////////////////////////////////////////////////////////////////////////////////
// UI Control IDs
////////////////////////////////////////////////////////////////////////////////
enum UIControlIDs 
{
    IDC_TOGGLEFULLSCREEN = 1,
    IDC_TOGGLEWARP,
    IDC_TOGGLEREF,
    IDC_CHANGEDEVICE,
    IDC_PRESET_STATIC,
    IDC_PRESET_COMBOBOX,
    IDC_LIGHTSIZE_STATIC,
    IDC_LIGHTSIZE_SLIDER,
    IDC_TECHNIQUE_STATIC,
    IDC_TECHNIQUE_COMBOBOX,
    IDC_TOGGLE_TEXTURE,
    IDC_TOGGLE_VISDEPTH,
};

////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////
static const float GROUND_PLANE_RADIUS = 8.0f;

static const int HUD_WIDTH = 200;
static const int HUD_HEIGHT = 200;

static const int UI_WIDTH = 250;
static const int UI_HEIGHT = 300;

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::SoftShadowsApp()
////////////////////////////////////////////////////////////////////////////////
SoftShadowsApp::SoftShadowsApp()
    : DXUTApp11()
    , m_dialogResourceManager()
    , m_d3dSettingsDlg()
    , m_hud()
    , m_sampleUI()
    , m_knightMesh()
    , m_podiumMesh()
    , m_renderer()
    , m_textHelper(nullptr)
    , m_showHelp(false)
    , m_drawUI(true)
{
    auto self = static_cast<void *>(this);

    DXUTSetMediaSearchPath(L"..\\..\\SoftShadows\\media");

    // Initialize dialogs
    m_d3dSettingsDlg.Init(&m_dialogResourceManager);
    m_sampleUI.Init(&m_dialogResourceManager);

    // Initialize HUD    
    m_hud.Init(&m_dialogResourceManager);
    m_hud.SetCallback(onGuiEvent, self);
    m_hud.SetSize(HUD_WIDTH, HUD_HEIGHT);

    int iY = 10;
    m_hud.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22);
    m_hud.AddButton(IDC_TOGGLEWARP, L"Toggle WARP (F4)", 35, iY += 24, 125, 22, VK_F4);
    m_hud.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3);
    m_hud.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2);

    iY += 24;
    CDXUTComboBox *presetComboBox;
    m_hud.AddStatic(IDC_PRESET_STATIC, L"Blocker Search / PCF", 30, iY += 24, 135, 22);
    m_hud.AddComboBox(IDC_PRESET_COMBOBOX, 35, iY += 24, 135, 22, 'P', false, &presetComboBox);
    presetComboBox->AddItem(L"25 / 25 Poisson", nullptr);
    presetComboBox->AddItem(L"32 / 64 Poisson", nullptr);
    presetComboBox->AddItem(L"100 / 100 Poisson", nullptr);
    presetComboBox->AddItem(L"64 / 128 Poisson", nullptr);
    presetComboBox->AddItem(L"49 / 225 Regular", nullptr);
    presetComboBox->SetSelectedByIndex(static_cast<UINT>(m_renderer.getPcssPreset()));

    iY += 24;
    WCHAR str[100];
    StringCchPrintf(str, 100, L"Light Size: %.2f", m_renderer.getLightRadiusWorld());
    m_hud.AddStatic(IDC_LIGHTSIZE_STATIC, str, 35, iY += 24, 125, 22);
    m_hud.AddSlider(IDC_LIGHTSIZE_SLIDER, 50, iY += 24, 100, 22, 0, 100, int(m_renderer.getLightRadiusWorld() * 100));

    iY += 24;
    CDXUTComboBox *techniqueComboBox;
    m_hud.AddStatic(IDC_TECHNIQUE_STATIC, L"Shadow Technique", 30, iY += 24, 135, 22);
    m_hud.AddComboBox(IDC_TECHNIQUE_COMBOBOX, 35, iY += 24, 135, 22, 'T', false, &techniqueComboBox);
    techniqueComboBox->AddItem(L"None", nullptr);
    techniqueComboBox->AddItem(L"PCSS", nullptr);
    techniqueComboBox->AddItem(L"PCF", nullptr);
    techniqueComboBox->AddItem(L"CHS", nullptr);
    techniqueComboBox->SetSelectedByIndex(static_cast<UINT>(m_renderer.getShadowTechnique()));

    iY += 24;
    m_hud.AddCheckBox(IDC_TOGGLE_TEXTURE, L"Use Texture", 50, iY += 24, 175, 22, true, 'X');
    m_hud.AddCheckBox(IDC_TOGGLE_VISDEPTH, L"Visualize Depth", 50, iY += 24, 175, 22, m_renderer.visualizeDepthTexture(), 'V');

    // Initialize sample UI
    m_sampleUI.SetCallback(onSampleGuiEvent, self);
    iY = 10;
    m_sampleUI.SetSize(UI_WIDTH, iY + 30);

    // Initialize the window and device
    DXUTInit(true, true, nullptr); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
    DXUTCreateWindow(L"Soft Shadows");
    DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 1024, 768);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::~SoftShadowsApp()
////////////////////////////////////////////////////////////////////////////////
SoftShadowsApp::~SoftShadowsApp()
{
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::modifyDeviceSettings()
////////////////////////////////////////////////////////////////////////////////
bool SoftShadowsApp::modifyDeviceSettings(DXUTDeviceSettings *deviceSettings)
{
    // Uncomment this to get debug information from D3D11
    deviceSettings->d3d11.CreateFlags &= !D3D11_CREATE_DEVICE_DEBUG;

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool firstTime = true;
    if (firstTime)
    {
        firstTime = false;
        if ((DXUT_D3D11_DEVICE == deviceSettings->ver &&
              deviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE))
        {
            DXUTDisplaySwitchingToREFWarning(deviceSettings->ver);
        }
    }

#if defined(_DEBUG)
    deviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Disable VSync
    deviceSettings->d3d11.SyncInterval = 0;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onFrameMove()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onFrameMove(double time, float elapsedTime)
{
    m_renderer.onFrameMove(time, elapsedTime);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onWindowsMessage()
////////////////////////////////////////////////////////////////////////////////
LRESULT SoftShadowsApp::onWindowsMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool *stopProcessing)
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *stopProcessing = m_dialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
    if (*stopProcessing)
        return 0;

    // Pass messages to settings dialog if its active
    if (m_d3dSettingsDlg.IsActive())
    {
        m_d3dSettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *stopProcessing = m_hud.MsgProc(hWnd, uMsg, wParam, lParam);
    if (*stopProcessing)
        return 0;
    *stopProcessing = m_sampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
    if (*stopProcessing)
        return 0;

    static unsigned mouseMask = 0;
    if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK)
    {
        mouseMask |= 1;
    }
    if (uMsg == WM_LBUTTONUP)
    {
        mouseMask &= ~1;
    }

    // Pass all remaining windows messages to camera so it can respond to user input
    if (mouseMask & 1) // left button pressed
        m_renderer.getLightCamera().HandleMessages(hWnd, uMsg, wParam, lParam);
    else
        m_renderer.getCamera().HandleMessages(hWnd, uMsg, wParam, lParam);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onKeyboardMessage()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onKeyboardMessage(UINT nChar, bool keyDown, bool altDown)
{
    if (keyDown)
    {
        switch(nChar)
        {
        case VK_F1:
            m_showHelp = !m_showHelp;
            break;
        case 'A':
            m_renderer.changeWorldWidthOffset(0.1f);
            break;
        case 'D':
            m_renderer.changeWorldWidthOffset(-0.1f);
            break;
        case 'W':
            m_renderer.changeWorldHeightOffset(-0.1f);
            break;
        case 'S':
            m_renderer.changeWorldHeightOffset(0.1f);
            break;
        case 'U':
            m_drawUI = !m_drawUI;
            break;
        case 'R':
            m_renderer.reloadEffect();
            break;
        case VK_ESCAPE:
            PostQuitMessage(0);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onCreateDevice()
////////////////////////////////////////////////////////////////////////////////
HRESULT SoftShadowsApp::onCreateDevice(
    ID3D11Device *device,
    const DXGI_SURFACE_DESC *backBufferSurfaceDesc)
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN(m_dialogResourceManager.OnD3D11CreateDevice(device, pd3dImmediateContext));
    V_RETURN(m_d3dSettingsDlg.OnD3D11CreateDevice(device));
    m_textHelper.reset(new CDXUTTextHelper(device, pd3dImmediateContext, &m_dialogResourceManager, 15));

    // Load the meshes
    WCHAR str[MAX_PATH];
    V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"goof_knight.sdkmesh"));
    V_RETURN(m_knightMesh.Create(device, str));
    V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"podium.sdkmesh"));
    V_RETURN(m_podiumMesh.Create(device, str));

    // Build the scene
    D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);
    m_renderer.setKnightMesh(m_renderer.addMeshInstance(&m_knightMesh, identity, L"Knight"));
    m_renderer.setPodiumMesh(m_renderer.addMeshInstance(&m_podiumMesh, identity, L"Podium"));

    // Setup the ground plane
    auto knight = m_renderer.getKnightMesh();
    D3DXVECTOR3 extents = knight->getExtents();
    D3DXVECTOR3 center = knight->getCenter();
    float height = center.y - extents.y;
    m_renderer.setGroundPlane(height, GROUND_PLANE_RADIUS);

    // Create the renderer device resources
    m_renderer.onCreateDevice(device, backBufferSurfaceDesc);    

#if defined(_DEBUG)
    ID3D11Debug *debug;
    device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debug));
    m_debug.reset(debug);
#endif

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onResizedSwapChain()
////////////////////////////////////////////////////////////////////////////////
HRESULT SoftShadowsApp::onResizedSwapChain(
    ID3D11Device *device,
    IDXGISwapChain *swapChain,
    const DXGI_SURFACE_DESC *backBufferSurfaceDesc)
{
    HRESULT hr;

    V_RETURN(m_dialogResourceManager.OnD3D11ResizedSwapChain(device, backBufferSurfaceDesc));
    V_RETURN(m_d3dSettingsDlg.OnD3D11ResizedSwapChain(device, backBufferSurfaceDesc));

    // Update the renderer
    m_renderer.onResizeSwapChain(device, swapChain,backBufferSurfaceDesc);

    m_hud.SetLocation(backBufferSurfaceDesc->Width - m_hud.GetWidth(), 0);
    m_hud.SetSize(HUD_WIDTH, HUD_HEIGHT);
    m_sampleUI.SetLocation(backBufferSurfaceDesc->Width - UI_WIDTH, m_hud.GetHeight() + 10);
    m_sampleUI.SetSize(UI_WIDTH, UI_HEIGHT);

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onReleasingSwapChain()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onReleasingSwapChain()
{
    m_dialogResourceManager.OnD3D11ReleasingSwapChain();
    m_renderer.onReleasingSwapChain();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onDestroyDevice()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onDestroyDevice()
{
    m_renderer.onDestroyDevice();
    m_knightMesh.Destroy();
    m_podiumMesh.Destroy();
    
    m_dialogResourceManager.OnD3D11DestroyDevice();
    m_d3dSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    m_textHelper.reset();

    if (m_debug)
    {
        m_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        m_debug.reset();
    }
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onFrameRender()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onFrameRender(ID3D11Device *device, ID3D11DeviceContext *deviceContext, double time, float elapsedTime)
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if (m_d3dSettingsDlg.IsActive())
    {
        m_d3dSettingsDlg.OnRender(elapsedTime);
        return;
    }

    // Render
    m_renderer.onRender(device, deviceContext, time, elapsedTime);

    DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
    m_hud.OnRender(elapsedTime);
    m_sampleUI.OnRender(elapsedTime);
    renderText();
    DXUT_EndPerfEvent();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onGuiEvent()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onGuiEvent(UINT eventId, int controlID, CDXUTControl *control)
{
    WCHAR str[100];
    switch (controlID)
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;
        case IDC_TOGGLEWARP:
            DXUTToggleWARP();
            break;
        case IDC_TOGGLEREF:
            DXUTToggleREF();
            break;
        case IDC_CHANGEDEVICE:
            m_d3dSettingsDlg.SetActive(!m_d3dSettingsDlg.IsActive());
            break;
        case IDC_LIGHTSIZE_SLIDER:
            m_renderer.setLightRadiusWorld((float)m_hud.GetSlider(IDC_LIGHTSIZE_SLIDER)->GetValue() / 100);
            StringCchPrintf(str, 100, L"Light Size: %.2f", m_renderer.getLightRadiusWorld());
            m_hud.GetStatic(IDC_LIGHTSIZE_STATIC)->SetText(str);
            break;
        case IDC_TECHNIQUE_COMBOBOX:
            m_renderer.setShadowTechnique(static_cast<SoftShadowsRenderer::ShadowTechnique>(
                m_hud.GetComboBox(IDC_TECHNIQUE_COMBOBOX)->GetSelectedIndex()));
            break;
        case IDC_TOGGLE_TEXTURE:
            m_renderer.useTexture(m_hud.GetCheckBox(IDC_TOGGLE_TEXTURE)->GetChecked());
            break;
        case IDC_TOGGLE_VISDEPTH:
            m_renderer.visualizeDepthTexture(m_hud.GetCheckBox(IDC_TOGGLE_VISDEPTH)->GetChecked());
            break;
        case IDC_PRESET_COMBOBOX:
            m_renderer.setPcssPreset(static_cast<SoftShadowsRenderer::PcssPreset>(
                m_hud.GetComboBox(IDC_PRESET_COMBOBOX)->GetSelectedIndex()));
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onSampleGuiEvent()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onSampleGuiEvent(UINT eventId, int controlId, CDXUTControl *control)
{    
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::renderText()
//
// Renders the help and statistics text
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::renderText()
{
    m_textHelper->Begin();
    m_textHelper->SetInsertionPos(5, 5);
    m_textHelper->SetForegroundColor(D3DXCOLOR(0.4f, 0.8f, 0.0f, 1.0f));
    m_textHelper->DrawTextLine(DXUTGetFrameStats(true));
    m_textHelper->DrawTextLine(DXUTGetDeviceStats());
    UINT64 numIndices = 0;
    UINT64 numVerts = 0;
    UINT lightRes = 0;
    m_renderer.getSceneStats(numIndices, numVerts, lightRes);
    UINT kTris = (UINT)(numIndices / 3000L);
    m_textHelper->DrawFormattedTextLine(L"NumTris: %dk - LightRes: %d", kTris, lightRes);

    if (m_showHelp)
    {
        UINT backBufferHeight = (DXUTIsAppRenderingWithD3D9()) ? DXUTGetD3D9BackBufferSurfaceDesc()->Height : DXUTGetDXGIBackBufferSurfaceDesc()->Height;
        m_textHelper->SetInsertionPos(2, backBufferHeight - 15 * 6);
        m_textHelper->DrawTextLine(L"Controls:");

        m_textHelper->SetInsertionPos(20, backBufferHeight - 15 * 5);
        m_textHelper->DrawTextLine(
            L"Rotate light:  Left mouse button\n"
            L"Zoom camera:  Mouse wheel scroll\n"
            L"Rotate camera: Right mouse button");

        m_textHelper->SetInsertionPos(250, backBufferHeight - 15 * 5);
        m_textHelper->DrawTextLine(
            L"Hide help: F1\n" 
            L"Quit: ESC\n");
    }
    else
    {
        m_textHelper->DrawTextLine(L"Press F1 for help");
    }

    m_textHelper->End();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onGuiEvent()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onGuiEvent(UINT eventId, int controlId, CDXUTControl *control, void *userContext)
{
    auto self = static_cast<SoftShadowsApp *>(userContext);
    self->onGuiEvent(eventId, controlId, control);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsApp::onSampleGuiEvent()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsApp::onSampleGuiEvent(UINT eventId, int controlId, CDXUTControl *control, void *userContext)
{
    auto self = static_cast<SoftShadowsApp *>(userContext);
    self->onSampleGuiEvent(eventId, controlId, control);
}