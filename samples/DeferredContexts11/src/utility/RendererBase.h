//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\utility/RendererBase.h
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

#include <vector>
#include "Scene.h"

#include "ShaderPermutations.h"

// our load allocation settings
enum DC_RENDER_PASSES
{
    DC_RP_SHADOW1 = 0,        // draw a shadow from light0
    DC_RP_MAIN,            // main render from camera
    DC_RP_MAX
};

const int    g_iMaxNumRenderThreads = 32; // the max # of threads to load balance across for rendering. MUST BE GREATER EQUAL THAN g_iNumShadows+1

//--------------------------------------------------------------------------------------
// Thread structs
//--------------------------------------------------------------------------------------

// Shared by any threaded subclasses, but not used by the IC renderer
struct DC_THREAD_PARAMS
{
    int iThreadIndex;    // index into various structures per thread
    class RendererBase* pThis;    // pointer to the class instance
};

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
struct CB_VS_PER_OBJECT
{
    D3DXMATRIX m_mWorld;
};

struct CB_VS_PER_PASS
{
    D3DXMATRIX m_mViewProj;
};

struct CB_PS_PER_OBJECT
{
    D3DXVECTOR4 m_vObjectColor;
};

struct CB_PS_PER_LIGHT
{
    struct LightDataStruct
    {
        D3DXMATRIX  m_mLightViewProj;
        D3DXVECTOR4 m_vLightPos;
        D3DXVECTOR4 m_vLightDir;
        D3DXVECTOR4 m_vLightColor;
        D3DXVECTOR4 m_vFalloffs;    // x = dist end, y = dist range, z = cos angle end, w = cos range
    } m_LightData[g_iNumLights];
};

struct CB_PS_PER_PASS
{
    D3DXVECTOR4 m_vAmbientColor;
    D3DXVECTOR4 m_vTintColor;
};

//--------------------------------------------------------------------------------------
// scene data structs
//--------------------------------------------------------------------------------------
struct SceneParamsStatic
{
    ID3D11DepthStencilState*    m_pDepthStencilState;
    UINT8                       m_iStencilRef;

    ID3D11RasterizerState*      m_pRasterizerState;

    D3DXVECTOR4                 m_vTintColor;

    // If m_pDepthStencilView is non-NULL then these
    // are for a shadow map.  Otherwise, use the DXUT
    // defaults.
    ID3D11DepthStencilView*     m_pDepthStencilView;
    D3D11_VIEWPORT*             m_pViewport;

    UINT                        m_iSceneIndex;
};

struct SceneParamsDynamic
{
    SceneParamsDynamic() {D3DXMatrixIdentity(&m_mViewProj);}
    D3DXMATRIX                  m_mViewProj;
};


//--------------------------------------------------------------------------------------
// Base renderer class.  Contains common shared methods and vars used by subclass render strategies.
//--------------------------------------------------------------------------------------
class RendererBase
{
public:
    RendererBase();
    virtual ~RendererBase();

    virtual MT_RENDER_STRATEGY GetContextType() = 0; // PV.  subclass must override

    virtual HRESULT OnD3D11CreateDevice(ID3D11Device* pd3dDevice);
    virtual void OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime);
    virtual void OnD3D11DestroyDevice();
    virtual void OnD3D11BufferResized(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);

    void SetActiveTargets(ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV) {m_pRTV = pRTV; m_pDSV = pDSV;}

    void SetViewMatrix(const D3DXMATRIX* pView) {memcpy(&m_viewMatrix, pView, sizeof(D3DXMATRIX));}
    void SetProjMatrix(const D3DXMATRIX* pProj) {memcpy(&m_projMatrix, pProj, sizeof(D3DXMATRIX));}

    void SetScene(Scene* scene)
    {
        if(m_pScene != scene)
        {
            m_pScene = scene;
            m_bDrawn = false;
        }

    }
    void SetActiveThreads(int num)
    {
        int newNum = max(1, min(g_iMaxNumRenderThreads, num));

        if(m_iTargetActiveThreads != newNum)
        {
            m_iTargetActiveThreads = newNum;
            m_bDrawn = false;
        }
    }

    virtual void SetUseVTF(bool bUseVTF)
    {
        if(bUseVTF != bVTFPositions)
        {
            bVTFPositions = bUseVTF;
            m_bDrawn = false;
        }

    }
    bool bVaryShaders;
    bool bUnifyVSPSCB;            // Unifi VS PS uinforms. PS uniforms will be passed via VS output.
    bool bSkipShadows;            // skip the render calls for the shadows scenes
    bool bReuseCommandLists;    // after finalizing the command lists once, skip all render calls and jsut execute (isolating patching cost)
    bool bSkipExecutes;            // don't execute command lists (avoid patching overhead but won't actually draw)
    bool bWireFrame;            // hmm now what could this do?

