//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src/DeferredContexts11.cpp
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

/*

    Greetings Developer!  You have come here seeking information on deferred contexts.  The most interesting bits of code live in the various render strategy classes.

    renderers/* - Rendering the scene in different ways, with or without DC use
    utility/* - The basic renderer class with standard d3d initialization and methods, as well as the scene class managing the instances and their updates
    testing/*  - a simple braindead test harness for sampling and dumping frame data

    This file is only the UI and basic window creation and hookup interface.

    Questions?  bdudash at nvidia.com

*/

#include <Windows.h>
#include <process.h>
#include <Strsafe.h>
#include "SimpleOpt.h"
#include "DeferredContexts11.h"
#include "AntTweakBar.h"
#include "Camera.h"
#include "Scene.h"
#include "RendererBase.h"
#include "testing/AutomatedTestingHarness.h"

bool g_autoSim = false;    // if true, we are in an automated test run, so disable input and gui
AutomatedTestHarness g_testHarness;

D3DXVECTOR3                    g_SceneWorldSize = D3DXVECTOR3(g_fSceneRadius, g_fSceneRadius / 2.f, g_fSceneRadius);

DeviceManager*              g_DeviceManager = NULL;
CFirstPersonCamera          g_Camera;
Scene                        g_Scene = Scene(g_SceneWorldSize);

D3DXVECTOR3                    g_vDefaultEye(30.0f, 150.0f, -150.0f);
D3DXVECTOR3                    g_vDefaultLookAt(0.0f, 60.0f, 0.0f);
const static float            g_fCameraClipNear  = 1.0f;
const static float            g_fCameraClipFar   = 10000.0f;

static bool g_bRenderHUD        = true;
static bool g_bRenderTestHUD    = false;
static bool g_bHelpIconified = true;

// --------------------------
// UI Variables
// ---------------------------
bool                g_bUnifyVSPSCB = false;
bool                g_bVaryShaders = false;
bool                g_bUseVTF = false;
bool                g_bNoShadows = false;
bool                g_bMovingMeshes = true;
bool                g_bMovingLights = false;
bool                g_bDisableExecutes = false;
bool                g_bReuseCommandLists = false;
MT_RENDER_STRATEGY    g_activeRenderPath = RS_MT_BATCHED_INSTANCES;
int                    g_activeMesh = 3;                    // ui sets this directly
int                    g_sceneActiveMesh = g_activeMesh;    // the processing will update this once the scene matches the ui
int                    g_iNumInstances = 5000;
int                    g_iNumRenderThreads = 5;
int                    g_iNumUpdateThreads = 3;
float                g_MeshScale = 25.f;
WCHAR                g_wzMeshesInfo[260];// used only to indicate varied on the UI bar or the mesh being rendered
int                    g_iMeshPolys = 0;    // used only for the hud info bar
WCHAR                g_FileName[MAX_PATH];

// Some meshes useful for testing
const LPWSTR        g_meshes[] =
{
    L"..\\..\\deferredcontexts11\\assets\\instancingtest_2.x",
    L"..\\..\\deferredcontexts11\\assets\\instancingtest_12.x",
    L"..\\..\\deferredcontexts11\\assets\\instancingtest_24.x",
    L"..\\..\\deferredcontexts11\\assets\\instancingtest_112.x",
    L"..\\..\\deferredcontexts11\\assets\\instancingtest_264.x",
    L"..\\..\\deferredcontexts11\\assets\\instancingtest_510.x",
    L"..\\..\\deferredcontexts11\\assets\\instancingtest_1012.x",
};
const int g_numMeshes = sizeof(g_meshes) / sizeof(LPWSTR);

// !!!NOTE!!! This array must match the above mesh list and is seperate because UI requires special struct.
const TwEnumVal g_renderMeshEV[] =
{
    {0, "2 poly mesh"},
    {1, "12 poly mesh"},
    {2, "24 poly mesh"},
    {3, "112 poly mesh"},
    {4, "264 poly mesh"},
    {5, "510 poly mesh"},
    {6, "1012 poly mesh"},
};

// ---------------- Render strategies --------------------------------
#include "renderers\IC_Renderer.h"
#include "renderers\DC_BatchInstances_Renderer.h"
#include "renderers\IC_Instancing_Renderer.h"

const TwEnumVal g_renderStrategyEV[] =
{
    {RS_IMMEDIATE, "Immediate Context(IC)"},
    {RS_IC_INSTANCING, "IC w/ Instancing"},
    {RS_MT_BATCHED_INSTANCES, "Deferred Context(DC)"},
};

