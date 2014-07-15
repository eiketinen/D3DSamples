//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\utility/RendererBase.cpp
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
#include "NvSimpleMesh.h"
#include <process.h>

#include "DeferredContexts11.h"

#include "Scene.h"
#include "RendererBase.h"

#define USING_MAP_FOR_UPDATING_VTF 0

// {8FC54140-E786-4950-8B00-A53154D95776}
// Used with Set/Get Private Data
static const GUID dcRendererGUID = { 0x8fc54140, 0xe786, 0x4950, { 0x8b, 0x0, 0xa5, 0x31, 0x54, 0xd9, 0x57, 0x76 } };

#if 0
#define DEBUG_THREADING_LOG(A)                 {    \
        char text[MAX_PATH];\
        sprintf_s(text,MAX_PATH,A);\
        OutputDebugStringA(text);\
    }
#define DEBUG_THREADING_LOG_1(A,B)                 {    \
        char text[MAX_PATH];\
        sprintf_s(text,MAX_PATH,A,B);\
        OutputDebugStringA(text);\
    }
#define DEBUG_THREADING_LOG_2(A,B,C)                 {    \
        char text[MAX_PATH];\
        sprintf_s(text,MAX_PATH,A,B,C);\
        OutputDebugStringA(text);\
    }

#else
#pragma warning (disable:4002)
#define DEBUG_THREADING_LOG(A)
#define DEBUG_THREADING_LOG_1(A,B)
#define DEBUG_THREADING_LOG_2(A,B)
#endif

RendererBase::RendererBase()
{
    m_iCBVSPerObjectBind = 0;
    m_iCBVSPerSceneBind = 1;
    m_iCBPSPerObjectBind = 0;
    m_iCBPSPerLightBind = 1;
    m_iCBPSPerSceneBind = 2;

    m_pSamPointClamp = NULL;
    m_pSamLinearWrap = NULL;
    m_pRasterizerStateNoCull = NULL;
    m_pRasterizerStateBackfaceCull = NULL;
    m_pRasterizerStateFrontfaceCull = NULL;
    m_pRasterizerStateNoCullWireFrame = NULL;
    m_pDepthStencilStateNoStencil = NULL;
    m_pVB_VTFCoords = NULL;

    m_pVTFWorldsTexture = NULL;
    m_pVTFWorldsSRV = NULL;

    for(int i = 0; i < g_iNumShadows; i++)
    {
        m_pShadowTexture[i] = NULL;
        m_pShadowResourceView[i] = NULL;
        m_pShadowDepthStencilView[i] = NULL;
        memset(&m_ShadowViewport[i], 0, sizeof(m_ShadowViewport[i]));
    }

    for(int i = 0; i < g_iMaxNumRenderThreads; i++)
    {
        m_pcbVSPerObject[i] = NULL;
        m_pcbVSPerScene[i] = NULL;
        m_pcbPSPerObject[i] = NULL;
        m_pcbPSPerScene[i] = NULL;
    }

    m_pcbPSPerLight = NULL;

    D3DXMatrixIdentity(&m_viewMatrix);
    D3DXMatrixIdentity(&m_projMatrix);

    m_vAmbientColor = D3DXVECTOR4(0.14f * 0.760f, 0.14f * 0.793f, 0.14f * 0.822f, 1.000f);

    bWireFrame = false;

    m_bDrawn = false;

    bVaryShaders = false;
    bUnifyVSPSCB = false;
    bSkipShadows = false;

    bUpdatePositionsPerFrame = true;
    m_SceneWidth = 0;
    m_SceneHeight = 0;

    m_pWorldsTemp = new CB_VS_PER_OBJECT[g_iMaxInstances];
    m_pScene = NULL;
}

RendererBase::~RendererBase()
{
    delete [] m_pWorldsTemp;
}

HRESULT RendererBase::OnD3D11CreateDevice(ID3D11Device* pd3dDevice)
{
    HRESULT hr = S_OK;

    // sampler states and other basic resources
    V_RETURN(InitializeMiscRender(pd3dDevice));

    V_RETURN(m_ShaderPermutations.OnD3D11CreateDevice(pd3dDevice));

    SetUseVTF(bVTFPositions);    // assert our shaders now that they are loaded

    V_RETURN(InitializeBuffers(pd3dDevice));

    // The parameters to pass to threads for the main render pass
    m_StaticSceneParams[DC_RP_MAIN].m_pDepthStencilState = m_pDepthStencilStateNoStencil;
    m_StaticSceneParams[DC_RP_MAIN].m_iStencilRef = 0;
    m_StaticSceneParams[DC_RP_MAIN].m_pRasterizerState = m_pRasterizerStateFrontfaceCull;
    m_StaticSceneParams[DC_RP_MAIN].m_vTintColor = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
    m_StaticSceneParams[DC_RP_MAIN].m_pDepthStencilView = NULL;
    m_StaticSceneParams[DC_RP_MAIN].m_pViewport = NULL;


    V_RETURN(InitializeShadows(pd3dDevice));

    // each scene should know which scene it is.
    for(int i = 0; i < DC_RP_MAX; i++)
        m_StaticSceneParams[i].m_iSceneIndex = i;

    return hr;
}

void RendererBase::OnD3D11BufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
    DC_UNREFERENCED_PARAM(pDevice);
    m_SceneWidth = pBackBufferSurfaceDesc->Width;
    m_SceneHeight = pBackBufferSurfaceDesc->Height;
}

