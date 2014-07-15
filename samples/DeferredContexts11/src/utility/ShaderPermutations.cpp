//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\utility/ShaderPermutations.cpp
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
#include "RendererBase.h"
#include "ShaderPermutations.h"
#include "NvSimpleRawMesh.h"

HRESULT ShaderPermutations::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, D3D_SHADER_MACRO* pDefines, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    // open the file
    HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if(INVALID_HANDLE_VALUE == hFile)
        return E_FAIL;

    // Get the file size
    LARGE_INTEGER FileSize;
    GetFileSizeEx(hFile, &FileSize);

    // create enough space for the file data
    BYTE* pFileData = new BYTE[ FileSize.LowPart ];

    if(!pFileData)
    {
        delete [] pFileData;
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
    }

    // read the data in
    DWORD BytesRead;

    if(!ReadFile(hFile, pFileData, FileSize.LowPart, &BytesRead, NULL))
    {
        delete [] pFileData;
        CloseHandle(hFile);
        return E_FAIL;
    }

    CloseHandle(hFile);

    // Compile the shader
    ID3DBlob* pErrorBlob;
    hr = D3DCompile(pFileData, FileSize.LowPart, "none", pDefines, NULL, szEntryPoint, szShaderModel,
                    D3D10_SHADER_ENABLE_STRICTNESS, 0, ppBlobOut, &pErrorBlob);

    delete [] pFileData;

    if(FAILED(hr))
    {
        OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        SAFE_RELEASE(pErrorBlob);
        return hr;
    }

    SAFE_RELEASE(pErrorBlob);

    return S_OK;
}