class AntTweakBarVisualController: public IVisualController
{
public:
    HWND hwnd;
    ID3D11Device* pd3dDevice;
    virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if(g_autoSim)
        {
            if(static_cast<int>(wParam) == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }

            return 0;
        }

        if(uMsg == WM_KEYDOWN)
        {
            int iKeyPressed = static_cast<int>(wParam);

            switch(iKeyPressed)
            {
            case VK_TAB:
                g_bRenderHUD = !g_bRenderHUD;
                return 0;
                break;

            case VK_ESCAPE:
                PostQuitMessage(0);
                break;

            case VK_F2:
            {
                g_bRenderTestHUD = !g_bRenderTestHUD;
                char option[30];
                sprintf_s(option, "barAutoTest visible=%s", g_bRenderTestHUD ? "true" : "false");
                TwDefine(option);
                break;
            }
            case VK_F1:
            {
                g_bHelpIconified = !g_bHelpIconified;
                char option[30];
                sprintf_s(option, "TW_HELP iconified=%s", g_bHelpIconified ? "true" : "false");
                TwDefine(option);
                break;
            }
            default:
            break;
            }

        }

        if(g_bRenderHUD || uMsg == WM_KEYDOWN || uMsg == WM_CHAR)
        {
            if(TwEventWin(hWnd, uMsg, wParam, lParam))
            {
                return 0; // Event has been handled by AntTweakBar
            }
        }

