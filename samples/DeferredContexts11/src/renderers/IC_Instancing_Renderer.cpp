//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\renderers/IC_Instancing_Renderer.cpp
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
#include "Scene.h"
#include "IC_Instancing_Renderer.h"

ID3D11PixelShader* IC_Instancing_Renderer::PickAppropriatePixelShader()
{
    return m_ShaderPermutations.m_pPixelShaderVertexColor[0];
}

ID3D11VertexShader* IC_Instancing_Renderer::PickAppropriateVertexShader()
{
    return m_ShaderPermutations.m_pVertexShaderVTFVertexColor[0];
}

void IC_Instancing_Renderer::OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime)
{
    DC_UNREFERENCED_PARAM(pd3dDevice);

    if(!m_pScene)
    {
#ifdef _DEBUG
        assert(0 && "No Scene set!!");
#endif
        return;
    }

    // This method can only be called by IC renderers
    assert(GetContextType() == RS_IMMEDIATE || GetContextType() == RS_IC_INSTANCING);

    // need to create sorted indices for VTF
    m_pScene->CreateSortedMeshIndices();

    if(bVTFPositions)
    {
        // Update our VTF texture if using it.  Operation on immediate context
        UpdateVTFPositions(pd3dImmediateContext, true);
    }

    UpdateLightBuffers(pd3dImmediateContext);

    // IC
    for(int iRenderPass = 0; iRenderPass < DC_RP_MAX; iRenderPass++)
        //int iRenderPass = DC_SCENE_MAIN;
    {
        if(iRenderPass >= DC_RP_SHADOW1 && iRenderPass < DC_RP_SHADOW1 + g_iNumShadows)
        {
            pd3dImmediateContext->ClearDepthStencilView(m_pShadowDepthStencilView[iRenderPass - DC_RP_SHADOW1], D3D11_CLEAR_DEPTH, 1.0, 0);

            if(bSkipShadows)
                continue;
        }

        // Common render scene specific prep work to immediate context
        PreRenderPass(pd3dImmediateContext, iRenderPass, 0);

        RenderPassToContext(pd3dImmediateContext, iRenderPass, 0);

        PostRenderPass(pd3dImmediateContext, iRenderPass, 0);
    }

    m_bDrawn = true;

    m_fTime = (float)fTime;
    m_fElapsedTime = (float)fElapsedTime;
}


HRESULT IC_Instancing_Renderer::RenderPassToContext(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iResourceIndex)
{
    HRESULT hr = S_OK;
    const SceneParamsStatic* pStaticParams = &m_StaticSceneParams[iRenderPass];

    // execute render state setup common to all scenes (parameterized per scene)
    V(RenderSetupToContext(pd3dContext, pStaticParams, iResourceIndex));

    // draw all meshes
    UINT* pRenderMeshes = NULL;
    UINT iNum = 0;
    m_pScene->GetAllActiveMeshes(&m_viewMatrix, &pRenderMeshes, &iNum);

    // sort of RLE instance draw the meshes
    int iToDrawMeshIndex = m_pScene->GetMeshIndexFor(m_pScene->GetSortedMeshIndex(pRenderMeshes[0]));
    UINT iToDrawFirstInstance = 0;
    UINT iToDrawMeshCount = 0;

    for(UINT iInstance = 0; iInstance < iNum; iInstance++)
    {
        const int currentMeshIndex = m_pScene->GetMeshIndexFor(m_pScene->GetSortedMeshIndex(pRenderMeshes[iInstance]));

        // We hit a new mesh
        if(iToDrawMeshIndex != currentMeshIndex)
        {
            // draw the last one we were counting instanced and update
            RenderMeshInstancedToContext(iToDrawMeshIndex, iToDrawFirstInstance, iToDrawMeshCount, pd3dContext, 0, 1, iResourceIndex);

            iToDrawMeshCount = 0;
            iToDrawFirstInstance = iInstance; // used to offset properly into instance data VB
            iToDrawMeshIndex = currentMeshIndex;
        }

        iToDrawMeshCount++;    // add another to the pile
    }

    // draw final batch (not covered in above loop)
    RenderMeshInstancedToContext(iToDrawMeshIndex, iToDrawFirstInstance, iToDrawMeshCount, pd3dContext, 0, 1, iResourceIndex);

    delete [] pRenderMeshes;

    return hr;
}

