//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src/Scene3D.cpp
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

#include "Scene3D.h"
#include "CompileHLSL.h"

struct Scene3DVertex
{
    D3DXVECTOR3 pos;
    D3DXVECTOR3 nor;
};

D3D11_INPUT_ELEMENT_DESC VertexLayout[] =
{
    { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
SceneRenderer::SceneRenderer()
    : m_pInputLayout(NULL)
    , m_VertexBuffer(NULL)
    , m_D3DImmediateContext(NULL)
    , m_pConstantBuffer(NULL)
{
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
HRESULT SceneRenderer::OnCreateDevice(ID3D11Device* pd3dDevice)
{
    HRESULT hr = S_OK;

    pd3dDevice->GetImmediateContext(&m_D3DImmediateContext);

    {
        // Create the floor plane
        float w = 1.0f;
        Scene3DVertex vertices[] =
        {
            { D3DXVECTOR3(-w, 0,  w), D3DXVECTOR3(0.0f, 1.0f, 0.0f) },
            { D3DXVECTOR3( w, 0,  w), D3DXVECTOR3(0.0f, 1.0f, 0.0f) },
            { D3DXVECTOR3( w, 0, -w), D3DXVECTOR3(0.0f, 1.0f, 0.0f) },
            { D3DXVECTOR3(-w, 0, -w), D3DXVECTOR3(0.0f, 1.0f, 0.0f) },
        };

        D3D11_BUFFER_DESC bd;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(Scene3DVertex) * 4;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        bd.MiscFlags = 0;
        bd.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = vertices;
        V( pd3dDevice->CreateBuffer(&bd, &InitData, &m_VertexBuffer) );
    }

    {
        // Create index buffer
        DWORD indices[] =
        {
            0,1,2,
            0,2,3,
        };

        D3D11_BUFFER_DESC bd;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(DWORD) * 6;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = 0;
        bd.MiscFlags = 0;
        bd.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = indices;
        V( pd3dDevice->CreateBuffer(&bd, &InitData, &m_IndexBuffer) );
    }
    
    {
        // Create constant buffer
        static D3D11_BUFFER_DESC desc = 
        {
             sizeof(m_Constants), //ByteWidth
             D3D11_USAGE_DEFAULT, //Usage
             D3D11_BIND_CONSTANT_BUFFER, //BindFlags
             0, //CPUAccessFlags
             0  //MiscFlags
        };
        V( pd3dDevice->CreateBuffer(&desc, NULL, &m_pConstantBuffer) );
    }

    {
        D3D11_BLEND_DESC BlendStateDesc;
        BlendStateDesc.AlphaToCoverageEnable = FALSE;
        BlendStateDesc.IndependentBlendEnable = TRUE;
        for (int i = 0; i < 8; ++i)
        {
            BlendStateDesc.RenderTarget[i].BlendEnable = FALSE;
            BlendStateDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        }
        V( pd3dDevice->CreateBlendState(&BlendStateDesc, &m_pBlendState_Disabled) );
    }

    {
        static D3D11_RASTERIZER_DESC RasterStateDesc = 
        {D3D11_FILL_SOLID, //FillMode
         D3D11_CULL_NONE, //CullMode
         0x0, //FrontCounterClockwise
         0x0/*0.000000f*/, //DepthBias
                       0.f, //DepthBiasClamp
                       0.f, //SlopeScaledDepthBias
         0x1, //DepthClipEnable
         0x0, //ScissorEnable
         0x0, //MultisampleEnable
         0x0  //AntialiasedLineEnable
        };
        V( pd3dDevice->CreateRasterizerState(&RasterStateDesc, &m_pRasterizerState_NoCull_NoScissor) );
    }

    {
        static D3D11_DEPTH_STENCIL_DESC DepthStencilStateDesc = 
        {0x0, //DepthEnable
         D3D11_DEPTH_WRITE_MASK_ZERO, //DepthWriteMask
         D3D11_COMPARISON_NEVER, //DepthFunc
         0x0, //StencilEnable
         0xFF, //StencilReadMask
         0xFF, //StencilWriteMask
     
        {D3D11_STENCIL_OP_KEEP, //StencilFailOp
         D3D11_STENCIL_OP_KEEP, //StencilDepthFailOp
         D3D11_STENCIL_OP_KEEP, //StencilPassOp
         D3D11_COMPARISON_ALWAYS  //StencilFunc
        }, //FrontFace
     
        {D3D11_STENCIL_OP_KEEP, //StencilFailOp
         D3D11_STENCIL_OP_KEEP, //StencilDepthFailOp
         D3D11_STENCIL_OP_KEEP, //StencilPassOp
         D3D11_COMPARISON_ALWAYS  //StencilFunc
        }  //BackFace
        };
        V( pd3dDevice->CreateDepthStencilState(&DepthStencilStateDesc, &m_pDepthStencilState_Disabled) );

        DepthStencilStateDesc.DepthEnable = TRUE;
        DepthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        DepthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        V( pd3dDevice->CreateDepthStencilState(&DepthStencilStateDesc, &m_pDepthStencilState_Enabled) );
    }

    {
        D3D11_SAMPLER_DESC samplerDesc;
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.BorderColor[0] = 0.f;
        samplerDesc.BorderColor[1] = 0.f;
        samplerDesc.BorderColor[2] = 0.f;
        samplerDesc.BorderColor[3] = 0.f;
        samplerDesc.MinLOD = -D3D11_FLOAT32_MAX;
        samplerDesc.MaxLOD =  D3D11_FLOAT32_MAX;
        V( pd3dDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState_PointClamp) );
    }

    {
        ID3DBlob* pShaderBlob = NULL;
        V( CompileShaderFromFile( L"Scene3D.hlsl", "GeometryVS", "vs_5_0", &pShaderBlob ) );
        V( pd3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), NULL, &m_pGeometryVS) );
        V( pd3dDevice->CreateInputLayout(VertexLayout, ARRAYSIZE(VertexLayout), pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), &m_pInputLayout) );
        pShaderBlob->Release();
    }

    {
        ID3DBlob* pShaderBlob = NULL;
        V( CompileShaderFromFile( L"Scene3D.hlsl", "GeometryPS", "ps_5_0", &pShaderBlob ) );
        V( pd3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), NULL, &m_pGeometryPS) );
        pShaderBlob->Release();
    }

    {
        ID3DBlob* pShaderBlob = NULL;
        V( CompileShaderFromFile( L"Scene3D.hlsl", "FullScreenTriangleVS", "vs_5_0", &pShaderBlob ) );
        V( pd3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), NULL, &m_pFullScreenTriangleVS) );
        pShaderBlob->Release();
    }

    {
        ID3DBlob* pShaderBlob = NULL;
        V( CompileShaderFromFile( L"Scene3D.hlsl", "CopyColorPS", "ps_5_0", &pShaderBlob ) );
        V( pd3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), NULL, &m_pCopyColorPS) );
        pShaderBlob->Release();
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void SceneRenderer::OnFrameRender(const SceneViewInfo* pSceneView, SceneMesh* pMesh)
{
    m_D3DImmediateContext->OMSetBlendState(m_pBlendState_Disabled, NULL, 0xFFFFFFFF);
    m_D3DImmediateContext->RSSetState(m_pRasterizerState_NoCull_NoScissor);

    m_D3DImmediateContext->VSSetShader(m_pGeometryVS, NULL, 0);
    m_D3DImmediateContext->PSSetShader(m_pGeometryPS, NULL, 0);
    m_D3DImmediateContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
    m_D3DImmediateContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);

    // Update constant buffer
    m_Constants.WorldView = pSceneView->WorldViewMatrix;
    m_Constants.WorldViewProjection = m_Constants.WorldView * pSceneView->ProjectionMatrix;
    m_Constants.IsWhite = float(!pMesh->UseShading());
    D3DXMatrixInverse(&m_Constants.WorldViewInverse, NULL, &m_Constants.WorldView);
    m_D3DImmediateContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &m_Constants, 0, 0);

    // Draw mesh
    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    CDXUTSDKMesh &SDKMesh = pMesh->GetSDKMesh();
    pVB[0] = SDKMesh.GetVB11(0,0);
    Strides[0] = (UINT)SDKMesh.GetVertexStride(0,0);
    Offsets[0] = 0;
    m_D3DImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
    m_D3DImmediateContext->IASetIndexBuffer(SDKMesh.GetIB11(0), SDKMesh.GetIBFormat11(0), 0);
    m_D3DImmediateContext->IASetInputLayout(m_pInputLayout);

    for (UINT subset = 0; subset < SDKMesh.GetNumSubsets(0); ++subset)
    {
        SDKMESH_SUBSET* pSubset = SDKMesh.GetSubset(0, subset);
        D3D11_PRIMITIVE_TOPOLOGY PrimType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        m_D3DImmediateContext->IASetPrimitiveTopology(PrimType);

        m_D3DImmediateContext->OMSetDepthStencilState(m_pDepthStencilState_Enabled, 0);
        m_D3DImmediateContext->DrawIndexed((UINT)pSubset->IndexCount, (UINT)pSubset->IndexStart, (UINT)pSubset->VertexStart);
    }

    // Draw ground plane
    if (pMesh->UseGroundPlane())
    {
        D3DXMATRIX mTranslate;
        D3DXMatrixTranslation(&mTranslate, 0.0f, pMesh->GetGroundHeight(), 0.0f);

        m_Constants.WorldView = mTranslate * m_Constants.WorldView;
        m_Constants.WorldViewProjection = m_Constants.WorldView * pSceneView->ProjectionMatrix;
        m_Constants.IsWhite = true;
        D3DXMatrixInverse(&m_Constants.WorldViewInverse, NULL, &m_Constants.WorldView);
        m_D3DImmediateContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &m_Constants, 0, 0);

        UINT stride = sizeof(Scene3DVertex);
        UINT offset = 0;
        m_D3DImmediateContext->IASetVertexBuffers    (0, 1, &m_VertexBuffer, &stride, &offset);
        m_D3DImmediateContext->IASetIndexBuffer      (m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        m_D3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_D3DImmediateContext->IASetInputLayout      (m_pInputLayout);

        m_D3DImmediateContext->OMSetDepthStencilState(m_pDepthStencilState_Enabled, 0);
        m_D3DImmediateContext->DrawIndexed(6, 0, 0);
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void SceneRenderer::CopyColors(ID3D11ShaderResourceView *pColorSRV)
{
    m_D3DImmediateContext->OMSetDepthStencilState(m_pDepthStencilState_Disabled, 0);
    m_D3DImmediateContext->OMSetBlendState(m_pBlendState_Disabled, NULL, 0xFFFFFFFF);
    m_D3DImmediateContext->RSSetState(m_pRasterizerState_NoCull_NoScissor);

    m_D3DImmediateContext->VSSetShader(m_pFullScreenTriangleVS, NULL, 0);
    m_D3DImmediateContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

    m_D3DImmediateContext->PSSetShader(m_pCopyColorPS, NULL, 0);
    m_D3DImmediateContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);
    m_D3DImmediateContext->PSSetSamplers(0, 1, &m_pSamplerState_PointClamp);
    m_D3DImmediateContext->PSSetShaderResources(0, 1, &pColorSRV);

    m_D3DImmediateContext->Draw(3, 0);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void SceneRenderer::OnDestroyDevice()
{
    SAFE_RELEASE(m_pInputLayout);
    SAFE_RELEASE(m_D3DImmediateContext);
    SAFE_RELEASE(m_pConstantBuffer);
    SAFE_RELEASE(m_VertexBuffer);
    SAFE_RELEASE(m_IndexBuffer);
    SAFE_RELEASE(m_pBlendState_Disabled);
    SAFE_RELEASE(m_pRasterizerState_NoCull_NoScissor);
    SAFE_RELEASE(m_pDepthStencilState_Disabled);
    SAFE_RELEASE(m_pDepthStencilState_Enabled);
    SAFE_RELEASE(m_pSamplerState_PointClamp);
    SAFE_RELEASE(m_pFullScreenTriangleVS);
    SAFE_RELEASE(m_pCopyColorPS);
    SAFE_RELEASE(m_pGeometryVS);
    SAFE_RELEASE(m_pGeometryPS);
}