        return 1;
    }

    void RenderText(bool bAutomation = false)
    {
        TwBeginText(2, 0, 0, 0);
        unsigned int color = 0xffffc0ff;
        char msg[256];

        double averageTime = g_DeviceManager->GetAverageFrameTime();
        double fps = (averageTime > 0) ? 1.0 / averageTime : 0.0;

        char szMeshInfo[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, g_wzMeshesInfo, -1, szMeshInfo, MAX_PATH, NULL, FALSE);

        sprintf_s(msg, "%.1f FPS %d polys drawn from mesh file \"%s\"", fps, g_iMeshPolys, szMeshInfo);
        TwAddTextLine(msg, color, 0);

        if(bAutomation)
        {
            color = 0xFFFF0000;
            sprintf_s(msg, "Running Automated Testing, please wait... Processing event %d / %d. ESC to abort and exit.", g_testHarness.GetCurrentEventIndex(), g_testHarness.GetNumEvents());
            TwAddTextLine(msg, color, 0);
        }

        TwEndText();
    }

    virtual void Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView*, ID3D11DepthStencilView*)
    {
        RenderText(g_autoSim);

        if(g_bRenderHUD && !g_autoSim)
        {
            TwDraw();
        }
    }

    static void TW_CALL CopyStdStringToClient(std::string& destinationClientString, const std::string& sourceLibraryString)
    {
        destinationClientString = sourceLibraryString;
    }

    virtual HRESULT DeviceCreated(ID3D11Device* pDevice)
    {
        TwInit(TW_DIRECT3D11, pDevice);
        TwCopyStdStringToClientFunc(CopyStdStringToClient);
        InitDialogs();

        pd3dDevice = pDevice;
        pd3dDevice->AddRef();
        return S_OK;
    }

    virtual void DeviceDestroyed()
    {
        SAFE_RELEASE(pd3dDevice);
        TwTerminate();
    }

    virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
    {
        (void)pDevice;
        TwWindowSize(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
    }

    static void TW_CALL VaryMeshesCallBack(void* clientData)
    {
        (void)clientData;
        g_Scene.VaryMeshes();
        g_iMeshPolys = g_Scene.GetTotalPolys();
        StringCchCopy(g_wzMeshesInfo, 260, L"VARIED");
    }

    static void TW_CALL OpenMeshFileCallBack(void* clientData)
    {
        AntTweakBarVisualController* pController = reinterpret_cast<AntTweakBarVisualController*>(clientData);

        OPENFILENAME info;
        ZeroMemory(&info, sizeof(OPENFILENAME));
        info.lStructSize = sizeof(OPENFILENAME);
        info.hwndOwner = pController->hwnd;
        info.lpstrFilter = L"All files\0*.*\0\0";
        info.nFilterIndex = 1;
        info.lpstrFile = (LPWSTR)g_FileName;
        info.nMaxFile = 256;
        info.lpstrTitle = L"Open";
        info.nMaxFileTitle = sizeof(info.lpstrTitle);
        info.dwReserved = OFN_EXPLORER;


        if(GetOpenFileName(&info))
        {
            // mesh create code
            g_Scene.AddMeshToLoad(g_FileName, g_FileName);
            g_Scene.LoadQueuedContent(pController->pd3dDevice);
            g_Scene.SetAllInstancesToMeshW(g_FileName);
            g_iMeshPolys = g_Scene.GetTotalPolys();
            StringCchCopy(g_wzMeshesInfo, 260, g_FileName);

        }
    }

    // Start a batch of tests with the current settings
    static void TW_CALL StartTestsCallBack(void* clientData)
    {
        DC_UNREFERENCED_PARAM(clientData);
        g_testHarness.InitializeExperiments();
        g_autoSim = true;
    }

    void InitDialogs()
    {
        TwBar* bar = TwNewBar("barMain");
        TwDefine("barMain label='Settings' color='19 25 19' text=light alpha=128 size='400 350' valueswidth=fit");

        char options[2048];
        sprintf_s(options, "min=1 max=%d step=1 group=Basic help='Number of rendered models.'", g_iMaxInstances);
        TwAddVarRW(bar, "Instances", TW_TYPE_INT32, &g_iNumInstances, options);

        sprintf_s(options, "min=1 max=%d step=1 group=Basic help='Number of rendering threads.\nThis number only has an effect under DC rendering.'", g_iMaxNumRenderThreads);
        TwAddVarRW(bar, "Render Threads", TW_TYPE_INT32, &g_iNumRenderThreads, options);
        sprintf_s(options, "min=0 max=%d step=1 group=Basic help=`Number of threads that calculate objects's movement.`", g_iMaxNumUpdateThreads);
        TwAddVarRW(bar, "Update Threads", TW_TYPE_INT32, &g_iNumUpdateThreads, options);\

        TwType enumModeType = TwDefineEnum("Render Strategy", g_renderStrategyEV, sizeof(g_renderStrategyEV) / sizeof(g_renderStrategyEV[0]));
        TwAddVarRW(bar, "Render Strategy", enumModeType, &g_activeRenderPath, "group=Basic help=`Immediate(IC):\n   Immediate context.\nIC w/ Instancing:\n   Immediate context with instancing drawcalls. This strategy uses VTF. 'Vary Shader'/'Unify VSPS CB' does not take effect under this strategy.\nDeferred(DC):\n   Deferred contexts.`");

        TwAddVarRW(bar, "Mesh Scale", TW_TYPE_FLOAT, &g_MeshScale, "group=Mesh min=-100.0 max=100.0 help=`Mesh scaling factor.`");
        TwType enumModeTypeMesh = TwDefineEnum("Render Mesh", g_renderMeshEV, sizeof(g_renderMeshEV) / sizeof(g_renderMeshEV[0]));
        TwAddVarRW(bar, "Render Mesh", enumModeTypeMesh, &g_activeMesh, "group=Mesh help=`Selected mesh will be rendered.`");
        TwAddButton(bar, "Vary All Meshes", AntTweakBarVisualController::VaryMeshesCallBack, NULL, "group=Mesh help=`Vary rendered mesh.\nRendered mesh will be changed at each draw calls in IC/DC strategy. In Instance strategy, objects will be sorted by kinds of meshes before drawing.`");
        TwAddButton(bar, "Open Mesh...", AntTweakBarVisualController::OpenMeshFileCallBack, (void*)this, "group=Mesh help=`Open x/sdkmesh file.`");
        TwAddVarRW(bar, "Vary Shaders?", TW_TYPE_BOOLCPP, &g_bVaryShaders, "group=Mesh help=`Slightly defferent shaders will be set at each draw calls.`");

        TwAddVarRW(bar, "Unify VS PS ConstantBuffers?", TW_TYPE_BOOLCPP, &g_bUnifyVSPSCB, "group=Advanced help=`Instead of setting PS constant at each draw calls, PS constant parameter will be passed from VS. When using VTF, this option will be ignored.`");
        TwAddVarRW(bar, "Use VTF?", TW_TYPE_BOOLCPP, &g_bUseVTF, "group=Advanced  help=`Instead of setting VS/PS constant at each draw calls, use VTF texture for constant parameters.`");
        TwAddVarRW(bar, "Skip Executes?", TW_TYPE_BOOLCPP, &g_bDisableExecutes, "group=Advanced help=`Skip ExecuteCommandList() call. This option only has an effect under DC rendering.`");
        TwAddVarRW(bar, "Reuse Cmdlists?", TW_TYPE_BOOLCPP, &g_bReuseCommandLists, "group=Advanced help=`Instead of building command list, reuse previous frame's command list.  This option only has an effect under DC rendering.`");
        TwAddVarRW(bar, "Skip shadow?", TW_TYPE_BOOLCPP, &g_bNoShadows, "group=Advanced help=`Skip shadow pass.`");
        TwAddVarRW(bar, "Animating Meshes?", TW_TYPE_BOOLCPP, &g_bMovingMeshes, "group=Advanced help=`Animating Meshes.`");
        TwAddVarRW(bar, "Animating Lights?", TW_TYPE_BOOLCPP, &g_bMovingLights, "group=Advanced help=`Animating Lights.`");

        {
            TwBar* autoTestBar = TwNewBar("barAutoTest");
            TwDefine("barAutoTest label='Automated Testing' help='Settings for running an automated test' visible=false position='420 20' color='19 25 19' text=light alpha=128 size='200 250' valueswidth=fit");
            char options[260];
            sprintf_s(options, "min=1 max=%d step=1", g_iMaxInstances);
            TwAddVarRW(autoTestBar, "StartInstances", TW_TYPE_INT32, &g_testHarness.iStartingInstances, options);

            TwAddVarRW(autoTestBar, "MaxInstances", TW_TYPE_INT32, &g_testHarness.iMaxInstances, "min=1 max=100000 step=1");
            TwAddVarRW(autoTestBar, "# of Samples", TW_TYPE_INT32, &g_testHarness.instSteps, "min=1 max=1000 step=1 help='Number of test samples along range from StartInstances to MaxInstances'");
            TwAddVarRW(autoTestBar, "Log Increment?", TW_TYPE_BOOLCPP, &g_testHarness.bLogIncrement, "");

            sprintf_s(options, "min=1 max=%d step=1", g_iMaxNumRenderThreads);
            TwAddVarRW(autoTestBar, "Max Render Threads", TW_TYPE_INT32, &g_testHarness.maxThreads, options);
            sprintf_s(options, "min=0 max=%d step=1", g_iMaxNumUpdateThreads);
            TwAddVarRW(autoTestBar, "Max Update Threads", TW_TYPE_INT32, &g_testHarness.updateThreads, options);
            TwAddVarRW(autoTestBar, "Smoothing Frames", TW_TYPE_INT32, &g_testHarness.smoothFrames, "min=1 max=100 step=1");
            TwAddVarRW(autoTestBar, "Dead Frames", TW_TYPE_INT32, &g_testHarness.eventChangeFrames, "min=1 max=10 step=1 help='Some frames where we don't count perf between tests.'");
            TwAddVarRW(autoTestBar, "VTF?", TW_TYPE_BOOLCPP, &g_testHarness.bVTF, "");
            TwAddVarRW(autoTestBar, "VaryMeshes?", TW_TYPE_BOOLCPP, &g_testHarness.bVaryOnChange, "");
            TwAddVarRW(autoTestBar, "VaryShaders?", TW_TYPE_BOOLCPP, &g_testHarness.bVaryShaders, "");
            TwAddVarRW(autoTestBar, "Unify VSPS CB?", TW_TYPE_BOOLCPP, &g_testHarness.bUnifyVSPSCB, "");
            TwAddVarRW(autoTestBar, "Tag", TW_TYPE_STDSTRING, &g_testHarness.szTag, "");
            TwAddButton(autoTestBar, "------", NULL, NULL, "");
            TwAddButton(autoTestBar, "Run Tests", StartTestsCallBack, NULL, "");
        }

        TwDefine("TW_HELP position='500 20' size='700 600' valueswidth=fit");

    }
};