protected:

    //------------
    // Immediate methods
    //------------

    virtual ID3D11PixelShader* PickAppropriatePixelShader(const UINT idx = 0);
    virtual ID3D11VertexShader* PickAppropriateVertexShader(const UINT idx = 0);

    // encapsulated all the direct render methods in the right order for a scene
    virtual HRESULT RenderPassToContext(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iResourceIndex);

    // draw the requested instance to the context
    void RenderMeshToContext(ID3D11DeviceContext* pd3dContext, UINT iMeshInstance, UINT iRenderPass, int iResourceIndex, bool bIssueDrawSetup = true);

    // Draw the mesh instanced to the context (must use VTF)
    void RenderMeshInstancedToContext(UINT iMeshInstance, UINT iNumInstanceOffset, UINT iNumInstances, ID3D11DeviceContext* pd3dDeviceContext, UINT iDiffuseSlot, UINT iNormalSlot, UINT iResourceIndex);

    // Shared state setup for all meshes (called once per command list)
    virtual HRESULT RenderSetupToContext(ID3D11DeviceContext* pd3dContext, const SceneParamsStatic* pStaticParams, int iResourceIndex);

    //------------
    // per scene specific methods
    //------------

    // These perform any render calls to setup before the deferred operations begin.  This could be clearing targets, initializing stencil, etc
    //  also calculates any per scene dynamic parameters that need to be passed to threads.
    virtual void PreRenderPass(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iResourceIndex);

    // Any cleanup or follow up operations such as RT copies, blends, etc
    virtual void PostRenderPass(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iResourceIndex);


    //------------
    // misc utils
    //------------
    void CalcLightViewProj(D3DXMATRIX* pmLightViewProj, int iLight);
    void UpdateVTFPositions(ID3D11DeviceContext* pd3dContext, bool useSortedIndices = false);
    void UpdateLightBuffers(ID3D11DeviceContext* pd3dContext);

    // finalize the context into the command list requested
    HRESULT FinishToCommandList(ID3D11DeviceContext* pd3dContext, ID3D11CommandList*& pd3dCommandList);

    HRESULT InitializeMiscRender(ID3D11Device* pd3dDevice);
    HRESULT InitializeBuffers(ID3D11Device* pd3dDevice);
    HRESULT InitializeShadows(ID3D11Device* pd3dDevice);

    HRESULT SetupD3D11Views(ID3D11DeviceContext* pd3dDeviceContext, int iScene);    // setups viewport, etc to allow tiling, or other render effects

    // Configuration bools
    bool bDifferentShaders;        // bind a different shader per draw
    bool bVTFPositions;            // push matrices into texture rather than constant buffer
    bool bUpdatePositionsPerFrame;    // true to constantly update all instance positions.  false will disable any motion

    // our temp have we drawn once with this config bool
    bool m_bDrawn;

    float m_fTime;                // total accumulated  time to be used by threads
    float m_fElapsedTime;        // current one frame time

    UINT                        m_SceneWidth;
    UINT                        m_SceneHeight;

    int                            m_iTargetActiveThreads;        // set by user to indicate the desired active thread count

    D3DXMATRIX                    m_viewMatrix;
    D3DXMATRIX                    m_projMatrix;
    Scene*                         m_pScene;
    ShaderPermutations             m_ShaderPermutations;
    ID3D11RenderTargetView*     m_pRTV;    // transient, no reference held
    ID3D11DepthStencilView*        m_pDSV;    // transient, no reference held


    // Params for specific scenes
    SceneParamsStatic           m_StaticSceneParams[DC_RP_MAX];

    //--------------------------------------------------------------------------------------
    // Constant buffer vars
    //--------------------------------------------------------------------------------------
    UINT                        m_iCBVSPerObjectBind;
    UINT                        m_iCBVSPerSceneBind;
    UINT                        m_iCBPSPerObjectBind;
    UINT                        m_iCBPSPerLightBind;
    UINT                        m_iCBPSPerSceneBind;

    // Need buffers per thread to avoid contention
    ID3D11Buffer*               m_pcbVSPerObject[g_iMaxNumRenderThreads];
    ID3D11Buffer*               m_pcbVSPerScene[g_iMaxNumRenderThreads];
    ID3D11Buffer*               m_pcbPSPerObject[g_iMaxNumRenderThreads];
    ID3D11Buffer*               m_pcbPSPerScene[g_iMaxNumRenderThreads];

    ID3D11Buffer*               m_pcbPSPerLight;    // light buffer updated at beginning of frame

    //--------------------------------------------------------------------------------------
    //  VTF Based world transforms
    //--------------------------------------------------------------------------------------
    ID3D11Texture2D*            m_pVTFWorldsTexture;
    ID3D11ShaderResourceView*    m_pVTFWorldsSRV;
    ID3D11Buffer*                m_pVB_VTFCoords;
    CB_VS_PER_OBJECT*            m_pWorldsTemp;    // just some temp scratch memory, used every frame, better to not new and delete

    //--------------------------------------------------------------------------------------
    // Rendering interfaces
    //--------------------------------------------------------------------------------------
    ID3D11SamplerState*         m_pSamPointClamp;
    ID3D11SamplerState*         m_pSamLinearWrap;
    ID3D11RasterizerState*      m_pRasterizerStateNoCull;
    ID3D11RasterizerState*      m_pRasterizerStateBackfaceCull;
    ID3D11RasterizerState*      m_pRasterizerStateFrontfaceCull;
    ID3D11RasterizerState*      m_pRasterizerStateNoCullWireFrame;
    ID3D11DepthStencilState*    m_pDepthStencilStateNoStencil;

    //--------------------------------------------------------------------------------------
    // Shadow map data and interface
    //--------------------------------------------------------------------------------------
    ID3D11Texture2D*            m_pShadowTexture[g_iNumShadows];
    ID3D11ShaderResourceView*   m_pShadowResourceView[g_iNumShadows];
    ID3D11DepthStencilView*     m_pShadowDepthStencilView[g_iNumShadows];
    D3D11_VIEWPORT              m_ShadowViewport[g_iNumShadows];
    FLOAT                       m_fShadowResolutionX[g_iNumShadows];
    FLOAT                       m_fShadowResolutionY[g_iNumShadows];


    //--------------------------------------------------------------------------------------
    // Lighting params (to be read from content when the pipeline supports it)
    //--------------------------------------------------------------------------------------
    D3DXVECTOR4                 m_vAmbientColor;

    int GetSceneInstanceFromContext(ID3D11DeviceContext* pd3dContext);
    void SetSceneInstanceToContext(ID3D11DeviceContext* pd3dContext, int iRenderPass);


};

