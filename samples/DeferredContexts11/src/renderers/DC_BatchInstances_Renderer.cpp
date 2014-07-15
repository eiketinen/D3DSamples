//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\renderers/DC_BatchInstances_Renderer.cpp
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
#include "DeferredContexts11.h"

#include "Scene.h"
#include "DC_BatchInstances_Renderer.h"

#define WAIT_AT_ONCE 0

#if 0
#define DEBUG_THREADING_LOG(A)                 {    \
        char text[MAX_PATH];\
        sprintf_s(text,MAX_PATH,A);\
        OutputDebugStringA(text);\
    }
#define DEBUG_THREADING_LOG_1(A,B)                 {    \
        char text[MAX_PATH];\
        sprintf_s(text,MAX_PATH,A,B);\
        OutputDebugStringA(text);\
    }
#define DEBUG_THREADING_LOG_2(A,B,C)                 {    \
        char text[MAX_PATH];\
        sprintf_s(text,MAX_PATH,A,B,C);\
        OutputDebugStringA(text);\
    }

#define DEBUG_THREADING_LOG_3(A,B,C,D)                 {    \
        char text[MAX_PATH];\
        sprintf_s(text,MAX_PATH,A,B,C,D);\
        OutputDebugStringA(text);\
    }

#else
#pragma warning (disable:4002)
#define DEBUG_THREADING_LOG(A)
#define DEBUG_THREADING_LOG_1(A,B)
#define DEBUG_THREADING_LOG_2(A,B,C)
#define DEBUG_THREADING_LOG_3(A,B,C,D)
#endif

#define SAFE_CLOSE_HANDLE(A) if(A != NULL) { CloseHandle(A); A = NULL; }

DC_BatchInstances_Renderer::DC_BatchInstances_Renderer() : RendererBase()
{
    for(int i = 0; i < g_iMaxNumRenderThreads; i++)
    {
        m_pd3dDeferredContexts[i] = NULL;

        for(int iPass = 0; iPass < DC_RP_MAX; iPass++)
            m_pd3dCommandLists[iPass * g_iMaxNumRenderThreads + i] = NULL;

        m_hRenderDeferredThreads[i] = NULL;
        m_hBeginRenderDeferredEvent[i] = NULL;
        m_hEndRenderDeferredEvent[i] = NULL;
    }

}

HRESULT DC_BatchInstances_Renderer::OnD3D11CreateDevice(ID3D11Device* pd3dDevice)
{
    HRESULT hr = S_OK;

    // Base class gets us most of our data
    RendererBase::OnD3D11CreateDevice(pd3dDevice);

    // Create worker threads and semaphores
    V_RETURN(InitializeWorkerThreads(pd3dDevice));

    return hr;
}

HRESULT DC_BatchInstances_Renderer::InitializeWorkerThreads(ID3D11Device* pd3dDevice)
{
    HRESULT hr;

    TerminateAndCloseWorkerThreads();

    // Per-scene data init
    for(int iInstance = 0; iInstance < g_iMaxNumRenderThreads; ++iInstance)
    {
        // some init data for the thread
        m_pPerThreadInstanceData[iInstance].iThreadIndex = iInstance;
        m_pPerThreadInstanceData[iInstance].pThis = this;
        m_pPerThreadInstanceData[iInstance].iStartMeshIndex = -1;
        m_pPerThreadInstanceData[iInstance].iEndMeshIndex = -1;

        // our signals to control work in the thread
        m_hBeginRenderDeferredEvent[iInstance] = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndRenderDeferredEvent[iInstance] = CreateEvent(NULL, FALSE, FALSE, NULL);

        // the DC used by this thread
        V_RETURN(pd3dDevice->CreateDeferredContext(0 /*Reserved for future use*/,
                 &m_pd3dDeferredContexts[iInstance]));

        m_hRenderDeferredThreads[iInstance] = (HANDLE)_beginthreadex(
                NULL,
                0,
                _BatchInstancesRenderDeferredProc,
                &m_pPerThreadInstanceData[iInstance],
                CREATE_SUSPENDED,
                NULL);

        ResumeThread(m_hRenderDeferredThreads[iInstance]);
    }

    return S_OK;
}

