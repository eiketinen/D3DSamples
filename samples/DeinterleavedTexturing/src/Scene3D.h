//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src/Scene3D.h
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
#include <DXUT.h>
#include <strsafe.h>
#include <SDKmisc.h>
#include <SDKmesh.h>

#define MAX_PATH_STR 512

typedef enum
{
    SCENE_NO_GROUND_PLANE,
    SCENE_USE_GROUND_PLANE,
} SceneGroundPlaneType;

typedef enum
{
    SCENE_NO_SHADING,
    SCENE_USE_SHADING,
} SceneShadingType;

typedef enum
{
    SCENE_USE_ORBITAL_CAMERA,
    SCENE_USE_FIRST_PERSON_CAMERA,
} SceneCameraType;

struct MeshDescriptor
{
    LPWSTR Name;
    LPWSTR Path;
    SceneGroundPlaneType UseGroundPlane;
    SceneShadingType UseShading;
    SceneCameraType CameraType;
    float SceneScale;
};

struct SceneMesh
{
    HRESULT OnCreateDevice(ID3D11Device* pd3dDevice, MeshDescriptor &MeshDesc)
    {
        HRESULT hr = S_OK;
        WCHAR Filepath[MAX_PATH_STR+1];
        V_RETURN( DXUTFindDXSDKMediaFileCch(Filepath, MAX_PATH_STR, MeshDesc.Path) );
        V_RETURN( m_SDKMesh.Create(pd3dDevice, Filepath) );

        D3DXVECTOR3 bbExtents = m_SDKMesh.GetMeshBBoxExtents(0);
        D3DXVECTOR3 bbCenter  = m_SDKMesh.GetMeshBBoxCenter(0);
        m_GroundHeight = bbCenter.y - bbExtents.y;
        m_Desc = MeshDesc;
        return hr;
    }

    void OnDestroyDevice()
    {
        m_SDKMesh.Destroy();
    }

    CDXUTSDKMesh &GetSDKMesh()
    {
        return m_SDKMesh;
    }

    bool UseShading()
    {
        return (m_Desc.UseShading == SCENE_USE_SHADING);
    }

    bool UseGroundPlane()
    {
        return (m_Desc.UseGroundPlane == SCENE_USE_GROUND_PLANE);
    }

    bool UseOrbitalCamera()
    {
        return (m_Desc.CameraType == SCENE_USE_ORBITAL_CAMERA);
    }

    float GetSceneScale()
    {
        return m_Desc.SceneScale;
    }

    float GetGroundHeight()
    {
        return m_GroundHeight;
    }

protected:
    CDXUTSDKMesh    m_SDKMesh;
    float           m_GroundHeight;
    MeshDescriptor  m_Desc;
};

struct GlobalConstantBuffer
{
    D3DXMATRIX WorldView;
    D3DXMATRIX WorldViewInverse;
    D3DXMATRIX WorldViewProjection;
    float      IsWhite;
    float      Pad[3];
};

struct SceneViewInfo
{
    D3DXMATRIX WorldViewMatrix;
    D3DXMATRIX ProjectionMatrix;
};

class SceneRenderer
{
public:
    SceneRenderer();

    HRESULT OnCreateDevice(ID3D11Device* pd3dDevice);
    void OnFrameRender(const SceneViewInfo* pSceneView, SceneMesh* pMesh);
    void CopyColors(ID3D11ShaderResourceView* pColorSRV);
    void OnDestroyDevice();

private:

    ID3D11DeviceContext*        m_D3DImmediateContext;

    ID3D11InputLayout*          m_pInputLayout;
    ID3D11Buffer*               m_VertexBuffer;
    ID3D11Buffer*               m_IndexBuffer;

    GlobalConstantBuffer        m_Constants;
    ID3D11Buffer*               m_pConstantBuffer;

    ID3D11BlendState*           m_pBlendState_Disabled;
    ID3D11RasterizerState*      m_pRasterizerState_NoCull_NoScissor;
    ID3D11DepthStencilState*    m_pDepthStencilState_Disabled;
    ID3D11DepthStencilState*    m_pDepthStencilState_Enabled;
    ID3D11SamplerState*         m_pSamplerState_PointClamp;

    ID3D11VertexShader*         m_pFullScreenTriangleVS;
    ID3D11PixelShader*          m_pCopyColorPS;
    ID3D11VertexShader*         m_pGeometryVS;
    ID3D11PixelShader*          m_pGeometryPS;
};