class SwitchingRendererController : public IVisualController
{
public:
    SwitchingRendererController(CFirstPersonCamera* pCam) :
        pCamera(pCam),
        pActiveRenderer(NULL),
        pInfoQueue(NULL)
    {

    }

    virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);
        return 1;
    }

    virtual void Animate(double fElapsedTimeSeconds)
    {
        if(fElapsedTimeSeconds > 1) fElapsedTimeSeconds = 1;

        static double totalTime = 0.0;
        totalTime += (double)fElapsedTimeSeconds;

        if(!g_autoSim)
            g_Camera.FrameMove((float)fElapsedTimeSeconds);

        bool bEventChange = false;

        if(g_autoSim)
        {
            bEventChange = g_testHarness.FrameUpdate((float)fElapsedTimeSeconds);

            g_iNumInstances = g_testHarness.GetCurrentEvent().instances;
            g_iNumUpdateThreads = g_testHarness.GetCurrentEvent().updatethreads;
            g_activeRenderPath = g_testHarness.GetCurrentEvent().dcType;
            g_bUseVTF = g_testHarness.GetCurrentEvent().bVTF;
            g_bVaryShaders = g_testHarness.GetCurrentEvent().bVaryShaders;
            g_bUnifyVSPSCB = g_testHarness.GetCurrentEvent().bUnifyVSPSCB;
            g_iNumRenderThreads = g_testHarness.GetCurrentEvent().threads;

            if(g_testHarness.IsComplete())
            {
                g_autoSim = false;
                PostQuitMessage(0);
            }
        }

        g_Scene.SetNumInstances(g_iNumInstances);
        g_Scene.SetUpdateThreads(g_iNumUpdateThreads);
        g_Scene.bMovingMeshes = g_bMovingMeshes;
        g_Scene.bMovingLights = g_bMovingLights;
        g_Scene.SetGlobalScale(g_MeshScale);

        // vary meshes after we set the instance count to properly vary the correct amount
        if(bEventChange && g_testHarness.bVaryOnChange)
        {
            g_Scene.VaryMeshes();
            StringCchCopy(g_wzMeshesInfo, 260, L"VARIED");
            g_iMeshPolys = g_Scene.GetTotalPolys();
        }
        else if(g_activeMesh != g_sceneActiveMesh)
        {
            g_Scene.SetAllInstancesToMeshW(g_meshes[g_activeMesh]);
            g_iMeshPolys = g_Scene.GetTotalPolys();
            StringCchCopy(g_wzMeshesInfo, 260, g_meshes[g_activeMesh]);
            g_sceneActiveMesh = g_activeMesh;
        }

        g_Scene.UpdateScene(totalTime, (float)fElapsedTimeSeconds);
    }

    virtual void Render(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext, ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV)
    {
        if(!pCamera) return;

        // Always assert the render path as set from the UI.  Will not do anything if no real change
        ChangeRenderer(pDevice, g_activeRenderPath);

        D3DXVECTOR3 eyePt = *pCamera->GetEyePt();
        D3DXVECTOR3 viewForward = *pCamera->GetWorldAhead();
        D3DXMATRIX viewMatrix = *pCamera->GetViewMatrix();
        D3DXMATRIX projMatrix = *pCamera->GetProjMatrix();
        D3DXMATRIX viewProjMatrix = viewMatrix * projMatrix;

        UINT numViewports = 1;
        D3D11_VIEWPORT viewport;
        pDeviceContext->RSGetViewports(&numViewports, &viewport);

        float clearColor[4] = { 0.1f, 0.1f, 0.1f, 0.0f };
        pDeviceContext->ClearRenderTargetView(pRTV, clearColor);
        pDeviceContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

        if(pActiveRenderer)
        {
            // Update new renderer with current settings
            pActiveRenderer->bWireFrame = false;
            pActiveRenderer->SetActiveThreads(g_iNumRenderThreads);
            pActiveRenderer->bSkipExecutes = g_bDisableExecutes;
            pActiveRenderer->SetUseVTF(g_bUseVTF);
            pActiveRenderer->bVaryShaders = g_bVaryShaders;
            pActiveRenderer->bUnifyVSPSCB = g_bUnifyVSPSCB;
            pActiveRenderer->bReuseCommandLists = g_bReuseCommandLists;
            pActiveRenderer->bSkipShadows = g_bNoShadows;

            pActiveRenderer->SetActiveTargets(pRTV, pDSV);
            pActiveRenderer->SetViewMatrix(pCamera->GetViewMatrix());
            pActiveRenderer->SetProjMatrix(pCamera->GetProjMatrix());
            pActiveRenderer->OnD3D11FrameRender(pDevice, pDeviceContext, 0, 0);
        }

        pDeviceContext->RSSetViewports(1, &viewport);
        pDeviceContext->OMSetRenderTargets(1, &pRTV, pDSV);
        pDeviceContext->OMSetDepthStencilState(NULL, 0);
        pDeviceContext->OMSetBlendState(NULL, NULL, 0xffffffff);
    }
    virtual HRESULT DeviceCreated(ID3D11Device* pDevice)
    {
        pDevice->QueryInterface<ID3D11InfoQueue>(&pInfoQueue);

        /*if(pInfoQueue)
        {
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_DEVICE_DRAW_SHADERRESOURCEVIEW_NOT_SET,
                // Add more message IDs here as needed
            };

            D3D11_INFO_QUEUE_FILTER filter;
            memset( &filter, 0, sizeof(filter) );
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            HRESULT hr = pInfoQueue->PushStorageFilter( &filter );

            //pInfoQueue->SetBreakOnID(D3D11_MESSAGE_ID_DEVICE_IASETPRIMITIVETOPOLOGY_TOPOLOGY_UNDEFINED,TRUE);
            //pInfoQueue->SetBreakOnID(D3D11_MESSAGE_ID_DEVICE_DRAW_VERTEX_BUFFER_NOT_SET,TRUE);


            //DEVICE_DRAW_SHADERRESOURCEVIEW_NOT_SET
        }    */

        static bool s_bFirstTime = true;

        if(s_bFirstTime)
        {
            s_bFirstTime = false;

            D3DXVECTOR3 eyePt = D3DXVECTOR3(-500.0f, 2500.0f, 5000.0f);
            D3DXVECTOR3 lookAtPt = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            g_Camera.SetViewParams(&eyePt, &lookAtPt);
            g_Camera.SetScalers(0.005f, 500.0f);
            g_Camera.SetRotateButtons(true, false, false, false);
        }

        for(int i = 0; i < g_numMeshes; i++)
        {
            g_Scene.AddMeshToLoad(g_meshes[i], g_meshes[i]);
        }

        g_Scene.LoadQueuedContent(pDevice);

        if(g_autoSim)
        {

        }
        else
        {
            g_Scene.SetAllInstancesToMeshW(g_meshes[g_activeMesh]);
            StringCchCopy(g_wzMeshesInfo, 260, g_meshes[g_activeMesh]);
        }

        g_Scene.SetNumInstances(g_iNumInstances);
        g_iMeshPolys = g_Scene.GetTotalPolys();

        ChangeRenderer(pDevice, RS_IMMEDIATE);

        return S_OK;
    }
    virtual void DeviceDestroyed()
    {
        SAFE_RELEASE(pInfoQueue);

        if(pActiveRenderer)
        {
            pActiveRenderer->OnD3D11DestroyDevice();
            SAFE_DELETE(pActiveRenderer);
        }

        g_Scene.FreeAllMeshes();
    }
    virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
    {
        if(pActiveRenderer)
            pActiveRenderer->OnD3D11BufferResized(pDevice, pBackBufferSurfaceDesc);

        memcpy(&BackBufferSurfaceDesc, pBackBufferSurfaceDesc, sizeof(DXGI_SURFACE_DESC));

        // Setup the camera's projection parameters
        float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
        g_Camera.SetProjParams(D3DX_PI / 4, fAspectRatio, g_fCameraClipNear, g_fCameraClipFar);

    }

    void ChangeRenderer(ID3D11Device* pd3dDevice, MT_RENDER_STRATEGY type)
    {
        if(pActiveRenderer)
        {
            // no need to do anything if we are already the right type
            if(pActiveRenderer->GetContextType() == type) return;

            pActiveRenderer->OnD3D11DestroyDevice();
            delete pActiveRenderer;
        }

        switch(type)
        {
        default:
        case RS_IMMEDIATE:
            pActiveRenderer = new IC_Renderer();
            break;

        case RS_MT_BATCHED_INSTANCES:
            pActiveRenderer = new DC_BatchInstances_Renderer();
            break;

        case RS_IC_INSTANCING:
            pActiveRenderer = new IC_Instancing_Renderer();
            break;
        }

        pActiveRenderer->OnD3D11CreateDevice(pd3dDevice);
        pActiveRenderer->OnD3D11BufferResized(pd3dDevice, &BackBufferSurfaceDesc);
        pActiveRenderer->SetViewMatrix(pCamera->GetViewMatrix());
        pActiveRenderer->SetProjMatrix(pCamera->GetProjMatrix());
        pActiveRenderer->SetScene(&g_Scene);

        // Update new renderer with current settings
        pActiveRenderer->bWireFrame = false;
        pActiveRenderer->SetActiveThreads(g_iNumRenderThreads);
        pActiveRenderer->bSkipExecutes = g_bDisableExecutes;
        pActiveRenderer->SetUseVTF(g_bUseVTF);
        pActiveRenderer->bVaryShaders = g_bVaryShaders;
        pActiveRenderer->bUnifyVSPSCB = g_bUnifyVSPSCB;
        pActiveRenderer->bReuseCommandLists = g_bReuseCommandLists;
        pActiveRenderer->bSkipShadows = g_bNoShadows;

    }
