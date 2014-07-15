//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\utility/Scene.cpp
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
#include <process.h>

#include "DeferredContexts11.h"

#include "Scene.h"
#include "SimpleShipController.h"

#include <Strsafe.h>

#include "NvSimpleRawMesh.h"
#include "NvSimpleMeshLoader.h"
#include "NvSimpleMesh.h"

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

#else
#pragma warning (disable:4002)
#define DEBUG_THREADING_LOG(A)
#define DEBUG_THREADING_LOG_1(A,B)
#define DEBUG_THREADING_LOG_2(A,B)
#endif

#define SAFE_CLOSE_HANDLE(A) if(A != NULL) { CloseHandle(A); A = NULL; }


// Just some hard coded lights
DC_Light        g_lights[4] =
{
    {
        D3DXVECTOR4(0.75f, 0.75f, 0.75f, 1.000f),    // vLightColor
        D3DXVECTOR3(0, 1.5f * g_fSceneRadius, 0),    // vLightPos
        D3DXVECTOR3(0, -1, 0),                    // vLightDir
        -1, -1, -1, -1,                                    // falloff params will be set on initialization, -1 means unset
        D3DX_PI / 4.0f,                                // fLightFOV
        1.f, 2.f, 8.f* g_fSceneRadius                    // fLightAspect, fLightNearPlane, fLightFarPlane
    },
    {
        D3DXVECTOR4(0.4f * 0.895f, 0.4f * 0.634f, 0.4f * 0.626f, 1.f),
        D3DXVECTOR3(-g_fSceneRadius / 4.f, g_fSceneRadius / 2, 0),
        D3DXVECTOR3(1.f, -1.f, 0.f),
        -1, -1, -1, -1,                                    // falloff params will be set on initialization, -1 means unset
        (45.f * (D3DX_PI / 180.f)),
        1.f, 2.f, 8.f* g_fSceneRadius
    },
    {
        D3DXVECTOR4(0.4f * 1.000f, 0.4f * 0.837f, 0.4f * 0.848f, 1.f),
        D3DXVECTOR3(g_fSceneRadius / 4.f, g_fSceneRadius / 2, 0),
        D3DXVECTOR3(-1.f, -1.f, 0),
        -1, -1, -1, -1,                                    // falloff params will be set on initialization, -1 means unset
        (45.f * (D3DX_PI / 180.f)),
        1.f, 2.f, 8.f* g_fSceneRadius
    },
    {
        D3DXVECTOR4(0.5f * 0.388f, 0.5f * 0.641f, 0.5f * 0.401f, 1.f),
        D3DXVECTOR3(0.0f, g_fSceneRadius / 2, -g_fSceneRadius / 4.f),
        D3DXVECTOR3(0.f, -1.f, 0.f),
        -1, -1, -1, -1,                                    // falloff params will be set on initialization, -1 means unset
        (45.f * (D3DX_PI / 180.f)),
        1.f, 2.f, 8.f* g_fSceneRadius
    }
};