void RendererBase::OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime)
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

    if(bVTFPositions)
    {
        // Update our VTF texture if using it.  Operation on immediate context
        UpdateVTFPositions(pd3dImmediateContext);
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

HRESULT RendererBase::RenderPassToContext(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iResourceIndex)
{
    HRESULT hr = S_OK;
    const SceneParamsStatic* pStaticParams = &m_StaticSceneParams[iRenderPass];

    // execute render state setup common to all scenes (parameterized per scene)
    V(RenderSetupToContext(pd3dContext, pStaticParams, iResourceIndex));

    // draw all meshes
    UINT* pRenderMeshes = NULL;
    UINT iNum = 0;
    m_pScene->GetAllActiveMeshes(&m_viewMatrix, &pRenderMeshes, &iNum);

    int iLastMeshIndex = -1;

    for(UINT iIndex = 0; iIndex < iNum; iIndex++)
    {
        // if we are drawing the same mesh as last time we can skip some API calls for binding buffers and whatnot
        const int currentMeshIndex = m_pScene->GetMeshIndexFor(pRenderMeshes[iIndex]);
        bool bDrawSetup = false;

        if(iLastMeshIndex != currentMeshIndex)
        {
            bDrawSetup = true;
            iLastMeshIndex = currentMeshIndex;
        }

        RenderMeshToContext(pd3dContext, pRenderMeshes[iIndex], iRenderPass, iResourceIndex, bDrawSetup);
    }

    delete [] pRenderMeshes;

    return hr;
}

ID3D11PixelShader* RendererBase::PickAppropriatePixelShader(const UINT idx)
{
    if(bSkipShadows)
    {
        if (bVTFPositions || bUnifyVSPSCB) {
            return m_ShaderPermutations.m_pPixelShaderNoShadowVertexColor[idx];
        }
        return m_ShaderPermutations.m_pPixelShaderNoShadow[idx];
    }

    if (bVTFPositions || bUnifyVSPSCB) {
        return m_ShaderPermutations.m_pPixelShaderVertexColor[idx];
    }
    return m_ShaderPermutations.m_pPixelShader[idx];
}

ID3D11VertexShader* RendererBase::PickAppropriateVertexShader(const UINT idx)
{
    if (bVTFPositions) {
        return  m_ShaderPermutations.m_pVertexShaderVTFVertexColor[idx];
    }

    if (bUnifyVSPSCB) {
        return m_ShaderPermutations.m_pVertexShaderNoVTFVertexColor[idx];
    }

    return m_ShaderPermutations.m_pVertexShaderNoVTF[idx];
}

HRESULT RendererBase::RenderSetupToContext(ID3D11DeviceContext* pd3dContext, const SceneParamsStatic* pStaticParams, int iResourceIndex)
{
    HRESULT hr = S_OK;

    BOOL bShadow = (pStaticParams->m_pDepthStencilView != NULL);

    // Use all shadow maps as textures, or else one shadow map as depth-stencil
    if(bShadow)
    {
        // No shadow maps as textures
        ID3D11ShaderResourceView* ppNullResources[g_iNumShadows] = { NULL };
        pd3dContext->PSSetShaderResources(2, g_iNumShadows, ppNullResources);

        // Given shadow map as depth-stencil, no render target
        pd3dContext->RSSetViewports(1, pStaticParams->m_pViewport);
        pd3dContext->OMSetRenderTargets(0, NULL, pStaticParams->m_pDepthStencilView);
    }
    else
    {
        // Standard DXUT render target and depth-stencil
        V(SetupD3D11Views(pd3dContext, pStaticParams->m_iSceneIndex));

        // All shadow maps as textures
        pd3dContext->PSSetShaderResources(2, g_iNumShadows, m_pShadowResourceView);
    }

    // Set the depth-stencil state
    pd3dContext->OMSetDepthStencilState(pStaticParams->m_pDepthStencilState, pStaticParams->m_iStencilRef);

    // Set the rasterizer state
    pd3dContext->RSSetState(bWireFrame ? m_pRasterizerStateNoCullWireFrame : pStaticParams->m_pRasterizerState);

    // Set the shaders
    pd3dContext->VSSetShader(PickAppropriateVertexShader(), NULL, 0);

    // Set the vertex buffer format
    if(bVTFPositions)
        pd3dContext->IASetInputLayout(m_ShaderPermutations.m_pVertexLayoutVTFStream[0]);
    else
        pd3dContext->IASetInputLayout(m_ShaderPermutations.m_pVertexLayoutNoVTF[0]);

    pd3dContext->VSSetConstantBuffers(m_iCBVSPerSceneBind, 1, &m_pcbVSPerScene[iResourceIndex]);

    if(bShadow)
    {
        pd3dContext->PSSetShader(NULL, NULL, 0);
    }
    else
    {
        pd3dContext->PSSetShader(PickAppropriatePixelShader(), NULL, 0);

        ID3D11SamplerState* ppSamplerStates[2] = { m_pSamPointClamp, m_pSamLinearWrap };
        pd3dContext->PSSetSamplers(0, 2, ppSamplerStates);

        pd3dContext->PSSetConstantBuffers(m_iCBPSPerSceneBind, 1, &m_pcbPSPerScene[iResourceIndex]);

        // $ Light buffers updated per frame in IC before everything else
        pd3dContext->PSSetConstantBuffers(m_iCBPSPerLightBind, 1, &m_pcbPSPerLight);
    }

    if(bVTFPositions)
    {
        // our VTF positions
        pd3dContext->VSSetShaderResources(0, 1, &m_pVTFWorldsSRV);
    }

    return hr;

}

void RendererBase::RenderMeshToContext(ID3D11DeviceContext* pd3dContext, UINT iMeshInstance, UINT iRenderPass, int iResourceIndex, bool bIssueDrawSetup)
{
    DC_UNREFERENCED_PARAM(iRenderPass);
    HRESULT hr = S_OK;

    if(!m_pScene) return;    // cannot render w/o a scene to render

    NvSimpleMesh* pMesh = m_pScene->GetMeshFor(iMeshInstance);

    // some extra CPU overhead for the mesh (needs to be used somehow, but the magic number should never trigger)
    //if(-1234 == CPUGameLoadMethod()) return;

    // optionally map the constant buffer per instance to update world position
    // Draw the mesh to the immediate context

    if (bVaryShaders) {
        // change sahders according to the instance index. 
        const size_t shIdx = iMeshInstance % m_ShaderPermutations.m_shaderVariations;
        const bool bShadow = (iRenderPass >= DC_RP_SHADOW1 && iRenderPass < DC_RP_SHADOW1 + g_iNumShadows) ? true : false;

        // Set the shaders
        pd3dContext->VSSetShader(PickAppropriateVertexShader((UINT)shIdx), NULL, 0);

        // Set the vertex buffer format
        if(bVTFPositions)
            pd3dContext->IASetInputLayout(m_ShaderPermutations.m_pVertexLayoutVTFStream[shIdx]);
        else
            pd3dContext->IASetInputLayout(m_ShaderPermutations.m_pVertexLayoutNoVTF[shIdx]);

        if(bShadow)
        {
            pd3dContext->PSSetShader(NULL, NULL, 0);
        }
        else
        {
            pd3dContext->PSSetShader(PickAppropriatePixelShader((UINT)shIdx), NULL, 0);
        }
    }

    // Set the PS per-object constant data
    // This should eventually also be stuck in VTF or VS constant buffer.
    if((! bVTFPositions) && (! bUnifyVSPSCB))
    {
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        V(pd3dContext->Map(m_pcbPSPerObject[iResourceIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
        CB_PS_PER_OBJECT* pPSPerObject = (CB_PS_PER_OBJECT*)MappedResource.pData;
        pPSPerObject->m_vObjectColor = m_pScene->GetMeshColorFor(iMeshInstance);
        pd3dContext->Unmap(m_pcbPSPerObject[iResourceIndex], 0);

        pd3dContext->PSSetConstantBuffers(m_iCBPSPerObjectBind, 1, &m_pcbPSPerObject[iResourceIndex]);
    }

    // Set the VS per-object constant data
    if(!bVTFPositions)
    {
        if (! bUnifyVSPSCB) {
            // constant buffer positioning
            D3D11_MAPPED_SUBRESOURCE MappedResource;

            D3DXMATRIX mWorld = *m_pScene->GetWorldMatrixFor(iMeshInstance);

            V(pd3dContext->Map(m_pcbVSPerObject[iResourceIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
            CB_VS_PER_OBJECT* pVSPerObject = (CB_VS_PER_OBJECT*)MappedResource.pData;
            D3DXMatrixTranspose(&pVSPerObject->m_mWorld, &mWorld);
            pd3dContext->Unmap(m_pcbVSPerObject[iResourceIndex], 0);

            pd3dContext->VSSetConstantBuffers(m_iCBVSPerObjectBind, 1, &m_pcbVSPerObject[iResourceIndex]);
        } else {
            // constant buffer positioning, color
            D3D11_MAPPED_SUBRESOURCE MappedResource;

            D3DXMATRIX mWorld = *m_pScene->GetWorldMatrixFor(iMeshInstance);

            V(pd3dContext->Map(m_pcbVSPerObject[iResourceIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
            CB_VS_PER_OBJECT* pVSPerObject = (CB_VS_PER_OBJECT*)MappedResource.pData;
            D3DXMatrixTranspose(&pVSPerObject->m_mWorld, &mWorld);

            // packing vertex color to 4th element of world matrix.
            const D3DXVECTOR4&    vColor(m_pScene->GetMeshColorFor(iMeshInstance));
            pVSPerObject->m_mWorld._41 = vColor.x;
            pVSPerObject->m_mWorld._42 = vColor.y;
            pVSPerObject->m_mWorld._43 = vColor.z;
            pVSPerObject->m_mWorld._44 = vColor.w;
    
            pd3dContext->Unmap(m_pcbVSPerObject[iResourceIndex], 0);

            pd3dContext->VSSetConstantBuffers(m_iCBVSPerObjectBind, 1, &m_pcbVSPerObject[iResourceIndex]);
        }
    }
    else    // use VTF
    {
        // Set our static VB of per instance tex coords, not used unless we are rendering with VTF
        UINT Stride = 0;
        UINT Offset = iMeshInstance * 2 * sizeof(float);    // will only be drawing one instance, so point to the data we want
        pd3dContext->IASetVertexBuffers(1, 1, &m_pVB_VTFCoords, &Stride, &Offset);
    }

    // Draw the mesh and all subparts to the context
    if(bIssueDrawSetup)
        pMesh->SetupDraw(pd3dContext, 0, 1);

    pMesh->Draw(pd3dContext);
}

void RendererBase::RenderMeshInstancedToContext(UINT iMeshInstance, UINT iNumInstanceOffset, UINT iNumInstances, ID3D11DeviceContext* pd3dDeviceContext, UINT iDiffuseSlot, UINT iNormalSlot, UINT /*iResourceIndex*/)
{
    NvSimpleMesh* pMesh = m_pScene->GetMeshFor(iMeshInstance);

    if(!pMesh)
        return;

#if 0
    // $$$ TODO This will make all instances from a batch the same color
    //  need to implement injecting instance color into VTF or instance data streams
    {
        HRESULT hr = S_OK;
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        V(pd3dDeviceContext->Map(m_pcbPSPerObject[iResourceIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
        CB_PS_PER_OBJECT* pPSPerObject = (CB_PS_PER_OBJECT*)MappedResource.pData;
        pPSPerObject->m_vObjectColor = m_pScene->GetMeshColorFor(iMeshInstance);//D3DXVECTOR4( 1, 1, 1, 1 );
        pd3dDeviceContext->Unmap(m_pcbPSPerObject[iResourceIndex], 0);

        pd3dDeviceContext->PSSetConstantBuffers(m_iCBPSPerObjectBind, 1, &m_pcbPSPerObject[iResourceIndex]);
    }
#endif

    // Set our static VB of per instance tex coords.
    UINT Stride = 2 * sizeof(float);
    UINT Offset = iNumInstanceOffset * 2 * sizeof(float);    // will only be drawing one instance, so point to the data we want
    pd3dDeviceContext->IASetVertexBuffers(1, 1, &m_pVB_VTFCoords, &Stride, &Offset);

    pMesh->SetupDraw(pd3dDeviceContext, iDiffuseSlot, iNormalSlot);
    pMesh->DrawInstanced(pd3dDeviceContext, iNumInstances);
}

void RendererBase::PreRenderPass(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iResourceIndex)
{
    HRESULT hr;
    SceneParamsDynamic dynamicParams;

    DC_UNREFERENCED_PARAM(pd3dContext);
    DC_UNREFERENCED_PARAM(iRenderPass);
    DC_UNREFERENCED_PARAM(iResourceIndex);

    if(iRenderPass == DC_RP_MAIN)
    {
        dynamicParams.m_mViewProj = m_viewMatrix * m_projMatrix;
    }
    else if(iRenderPass == DC_RP_SHADOW1)
    {
        int iShadow = iRenderPass - DC_RP_SHADOW1;
        CalcLightViewProj(&dynamicParams.m_mViewProj, iShadow);
    }

    const SceneParamsStatic* pStaticParams = &m_StaticSceneParams[iRenderPass];

    D3D11_MAPPED_SUBRESOURCE MappedResource;

    // Set the VS per-scene constant data
    V(pd3dContext->Map(m_pcbVSPerScene[iResourceIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CB_VS_PER_PASS* pVSPerScene = (CB_VS_PER_PASS*)MappedResource.pData;
    D3DXMatrixTranspose(&pVSPerScene->m_mViewProj, &dynamicParams.m_mViewProj);
    pd3dContext->Unmap(m_pcbVSPerScene[iResourceIndex], 0);

    // Set the PS per-scene constant data
    V(pd3dContext->Map(m_pcbPSPerScene[iResourceIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CB_PS_PER_PASS* pPSPerScene = (CB_PS_PER_PASS*)MappedResource.pData;
    pPSPerScene->m_vAmbientColor = m_vAmbientColor;
    pPSPerScene->m_vTintColor = pStaticParams->m_vTintColor;
    pd3dContext->Unmap(m_pcbPSPerScene[iResourceIndex], 0);
}

// Any cleanup or follow up operations such as RT copies, blends, etc
void RendererBase::PostRenderPass(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iResourceIndex)
{
    DC_UNREFERENCED_PARAM(pd3dContext);
    DC_UNREFERENCED_PARAM(iResourceIndex);

    if(iRenderPass == DC_RP_MAIN)
    {
    }
}


//--------------------------------------------------------------------------------------
// Figure out the ViewProj matrix from the light's perspective
//--------------------------------------------------------------------------------------
void RendererBase::CalcLightViewProj(D3DXMATRIX* pmLightViewProj, int iLight)
{
    DC_Light& light = m_pScene->GetLight(iLight);

    D3DXMATRIX mLightView;

#if 1
    // place light at its location and generate lookat from there
    const D3DXVECTOR3& vLightDir = light.vLightDir;
    const D3DXVECTOR3& vLightPos = light.vLightPos;
    D3DXVECTOR3 vLookAt = vLightPos + g_fSceneRadius * vLightDir;
    D3DXMatrixLookAtLH(&mLightView, &vLightPos, &vLookAt, &g_vUp);
#else
    // just use light direction, centered on origin
    const D3DXVECTOR3& vLightDir = light.vLightDir;
    const D3DXVECTOR3 vLightPos = vLightDir * -g_fSceneRadius;
    const D3DXVECTOR3 vLookAt = vLightDir * g_fSceneRadius;
    D3DXMatrixLookAtLH(&mLightView, &vLightPos, &vLookAt, &g_vUp);
#endif

    D3DXMATRIX mLightProj;
    D3DXMatrixOrthoLH(&mLightProj, 2.f * light.fLightAspect * g_fSceneRadius, 2.f * g_fSceneRadius, light.fLightNearPlane, light.fLightFarPlane);

    *pmLightViewProj = mLightView * mLightProj;
}

void RendererBase::UpdateVTFPositions(ID3D11DeviceContext* pd3dContext, bool useSortedIndices)
{
    if(!m_pScene) return;

    unsigned int updateInstances = m_pScene->NumActiveInstances();

    for(unsigned int i = 0; i < updateInstances; i++)
    {
        int idx = i;

        if(useSortedIndices)
            idx = m_pScene->GetSortedMeshIndex(i);

        // positions
        m_pWorldsTemp[i].m_mWorld = *m_pScene->GetWorldMatrixFor(idx);

        // Encode our matrix into a 3 float4's so in the shader we only load 3 times
        m_pWorldsTemp[i].m_mWorld._14 = m_pWorldsTemp[i].m_mWorld._41;
        m_pWorldsTemp[i].m_mWorld._24 = m_pWorldsTemp[i].m_mWorld._42;
        m_pWorldsTemp[i].m_mWorld._34 = m_pWorldsTemp[i].m_mWorld._43;
        m_pWorldsTemp[i].m_mWorld._41 = m_pWorldsTemp[i].m_mWorld._42 = m_pWorldsTemp[i].m_mWorld._43 = m_pWorldsTemp[i].m_mWorld._44 = 0;

        // instance color
        auto& color = m_pScene->GetMeshColorFor(idx);

        m_pWorldsTemp[i].m_mWorld._41 = color.x;
        m_pWorldsTemp[i].m_mWorld._42 = color.y;
        m_pWorldsTemp[i].m_mWorld._43 = color.z;
        m_pWorldsTemp[i].m_mWorld._44 = color.w;;
    }

#if USING_MAP_FOR_UPDATING_VTF
    D3D11_MAPPED_SUBRESOURCE Subresource;
    pd3dContext->Map(m_pVTFWorldsTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &Subresource);
    memcpy(Subresource.pData, (const void*)m_pWorldsTemp, updateInstances * sizeof(D3DXMATRIX));
    pd3dContext->Unmap(m_pVTFWorldsTexture, 0);
#else
    D3D11_TEXTURE2D_DESC Desc;
    m_pVTFWorldsTexture->GetDesc(&Desc);
    D3D11_BOX            dstBox;
    ZeroMemory(&dstBox, sizeof(dstBox));
    dstBox.right = Desc.Width;
    dstBox.bottom = (updateInstances * 4 + (Desc.Width - 1)) / Desc.Width;
    dstBox.back = 1;
    pd3dContext->UpdateSubresource(m_pVTFWorldsTexture, 0, &dstBox, (const void*)m_pWorldsTemp, Desc.Width * sizeof(D3DXVECTOR4), 0);
#endif
}

HRESULT RendererBase::InitializeMiscRender(ID3D11Device* pd3dDevice)
{
    HRESULT hr;

    // The standard depth-stencil state
    D3D11_DEPTH_STENCIL_DESC DepthStencilDescNoStencil =
    {
        TRUE,                           // BOOL DepthEnable;
        D3D11_DEPTH_WRITE_MASK_ALL,     // D3D11_DEPTH_WRITE_MASK DepthWriteMask;
        D3D11_COMPARISON_LESS_EQUAL,    // D3D11_COMPARISON_FUNC DepthFunc;
        FALSE,                          // BOOL StencilEnable;
        0,                              // UINT8 StencilReadMask;
        0,                              // UINT8 StencilWriteMask;
        {
            // D3D11_DEPTH_STENCILOP_DESC FrontFace;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilFailOp;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilDepthFailOp;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilPassOp;
            D3D11_COMPARISON_NEVER,     // D3D11_COMPARISON_FUNC StencilFunc;
        },
        {
            // D3D11_DEPTH_STENCILOP_DESC BackFace;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilFailOp;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilDepthFailOp;
            D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilPassOp;
            D3D11_COMPARISON_NEVER,     // D3D11_COMPARISON_FUNC StencilFunc;
        },
    };
    V_RETURN(pd3dDevice->CreateDepthStencilState(
                 &DepthStencilDescNoStencil,
                 &m_pDepthStencilStateNoStencil));

    // Create sampler states for point/clamp (shadow map) and linear/wrap (everything else)
    D3D11_SAMPLER_DESC SamDesc;
    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.MipLODBias = 0.0f;
    SamDesc.MaxAnisotropy = 1;
    SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 1;
    SamDesc.MinLOD = 0;
    SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &m_pSamLinearWrap));

    SamDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    SamDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
    V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &m_pSamPointClamp));

    // Setup backface culling states:
    //  1) m_pRasterizerStateNoCull --- no culling (debugging only)
    //  2) m_pRasterizerStateBackfaceCull --- backface cull
    //  3) m_pRasterizerStateFrontfaceCull --- frontface cull (pre-built assets from
    //      the content pipeline)
    D3D11_RASTERIZER_DESC RasterizerDescNoCull =
    {
        D3D11_FILL_SOLID,   // D3D11_FILL_MODE FillMode;
        D3D11_CULL_NONE,    // D3D11_CULL_MODE CullMode;
        TRUE,               // BOOL FrontCounterClockwise;
        0,                  // INT DepthBias;
        0,                  // FLOAT DepthBiasClamp;
        0,                  // FLOAT SlopeScaledDepthBias;
        FALSE,              // BOOL DepthClipEnable;
        FALSE,              // BOOL ScissorEnable;
        TRUE,               // BOOL MultisampleEnable;
        FALSE,              // BOOL AntialiasedLineEnable;
    };
    V_RETURN(pd3dDevice->CreateRasterizerState(&RasterizerDescNoCull, &m_pRasterizerStateNoCull));
    RasterizerDescNoCull.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN(pd3dDevice->CreateRasterizerState(&RasterizerDescNoCull, &m_pRasterizerStateNoCullWireFrame));

    D3D11_RASTERIZER_DESC RasterizerDescBackfaceCull =
    {
        D3D11_FILL_SOLID,   // D3D11_FILL_MODE FillMode;
        D3D11_CULL_BACK,    // D3D11_CULL_MODE CullMode;
        TRUE,               // BOOL FrontCounterClockwise;
        0,                  // INT DepthBias;
        0,                  // FLOAT DepthBiasClamp;
        0,                  // FLOAT SlopeScaledDepthBias;
        FALSE,              // BOOL DepthClipEnable;
        FALSE,              // BOOL ScissorEnable;
        TRUE,               // BOOL MultisampleEnable;
        FALSE,              // BOOL AntialiasedLineEnable;
    };
    V_RETURN(pd3dDevice->CreateRasterizerState(&RasterizerDescBackfaceCull, &m_pRasterizerStateBackfaceCull));

    D3D11_RASTERIZER_DESC RasterizerDescFrontfaceCull =
    {
        D3D11_FILL_SOLID,   // D3D11_FILL_MODE FillMode;
        D3D11_CULL_FRONT,   // D3D11_CULL_MODE CullMode;
        TRUE,               // BOOL FrontCounterClockwise;
        0,                  // INT DepthBias;
        0,                  // FLOAT DepthBiasClamp;
        0,                  // FLOAT SlopeScaledDepthBias;
        FALSE,              // BOOL DepthClipEnable;
        FALSE,              // BOOL ScissorEnable;
        TRUE,               // BOOL MultisampleEnable;
        FALSE,              // BOOL AntialiasedLineEnable;
    };
    V_RETURN(pd3dDevice->CreateRasterizerState(&RasterizerDescFrontfaceCull, &m_pRasterizerStateFrontfaceCull));


    return hr;
}

HRESULT RendererBase::InitializeBuffers(ID3D11Device* pd3dDevice)
{
    HRESULT hr;


    // Setup constant buffers
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;

    // $$ Duplicate all buffers enough for all threads to have their own.  Since each isn't very big this shouldn't be that much overhead.
    //    alternatively would want to identify which buffers can be updated and owned on main thread only and jsut used by other threads(not mapped).
    for(int i = 0; i < g_iMaxNumRenderThreads; i++)
    {
        Desc.ByteWidth = sizeof(CB_VS_PER_PASS);
        V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &m_pcbVSPerScene[i]));

        Desc.ByteWidth = sizeof(CB_VS_PER_OBJECT);
        V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &m_pcbVSPerObject[i]));

        Desc.ByteWidth = sizeof(CB_PS_PER_PASS);
        V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &m_pcbPSPerScene[i]));

        Desc.ByteWidth = sizeof(CB_PS_PER_OBJECT);
        V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &m_pcbPSPerObject[i]));

    }

    Desc.ByteWidth = sizeof(CB_PS_PER_LIGHT);
    V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &m_pcbPSPerLight));


    // make up some dimensions based on max instances and data size per instance
    float width;
    int iWidth, iHeight;
    float texelsPerStruct = (float)sizeof(CB_VS_PER_OBJECT) / (4 * sizeof(float));
    float totalTexels = (float)g_iMaxInstances * texelsPerStruct;
    width = totalTexels; // start off with the number of float4's we need
    width = sqrtf(width) + texelsPerStruct    ;    // get a reasonable width
    iWidth = (int)width;
    iWidth &= 0xFFF8;    // mask off bottom 3 bits to make multiple of 4 and ensure every 4 elements are on the same line
    iHeight = ((int)totalTexels / iWidth) + 1;

    D3D11_TEXTURE2D_DESC VTFTexDesc =
    {
        (int) iWidth,                            // UINT Width;
        (int) iHeight,                            // UINT Height;
        1,                                      // UINT MipLevels;
        1,                                      // UINT ArraySize;
        DXGI_FORMAT_R32G32B32A32_TYPELESS,         // DXGI_FORMAT Format;
        { 1, 0, },                              // DXGI_SAMPLE_DESC SampleDesc;
#if USING_MAP_FOR_UPDATING_VTF
        D3D11_USAGE_DYNAMIC,                    // D3D11_USAGE Usage;
#else
        D3D11_USAGE_DEFAULT,
#endif
        D3D11_BIND_SHADER_RESOURCE,                // UINT BindFlags;
#if USING_MAP_FOR_UPDATING_VTF
        D3D11_CPU_ACCESS_WRITE,                    // UINT CPUAccessFlags;
#else
        0,
#endif
        0,                                      // UINT MiscFlags;
    };

    D3D11_SHADER_RESOURCE_VIEW_DESC VTFResourceViewDesc =
    {
        DXGI_FORMAT_R32G32B32A32_FLOAT,                  // DXGI_FORMAT Format;
        D3D11_SRV_DIMENSION_TEXTURE2D,          // D3D11_SRV_DIMENSION ViewDimension;
        {0, 1, },                               // D3D11_TEX2D_SRV Texture2D;
    };
    V_RETURN(pd3dDevice->CreateTexture2D(&VTFTexDesc, NULL, &m_pVTFWorldsTexture));
    V_RETURN(pd3dDevice->CreateShaderResourceView(m_pVTFWorldsTexture, &VTFResourceViewDesc,
             &m_pVTFWorldsSRV));

    // Make our static VB with tex coords
    D3D11_BUFFER_DESC VTFVBDesc =
    {
        g_iMaxInstances * 2 * sizeof(float),
        D3D11_USAGE_IMMUTABLE,
        D3D11_BIND_VERTEX_BUFFER,
        0,
        0,
        2 * sizeof(float)    // stride is float2
    };

    // pre-gen the offset required to offset an instance to the right location in our VTF tex
    UINT* pData = new UINT[g_iMaxInstances * 2];
    int iElementWidth = 4;    // number of texels per struct

    for(int i = 0; i < g_iMaxInstances; i++)
    {
        pData[i * 2] = (i * iElementWidth) % iWidth;
        pData[i * 2 + 1] = (i * iElementWidth) / iWidth;
    }

    D3D11_SUBRESOURCE_DATA InitialData = {    (void*)pData, 0, 0 };
    V_RETURN(pd3dDevice->CreateBuffer(&VTFVBDesc, &InitialData, &m_pVB_VTFCoords));
    delete [] pData;
    return hr;
}

//--------------------------------------------------------------------------------------
// Create D3D11 resources for the shadows
//--------------------------------------------------------------------------------------
HRESULT RendererBase::InitializeShadows(ID3D11Device* pd3dDevice)
{
    HRESULT hr = S_OK;

    for(int iShadow = 0; iShadow < g_iNumShadows; ++iShadow)
    {
        // constant for now
        m_fShadowResolutionX[iShadow] = 2048.0f;
        m_fShadowResolutionY[iShadow] = 2048.0f;

        // The shadow map, along with depth-stencil and texture view
        D3D11_TEXTURE2D_DESC ShadowDesc =
        {
            (int) m_fShadowResolutionX[iShadow],    // UINT Width;
            (int) m_fShadowResolutionY[iShadow],    // UINT Height;
            1,                                      // UINT MipLevels;
            1,                                      // UINT ArraySize;
            DXGI_FORMAT_R24G8_TYPELESS,             // DXGI_FORMAT Format;
            { 1, 0, },                              // DXGI_SAMPLE_DESC SampleDesc;
            D3D11_USAGE_DEFAULT,                    // D3D11_USAGE Usage;
            D3D11_BIND_SHADER_RESOURCE
            | D3D11_BIND_DEPTH_STENCIL,         // UINT BindFlags;
            0,                                      // UINT CPUAccessFlags;
            0,                                      // UINT MiscFlags;
        };
        D3D11_DEPTH_STENCIL_VIEW_DESC ShadowDepthStencilViewDesc =
        {
            DXGI_FORMAT_D24_UNORM_S8_UINT,                  // DXGI_FORMAT Format;
            D3D11_DSV_DIMENSION_TEXTURE2D,          // D3D11_DSV_DIMENSION ViewDimension;
            0,                                      // UINT ReadOnlyUsage;
            {0, },                                  // D3D11_TEX2D_RTV Texture2D;
        };
        D3D11_SHADER_RESOURCE_VIEW_DESC ShadowResourceViewDesc =
        {
            DXGI_FORMAT_R24_UNORM_X8_TYPELESS,                  // DXGI_FORMAT Format;
            D3D11_SRV_DIMENSION_TEXTURE2D,          // D3D11_SRV_DIMENSION ViewDimension;
            {0, 1, },                               // D3D11_TEX2D_SRV Texture2D;
        };
        V_RETURN(pd3dDevice->CreateTexture2D(&ShadowDesc, NULL, &m_pShadowTexture[iShadow]));
        V_RETURN(pd3dDevice->CreateDepthStencilView(m_pShadowTexture[iShadow], &ShadowDepthStencilViewDesc,
                 &m_pShadowDepthStencilView[iShadow]));
        V_RETURN(pd3dDevice->CreateShaderResourceView(m_pShadowTexture[iShadow], &ShadowResourceViewDesc,
                 &m_pShadowResourceView[iShadow]));

        m_ShadowViewport[iShadow].Width = m_fShadowResolutionX[iShadow];
        m_ShadowViewport[iShadow].Height = m_fShadowResolutionY[iShadow];
        m_ShadowViewport[iShadow].MinDepth = 0;
        m_ShadowViewport[iShadow].MaxDepth = 1;
        m_ShadowViewport[iShadow].TopLeftX = 0;
        m_ShadowViewport[iShadow].TopLeftY = 0;

        // The parameters to pass to per-chunk threads for the shadow scenes
        m_StaticSceneParams[DC_RP_SHADOW1 + iShadow].m_pDepthStencilState = m_pDepthStencilStateNoStencil;
        m_StaticSceneParams[DC_RP_SHADOW1 + iShadow].m_iStencilRef = 0;
        m_StaticSceneParams[DC_RP_SHADOW1 + iShadow].m_pRasterizerState = m_pRasterizerStateFrontfaceCull;
        m_StaticSceneParams[DC_RP_SHADOW1 + iShadow].m_vTintColor = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
        m_StaticSceneParams[DC_RP_SHADOW1 + iShadow].m_pDepthStencilView = m_pShadowDepthStencilView[iShadow];
        m_StaticSceneParams[DC_RP_SHADOW1 + iShadow].m_pViewport = &m_ShadowViewport[iShadow];
    }

    return hr;
}

void RendererBase::OnD3D11DestroyDevice()
{
    // destroy handled by caller
    //m_pMesh11->Destroy();

    m_ShaderPermutations.OnD3D11DestroyDevice();

    SAFE_RELEASE(m_pSamPointClamp);
    SAFE_RELEASE(m_pSamLinearWrap);
    SAFE_RELEASE(m_pRasterizerStateNoCull);
    SAFE_RELEASE(m_pRasterizerStateBackfaceCull);
    SAFE_RELEASE(m_pRasterizerStateFrontfaceCull);
    SAFE_RELEASE(m_pRasterizerStateNoCullWireFrame);

    for(int iShadow = 0; iShadow < g_iNumShadows; ++iShadow)
    {
        SAFE_RELEASE(m_pShadowTexture[iShadow]);
        SAFE_RELEASE(m_pShadowResourceView[iShadow]);
        SAFE_RELEASE(m_pShadowDepthStencilView[iShadow]);
    }

    SAFE_RELEASE(m_pVTFWorldsTexture);
    SAFE_RELEASE(m_pVTFWorldsSRV);
    SAFE_RELEASE(m_pVB_VTFCoords) ;

    for(int iInstance = 0; iInstance < g_iMaxNumRenderThreads; ++iInstance)
    {
        SAFE_RELEASE(m_pcbVSPerScene[iInstance]);
        SAFE_RELEASE(m_pcbVSPerObject[iInstance]);
        SAFE_RELEASE(m_pcbPSPerScene[iInstance]);
        SAFE_RELEASE(m_pcbPSPerObject[iInstance]);
    }

    SAFE_RELEASE(m_pcbPSPerLight);
}

HRESULT RendererBase::SetupD3D11Views(ID3D11DeviceContext* pd3dDeviceContext, int iScene)
{
    HRESULT hr = S_OK;

#if 0
    const int rows = 3;
    const int columns = 2;
#else
    const int rows = 1;
    const int columns = 1;
#endif
    int x = iScene % rows;
    int y = (iScene / rows) % columns;

    // Setup the viewport to match the backbuffer
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)m_SceneWidth / rows;
    vp.Height = (FLOAT)m_SceneHeight / columns;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = x * vp.Width;
    vp.TopLeftY = y * vp.Height;
    pd3dDeviceContext->RSSetViewports(1, &vp);


    // $$$ TODO TODO Allow scene color registration for off screen buffer renders
    // Set the render targets
    pd3dDeviceContext->OMSetRenderTargets(1, &m_pRTV, m_pDSV);

    return hr;
}

void RendererBase::UpdateLightBuffers(ID3D11DeviceContext* pd3dContext)
{
    HRESULT hr;

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    // Set the PS per-light constant data
    V(pd3dContext->Map(m_pcbPSPerLight, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CB_PS_PER_LIGHT* pPSPerLight = (CB_PS_PER_LIGHT*)MappedResource.pData;

    for(int iLight = 0; iLight < m_pScene->NumLights(); ++iLight)
    {
        DC_Light& light = m_pScene->GetLight(iLight);
        D3DXVECTOR4 vLightPos = D3DXVECTOR4(light.vLightPos.x, light.vLightPos.y, light.vLightPos.z, 1.0f);
        D3DXVECTOR4 vLightDir = D3DXVECTOR4(light.vLightDir.x, light.vLightDir.y, light.vLightDir.z, 0.0f);
        D3DXMATRIX mLightViewProj;

        CalcLightViewProj(&mLightViewProj, iLight);

        pPSPerLight->m_LightData[iLight].m_vLightColor = light.vLightColor;
        pPSPerLight->m_LightData[iLight].m_vLightPos = vLightPos;
        pPSPerLight->m_LightData[iLight].m_vLightDir = vLightDir;
        D3DXMatrixTranspose(&pPSPerLight->m_LightData[iLight].m_mLightViewProj,
                            &mLightViewProj);
        pPSPerLight->m_LightData[iLight].m_vFalloffs = D3DXVECTOR4(
                    light.fLightFalloffDistEnd,
                    light.fLightFalloffDistRange,
                    light.fLightFalloffCosAngleEnd,
                    light.fLightFalloffCosAngleRange);
    }

    pd3dContext->Unmap(m_pcbPSPerLight, 0);
}

// finalize the context into the command list requested
HRESULT RendererBase::FinishToCommandList(ID3D11DeviceContext* pd3dContext, ID3D11CommandList*& pd3dCommandList)
{
    HRESULT hr = S_OK;

    if(pd3dCommandList != NULL)            // clear a previous command list
        SAFE_RELEASE(pd3dCommandList);

    // Note FALSE below.  There is a perf penalty to save and restore IC state, don't need it, so avoid.
    V(pd3dContext->FinishCommandList(FALSE, &pd3dCommandList));

    //pd3dContext->ClearState();    // ensure that the state releases all buffers

    return hr;
}