protected:
    ID3D11InfoQueue* pInfoQueue;
    DXGI_SURFACE_DESC BackBufferSurfaceDesc;
    RendererBase* pActiveRenderer;
    CFirstPersonCamera* pCamera;
};

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------

enum ECommandLineIDs
{
    CMDLN_INSTANCES = 0,
    CMDLN_RENDERSTRAT,
    CMDLN_UPDATETHREADS,
    CMDLN_RENDERTHREADS,
    CMDLN_MESHFILE,
    CMDLN_VTF,
    CMDLN_VARY_SHADERS,
    CMDLN_UNIFY_VSPSCB,
    CMDLN_BENCH,
    CMDLN_B_STARTINST,
    CMDLN_B_MAXINST,
    CMDLN_B_SAMPLES,
    CMDLN_B_MAXRENDER,
    CMDLN_B_MAXUPDATE,
    CMDLN_B_SMOOTHING,
    CMDLN_B_TAG,
    CMDLN_B_VTF,
    CMDLN_B_VARY,
    CMDLN_B_VARY_SHADERS,
    CMDLN_B_UNIFY_VSPSCB,
};

CSimpleOpt::SOption g_rgOptions[] =
{
    { CMDLN_INSTANCES,        L"-instances",            SO_REQ_CMB }, // - instances=10000
    { CMDLN_RENDERSTRAT,    L"-strategy",            SO_REQ_CMB }, // -strategy=2
    { CMDLN_UPDATETHREADS,    L"-updatethreads",        SO_REQ_CMB }, // -updatethreads=8
    { CMDLN_RENDERTHREADS,    L"-renderthreads",        SO_REQ_CMB }, // -renderthreads=8
    { CMDLN_MESHFILE,        L"-meshfile",            SO_REQ_SEP },
    { CMDLN_VTF,            L"-vtf",                SO_REQ_CMB },
    { CMDLN_VARY_SHADERS,    L"-varyshaders",        SO_REQ_CMB },
    { CMDLN_UNIFY_VSPSCB,    L"-unifyvspscb",        SO_REQ_CMB },
    { CMDLN_BENCH,            L"-benchmark",            SO_NONE    }, // run in benchmark mode, which dumps csv
    { CMDLN_B_STARTINST,    L"-b_startinstances",    SO_REQ_CMB },
    { CMDLN_B_MAXINST,        L"-b_maxinstances",        SO_REQ_CMB },
    { CMDLN_B_SAMPLES,        L"-b_testsamples",        SO_REQ_CMB },
    { CMDLN_B_MAXRENDER,    L"-b_maxrender",        SO_REQ_CMB },
    { CMDLN_B_MAXUPDATE,    L"-b_maxupdate",        SO_REQ_CMB },
    { CMDLN_B_SMOOTHING,    L"-b_smoothing",        SO_REQ_CMB },
    { CMDLN_B_TAG,            L"-b_tag",                SO_REQ_CMB },
    { CMDLN_B_VTF,            L"-b_vtf",                SO_REQ_CMB },
    { CMDLN_B_VARY,            L"-b_varymesh",            SO_REQ_CMB },
    { CMDLN_B_VARY_SHADERS,    L"-b_varyshaders",        SO_REQ_CMB },
    { CMDLN_B_UNIFY_VSPSCB,    L"-b_unifyvspscb",        SO_REQ_CMB },
    SO_END_OF_OPTIONS                       // END
};

