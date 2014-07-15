//----------------------------------------------------------------------------------
// File:        SoftShadows\src/stdafx.h
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

#define NOMINMAX

#include <DXUT.h>
#include <DXUTcamera.h>
#include <DXUTgui.h>
#include <DXUTsettingsDlg.h>

#include <d3dx11effect.h>

#include <NvSimpleRawMesh.h>
#include <NvSimpleMeshLoader.h>
#include <NvSimpleMesh.h>

#include <SDKmisc.h>
#include <SDKMesh.h>

#include <strsafe.h>

#include <limits>
#include <memory>
#include <string>

#include "resource.h"
#include "D3D11SavedState.h"

#define VB(exp) V((exp) ? S_OK : E_FAIL)
#define VB_RETURN(exp) V_RETURN((exp) ? S_OK : E_FAIL)

//--------------------------------------------------------------------------------------
// Deleter for COM objects. For use with std::unique_ptr.
//--------------------------------------------------------------------------------------
template <typename Type>
struct safe_release
{
    safe_release()
    {
    }

    // Undefined...
    template <typename Other>
    safe_release(const safe_release<Other> &);

    void operator()(Type *p) const
    {
        if (nullptr != p)
        {
            p->Release();
        }
    }
};

// Workaround for missing typedef templates in VS2010
template <typename T>
struct unique_ref_ptr
{
    typedef std::unique_ptr<T, safe_release<T>> type;
};

template <typename T>
inline typename unique_ref_ptr<T>::type unique_ref(T *value)
{
    return unique_ref_ptr<T>::type(value);
}

//--------------------------------------------------------------------------------------
// RAII scope object for D3DPERF_BeginEvent/D3DPERF_EndEvent
//--------------------------------------------------------------------------------------
class D3DPerfScope
{
public:

    D3DPerfScope(const D3DCOLOR &color, LPCWSTR name) { D3DPERF_BeginEvent(color, name); }
    ~D3DPerfScope() { D3DPERF_EndEvent(); }

private:

    // Non-copyable
    D3DPerfScope(const D3DPerfScope &);
    D3DPerfScope &operator =(const D3DPerfScope &);
};

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
inline HRESULT compileFromFile(
    unique_ref_ptr<ID3DBlob>::type &blobOut,
    wchar_t *fileName,
    LPCSTR szEntryPoint,
    LPCSTR szShaderModel,
    D3D10_SHADER_MACRO* defines = nullptr,
    LPD3D10INCLUDE include = nullptr)
{
    HRESULT hr = S_OK;

    // find the file
    wchar_t str[MAX_PATH];
    V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, fileName));

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob *pBlobOut = nullptr;
    ID3DBlob *pErrorBlob = nullptr;
    hr = D3DX11CompileFromFile(str, defines, include, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, &pBlobOut, &pErrorBlob, NULL);
    blobOut.reset(pBlobOut);
    auto errorBlob = unique_ref(pErrorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA((char *)errorBlob->GetBufferPointer());
        return hr;
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    D3DXVECTOR3 pos;
    D3DXVECTOR3 normal; 
};

//--------------------------------------------------------------------------------------
inline HRESULT createPlane(
    unique_ref_ptr<ID3D11Buffer>::type &indexBuffer,
    unique_ref_ptr<ID3D11Buffer>::type &vertexBuffer,
    ID3D11Device *device,
    float radius,
    float height)
{
    HRESULT hr;
    SimpleVertex vertices[] =
    {
        { D3DXVECTOR3( -radius, height,  radius ), D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) },
        { D3DXVECTOR3(  radius, height,  radius ), D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) },
        { D3DXVECTOR3(  radius, height, -radius ), D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) },
        { D3DXVECTOR3( -radius, height, -radius ), D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) },
    };
    D3D11_BUFFER_DESC bd;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = vertices;
    ID3D11Buffer *vb = nullptr;
    V_RETURN(device->CreateBuffer(&bd, &initData, &vb));
    vertexBuffer.reset(vb);

    DWORD indices[] =
    {
        0,1,2,
        0,2,3,
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(DWORD) * 6;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    initData.pSysMem = indices;
    ID3D11Buffer *ib = nullptr;
    V_RETURN(device->CreateBuffer(&bd, &initData, &ib));
    indexBuffer.reset(ib);
    
    return S_OK;
}

//--------------------------------------------------------------------------------------
inline void transformBoundingBox(D3DXVECTOR3 bbox2[2], const D3DXVECTOR3 bbox1[2], const D3DXMATRIX &matrix)
{
    bbox2[0][0] = bbox2[0][1] = bbox2[0][2] =  std::numeric_limits<float>::max();
    bbox2[1][0] = bbox2[1][1] = bbox2[1][2] = -std::numeric_limits<float>::max();
    // Transform the vertices of BBox1 and extend BBox2 accordingly
    for (int i = 0; i < 8; ++i)
    {
        D3DXVECTOR3 v, v1;
        v[0] = bbox1[(i & 1) ? 0 : 1][0];
        v[1] = bbox1[(i & 2) ? 0 : 1][1];
        v[2] = bbox1[(i & 4) ? 0 : 1][2];
        D3DXVec3TransformCoord(&v1, &v, &matrix);
        for (int j = 0; j < 3; ++j)
        {
            bbox2[0][j] = std::min(bbox2[0][j], v1[j]);
            bbox2[1][j] = std::max(bbox2[1][j], v1[j]);
        }
    }
}