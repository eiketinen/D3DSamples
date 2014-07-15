//----------------------------------------------------------------------------------
// File:        include\nvsimplemesh/NvSimpleMesh.h
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

class NvSimpleRawMesh;
class NvSimpleMeshLoader;

class NvSimpleMesh
{
public:
    NvSimpleMesh();

    HRESULT Initialize(ID3D11Device *pd3dDevice,NvSimpleRawMesh *pRawMesh);
    HRESULT InitializeWithInputLayout(ID3D11Device *pd3dDevice,NvSimpleRawMesh *pRawMesh,BYTE*pIAsig, SIZE_T pIAsigSize);
    HRESULT CreateInputLayout(ID3D11Device *pd3dDevice,BYTE*pIAsig, SIZE_T pIAsigSize);
    void Release();

    void SetupDraw(ID3D11DeviceContext *pd3dContext, int iDiffuseTexSlot=-1, int iNormalsTexSlot=-1);
    void Draw(ID3D11DeviceContext *pd3dContext);
    void DrawInstanced(ID3D11DeviceContext *pd3dContext, int iNumInstances);

    UINT iNumVertices;
    UINT iNumIndices;
    DXGI_FORMAT IndexFormat;
    UINT VertexStride;

    D3DXVECTOR3 Extents;
    D3DXVECTOR3 Center;

    ID3D11InputLayout *pInputLayout;

    ID3D11Buffer *pVB;
    ID3D11Buffer *pIB;
    ID3D11Texture2D *pDiffuseTexture;
    ID3D11ShaderResourceView *pDiffuseSRV;
    ID3D11Texture2D *pNormalsTexture;
    ID3D11ShaderResourceView *pNormalsSRV;
    char szName[260];
};

class NvAggregateSimpleMesh
{
public:
    NvAggregateSimpleMesh();
    ~NvAggregateSimpleMesh();

    HRESULT Initialize(ID3D11Device *pd3dDevice,NvSimpleMeshLoader *pMeshLoader);
    HRESULT InitializeWithInputLayout(ID3D11Device *pd3dDevice,NvSimpleMeshLoader *pMeshLoader,BYTE*pIAsig, SIZE_T pIAsigSize);
    void Release();

    void Draw(ID3D11DeviceContext *pd3dContext, int iDiffuseTexSlot=-1, int iNormalsTexSlot=-1);

    int NumSimpleMeshes;
    NvSimpleMesh *pSimpleMeshes;
};