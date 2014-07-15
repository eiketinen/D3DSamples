//----------------------------------------------------------------------------------
// File:        include\nvsimplemesh/NvSimpleRawMesh.h
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

class NvSimpleRawMesh
{
public:

    class Vertex
    {
    public:
        float Position[3];
        float Normal[3];
        float UV[2];
        float Tangent[3];
    };

    NvSimpleRawMesh();
    ~NvSimpleRawMesh();

    UINT GetIndexSize();
    INT GetVertexStride();
    INT GetNumVertices();
    INT GetNumIndices();

    Vertex* GetVertices();
    BYTE* GetRawVertices();
    BYTE* GetRawIndices();

    float* GetExtents() {return m_extents;}
    float* GetCenter() {return m_center;}

    static HRESULT TryGuessFilename(WCHAR *szDestBuffer,WCHAR *szMeshFilename, WCHAR *szGuessSuffix);
    ID3D11Texture2D *CreateD3D11DiffuseTextureFor(ID3D11Device *pd3dDevice);
    ID3D11Texture2D *CreateD3D11NormalsTextureFor(ID3D11Device *pd3dDevice);
    ID3D11Buffer *CreateD3D11IndexBufferFor(ID3D11Device *pd3dDevice);
    ID3D11Buffer *CreateD3D11VertexBufferFor(ID3D11Device *pd3dDevice);

    Vertex *m_pVertexData;
    BYTE *m_pIndexData;
    UINT m_iNumVertices;
    UINT m_iNumIndices;
    UINT m_IndexSize;

    float m_extents[3];
    float m_center[3];

    WCHAR m_szMeshFilename[MAX_PATH];
    WCHAR m_szDiffuseTexture[MAX_PATH];
    WCHAR m_szNormalTexture[MAX_PATH];

    // Utils to wrap making d3d11 render buffers from a loader mesh
    static const D3D11_INPUT_ELEMENT_DESC D3D11InputElements[];
    static const int D3D11ElementsSize; 
};