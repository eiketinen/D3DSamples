//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\renderers/DC_BatchInstances_Renderer.h
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

#include "RendererBase.h"

// Shared by any threaded subclasses, but not used by the IC renderer
struct DC_BATCHED_THREAD_PARAMS : public DC_THREAD_PARAMS
{
    DC_RENDER_PASSES RenderPass; // which pass are we on?
    int iStartMeshIndex;        // where we start in scene's mesh list
    int iEndMeshIndex;            // where we end in scene's mesh list
};


/*
    This is a Multi threaded deferred context renderer.  This renderer encapsulates per frame updates of instance animation as well as render calls.

    Pseudo code
    --------------

    For each pass:
    - Assign mesh subsets (ala index range) to each active worker thread (via DC_BATCHED_THREAD_PARAMS)
    - Kick off Event to each thread to render its subset
    - Wait for all threads to finish rendering

    After all passes have command lists, Execute all command lists for each pass in the proper order

*/
class DC_BatchInstances_Renderer : public RendererBase
{
public:
    DC_BatchInstances_Renderer();

    virtual MT_RENDER_STRATEGY GetContextType() {return RS_MT_BATCHED_INSTANCES;}

    virtual HRESULT OnD3D11CreateDevice(ID3D11Device* pd3dDevice);
    virtual void OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime);
    virtual void OnD3D11DestroyDevice();

protected:

    HRESULT InitializeWorkerThreads(ID3D11Device* pd3dDevice);
    void TerminateAndCloseWorkerThreads();

    int GetCommandListFor(int iThreadIndex, int iPass)
    {
        // Pass 0 T0..Tn, Pass1 T0..Tn, ... Pass M T0...Tn
        return iPass * g_iMaxNumRenderThreads + iThreadIndex;
    }

    virtual HRESULT RenderPassSubsetToContext(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iMeshStart, int iMeshEnd, int iResourceIndex = 0);

    // Executes all command lists
    void ExecuteCommandLists(ID3D11DeviceContext* pd3dImmediateContext, int iPass, int iThreadIndex);

    static unsigned int WINAPI         _BatchInstancesRenderDeferredProc(LPVOID lpParameter);
    bool m_bTermThreads;            // used to signal we are quitting or changing path

    // our deferred contexts and command lists (pool is shared by both per scene and per instance methods)
    ID3D11DeviceContext*        m_pd3dDeferredContexts[g_iMaxNumRenderThreads];
    ID3D11CommandList*          m_pd3dCommandLists[g_iMaxNumRenderThreads* DC_RP_MAX];    // DC_RP_MAX cmd lists per thread

    // our thread handles and begin/end event handles
    HANDLE                        m_hRenderDeferredThreads[g_iMaxNumRenderThreads];
    HANDLE                        m_hBeginRenderDeferredEvent[g_iMaxNumRenderThreads];
    HANDLE                        m_hEndRenderDeferredEvent[g_iMaxNumRenderThreads];

    DC_BATCHED_THREAD_PARAMS    m_pPerThreadInstanceData[g_iMaxNumRenderThreads];

    int                            m_iActiveNumRenderThreads;    // allows us to throttle back the threads (only applicable per instance MT)
};