Scene::Scene(D3DXVECTOR3& initialWorldSize) :
    m_iNumUpdateThreads(0),    // default
    m_bTermThreads(false),
    m_iNumActiveInstances(1),
    m_fScale(1.f)
{

    for(int i = 0; i < g_iMaxInstances; i++)
    {
        m_iInstanceMeshIndices[i] = 0;

        int instancesPerAxis = (int)pow((double)g_iMaxInstances, 0.333333) + 1;    // cuberoot
        D3DXVECTOR3 unitsPerInstance = initialWorldSize / (float)instancesPerAxis;

        D3DXVECTOR3 halfWorldSize = initialWorldSize / 2.f;
        int y = i / (instancesPerAxis * instancesPerAxis);
        int xzIndex = i % (instancesPerAxis * instancesPerAxis);
        int z = xzIndex / instancesPerAxis;
        int x = xzIndex % instancesPerAxis;
        D3DXVECTOR3 zeroBasedPosition = D3DXVECTOR3((float)x * unitsPerInstance.x, (float)y * unitsPerInstance.y, (float)z * unitsPerInstance.z);
        D3DXVECTOR3 initialPos = -halfWorldSize + zeroBasedPosition;

        m_pShips[i] = new SimpleShipController(this, i, initialPos);
        UpdateInstance(i, 0.f, 1.f / 30.f);
    }

    for(int i = 0; i < g_iMaxNumUpdateThreads; i++)
    {
        m_hInstanceUpdateThreads[i] = NULL;
        m_hBeginInstanceUpdateEvent[i] = NULL;
        m_hEndInstanceUpdateEvent[i] = NULL;
        m_threadActive[i] = false;
    }

    for(int index = 0; index < g_iNumLights ; index++)
    {
        memcpy(&m_lights[index], &g_lights[index], sizeof(DC_Light));

        // negative falloffdistend means unset, so we set it
        m_lights[index].fLightFalloffDistEnd = m_lights[index].fLightFarPlane;
        m_lights[index].fLightFalloffDistRange = m_lights[index].fLightFarPlane / 4.f;
        m_lights[index].fLightFalloffCosAngleEnd = cosf(m_lights[index].fLightFOV / 2.0f);
        m_lights[index].fLightFalloffCosAngleRange = 0.1f;

        D3DXVec3Normalize(&m_lights[index].vLightDir, &m_lights[index].vLightDir);
    }

}

Scene::~Scene()
{
    // kill all threads
    m_bTermThreads = true;

    for(int iThread = 0; iThread < m_iNumUpdateThreads; iThread++)
    {
        SetEvent(m_hBeginInstanceUpdateEvent[iThread]);

        SAFE_CLOSE_HANDLE(m_hInstanceUpdateThreads[iThread]);
        SAFE_CLOSE_HANDLE(m_hBeginInstanceUpdateEvent[iThread]);
        SAFE_CLOSE_HANDLE(m_hEndInstanceUpdateEvent[iThread]);
    }

    FreeAllMeshes();    // this can be called multiple times, might already be freed

    for(int i = 0; i < g_iMaxInstances; i++)
    {
        SAFE_DELETE(m_pShips[i])
    }

    // in case we quued meshes but never loaded them
    for(int iIndex = 0; iIndex < (int)m_toLoad.size(); iIndex++)
    {
        delete [] m_toLoad[iIndex].wzName;
        delete [] m_toLoad[iIndex].wzFile;
    }
}

//////////////////////////////////////////////////////////////////////////
// Queue a mesh file to be loaded into our mesh bank
void Scene::AddMeshToLoad(LPWSTR wzName, LPWSTR wzFile)
{
    LPWSTR ourName = new WCHAR[MAX_PATH];
    LPWSTR ourFile = new WCHAR[MAX_PATH];

    StringCchPrintf(ourName, MAX_PATH, wzName);
    StringCchPrintf(ourFile, MAX_PATH, wzFile);
    TOLOAD localToLoad;
    localToLoad.wzFile = ourFile;
    localToLoad.wzName = ourName;
    m_toLoad.insert(m_toLoad.end(), localToLoad);
}

//////////////////////////////////////////////////////////////////////////
// Load a texture and make a resource view for it
void Scene::CreateTextureFromFile(ID3D11Device* pDev, char* szFileName, ID3D11ShaderResourceView** ppRV)
{
    DC_UNREFERENCED_PARAM(pDev);
    DC_UNREFERENCED_PARAM(szFileName);
    *ppRV = NULL;
}

//////////////////////////////////////////////////////////////////////////
// Process all queued meshes to load
void Scene::LoadQueuedContent(ID3D11Device* pd3dDevice)
{
    for(int iIndex = 0; iIndex < (int)m_toLoad.size(); iIndex++)
    {
        LoadQueueContentDirect(pd3dDevice, iIndex);
    }

    for(int iIndex = 0; iIndex < (int)m_toLoad.size(); iIndex++)
    {
        delete [] m_toLoad[iIndex].wzName;
        delete [] m_toLoad[iIndex].wzFile;
    }

    m_toLoad.clear();    // all loaded, need not the to load
}