void DC_BatchInstances_Renderer::OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime)
{
    DC_UNREFERENCED_PARAM(pd3dDevice);

    if(!m_pScene)
    {
#ifdef _DEBUG
        assert(0 && "No Scene set!!");
#endif
        return;
    }

    m_fTime = (float)fTime;
    m_fElapsedTime = (float)fElapsedTime;

    // set our active thread count from user controls
    m_iActiveNumRenderThreads = m_iTargetActiveThreads;

    if(bVTFPositions)
    {
        // Update our VTF texture if using it.  Operation on immediate context
        UpdateVTFPositions(pd3dImmediateContext);
    }

    UpdateLightBuffers(pd3dImmediateContext);

    // Reuse of command list means keep our threads idle, do no graphics work and just
    //        use command list from a previous render.  Of course we must render at least once
    //        to fill the command lists
    if(!bReuseCommandLists || !m_bDrawn)
    {
        int initThreadIndex = bReuseCommandLists ? 0 : 1;

        for(int iRenderPass = 0; iRenderPass < DC_RP_MAX; iRenderPass++)
        {
            if(iRenderPass >= DC_RP_SHADOW1 && iRenderPass < DC_RP_SHADOW1 + g_iNumShadows)
            {
                pd3dImmediateContext->ClearDepthStencilView(m_pShadowDepthStencilView[iRenderPass - DC_RP_SHADOW1], D3D11_CLEAR_DEPTH, 1.0, 0);

                if(bSkipShadows)
                    continue;
            }

            int iMeshesPerThread = m_pScene->NumActiveInstances() / m_iTargetActiveThreads;

            // trigger drawing of ranges to worked threads, reserving the first range to start later here on IC
            for(int iThreadIndex = initThreadIndex; iThreadIndex < m_iTargetActiveThreads; iThreadIndex++)
            {
                // $$ NOTE, we are writing to a thread shared memory struct here.  But all workers should be waiting on events at this point, so it is contention free.
                m_pPerThreadInstanceData[iThreadIndex].iStartMeshIndex = iThreadIndex * iMeshesPerThread;
                m_pPerThreadInstanceData[iThreadIndex].iEndMeshIndex = m_pPerThreadInstanceData[iThreadIndex].iStartMeshIndex + iMeshesPerThread;
                m_pPerThreadInstanceData[iThreadIndex].RenderPass = (DC_RENDER_PASSES)iRenderPass;

                // handle rounding error by assigning slop to final thread
                if(iThreadIndex == m_iTargetActiveThreads - 1)
                    m_pPerThreadInstanceData[iThreadIndex].iEndMeshIndex = m_pScene->NumActiveInstances();

                // set event to begin on all scene thread
                SetEvent(m_hBeginRenderDeferredEvent[iThreadIndex]);
            }

            if(initThreadIndex > 0)
            {
                // run range 0 here on IC to get GPU active while we wait for threads
                PreRenderPass(pd3dImmediateContext, iRenderPass, 0);
                RenderPassSubsetToContext(pd3dImmediateContext, iRenderPass, 0, iMeshesPerThread, 0);
                PostRenderPass(pd3dImmediateContext, iRenderPass, 0);
            }

#if WAIT_AT_ONCE
            // wait for completion of all threads, then execute all at once.
            WaitForMultipleObjects(m_iTargetActiveThreads - 1,
                                   m_hEndRenderDeferredEvent + 1,
                                   TRUE,
                                   INFINITE);

            for(int iThreadIndex = initThreadIndex; iThreadIndex < m_iTargetActiveThreads; iThreadIndex++)
            {
                PreRenderPass(pd3dImmediateContext, iRenderPass, iThreadIndex);
                ExecuteCommandLists(pd3dImmediateContext, iRenderPass, iThreadIndex);
                PostRenderPass(pd3dImmediateContext, iRenderPass, iThreadIndex);
            }

#else
            // wait for completion of individual threads. when a thread has been finished, execute it immediately.
            {
                HANDLE    evArr[g_iMaxNumRenderThreads];
                DWORD    threadIdcs[g_iMaxNumRenderThreads];
                DWORD    nbHandle = 0;

                for(int i = initThreadIndex; i < m_iTargetActiveThreads; i++)
                {
                    evArr[nbHandle] = m_hEndRenderDeferredEvent[i];
                    threadIdcs[nbHandle++] = i;
                }

                while(nbHandle > 0)
                {
                    // wait for completion
                    DWORD result = WaitForMultipleObjects(nbHandle,
                                                          evArr,
                                                          FALSE,
                                                          INFINITE);

                    if(result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + nbHandle)
                    {
                        // Execute command list that has been finished.
                        DWORD cmpThreadId = result - WAIT_OBJECT_0;
                        int    iThreadIndex = threadIdcs[cmpThreadId];

                        PreRenderPass(pd3dImmediateContext, iRenderPass, iThreadIndex);
                        ExecuteCommandLists(pd3dImmediateContext, iRenderPass, iThreadIndex);
                        PostRenderPass(pd3dImmediateContext, iRenderPass, iThreadIndex);

                        for(size_t i = 0; i < nbHandle; i++)
                        {
                            if(i > cmpThreadId)
                            {
                                evArr[i - 1] = evArr[i];
                                threadIdcs[i - 1] = threadIdcs[i];
                            }
                        }

                        nbHandle -= 1;
                    }
                    else
                    {
                        assert(0 && "An error occurred while waiting for completion of worker threads.");
                    }
                }
            }
#endif
        }

        m_bDrawn = bReuseCommandLists;    // if not reusing then always draw
    }
    else
    {
        // replay command lists.
        for(int iRenderPass = 0; iRenderPass < DC_RP_MAX; iRenderPass++)
        {
            if(iRenderPass >= DC_RP_SHADOW1 && iRenderPass < DC_RP_SHADOW1 + g_iNumShadows)
            {
                pd3dImmediateContext->ClearDepthStencilView(m_pShadowDepthStencilView[iRenderPass - DC_RP_SHADOW1], D3D11_CLEAR_DEPTH, 1.0, 0);

                if(bSkipShadows)
                    continue;
            }

            for(int iThreadIndex = 0; iThreadIndex < m_iTargetActiveThreads; iThreadIndex++)
            {
                PreRenderPass(pd3dImmediateContext, iRenderPass, iThreadIndex);
                ExecuteCommandLists(pd3dImmediateContext, iRenderPass, iThreadIndex);
                PostRenderPass(pd3dImmediateContext, iRenderPass, iThreadIndex);
            }
        }
    }
}

