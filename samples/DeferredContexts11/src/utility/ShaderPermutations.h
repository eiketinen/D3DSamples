//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\utility/ShaderPermutations.h
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

//#include "RendererBase.h"

/*
    Just a utility class that manages the various shader permutations for the various modes of the deferred contexts sample
*/
class ShaderPermutations
{
public:
    static const int            m_shaderVariations = 3;

    ShaderPermutations()
    {
        for (size_t i=0; i<m_shaderVariations; i++) {
            m_pVertexLayoutVTFStream[i] = NULL;
            m_pVertexShaderVTF[i] = NULL;
            m_pVertexShaderNoVTF[i] = NULL;
            m_pVertexShaderNoVTFVertexColor[i] = NULL;
            m_pPixelShader[i] = NULL;
            m_pPixelShaderNoShadow[i] = NULL;
            m_pVertexLayoutNoVTF[i] = NULL;
            m_pVertexShaderVTFVertexColor[i] = NULL;
            m_pPixelShaderVertexColor[i] = NULL;
            m_pPixelShaderNoShadowVertexColor[i] = NULL;
        }
    };
    HRESULT OnD3D11CreateDevice(ID3D11Device* pd3dDevice);
    void OnD3D11DestroyDevice();

    static HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, D3D_SHADER_MACRO* pDefines, ID3DBlob** ppBlobOut);

    ID3D11InputLayout*          m_pVertexLayoutVTFStream[m_shaderVariations];
    ID3D11InputLayout*          m_pVertexLayoutNoVTF[m_shaderVariations];
    ID3D11VertexShader*         m_pVertexShaderVTF[m_shaderVariations];
    ID3D11VertexShader*         m_pVertexShaderNoVTF[m_shaderVariations];
    ID3D11VertexShader*         m_pVertexShaderNoVTFVertexColor[m_shaderVariations];
    ID3D11PixelShader*          m_pPixelShader[m_shaderVariations];
    ID3D11PixelShader*          m_pPixelShaderNoShadow[m_shaderVariations];

    ID3D11VertexShader*         m_pVertexShaderVTFVertexColor[m_shaderVariations];
    ID3D11PixelShader*          m_pPixelShaderVertexColor[m_shaderVariations];
    ID3D11PixelShader*          m_pPixelShaderNoShadowVertexColor[m_shaderVariations];
};