// Load a mesh into a DXUT sdk mesh class
void Scene::LoadQueueContentDirect(ID3D11Device* pd3dDevice, int contentIndex)
{
    MESHINFO mi;
    NvSimpleMesh* pMesh;
    NvSimpleMeshLoader MeshLoader;    // stack var will clean up raw data when scope is lost
    LPWSTR wzName = m_toLoad[contentIndex].wzName;
    LPWSTR wzFile = m_toLoad[contentIndex].wzFile;
    MeshLoader.LoadFile(wzFile);

    wsprintf(MeshLoader.pMeshes[0].m_szDiffuseTexture, L"..\\..\\deferredcontexts11\\assets\\cloth.dds");
    wsprintf(MeshLoader.pMeshes[0].m_szNormalTexture, L"..\\..\\deferredcontexts11\\assets\\cloth_normal.dds");

    pMesh = new NvSimpleMesh();
    pMesh->Initialize(pd3dDevice, &MeshLoader.pMeshes[0]);

    char szName[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, wzName, -1, szName, MAX_PATH, NULL, FALSE);

    // copy name so we can look it up later
    strncpy_s(pMesh->szName, szName, 260);

    mi.pMesh = pMesh;
    mi.iPolys = pMesh->iNumIndices / 3;
    mi.fScale = 1.f;

    m_SDKMeshes.insert(m_SDKMeshes.end(), mi);
}

void Scene::FreeAllMeshes()
{
    for(std::vector<MESHINFO>::iterator it = m_SDKMeshes.begin(); it != m_SDKMeshes.end(); it++)
    {
        NvSimpleMesh* pMesh = it->pMesh;

        if(pMesh)
        {
            pMesh->Release();
            delete pMesh;
        }
    }

    m_SDKMeshes.clear();
}

void Scene::SetNumInstances(int num)
{
    m_iNumActiveInstances = num;
}

void Scene::SetGlobalScale(float scale)
{
    if((scale - m_fScale) < 0.01 && (scale - m_fScale) > -0.01) return;

    m_fScale = scale;

    for(int i = 0; i < g_iMaxInstances; i++)
    {
        UpdateInstance(i, 0.f, 1.f / 30.f);
    }
}

void Scene::SetLight(int index, DC_Light& light)
{
    if(index >= 0 && index < g_iNumLights)
    {
        memcpy(&m_lights[index], &light, sizeof(DC_Light));
    }

    // negative falloffdistend means unset, so we set it
    if(light.fLightFalloffDistEnd <= 0)
    {
        m_lights[index].fLightFalloffDistEnd = m_lights[index].fLightFarPlane;
        m_lights[index].fLightFalloffDistRange = m_lights[index].fLightFarPlane / 4.f;
        m_lights[index].fLightFalloffCosAngleEnd = cosf(m_lights[index].fLightFOV / 2.0f);
        m_lights[index].fLightFalloffCosAngleRange = 0.1f;
    }

    D3DXVec3Normalize(&m_lights[index].vLightDir, &m_lights[index].vLightDir);
}


void Scene::SetAllInstancesToMeshW(LPWSTR wzName)
{
    char szName[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, wzName, -1, szName, MAX_PATH, NULL, FALSE);
    SetAllInstancesToMesh(szName);
}

void Scene::SetAllInstancesToMesh(LPSTR szName)
{
    for(int iInstance = 0; iInstance < g_iMaxInstances; iInstance++)
    {
        SetMeshForInstance(iInstance, szName);
    }
}

void Scene::SetMeshForInstance(int iInstance, LPSTR szName)
{
    for(UINT iMesh = 0; iMesh < m_SDKMeshes.size(); iMesh++)
    {
        if(m_SDKMeshes[iMesh].pMesh)
        {
            if(_strcmpi(m_SDKMeshes[iMesh].pMesh->szName, szName) == 0)
            {
                m_iInstanceMeshIndices[iInstance] = iMesh;
            }
        }
    }
}