void DC_BatchInstances_Renderer::ExecuteCommandLists(ID3D11DeviceContext* pd3dImmediateContext, int iPass, int iThreadIndex)
{
    if(bSkipExecutes) return;

    if(bSkipShadows && iPass >= DC_RP_SHADOW1 && iPass < DC_RP_SHADOW1 + g_iNumShadows)
        return;

    int iCmdListIndex = GetCommandListFor(iThreadIndex, iPass);

    // may be NULL if skipping certain scenes
    if(m_pd3dCommandLists[iCmdListIndex] == NULL)
        return;

    // apply command list calls to IC.
    // NOTE slight perf penalty to save and restore IC state, don't need it, so avoid.
    pd3dImmediateContext->ExecuteCommandList(m_pd3dCommandLists[iCmdListIndex], FALSE);

    //SAFE_RELEASE(m_pd3dCommandLists[iCmdListIndex]);    // release so that we don't hold references between frames in case we want to resize, etc
}

HRESULT DC_BatchInstances_Renderer::RenderPassSubsetToContext(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iMeshStart, int iMeshEnd, int iResourceIndex)
{
    HRESULT hr = S_OK;
    const SceneParamsStatic* pStaticParams = &m_StaticSceneParams[iRenderPass];

    // execute render state setup common to all scenes (parameterized per scene)
    V(RenderSetupToContext(pd3dContext, pStaticParams, iResourceIndex));

    DEBUG_THREADING_LOG_3("WorkThread ( %d ) :     Draw meshes %d to %d !!\n", iResourceIndex, iMeshStart, iMeshEnd);

    // draw all meshes from our list
    int iLastMeshIndex = -1;

    for(int iIndex = iMeshStart; iIndex < iMeshEnd; iIndex++)
    {
        // if we are drawing the same mesh as last time we can skip some API calls for binding buffers and whatnot
        const int currentMeshIndex = m_pScene->GetMeshIndexFor(iIndex);
        bool bDrawSetup = false;

        if(iLastMeshIndex != currentMeshIndex)
        {
            bDrawSetup = true;
            iLastMeshIndex = currentMeshIndex;
        }

        RenderMeshToContext(pd3dContext, iIndex, iRenderPass, iResourceIndex, bDrawSetup);
    }

    return hr;
}