bool ParseBool(const wchar_t* s)
{
    if(_wcsicmp(s, L"0") == 0 || _wcsicmp(s, L"False") == 0 || _wcsicmp(s, L"FALSE") == 0 || _wcsicmp(s, L"false") == 0 || s[0] == L'f' || s[0] == L'F')
        return false;

    return true;
}

int ParseIntClamped(const wchar_t* s, int min, int max)
{
    int num = _wtoi(s);

    if(num <= min) num = min;

    if(num > max) num = max;

    return num;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // command line options
    int nArgs;
    LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

    if(NULL == szArglist)
    {
        wprintf(L"CommandLineToArgvW failed\n");
        return 0;
    }

    CSimpleOpt args(nArgs, szArglist, g_rgOptions);

    bool bTriggerAutoSim = false;

    while(args.Next())
    {
        if(args.LastError() == SO_SUCCESS)
        {
            switch(args.OptionId())
            {
            case CMDLN_INSTANCES:
                g_iNumInstances = ParseIntClamped(args.OptionArg(), 1, g_iMaxInstances);
                break;

            case CMDLN_RENDERSTRAT:
                g_activeRenderPath = (MT_RENDER_STRATEGY)ParseIntClamped(args.OptionArg(), 0, RS_MAX - 1);
                break;

            case CMDLN_UPDATETHREADS:
                g_iNumUpdateThreads = ParseIntClamped(args.OptionArg(), 1, g_iMaxNumUpdateThreads);
                break;

            case CMDLN_RENDERTHREADS:
                g_iNumRenderThreads = ParseIntClamped(args.OptionArg(), 1, g_iMaxNumRenderThreads);
                break;

            case CMDLN_MESHFILE:
            {
                g_Scene.AddMeshToLoad(args.OptionArg(), args.OptionArg());
            }
            break;

            case CMDLN_VTF:
                g_bUseVTF = ParseBool(args.OptionArg());
                break;

            case CMDLN_VARY_SHADERS:
                g_bVaryShaders = ParseBool(args.OptionArg());
                break;

            case CMDLN_UNIFY_VSPSCB:
                g_bUnifyVSPSCB = ParseBool(args.OptionArg());
                break;

            case CMDLN_BENCH:
                bTriggerAutoSim = true;    // need to parse all options before initiating benchmark mode
                break;

            case CMDLN_B_STARTINST:
                g_testHarness.iStartingInstances = ParseIntClamped(args.OptionArg(), 1, g_iMaxInstances);
                break;

            case CMDLN_B_MAXINST:
                g_testHarness.iMaxInstances = ParseIntClamped(args.OptionArg(), 1, g_iMaxInstances);
                break;

            case CMDLN_B_SAMPLES:
                g_testHarness.instSteps = ParseIntClamped(args.OptionArg(), 1, 1000);
                break;

            case CMDLN_B_MAXRENDER:
                g_testHarness.maxThreads = (unsigned char)ParseIntClamped(args.OptionArg(), 1, g_iMaxNumRenderThreads);
                break;

            case CMDLN_B_MAXUPDATE:
                g_testHarness.updateThreads = (unsigned char)ParseIntClamped(args.OptionArg(), 1, g_iMaxNumUpdateThreads);
                break;

            case CMDLN_B_SMOOTHING:
                g_testHarness.smoothFrames = ParseIntClamped(args.OptionArg(), 1, 1000);
                break;

            case CMDLN_B_TAG:
            {
                char szName[260];
                WideCharToMultiByte(CP_ACP, 0, args.OptionArg(), -1, szName, MAX_PATH, NULL, FALSE);
                g_testHarness.szTag = szName;
            }
            break;

            case CMDLN_B_VTF:
                g_testHarness.bVTF = ParseBool(args.OptionArg());
                break;

            case CMDLN_B_VARY:
                g_testHarness.bVaryOnChange = ParseBool(args.OptionArg());
                break;

            case CMDLN_B_VARY_SHADERS:
                g_testHarness.bVaryShaders = ParseBool(args.OptionArg());
                break;

            case CMDLN_B_UNIFY_VSPSCB:
                g_testHarness.bUnifyVSPSCB = ParseBool(args.OptionArg());
                break;

            default:
#ifdef _DEBUG
                assert(0 && "Unhandled supported command line option found.  Ignoring.\n");
#endif
                break;
            }

        }
        else
        {
            // handle command line parsing errors?
            printf("Bad command line option found.  Ignoring.\n");
        }
    }

    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    g_Scene.InitializeUpdateThreads();

    g_Camera.SetViewParams(&g_vDefaultEye, &g_vDefaultLookAt);

    g_DeviceManager = new DeviceManager();
    AntTweakBarVisualController atbController;
    atbController.hwnd = g_DeviceManager->GetHWND();
    auto sceneController = SwitchingRendererController(&g_Camera);
    g_DeviceManager->AddControllerToFront(&sceneController);
    g_DeviceManager->AddControllerToFront(&atbController);

    DeviceCreationParameters deviceParams;
#if _DEBUG
    deviceParams.createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    deviceParams.swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    deviceParams.swapChainSampleCount = 1;
    deviceParams.startFullscreen = false;
    deviceParams.backBufferWidth = 1280;
    deviceParams.backBufferHeight = 800;

    if(FAILED(g_DeviceManager->CreateWindowDeviceAndSwapChain(deviceParams, L"NVIDIA Graphics Library : Deferred Contexts")))
    {
        MessageBox(NULL, L"Cannot initialize the D3D11 device with the requested parameters", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if(bTriggerAutoSim)
    {
        g_testHarness.InitializeExperiments();
        g_autoSim = true;
    }

    g_DeviceManager->MessageLoop();


    delete g_DeviceManager;    // destructor calls Shutdown()


    return 0;
}