/*
    $$ NOTE the caller MUST free this memory!!

    $$$ Double note, this function is dumb right now.  Need to update it to do view frustum culling
*/
void Scene::GetAllActiveMeshes(const D3DXMATRIX* view, UINT** ppRenderMeshes, UINT* iNum)
{
    DC_UNREFERENCED_PARAM(view);
    *ppRenderMeshes = new UINT[m_iNumActiveInstances];
    *iNum = m_iNumActiveInstances;

    for(int i = 0; i < m_iNumActiveInstances; i++)
    {
        (*ppRenderMeshes)[i] = i;
    }
}

/*
    Returns all meshe instances sorted by mesh
*/
void GenerateRenderMeshesByMesh(const D3DXMATRIX* view, UINT** ppRenderMeshes, UINT* iNum)
{
    (void)view;
    (void)ppRenderMeshes;
    (void)iNum;
}

// This method can be called from various render threads
D3DXMATRIX* Scene::GetWorldMatrixFor(UINT iMeshInstance)
{
    // $$ TODO, put thread blocking code here on mesh update completion

    if(iMeshInstance < 0 || iMeshInstance >= g_iMaxInstances)
        return &m_MeshWorlds[1];

    return &m_MeshWorlds[iMeshInstance];
}

D3DXVECTOR4& Scene::GetMeshColorFor(UINT iMeshInstance)
{
    if(iMeshInstance < 0 || iMeshInstance >= g_iMaxInstances)
        return m_MeshColors[0];

    return m_MeshColors[iMeshInstance];
}

D3DXMATRIX* Scene::GetPreviousWorldMatrixFor(UINT iMeshInstance)
{
    if(iMeshInstance < 0 || iMeshInstance >= g_iMaxInstances)
        return &m_MeshPreviousWorlds[1];

    return &m_MeshPreviousWorlds[iMeshInstance];
}

// Note this only varies active instances, inactive instances will not change, so if the instance count goes up
//  the added instances will have whatever mesh they had previously
void Scene::VaryMeshes()
{
    for(unsigned int iInstance = 0; iInstance < (unsigned int)m_iNumActiveInstances; iInstance++)
        m_iInstanceMeshIndices[iInstance] = iInstance % m_SDKMeshes.size();
}


struct MeshIndex
{
    int iInstanceIndex;
    int iMeshIndex;
};

static int CreateSortedMeshIndices_CompareFunc(const void* a, const void* b)
{
    const MeshIndex* sa = reinterpret_cast<const MeshIndex*>(a);
    const MeshIndex* sb = reinterpret_cast<const MeshIndex*>(b);

    return sa->iInstanceIndex - sb->iInstanceIndex;
}

// craete another mesh indices for instance rendering.
void Scene::CreateSortedMeshIndices()
{
    MeshIndex* sortWrk = new MeshIndex[m_iNumActiveInstances];

    for(unsigned int iInstance = 0; iInstance < (unsigned int)m_iNumActiveInstances; iInstance++)
    {
        sortWrk[iInstance].iInstanceIndex = m_iInstanceMeshIndices[iInstance];
        sortWrk[iInstance].iMeshIndex = iInstance;
    }

    std::qsort(sortWrk, m_iNumActiveInstances, sizeof(MeshIndex), CreateSortedMeshIndices_CompareFunc);

    for(unsigned int iInstance = 0; iInstance < (unsigned int)m_iNumActiveInstances; iInstance++)
    {
        m_iSortedMeshIndices[iInstance] = sortWrk[iInstance].iMeshIndex;
    }

    delete [] sortWrk;
}