unsigned int WINAPI         DC_BatchInstances_Renderer::_BatchInstancesRenderDeferredProc(LPVOID lpParameter)
{
    HRESULT hr;

    // thread local data
    const DC_BATCHED_THREAD_PARAMS* pParams = (DC_BATCHED_THREAD_PARAMS*)lpParameter;

    DC_BatchInstances_Renderer* pRenderer = (DC_BatchInstances_Renderer*)pParams->pThis;

    ID3D11DeviceContext* pd3dDeferredContext = pRenderer->m_pd3dDeferredContexts[pParams->iThreadIndex];
    int iThreadIndex = pParams->iThreadIndex;

    if(iThreadIndex < 0) return (unsigned int) - 1;

    if(iThreadIndex >= g_iMaxNumRenderThreads) return (unsigned int) - 1;

    for(;;)
    {
        // Wait for main thread to signal ready
        WaitForSingleObject(pRenderer->m_hBeginRenderDeferredEvent[pParams->iThreadIndex], INFINITE);

        DEBUG_THREADING_LOG_2("WorkThread ( %d ) : Start work on %d !!\n", iThreadIndex, (int)pParams->RenderPass);

        if(pRenderer->m_bTermThreads)
        {
            DEBUG_THREADING_LOG_1("WorkThread ( %d ) : _endthreadex !!\n", iThreadIndex);
            _endthreadex(0);    // term and signal main thread of completion
        }

        // No assigned meshes?  just skip then!
        if(pParams->iStartMeshIndex >= 0 && pParams->iEndMeshIndex > 0)
        {
            // cmd list index changes per render pass to point to a different command list
            int iCmdListIndex = pRenderer->GetCommandListFor(pParams->iThreadIndex, (int)pParams->RenderPass);
            ID3D11CommandList*& pd3dCommandList = pRenderer->m_pd3dCommandLists[iCmdListIndex];

            // command lists might be replayed so release it here.
            SAFE_RELEASE(pd3dCommandList);

            // Render them
            {
                // render the specified scene
                V(pRenderer->RenderPassSubsetToContext(pd3dDeferredContext, pParams->RenderPass,
                                                       pParams->iStartMeshIndex, pParams->iEndMeshIndex, iThreadIndex));

                // make us a command list, yar!
                V(pRenderer->FinishToCommandList(pd3dDeferredContext, pd3dCommandList));
            }
        }
        else
        {
            DEBUG_THREADING_LOG_1("WorkThread ( %d ) : No meshes, sleep then SetEvent!!\n", iThreadIndex);
            Sleep(10);
        }

        // Tell main thread command list is finished
        SetEvent(pRenderer->m_hEndRenderDeferredEvent[iThreadIndex]);
    }
}

void DC_BatchInstances_Renderer::OnD3D11DestroyDevice()
{
    RendererBase::OnD3D11DestroyDevice();

    // will instruct the threads to terminate themselves and then close the handles
    TerminateAndCloseWorkerThreads();
}

void DC_BatchInstances_Renderer::TerminateAndCloseWorkerThreads()
{
    // set the killing bool
    m_bTermThreads = true;

    // first go through our handles and close any open handles (killing the threads from previous run, etc)
    for(int iInstance = 0; iInstance < g_iMaxNumRenderThreads; iInstance++)
    {
        // trigger the thread which should have it term itself
        if(m_hRenderDeferredThreads[iInstance] != NULL)
        {
            SetEvent(m_hBeginRenderDeferredEvent[iInstance]);

            DWORD waitResult = WaitForSingleObject(m_hRenderDeferredThreads[iInstance], 5000);
            assert(!waitResult);

            if(waitResult) continue;
        }

        SAFE_CLOSE_HANDLE(m_hRenderDeferredThreads[iInstance]);
        SAFE_CLOSE_HANDLE(m_hBeginRenderDeferredEvent[iInstance]);
        SAFE_CLOSE_HANDLE(m_hEndRenderDeferredEvent[iInstance]);
        SAFE_RELEASE(m_pd3dDeferredContexts[iInstance]);

        for(int iPass = 0; iPass < DC_RP_MAX; iPass++)
            SAFE_RELEASE(m_pd3dCommandLists[iPass * g_iMaxNumRenderThreads + iInstance]);
    }

    // unset death bool
    m_bTermThreads = false;

}