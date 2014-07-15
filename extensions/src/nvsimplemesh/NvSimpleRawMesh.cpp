//----------------------------------------------------------------------------------
// File:        src\nvsimplemesh/NvSimpleRawMesh.cpp
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

#include <assert.h>

// Direct3D9 includes
#include <d3d9.h>
#include <d3dx9.h>

// Direct3D11 includes
#include <d3dcommon.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>

// XInput includes
#include <xinput.h>

// HRESULT translation for Direct3D and other APIs 
#include <dxerr.h>

#ifndef V
#define V(x)           { hr = (x); }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif
#include "strsafe.h"
#include <string>
#include "NvSimpleRawMesh.h"

const D3D11_INPUT_ELEMENT_DESC NvSimpleRawMesh::D3D11InputElements[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

const int NvSimpleRawMesh::D3D11ElementsSize = sizeof(NvSimpleRawMesh::D3D11InputElements)/sizeof(D3D11_INPUT_ELEMENT_DESC);


NvSimpleRawMesh::NvSimpleRawMesh() : 
    m_pVertexData(NULL),
    m_pIndexData(NULL),
    m_iNumVertices(0),
    m_iNumIndices(0),
    m_IndexSize(sizeof(UINT16))
{
    m_szMeshFilename[0] = 0;
    m_szDiffuseTexture[0] = 0;
    m_szNormalTexture[0] = 0;

    m_extents[0] = m_extents[1] = m_extents[2] = 0.f;
    m_center[0] = m_center[1] = m_center[2] = 0.f;
}


NvSimpleRawMesh::~NvSimpleRawMesh()
{
    SAFE_DELETE_ARRAY(m_pVertexData);
    SAFE_DELETE_ARRAY(m_pIndexData);
}

UINT NvSimpleRawMesh::GetIndexSize()
{
    return m_IndexSize;
}

INT NvSimpleRawMesh::GetVertexStride()
{
    return sizeof(Vertex);
}
INT NvSimpleRawMesh::GetNumVertices()
{
    return m_iNumVertices;
}
INT NvSimpleRawMesh::GetNumIndices()
{
    return m_iNumIndices;
}

BYTE * NvSimpleRawMesh::GetRawVertices()
{
    return (BYTE*)m_pVertexData;
}

NvSimpleRawMesh::Vertex * NvSimpleRawMesh::GetVertices()
{
    return m_pVertexData;
}

BYTE * NvSimpleRawMesh::GetRawIndices()
{
    return m_pIndexData;
}

HRESULT NvSimpleRawMesh::TryGuessFilename(WCHAR *szDestBuffer,WCHAR *szMeshFilename, WCHAR *szGuessSuffix)
{
    HRESULT hr = E_FAIL;
/*
    WCHAR szBaseFilename[MAX_PATH];
    WCHAR szGuessFilename[MAX_PATH];
    // Start with mesh file
    StringCchCopyW(szBaseFilename,MAX_PATH,szMeshFilename);
    size_t len = 0;
    StringCchLength(szBaseFilename,MAX_PATH,&len);

    // work backwards to first "." and null which strips extension
    for(int i=(int)len-1;i>=0;i--) 
    {
        if(szBaseFilename[i] == L'.') {szBaseFilename[i] = 0; break;}
    }

    StringCchCopy(szGuessFilename,MAX_PATH,szBaseFilename);
    StringCchCat(szGuessFilename,MAX_PATH,szGuessSuffix);

    hr = DXUTFindDXSDKMediaFileCch(szDestBuffer,MAX_PATH,szGuessFilename);
    */
    return hr;
}

ID3D11Texture2D *NvSimpleRawMesh::CreateD3D11DiffuseTextureFor(ID3D11Device *pd3dDevice)
{
    HRESULT hr = S_OK;
    //if(m_szDiffuseTexture[0] == 0) return NULL;
/*
    WCHAR szTextureFilename[MAX_PATH];

    if(m_szDiffuseTexture[0] != 0)
    {
        hr = DXUTFindDXSDKMediaFileCch(szTextureFilename,MAX_PATH,m_szDiffuseTexture);
    }

    // Try to guess a file name in same location 
    if(hr != S_OK)
    {
        hr = TryGuessFilename(szTextureFilename,m_szMeshFilename,L"_diffuse.dds");
    }
    */
    ID3D11Resource *pTexture = NULL;
    if(hr == S_OK)
    {
        hr = D3DX11CreateTextureFromFile(pd3dDevice,m_szDiffuseTexture,NULL,NULL,&pTexture,&hr);
    }

    return (ID3D11Texture2D*)pTexture;
}

ID3D11Texture2D *NvSimpleRawMesh::CreateD3D11NormalsTextureFor(ID3D11Device *pd3dDevice)
{
    HRESULT hr = S_OK;
    //if(m_szNormalTexture[0] == 0) return NULL;
/*
    WCHAR szTextureFilename[MAX_PATH];
    

    if(m_szNormalTexture[0] != 0)
    {
        hr = DXUTFindDXSDKMediaFileCch(szTextureFilename,MAX_PATH,m_szNormalTexture);
    }

    // Try to guess a file name in same location for normals
    if(hr != S_OK)
    {
        hr = TryGuessFilename(szTextureFilename,m_szMeshFilename,L"_normals.dds");
    }
    if(hr != S_OK)
    {
        hr = TryGuessFilename(szTextureFilename,m_szDiffuseTexture,L"_nm.dds");
    }
    */
    ID3D11Resource *pTexture = NULL;
    if(hr == S_OK)
    {
        hr = D3DX11CreateTextureFromFile(pd3dDevice,m_szNormalTexture,NULL,NULL,&pTexture,&hr);
    }

    return (ID3D11Texture2D*)pTexture;
}

ID3D11Buffer *NvSimpleRawMesh::CreateD3D11IndexBufferFor(ID3D11Device *pd3dDevice)
{
    ID3D11Buffer *pIB = NULL;

    D3D11_BUFFER_DESC Desc;
    ::ZeroMemory(&Desc,sizeof(D3D11_BUFFER_DESC));
    Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    Desc.ByteWidth = GetNumIndices() * GetIndexSize();
    Desc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA SubResData;
    ::ZeroMemory(&SubResData,sizeof(D3D11_SUBRESOURCE_DATA));
    SubResData.pSysMem = (void*)GetRawIndices();

    pd3dDevice->CreateBuffer(&Desc,&SubResData,&pIB);

    return pIB;
}
ID3D11Buffer *NvSimpleRawMesh::CreateD3D11VertexBufferFor(ID3D11Device *pd3dDevice)
{
    ID3D11Buffer *pVB = NULL;

    D3D11_BUFFER_DESC Desc;
    ::ZeroMemory(&Desc,sizeof(D3D11_BUFFER_DESC));
    Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    Desc.ByteWidth = GetNumVertices() * GetVertexStride();
    Desc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA SubResData;
    ::ZeroMemory(&SubResData,sizeof(D3D11_SUBRESOURCE_DATA));
    SubResData.pSysMem = (void*)GetRawVertices();

    pd3dDevice->CreateBuffer(&Desc,&SubResData,&pVB);

    return pVB;
}