void Scene::InitializeUpdateThreads()
{
    for(int iThread = 0; iThread < g_iMaxNumUpdateThreads; iThread++)
    {
        m_hBeginInstanceUpdateEvent[iThread] = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndInstanceUpdateEvent[iThread] = CreateEvent(NULL, FALSE, FALSE, NULL);

        m_threadInitData[iThread].iThreadIndex = iThread;
        m_threadInitData[iThread].pd3dDevice = NULL;    // TODO, set if necessary!
        m_threadInitData[iThread].pThis = this;

        m_hInstanceUpdateThreads[iThread] = (HANDLE)_beginthreadex(
                                                NULL,
                                                0,
                                                _UpdateMeshInstancesProc,
                                                &m_threadInitData[iThread],
                                                CREATE_SUSPENDED,
                                                NULL);

        ResumeThread(m_hInstanceUpdateThreads[iThread]);
    }
}

unsigned int WINAPI Scene::_UpdateMeshInstancesProc(LPVOID lpParameter)
{
    const UPDATE_THREAD_PARAMS* pParams = (UPDATE_THREAD_PARAMS*)lpParameter;

    Scene* pScene = (Scene*)pParams->pThis;

    int iThreadIndex = pParams->iThreadIndex;

    if(iThreadIndex < 0 || iThreadIndex >= g_iMaxNumUpdateThreads) return (unsigned int) - 1;


    for(;;)
    {
        // Wait for main thread to signal ready
        WaitForSingleObject(pScene->m_hBeginInstanceUpdateEvent[iThreadIndex], INFINITE);

        if(pScene->m_bTermThreads)
        {
            DEBUG_THREADING_LOG_1("InstanceUpdate ( %d ) : _endthreadex !!\n", iThreadIndex);
            _endthreadex(0);    // term and signal main thread of completion
        }

        DEBUG_THREADING_LOG_1("InstanceUpdate ( %d ) : Start updating !!\n", iThreadIndex);
        int iStart = pScene->m_threadShared[iThreadIndex].iStartMeshIndex;
        int iEnd = pScene->m_threadShared[iThreadIndex].iEndMeshIndex;
        float fTime = pScene->m_threadShared[iThreadIndex].fTime;
        float fElapsedTime = pScene->m_threadShared[iThreadIndex].fElapsedTime;

        for(int iMesh = iStart; iMesh < iEnd; iMesh++)
        {
            DEBUG_THREADING_LOG_2("InstanceUpdate ( %d ) : Update mesh %d !!\n", iThreadIndex, iMesh);
            pScene->UpdateInstance(iMesh, fTime, fElapsedTime);
        }

        // Tell main thread command list is finished
        SetEvent(pScene->m_hEndInstanceUpdateEvent[iThreadIndex]);

        DEBUG_THREADING_LOG_1("InstanceUpdate ( %d ) : Set End Event !!\n", iThreadIndex);
    }
}

void Scene::SetUpdateThreads(int threads)
{
    m_iNumUpdateThreads = threads;
}

bool Scene::HasMeshUpdated(UINT iMeshInstance)
{
    DC_UNREFERENCED_PARAM(iMeshInstance);
    return true; // $$TODO
}


NvSimpleMesh* Scene::GetMeshFor(UINT iInstance)
{
    if(iInstance < 0 || iInstance >= g_iMaxInstances)
        return m_SDKMeshes[0].pMesh;

    int iMesh = m_iInstanceMeshIndices[iInstance];

    return m_SDKMeshes[iMesh].pMesh;
}

//just rotate lights a little in place
void Scene::UpdateLights(double fTime)
{
    D3DXVECTOR3 offAngleVec3 = D3DXVECTOR3(1, -4, 0);
    D3DXMATRIX rotY;
    D3DXVECTOR3 dir;

    D3DXVec3Normalize(&offAngleVec3, &offAngleVec3);
    D3DXMatrixRotationY(&rotY, (float)(fTime / 4.f) * 3.14159265f);
    D3DXVec3TransformNormal(&dir, &offAngleVec3, &rotY);
    m_lights[0].vLightDir = dir;

    D3DXVec3Normalize(&offAngleVec3, &offAngleVec3);
    D3DXMatrixRotationY(&rotY, (float)(-fTime / 3.f) * 3.14159265f);
    D3DXVec3TransformNormal(&dir, &offAngleVec3, &rotY);
    m_lights[1].vLightDir = dir;

    D3DXVec3Normalize(&offAngleVec3, &offAngleVec3);
    D3DXMatrixRotationY(&rotY, (float)(fTime / 2.f) * 3.14159265f);
    D3DXVec3TransformNormal(&dir, &offAngleVec3, &rotY);
    m_lights[2].vLightDir = dir;

    D3DXVec3Normalize(&offAngleVec3, &offAngleVec3);
    D3DXMatrixRotationY(&rotY, (float)(-fTime / 4.f) * 3.14159265f);
    D3DXVec3TransformNormal(&dir, &offAngleVec3, &rotY);
    m_lights[3].vLightDir = dir;
}

