//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\utility/Scene.h
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

const D3DXVECTOR3                 g_vUp(0.0f, 1.0f, 0.0f);
const D3DXVECTOR3                 g_vDown                 = -g_vUp;
const FLOAT                       g_fSceneRadius          = 4000.0f;

// By convention, the first g_iNumShadows lights are shadow casting, and the rest just illuminate.
const int   g_iNumShadows = 1;
const int   g_iNumLights = 4;
const int    g_iDeferredLoadMaxThreadCount = 8;
const unsigned int g_iMaxNumUpdateThreads = 8;

struct DC_Light
{
    D3DXVECTOR4                 vLightColor;
    D3DXVECTOR3                 vLightPos;
    D3DXVECTOR3                 vLightDir;
    FLOAT                       fLightFalloffDistEnd;
    FLOAT                       fLightFalloffDistRange;
    FLOAT                       fLightFalloffCosAngleEnd;
    FLOAT                       fLightFalloffCosAngleRange;
    FLOAT                       fLightFOV;
    FLOAT                       fLightAspect;
    FLOAT                       fLightNearPlane;
    FLOAT                       fLightFarPlane;
};


class NvSimpleMesh;

// Simple scene structure used when rendering
//  This can be thought of as the game engine owning and driving object instances in the world and providing the mesh and position information to the renderer
class Scene
{

public:
    Scene(D3DXVECTOR3& initialWorldSize);
    ~Scene();

    void AddMeshToLoad(LPWSTR wzName, LPWSTR wzFile);

    void LoadQueuedContent(ID3D11Device* pd3dDevice);
    void FreeAllMeshes();

    void SetAllInstancesToMesh(LPSTR szName);
    void SetAllInstancesToMeshW(LPWSTR wzName);
    void SetGlobalScale(float scale);
    void SetNumInstances(int num);
    void SetMeshForInstance(int i, LPSTR szName);

    void SetLight(int index, DC_Light& light);
    int NumLights() {return g_iNumLights;}
    DC_Light& GetLight(int i) {return m_lights[i];}

    void GetAllActiveMeshes(const D3DXMATRIX* view, UINT** ppRenderMeshes, UINT* iNum);
    void GenerateRenderMeshesByMesh(const D3DXMATRIX* view, UINT** ppRenderMeshes, UINT* iNum);

    D3DXMATRIX* GetWorldMatrixFor(UINT iMeshInstance);
    D3DXVECTOR4& GetMeshColorFor(UINT iMeshInstance);
    D3DXMATRIX* GetPreviousWorldMatrixFor(UINT iMeshInstance);
    NvSimpleMesh* GetMeshFor(UINT iMeshInstance);
    int GetMeshIndexFor(UINT iMeshInstance) {return m_iInstanceMeshIndices[iMeshInstance];}
    int GetSortedMeshIndex(UINT iMeshInstance) {return m_iSortedMeshIndices[iMeshInstance];}

    void InitializeUpdateThreads();
    int GetUpdateThreads() {return m_iNumUpdateThreads;}
    void SetUpdateThreads(int threads);
    bool HasMeshUpdated(UINT iMeshInstance);

    void UpdateLights(double fTime);
    void UpdateScene(double fTime, float fElapsedTime);

    int NumActiveInstances() {return m_iNumActiveInstances;}

    void VaryMeshes();
    void CreateSortedMeshIndices();
    UINT GetTotalPolys();

    bool bMovingMeshes;
    bool bMovingLights;

protected:

    friend class SimpleShipController;

    SimpleShipController* m_pShips[g_iMaxInstances];

    void UpdateInstance(int iInstance, float fTime, float fElapsedTime);
    void SetWorldMatrixFor(int index, D3DXMATRIX* pMatrix) { memcpy(&m_MeshWorlds[index], pMatrix, sizeof(D3DXMATRIX));}
    void SetMeshColorFor(int index, D3DXVECTOR4* pColor) {memcpy(&m_MeshColors[index], pColor, sizeof(D3DXVECTOR4));}

    float CPUGameLoadMethod(float fTime);    // simulates some load

    void CreateTextureFromFile(ID3D11Device* pDev, char* szFileName, ID3D11ShaderResourceView** ppRV);

    void LoadQueueContentDirect(ID3D11Device* pd3dDevice, int contentIndex);

    struct TOLOAD
    {
        LPWSTR wzName;
        LPWSTR wzFile;
    };

    struct MESHINFO
    {
        NvSimpleMesh* pMesh;
        UINT64        iPolys;
        float        fScale;
    };

    std::vector<TOLOAD>            m_toLoad;    // temp buffer containing meshes to load

    D3DXMATRIX                    m_MeshWorlds[g_iMaxInstances];
    D3DXVECTOR4                    m_MeshColors[g_iMaxInstances];
    D3DXMATRIX                    m_MeshPreviousWorlds[g_iMaxInstances];
    std::vector<MESHINFO>        m_SDKMeshes;
    int                            m_iInstanceMeshIndices[g_iMaxInstances];    // for each mesh, it has an index of the sdk mesh it uses
    int                            m_iSortedMeshIndices[g_iMaxInstances];

    bool                         m_bPositionsDirty;
    float                         m_fScale;    // cache of the last scale we used.

    int                            m_iNumActiveInstances;


    struct UPDATE_THREAD_PARAMS
    {
        int iThreadIndex;    // index into various structures per thread
        class Scene* pThis;    // pointer to the class instance
        ID3D11Device* pd3dDevice;
    };

    struct UPDATE_THREAD_SHARED
    {
        int iStartMeshIndex;
        int iEndMeshIndex;
        float fTime;
        float fElapsedTime;
    };

    bool                        m_bTermThreads;
    static unsigned int WINAPI  _UpdateMeshInstancesProc(LPVOID lpParameter);
    int                            m_iNumUpdateThreads;
    HANDLE                        m_hInstanceUpdateThreads[g_iMaxNumUpdateThreads];
    HANDLE                        m_hBeginInstanceUpdateEvent[g_iMaxNumUpdateThreads];
    HANDLE                        m_hEndInstanceUpdateEvent[g_iMaxNumUpdateThreads];
    UPDATE_THREAD_PARAMS        m_threadInitData[g_iMaxNumUpdateThreads];
    UPDATE_THREAD_SHARED        m_threadShared[g_iMaxNumUpdateThreads];
    bool                        m_threadActive[g_iMaxNumUpdateThreads];


    // our parameterized lights (set by caller)
    DC_Light                    m_lights[g_iNumLights];

};