HRESULT ShaderPermutations::OnD3D11CreateDevice(ID3D11Device* pd3dDevice)
{
    HRESULT hr = S_OK;
    // Compile the shaders to a model based on the feature level we acquired
    LPCSTR pszVSModel = NULL;
    LPCSTR pszPSModel = NULL;

    switch(pd3dDevice->GetFeatureLevel())
    {
    case D3D_FEATURE_LEVEL_11_0:
        pszVSModel = "vs_5_0";
        pszPSModel = "ps_5_0";
        break;

    case D3D_FEATURE_LEVEL_10_1:
        pszVSModel = "vs_4_1";
        pszPSModel = "ps_4_1";
        break;

    case D3D_FEATURE_LEVEL_10_0:
        pszVSModel = "vs_4_0";
        pszPSModel = "ps_4_0";
        break;

    case D3D_FEATURE_LEVEL_9_3:
        pszVSModel = "vs_4_0_level_9_3";
        pszPSModel = "ps_4_0_level_9_3";
        break;

    case D3D_FEATURE_LEVEL_9_2: // Shader model 2 fits feature level 9_1
    case D3D_FEATURE_LEVEL_9_1:
        pszVSModel = "vs_4_0_level_9_1";
        pszPSModel = "ps_4_0_level_9_1";
        break;
    }

    D3D_SHADER_MACRO  deflistVS[] =
    {
        {  "VTF_OBJECT_DATA",  "0", },
        {  "ENABLE_VERTEX_COLOR",  "0", },
        {  "SHADER_VARIATION_IDX",  "0", },
        {  NULL, NULL },
    };

    char szNumLights[MAX_PATH];
    char szShadowMap[MAX_PATH];
    sprintf_s(szNumLights, MAX_PATH, "%d", g_iNumLights);
    sprintf_s(szShadowMap, MAX_PATH, "%d", g_iNumLights);
    D3D_SHADER_MACRO  deflistPS[] =
    {
        {  "NUMLIGHTS",  szNumLights, },
        { "NO_SHADOW_MAP", "0", },
        {  "ENABLE_VERTEX_COLOR",  "0", },
        {  "SHADER_VARIATION_IDX",  "0", },
        {  NULL, NULL },
    };


    for (size_t i=0; i<m_shaderVariations; i++) {
        static const char *idxToStr[m_shaderVariations] = {"0", "1", "2"};
        deflistVS[2].Definition = idxToStr[i];    // shader variations.
        deflistPS[3].Definition = idxToStr[i];    // shader variations.

        ID3DBlob* pVertexShaderBuffer = NULL;
        ID3DBlob* pVertexShaderBufferVTF = NULL;
        ID3DBlob* pPixelShaderBuffer = NULL;
        ID3DBlob* pBlob = NULL;

        deflistVS[0].Definition = "0";    // enable VTF world matrices
        deflistVS[1].Definition = "0";    // enabled vertex colors
        V_RETURN(CompileShaderFromFile(L"..\\..\\deferredcontexts11\\assets\\MultithreadedTests11_VS.hlsl", "VSMain", pszVSModel, deflistVS, &pVertexShaderBuffer));
        V_RETURN(pd3dDevice->CreateVertexShader(pVertexShaderBuffer->GetBufferPointer(),
            pVertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShaderNoVTF+i));

#if 0
        deflistVS[0].Definition = "1";    // enable VTF world matrices
        V_RETURN(CompileShaderFromFile(L"..\\..\\deferredcontexts11\\assets\\MultithreadedTests11_VS.hlsl", "VSMain", pszVSModel, deflistVS, &pVertexShaderBufferVTF));
        V_RETURN(pd3dDevice->CreateVertexShader(pVertexShaderBufferVTF->GetBufferPointer(),
            pVertexShaderBufferVTF->GetBufferSize(), NULL, m_pVertexShaderVTF+i));
#endif

        deflistVS[0].Definition = "1";    // enable VTF world matrices
        deflistVS[1].Definition = "1";    // enabled vertex colors
        V_RETURN(CompileShaderFromFile(L"..\\..\\deferredcontexts11\\assets\\MultithreadedTests11_VS.hlsl", "VSMain", pszVSModel, deflistVS, &pVertexShaderBufferVTF));
        V_RETURN(pd3dDevice->CreateVertexShader(pVertexShaderBufferVTF->GetBufferPointer(),
            pVertexShaderBufferVTF->GetBufferSize(), NULL, m_pVertexShaderVTFVertexColor+i));
//        SAFE_RELEASE(pBlob);

        deflistVS[0].Definition = "0";    // enable VTF world matrices
        deflistVS[1].Definition = "1";    // enabled vertex colors
        V_RETURN(CompileShaderFromFile(L"..\\..\\deferredcontexts11\\assets\\MultithreadedTests11_VS.hlsl", "VSMain", pszVSModel, deflistVS, &pBlob));
        V_RETURN(pd3dDevice->CreateVertexShader(pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(), NULL, m_pVertexShaderNoVTFVertexColor+i));
        SAFE_RELEASE(pBlob);

        deflistPS[1].Definition = "0";
        deflistPS[2].Definition = "0";
        V_RETURN(CompileShaderFromFile(L"..\\..\\deferredcontexts11\\assets\\MultithreadedTests11_PS.hlsl", "PSMain", pszPSModel, deflistPS, &pPixelShaderBuffer));
        V_RETURN(pd3dDevice->CreatePixelShader(pPixelShaderBuffer->GetBufferPointer(),
            pPixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader+i));

        deflistPS[1].Definition = "1";    // disable shadow map
        deflistPS[2].Definition = "0";    // disable vertex color
        V_RETURN(CompileShaderFromFile(L"..\\..\\deferredcontexts11\\assets\\MultithreadedTests11_PS.hlsl", "PSMain", pszPSModel, deflistPS, &pBlob));
        V_RETURN(pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(), NULL, m_pPixelShaderNoShadow+i));
        SAFE_RELEASE(pBlob);

        deflistPS[1].Definition = "0";    // enable shadow map
        deflistPS[2].Definition = "1";    // enable vertex color
        V_RETURN(CompileShaderFromFile(L"..\\..\\deferredcontexts11\\assets\\MultithreadedTests11_PS.hlsl", "PSMain", pszPSModel, deflistPS, &pBlob));
        V_RETURN(pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(), NULL, m_pPixelShaderVertexColor+i));
        SAFE_RELEASE(pBlob);

        deflistPS[1].Definition = "1";    // disable shadow map
        deflistPS[2].Definition = "1";    // enable vertex color
        V_RETURN(CompileShaderFromFile(L"..\\..\\deferredcontexts11\\assets\\MultithreadedTests11_PS.hlsl", "PSMain", pszPSModel, deflistPS, &pBlob));
        V_RETURN(pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(), NULL, m_pPixelShaderNoShadowVertexColor+i));
        SAFE_RELEASE(pBlob);

        // One straightup from the mesh class
        V_RETURN(pd3dDevice->CreateInputLayout(NvSimpleRawMesh::D3D11InputElements,
            NvSimpleRawMesh::D3D11ElementsSize,
            pVertexShaderBuffer->GetBufferPointer(),
            pVertexShaderBuffer->GetBufferSize(),
            m_pVertexLayoutNoVTF+i));

        // copy the simple mesh input layout as that is what we are loading
        D3D11_INPUT_ELEMENT_DESC* UncompressedLayout = new D3D11_INPUT_ELEMENT_DESC[NvSimpleRawMesh::D3D11ElementsSize + 1];
        memcpy(UncompressedLayout, NvSimpleRawMesh::D3D11InputElements, (NvSimpleRawMesh::D3D11ElementsSize + 1)*sizeof(D3D11_INPUT_ELEMENT_DESC));

        // but add in a separate stream which will contain the texcoords to lookup into VTF texture
        const D3D11_INPUT_ELEMENT_DESC InstanceStreamTexCoords[] = {{ "TEXCOORD",  1, DXGI_FORMAT_R32G32_UINT,   1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 }};
         UncompressedLayout[NvSimpleRawMesh::D3D11ElementsSize] = InstanceStreamTexCoords[0];

        V_RETURN(pd3dDevice->CreateInputLayout(UncompressedLayout,
            NvSimpleRawMesh::D3D11ElementsSize + 1,
            pVertexShaderBufferVTF->GetBufferPointer(),
            pVertexShaderBufferVTF->GetBufferSize(),
            m_pVertexLayoutVTFStream+i));

        delete [] UncompressedLayout;
        SAFE_RELEASE(pVertexShaderBufferVTF);
        SAFE_RELEASE(pVertexShaderBuffer);
        SAFE_RELEASE(pPixelShaderBuffer);
    }

    return hr;
}

void ShaderPermutations::OnD3D11DestroyDevice()
{
    for (size_t i=0; i<m_shaderVariations; i++) {
        SAFE_RELEASE(m_pVertexLayoutNoVTF[i]);
        SAFE_RELEASE(m_pVertexLayoutVTFStream[i]);
        SAFE_RELEASE(m_pVertexShaderNoVTF[i]);
        SAFE_RELEASE(m_pVertexShaderNoVTFVertexColor[i]);
        SAFE_RELEASE(m_pPixelShader[i]);
        SAFE_RELEASE(m_pPixelShaderNoShadow[i]);
        SAFE_RELEASE(m_pPixelShaderNoShadowVertexColor[i]);
        SAFE_RELEASE(m_pVertexShaderVTF[i]);
        SAFE_RELEASE(m_pVertexShaderVTFVertexColor[i]);
        SAFE_RELEASE(m_pPixelShaderVertexColor[i]);
    }
}