void Scene::UpdateScene(double fTime, float fElapsedTime)
{
    {
        static bool bOneTime = true;

        if(bMovingLights || bOneTime)
        {
            UpdateLights(fTime);
            bOneTime = false;
        }
    }

        if(bMovingMeshes)
    {
        // copy current to previous (for use in per instance logic)
        memcpy((void*)m_MeshPreviousWorlds, (void*)m_MeshWorlds, m_iNumActiveInstances * sizeof(D3DXMATRIX));

        if(m_iNumUpdateThreads == 0)    // non threaded updates
        {
            for(int i = 0; i < m_iNumActiveInstances; i++)
            {
                UpdateInstance(i, (float)fTime, fElapsedTime);
            }
        }
        else
        {
            for(int iThread = 0; iThread < m_iNumUpdateThreads; iThread++)
            {
                int iStart = iThread * m_iNumActiveInstances / m_iNumUpdateThreads;
                int iEnd = iStart + (m_iNumActiveInstances / m_iNumUpdateThreads) - 1;

                if(iThread == m_iNumUpdateThreads - 1)
                    iEnd = m_iNumActiveInstances - 1;    // last thread catches the slop

                m_threadShared[iThread].iStartMeshIndex = iStart;
                m_threadShared[iThread].iEndMeshIndex = iEnd;
                m_threadShared[iThread].fTime = (float)fTime;
                m_threadShared[iThread].fElapsedTime = (float)fElapsedTime;


                // set event to begin on all scene thread
                SetEvent(m_hBeginInstanceUpdateEvent[iThread]);

                m_threadActive[iThread] = true;
            }

            // $$ For now just block, but in the future, render can use m_threadActive + start/end to know if a mesh is ready to be rendered;

            // wait for completion
            WaitForMultipleObjects(m_iNumUpdateThreads,
                                   m_hEndInstanceUpdateEvent,
                                   TRUE,
                                   INFINITE);

            for(int iThread = 0; iThread < m_iNumUpdateThreads; iThread++)
            {
                m_threadActive[iThread] = false;
            }
        }
    }
}

void Scene::UpdateInstance(int iInstance, float fTime, float fElapsedTime)
{
    if(!bMovingMeshes) return;

    float aggregateScale = m_fScale;

    if(m_SDKMeshes.size() > 0)
    {
        // aggregate in a per mesh scale
        aggregateScale *= m_SDKMeshes[m_iInstanceMeshIndices[iInstance]].fScale;
    }

    if(m_pShips[iInstance] != NULL)
        m_pShips[iInstance]->Update(fTime, fElapsedTime, aggregateScale);
}

UINT Scene::GetTotalPolys()
{
    UINT polys = 0;

    for(int i = 0; i < m_iNumActiveInstances; i++)
    {
        polys += (UINT)m_SDKMeshes[m_iInstanceMeshIndices[i]].iPolys;
    }

    return polys;
}

// simulates some load
float Scene::CPUGameLoadMethod(float fTime)
{
    UINT d = 0;
    FLOAT f = fTime;

    for(UINT i = 0; i < 100; i++)
    {
        d++;
        f += sin(f);
        f = sin(f);

        if(d % m_iNumActiveInstances || f == 0)
            continue;

        d += -1 + (int)f ;
    }

    return (float)d / 100.0f;
}