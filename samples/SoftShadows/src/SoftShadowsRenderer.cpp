//----------------------------------------------------------------------------------
// File:        SoftShadows\src/SoftShadowsRenderer.cpp
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
#include "stdafx.h"
#include "SoftShadowsRenderer.h"

#include <SDKmesh.h>

#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////
static const UINT LIGHT_RES = 1024;
static const float LIGHT_ZFAR = 32.0f;

////////////////////////////////////////////////////////////////////////////////
// File names
////////////////////////////////////////////////////////////////////////////////
static const wchar_t *ROCK_DIFFUSE_MAP_FILENAME = L"lichen6_rock.dds";
static const wchar_t *GROUND_DIFFUSE_MAP_FILENAME = L"lichen6.dds";
static const wchar_t *GROUND_NORMAL_MAP_FILENAME = L"lichen6_normal.dds";

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::MeshInstance::getWorldCenter()
////////////////////////////////////////////////////////////////////////////////
D3DXVECTOR3 SoftShadowsRenderer::MeshInstance::getWorldCenter() const
{
    D3DXVECTOR3 result;
    D3DXVECTOR3 center = getCenter();
    D3DXVec3TransformCoord(&result, &center, &m_worldTransform);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::SimpleMeshInstance class
////////////////////////////////////////////////////////////////////////////////
class SoftShadowsRenderer::SimpleMeshInstance
    : public SoftShadowsRenderer::MeshInstance
{
public:

    SimpleMeshInstance(NvSimpleMesh *mesh, const wchar_t *name, const D3DMATRIX &worldTransform)
        : MeshInstance(name, worldTransform)
        , m_mesh(mesh)
    {
    }

    virtual ~SimpleMeshInstance()
    {
    }

    virtual D3DXVECTOR3 getCenter() const
    {
        return m_mesh->Center;
    }

    virtual D3DXVECTOR3 getExtents() const
    {
        return m_mesh->Extents;
    }

    virtual HRESULT createInputLayout(ID3D11Device *device, BYTE *iaSig, SIZE_T iaSigSize) override
    {
        if (m_mesh->pInputLayout == nullptr)
            return m_mesh->CreateInputLayout(device, iaSig, iaSigSize);

        return S_OK;
    }

    virtual void releaseInputLayout() override
    {
        SAFE_RELEASE(m_mesh->pInputLayout);
    }

    virtual void draw(
        ID3D11DeviceContext *immediateContext,
        ID3D11InputLayout *vertexLayout,
        ID3DX11EffectPass *pass,
        ID3DX11EffectMatrixVariable *worldTransformVariable,
        ID3DX11EffectShaderResourceVariable *diffuseVariable,
        ID3DX11EffectShaderResourceVariable *normalVariable,
        ID3DX11EffectShaderResourceVariable *specularVariable)
    {
        worldTransformVariable->SetMatrix(m_worldTransform);

        if (diffuseVariable != nullptr)
        {
            diffuseVariable->AsShaderResource()->SetResource(m_mesh->pDiffuseSRV);
        }

        if (normalVariable != nullptr)
        {
            normalVariable->AsShaderResource()->SetResource(m_mesh->pNormalsSRV);
        }

        pass->Apply(0, immediateContext);
        m_mesh->SetupDraw(immediateContext);
        m_mesh->Draw(immediateContext);
    }

    virtual void accumStats(UINT64 &numIndices, UINT64 &numVertices)
    {
        numIndices += m_mesh->iNumIndices;
        numVertices += m_mesh->iNumVertices;
    }

private:

    NvSimpleMesh *m_mesh;
};

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::SdkMeshInstance class
////////////////////////////////////////////////////////////////////////////////
class SoftShadowsRenderer::SdkMeshInstance
    : public SoftShadowsRenderer::MeshInstance
{
public:

    SdkMeshInstance(CDXUTSDKMesh *mesh, const wchar_t *name, const D3DMATRIX &worldTransform)
        : MeshInstance(name, worldTransform)
        , m_mesh(mesh)
        , m_center()
    {
        // Precalculate a composite bounding box
        float mins[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
        float maxs[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        UINT numMeshes = m_mesh->GetNumMeshes();
        for (UINT i = 0; i < numMeshes; ++i)
        {
            D3DXVECTOR3 center = m_mesh->GetMeshBBoxCenter(i);
            D3DXVECTOR3 extents = m_mesh->GetMeshBBoxExtents(i);
            float meshMins[3] = { center.x - extents.x, center.y - extents.y, center.z - extents.z };
            float meshMaxs[3] = { center.x + extents.x, center.y + extents.y, center.z + extents.z };
            for (int j = 0; j < 3; ++j)
            {
                mins[j] = std::min(meshMins[j], mins[j]);
                maxs[j] = std::max(meshMaxs[j], maxs[j]);
            }
        }

        // Calculate the center and extents from the composite bounding box
        m_extents = D3DXVECTOR3(maxs[0] - mins[0], maxs[1] - mins[1], maxs[2] - mins[2]) * 0.5f;
        m_center = D3DXVECTOR3(mins[0], mins[1], mins[2]) + m_extents;
    }

    virtual ~SdkMeshInstance()
    {
    }

    virtual D3DXVECTOR3 getCenter() const
    {
        return m_center;
    }

    virtual D3DXVECTOR3 getExtents() const
    {
        return m_extents;
    }

    virtual void draw(
        ID3D11DeviceContext *immediateContext,
        ID3D11InputLayout *vertexLayout,
        ID3DX11EffectPass *pass,
        ID3DX11EffectMatrixVariable *worldTransformVariable,
        ID3DX11EffectShaderResourceVariable *diffuseVariable,
        ID3DX11EffectShaderResourceVariable *normalVariable,
        ID3DX11EffectShaderResourceVariable *specularVariable)
    {
        worldTransformVariable->SetMatrix(m_worldTransform);

        for (UINT mesh = 0; mesh < m_mesh->GetNumMeshes(); ++mesh)
        {
            UINT strides[1];
            UINT offsets[1];
            ID3D11Buffer *vb[1];
            vb[0] = m_mesh->GetVB11(mesh, 0);
            strides[0] = (UINT)m_mesh->GetVertexStride(0, 0);
            offsets[0] = 0;
            immediateContext->IASetVertexBuffers(0, 1, vb, strides, offsets);
            immediateContext->IASetIndexBuffer(m_mesh->GetIB11(mesh), m_mesh->GetIBFormat11(mesh), 0);
            immediateContext->IASetInputLayout(vertexLayout);

            for (UINT subset = 0; subset < m_mesh->GetNumSubsets(0); ++subset)
            {
                SDKMESH_SUBSET *meshSubset = m_mesh->GetSubset(mesh, subset);
                SDKMESH_MATERIAL *material = m_mesh->GetMaterial(meshSubset->MaterialID);

                if (diffuseVariable != nullptr)
                {
                    diffuseVariable->AsShaderResource()->SetResource(material->pDiffuseRV11);
                }

                if (normalVariable != nullptr)
                {
                    normalVariable->AsShaderResource()->SetResource(material->pNormalRV11);
                }

                if (specularVariable != nullptr)
                {
                    specularVariable->AsShaderResource()->SetResource(material->pSpecularRV11);
                }

                pass->Apply(0, immediateContext);

                D3D10_PRIMITIVE_TOPOLOGY PrimType = m_mesh->GetPrimitiveType11((SDKMESH_PRIMITIVE_TYPE)meshSubset->PrimitiveType);
                immediateContext->IASetPrimitiveTopology( PrimType );
                immediateContext->DrawIndexed((UINT)meshSubset->IndexCount, 0, (UINT)meshSubset->VertexStart);
            }
        }
    }

    virtual void accumStats(UINT64 &numIndices, UINT64 &numVertices)
    {
        for (UINT mesh = 0; mesh < m_mesh->GetNumMeshes(); ++mesh)
        {
            numIndices += m_mesh->GetNumIndices(mesh);
            numVertices += m_mesh->GetNumIndices(mesh);
        }
    }

private:

    D3DXVECTOR3 m_center;
    D3DXVECTOR3 m_extents;
    CDXUTSDKMesh *m_mesh;
};

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::SoftShadowsRenderer()
////////////////////////////////////////////////////////////////////////////////
SoftShadowsRenderer::SoftShadowsRenderer()
    : m_meshInstances()
    , m_knightMesh()
    , m_podiumMesh()
    , m_camera()
    , m_lightCamera()
    , m_viewport()
    , m_lightViewport()
    , m_lightRadiusWorld(0.5f)
    , m_vertexLayout()
    , m_shadowMap()
    , m_effect()
    , m_technique()
    , m_lightViewVariable(nullptr)
    , m_lightViewProjVariable(nullptr)
    , m_lightViewProjClip2TexVariable(nullptr)
    , m_lightZNearVariable(nullptr)
    , m_lightZFarVariable(nullptr)
    , m_lightPosVariable(nullptr)
    , m_lightWidthVariable(nullptr)
    , m_lightRadiusUvVariable(nullptr)
    , m_viewProjVariable(nullptr)
    , m_worldVariable(nullptr)
    , m_useDiffuseVariable(nullptr)
    , m_useTextureVariable(nullptr)
    , m_depthMapVariable(nullptr)    
    , m_diffuseVariable(nullptr)
    , m_normalVariable(nullptr)
    , m_specularVariable(nullptr)
    , m_groundHeight(0)
    , m_groundRadius(0)
    , m_groundVertexBuffer()
    , m_groundIndexBuffer()
    , m_rockDiffuse()
    , m_groundDiffuse()
    , m_groundNormal()
    , m_frameNumber(0)
    , m_backgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
    , m_worldHeightOffset(0.2f)
    , m_worldWidthOffset(0.2f)
    , m_pcssPreset(Poisson_64_128)
    , m_shadowTechnique(PCSS)
    , m_reloadEffect(false)
    , m_useTexture(true)
    , m_visualizeDepthTexture(false)
{
    m_lightViewport.Width  = LIGHT_RES;
    m_lightViewport.Height = LIGHT_RES;
    m_lightViewport.MinDepth = 0;
    m_lightViewport.MaxDepth = 1;
    m_lightViewport.TopLeftX = 0;
    m_lightViewport.TopLeftY = 0;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::~SoftShadowsRenderer()
////////////////////////////////////////////////////////////////////////////////
SoftShadowsRenderer::~SoftShadowsRenderer()
{
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::onFrameMove()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::onFrameMove(double time, float elapsedTime)
{
    m_camera.FrameMove(elapsedTime);
    m_lightCamera.FrameMove(elapsedTime);

    updateCamera();
    updateLightCamera();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::onCreateDevice()
////////////////////////////////////////////////////////////////////////////////
HRESULT SoftShadowsRenderer::onCreateDevice(ID3D11Device *device, const DXGI_SURFACE_DESC* backBufferSurfaceDesc)
{
    HRESULT hr = S_OK;

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye = D3DXVECTOR3(-0.644995f, 0.614183f, -0.660632f);
    D3DXVECTOR3 vecLight = D3DXVECTOR3(3.57088f, 6.989f, -5.19698f);
    D3DXVECTOR3 vecAt = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_camera.SetViewParams(&vecEye, &vecAt);
    m_lightCamera.SetViewParams(&vecLight, &vecAt);
    m_camera.SetRadius(1.0f, 0.001f, 4.0f);

    // Create resources
    V_RETURN(createShadowMap(device));
    V_RETURN(createGeometry(device));
    V_RETURN(createTextures(device));
    V_RETURN(loadEffect(device));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::onDestroyDevice()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::onDestroyDevice()
{
    // Release effect
    releaseEffect();

    // Relase the meshes
    m_meshInstances.clear();
    m_knightMesh.reset();
    m_podiumMesh.reset();

    // Release ground plane geometry
    m_groundIndexBuffer.reset();
    m_groundVertexBuffer.reset();

    // shadow map
    m_shadowMap.reset();

    // Release textures
    m_rockDiffuse.reset();
    m_groundDiffuse.reset();
    m_groundNormal.reset();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::onResizeSwapChain()
////////////////////////////////////////////////////////////////////////////////
HRESULT SoftShadowsRenderer::onResizeSwapChain(ID3D11Device *device, IDXGISwapChain *swapChain, const DXGI_SURFACE_DESC *backBufferSurfaceDesc)
{
    HRESULT hr = S_OK;

    // Update the cameras
    float aspectRatio = (float)backBufferSurfaceDesc->Width / (float)backBufferSurfaceDesc->Height;
    float fovy = D3DX_PI / 4;
    m_camera.SetProjParams(fovy, aspectRatio, 0.01f, 100.0f);
    m_camera.SetWindow(backBufferSurfaceDesc->Width, backBufferSurfaceDesc->Height);
    m_lightCamera.SetWindow(backBufferSurfaceDesc->Width, backBufferSurfaceDesc->Height);

    // Update the viewport
    m_viewport.Width  = (float)backBufferSurfaceDesc->Width;
    m_viewport.Height = (float)backBufferSurfaceDesc->Height;
    m_viewport.MinDepth = 0;
    m_viewport.MaxDepth = 1;
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::onReleasingSwapChain()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::onReleasingSwapChain()
{
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::onRender()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::onRender(ID3D11Device *device, ID3D11DeviceContext *immediateContext, double time, float elapsedTime)
{
    if (m_reloadEffect)
        loadEffect(device);

    D3DPerfScope perfScope(0xff800000, L"OnRender()");

    D3D11SavedState saveRestoreState(immediateContext);    // restore on loss of context

    // frame counter
    m_frameNumber++;

    //
    // STEP 1: render shadow map from the light's point of view
    //

    if (m_shadowTechnique != None || m_visualizeDepthTexture)
    {
        immediateContext->ClearDepthStencilView(m_shadowMap->getDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0, 0);
        immediateContext->RSSetViewports(1, &m_lightViewport);

        // Set SRV to NULL to avoid runtime warning in OMSetRenderTargets
        m_depthMapVariable->SetResource(nullptr);
        m_technique->GetPassByName("ZEqualPCF")->Apply(0, immediateContext);

        const char *passName = nullptr;
        switch (m_shadowTechnique)
        {
        case PCSS: // fallthrough
        case PCF:  passName = "ShadowMapPass"; break;
        case CHS:  passName = "ShadowMapPassCHS"; break;
        }
        auto shadowMapPass = m_technique->GetPassByName(passName);
        shadowMapPass->Apply(0, immediateContext);
        immediateContext->OMSetRenderTargets(0, nullptr, m_shadowMap->getDepthStencilView());

        drawShadowMap(immediateContext, shadowMapPass);
    }

    //
    // STEP 2: render scene from the eye's point of view
    //

    ID3D11RenderTargetView *rt = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView *ds = DXUTGetD3D11DepthStencilView();

    immediateContext->ClearRenderTargetView(rt, m_backgroundColor);
    immediateContext->ClearDepthStencilView(ds, D3D11_CLEAR_DEPTH, 1.0, 0);

    immediateContext->OMSetRenderTargets(1, &rt, ds);
    immediateContext->RSSetViewports(1, &m_viewport);

    m_depthMapVariable->SetResource(m_shadowMap->getShaderResourceView());

    if (m_visualizeDepthTexture)
    {
        m_technique->GetPassByName("VisTex")->Apply(0, immediateContext);
        immediateContext->Draw(3, 0);
    }
    else
    {
        // To reduce overdraw, do a depth prepass to layout z, followed by a shading pass
        drawScene(immediateContext, m_technique->GetPassByName("ZPrepass"));
        const char *passName = nullptr;
        switch (m_shadowTechnique)
        {
        case None: passName = "ZEqualNoShadows"; break;
        case PCSS: passName = "ZEqualPCSS"; break;
        case PCF:  passName = "ZEqualPCF"; break;
        case CHS:  passName = "CHS"; break;
        }
        drawScene(immediateContext, m_technique->GetPassByName(passName));
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::setLightRadiusWorld()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::setLightRadiusWorld(float radius)
{
    m_lightRadiusWorld = radius;
    updateLightCamera();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::addMeshInstance()
////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<SoftShadowsRenderer::MeshInstance> SoftShadowsRenderer::addMeshInstance(NvSimpleMesh *mesh, const D3DXMATRIX &worldTransform, const wchar_t *name)
{
    m_meshInstances.emplace_back(new SimpleMeshInstance(mesh, name, worldTransform));
    return m_meshInstances.back();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::addMeshInstance()
////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<SoftShadowsRenderer::MeshInstance> SoftShadowsRenderer::addMeshInstance(CDXUTSDKMesh *mesh, const D3DXMATRIX &worldTransform, const wchar_t *name)
{
    m_meshInstances.emplace_back(new SdkMeshInstance(mesh, name, worldTransform));
    return m_meshInstances.back();
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::removeMeshInstance()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::removeMeshInstance(const std::shared_ptr<MeshInstance> &instance)
{
    m_meshInstances.remove(instance);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::getSceneStats()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::getSceneStats(UINT64 &numIndices, UINT64 &numVertices, UINT &lightResolution) const
{
    numIndices = 0;
    numVertices = 0;
    lightResolution = LIGHT_RES;
    std::for_each(m_meshInstances.begin(), m_meshInstances.end(), [&] (const MeshInstanceList::value_type &instance)
    {
        instance->accumStats(numIndices, numVertices);
    });
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateCamera()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateCamera()
{
    const D3DXMATRIX &proj = *m_camera.GetProjMatrix();
    const D3DXMATRIX &view1 = *m_camera.GetViewMatrix();
    const D3DXMATRIX &world = *m_camera.GetWorldMatrix();

    D3DXMATRIX view;
    D3DXMatrixMultiply(&view, &world, &view1);

    D3DXMATRIX offset;
    D3DXMatrixTranslation(&offset, m_worldWidthOffset, 0, m_worldHeightOffset);

    D3DXMATRIX offsetView;
    D3DXMatrixMultiply (&offsetView, &view, &offset);

    D3DXMATRIX viewProj;
    D3DXMatrixMultiply(&viewProj, &offsetView, &proj);

    HRESULT hr;
    V(m_viewProjVariable->SetMatrix(viewProj));
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateLightCamera()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateLightCamera()
{
    const D3DXMATRIX &view1 = *m_lightCamera.GetViewMatrix();
    const D3DXMATRIX &world = *m_lightCamera.GetWorldMatrix();

    D3DXMATRIX view;
    D3DXMatrixMultiply(&view, &world, &view1);

    D3DXMATRIX inverseView;
    D3DXMatrixInverse(&inverseView, nullptr, &view);

    D3DXVECTOR3 lightCenterWorld;
    D3DXVECTOR3 lightCenterEye = D3DXVECTOR3(0, 0, 0);
    D3DXVec3TransformCoord(&lightCenterWorld, &lightCenterEye, &inverseView);

    static D3DXMATRIX s_view;
    // if the light source is high enough above the ground plane
    if (lightCenterWorld.y > 1.0f)
    {
        s_view = view;
    }
    else
    {
        m_lightCamera.SetWorldMatrix(s_view);        
    }
    updateLightCamera(s_view);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateLightCamera()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateLightCamera(const D3DXMATRIX &view)
{
    HRESULT hr;

    // Assuming that the bbox of mesh1 contains everything, transform the bounding box
    // to light space so we can build a bounding frustum.
    D3DXVECTOR3 center = m_knightMesh->getWorldCenter();
    D3DXVECTOR3 extents = m_knightMesh->getExtents();

    D3DXVECTOR3 box[2];
    box[0] = center - extents;
    box[1] = center + extents;

    D3DXVECTOR3 bbox[2];
    transformBoundingBox(bbox, box, view);

    float frustumWidth = std::max(fabs(bbox[0][0]), fabs(bbox[1][0])) * 2.0f;
    float frustumHeight = std::max(fabs(bbox[0][1]), fabs(bbox[1][1])) * 2.0f;
    float zNear = bbox[0][2];
    float zFar = LIGHT_ZFAR;

    // Build a bounding frustum that tightly contains the bounding box
    D3DXMATRIX proj;
    D3DXMatrixPerspectiveLH(&proj, frustumWidth, frustumHeight, zNear, zFar);
    D3DXMATRIX viewProj;
    D3DXMatrixMultiply(&viewProj, &view, &proj);

    D3DXMATRIX clip2Tex(
        0.5f,  0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.5f,  0.5f, 0.0f, 1.0f);
    D3DXMATRIX viewProjClip2Tex;
    D3DXMatrixMultiply(&viewProjClip2Tex, &viewProj, &clip2Tex);

    V(m_lightViewVariable->SetMatrix(view));
    V(m_lightViewProjVariable->SetMatrix(viewProj));
    V(m_lightViewProjClip2TexVariable->SetMatrix(viewProjClip2Tex));
    V(m_lightZNearVariable->SetFloat(zNear));
    V(m_lightZFarVariable->SetFloat(zFar));

    // Get the light position in world space
    D3DXVECTOR3 lightCenterWorld;
    D3DXVECTOR3 lightCenterEye = D3DXVECTOR3(0, 0, 0);
    D3DXMATRIX inverseView;
    D3DXMatrixInverse(&inverseView, nullptr, &view);
    D3DXVec3TransformCoord(&lightCenterWorld, &lightCenterEye, &inverseView);
    V(m_lightPosVariable->SetFloatVector(lightCenterWorld));

    // Update the light size
    updateLightSize(frustumWidth, frustumHeight);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::updateLightSize()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::updateLightSize(float frustumWidth, float frustumHeight)
{
    if (m_lightWidthVariable != nullptr)
    {
        m_lightWidthVariable->SetFloat(m_lightRadiusWorld * 2.0f);
    }

    if (m_lightRadiusUvVariable != nullptr)
    {
        float lightRadiusUV[2] =
        { 
            m_lightRadiusWorld / frustumWidth,
            m_lightRadiusWorld / frustumHeight
        };
        m_lightRadiusUvVariable->SetFloatVector(lightRadiusUV);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::createShadowMap()
////////////////////////////////////////////////////////////////////////////////
HRESULT SoftShadowsRenderer::createShadowMap(ID3D11Device *device)
{
    HRESULT hr;

    m_shadowMap.reset(new SimpleTexture2D(
        device,    "ShadowMapDepthRT",
        DXGI_FORMAT_R32_TYPELESS,
        LIGHT_RES, LIGHT_RES, 1,
        SimpleTexture2D::ShaderResourceView | SimpleTexture2D::DepthStencilView));

    if (!m_shadowMap) V_RETURN(E_OUTOFMEMORY);
    V_RETURN(m_shadowMap->errorCode());

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::createGeometry()
////////////////////////////////////////////////////////////////////////////////
HRESULT SoftShadowsRenderer::createGeometry(ID3D11Device *device)
{
    return createPlane(m_groundIndexBuffer, m_groundVertexBuffer, device, m_groundRadius, m_groundHeight);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::createTextures()
////////////////////////////////////////////////////////////////////////////////
HRESULT SoftShadowsRenderer::createTextures(ID3D11Device *device)
{
    HRESULT hr;

    m_groundDiffuse.reset(new SimpleTexture2D(device, "Ground Diffuse", GROUND_DIFFUSE_MAP_FILENAME));
    if (!m_groundDiffuse) V_RETURN(E_OUTOFMEMORY);
    V_RETURN(m_groundDiffuse->errorCode());

    m_groundNormal.reset(new SimpleTexture2D(device, "Ground Normal", GROUND_NORMAL_MAP_FILENAME));
    if (!m_groundNormal) V_RETURN(E_OUTOFMEMORY);
    V_RETURN(m_groundNormal->errorCode());

    m_rockDiffuse.reset(new SimpleTexture2D(device, "Rock Diffuse", ROCK_DIFFUSE_MAP_FILENAME));
    if (!m_groundNormal) V_RETURN(E_OUTOFMEMORY);
    V_RETURN(m_groundNormal->errorCode());

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::loadEffect()
////////////////////////////////////////////////////////////////////////////////
HRESULT SoftShadowsRenderer::loadEffect(ID3D11Device* device)
{
    HRESULT hr;

    // Release the effect.
    releaseEffect();

    // Setup macros
    D3D10_SHADER_MACRO Shader_Macros[3];
    memset(Shader_Macros, 0, sizeof(Shader_Macros));

    CHAR presetStr[2];
    StringCchPrintfA(presetStr, 2, "%d", static_cast<int>(m_pcssPreset));
    Shader_Macros[0].Name = "PRESET";
    Shader_Macros[0].Definition = presetStr;

    // Compile the effect file
    unique_ref_ptr<ID3D10Blob>::type effectBuffer;
    V_RETURN(compileFromFile(effectBuffer, L"SoftShadows.fx", nullptr, "fx_5_0", Shader_Macros));

    // Create the effect
    ID3DX11Effect *effect = nullptr;
    V_RETURN(D3DX11CreateEffectFromMemory(effectBuffer->GetBufferPointer(), effectBuffer->GetBufferSize(), 0, device, &effect));
    m_effect.reset(effect);
    DXUT_SetDebugName(m_vertexLayout.get(), "PCSSEffect");
    VB_RETURN(m_effect && m_effect->IsValid());

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    D3DX11_PASS_DESC passDesc;
    m_technique = m_effect->GetTechniqueByName("SoftShadows");
    VB_RETURN(m_technique && m_technique->IsValid());
    m_technique->GetPassByIndex(0)->GetDesc(&passDesc);
    ID3D11InputLayout *vertexLayout = nullptr;
    V_RETURN(device->CreateInputLayout(layout, ARRAYSIZE(layout), passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &vertexLayout));
    m_vertexLayout.reset(vertexLayout);
    DXUT_SetDebugName(m_vertexLayout.get(), "PrimaryVertexLayout");

    // Make input layouts for all the meshes for our instances
    std::for_each(m_meshInstances.begin(), m_meshInstances.end(), [&] (const MeshInstanceList::value_type &instance)
    {
        instance->createInputLayout(device, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize);
    });

    // Set textures
    VB_RETURN(m_rockDiffuse && m_rockDiffuse->isValid());
    auto rockDiffuseVariable = getShaderResourceVariable("g_rockDiffuse");    
    VB_RETURN(rockDiffuseVariable && rockDiffuseVariable->IsValid());
    V_RETURN(rockDiffuseVariable->SetResource(m_rockDiffuse->getShaderResourceView()));

    VB_RETURN(m_groundDiffuse && m_groundDiffuse->isValid());
    auto groundDiffuseVariable = getShaderResourceVariable("g_groundDiffuse");    
    VB_RETURN(groundDiffuseVariable && groundDiffuseVariable->IsValid())
        V_RETURN(groundDiffuseVariable->SetResource(m_groundDiffuse->getShaderResourceView()));

    VB_RETURN(m_groundNormal && m_groundNormal->isValid());
    auto groundNormalVariable = getShaderResourceVariable("g_groundNormal");    
    VB_RETURN(groundNormalVariable && groundNormalVariable->IsValid());
    V_RETURN(groundNormalVariable->SetResource(m_groundNormal->getShaderResourceView()));

    // Set the podium center
    D3DXVECTOR3 podiumCenter = m_podiumMesh->getCenter();
    auto podiumCenterVariable = getVectorVariable("g_podiumCenterWorld");
    VB_RETURN(podiumCenterVariable && podiumCenterVariable->IsValid());
    V_RETURN(podiumCenterVariable->SetFloatVector(&podiumCenter[0]));

    // Set the shadow map dimensions variable
    float dim = (float)LIGHT_RES;
    D3DXVECTOR4 shadowMapDim(dim, dim, 1.0f / dim, 1.0f / dim);
    auto shadowMapDimVariable = getVectorVariable("g_shadowMapDimensions");
    VB_RETURN(shadowMapDimVariable && shadowMapDimVariable->IsValid());
    V_RETURN(shadowMapDimVariable->SetFloatVector(shadowMapDim));

    // Get light parameters
    m_lightViewVariable = getMatrixVariable("g_lightView");
    VB_RETURN(m_lightViewVariable && m_lightViewVariable->IsValid());

    m_lightViewProjVariable = getMatrixVariable("g_lightViewProj");
    VB_RETURN(m_lightViewProjVariable && m_lightViewProjVariable->IsValid());

    m_lightViewProjClip2TexVariable = getMatrixVariable("g_lightViewProjClip2Tex");
    VB_RETURN(m_lightViewProjClip2TexVariable && m_lightViewProjClip2TexVariable->IsValid());

    m_lightZNearVariable = getScalarVariable("g_lightZNear");
    VB_RETURN(m_lightZNearVariable && m_lightZNearVariable->IsValid());

    m_lightZFarVariable = getScalarVariable("g_lightZFar");
    VB_RETURN(m_lightZFarVariable && m_lightZFarVariable->IsValid());

    m_lightPosVariable = getVectorVariable("g_lightPos");
    VB_RETURN(m_lightPosVariable && m_lightPosVariable->IsValid());

    m_lightWidthVariable = getScalarVariable("g_lightWidth");
    VB_RETURN(m_lightWidthVariable && m_lightWidthVariable->IsValid());

    m_lightRadiusUvVariable = getVectorVariable("g_lightRadiusUV");
    VB_RETURN(m_lightRadiusUvVariable && m_lightRadiusUvVariable->IsValid());

    // Get the view projection matrix variable
    m_viewProjVariable = getMatrixVariable("g_viewProj");
    VB_RETURN(m_viewProjVariable && m_viewProjVariable->IsValid());

    // Get the world matrix variable
    m_worldVariable = getMatrixVariable("g_world");
    VB_RETURN(m_worldVariable && m_worldVariable->IsValid());

    // Get the texture usage variables
    m_useDiffuseVariable = getScalarVariable("g_useDiffuse");
    VB_RETURN(m_useDiffuseVariable && m_useDiffuseVariable->IsValid());

    m_useTextureVariable = getScalarVariable("g_UseTexture");
    VB_RETURN(m_useDiffuseVariable && m_useDiffuseVariable->IsValid());

    // Get the depth map variable
    m_depthMapVariable = getShaderResourceVariable("g_shadowMap");
    VB_RETURN(m_depthMapVariable && m_depthMapVariable->IsValid());

    // Get optional variables
    m_diffuseVariable = getShaderResourceVariable("g_diffuseTexture");
    m_normalVariable = getShaderResourceVariable("g_normalTexture");
    m_specularVariable = getShaderResourceVariable("g_specularTexture");

    // Update the light
    updateLightCamera();

    m_reloadEffect = false;
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::releaseEffect()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::releaseEffect()
{
    // Release mesh input layouts for all the meshes for our instances
    std::for_each(m_meshInstances.begin(), m_meshInstances.end(), [&] (const MeshInstanceList::value_type &instance)
    {
        instance->releaseInputLayout();
    });

    m_vertexLayout.reset();
    m_effect.reset();
    m_technique = nullptr;
    m_lightViewVariable = nullptr;
    m_lightViewProjVariable = nullptr;
    m_lightViewProjClip2TexVariable = nullptr;
    m_lightZNearVariable = nullptr;
    m_lightZFarVariable = nullptr;
    m_lightPosVariable = nullptr;
    m_lightWidthVariable = nullptr;
    m_lightRadiusUvVariable = nullptr;
    m_viewProjVariable = nullptr;
    m_worldVariable = nullptr;
    m_useDiffuseVariable = nullptr;
    m_useTextureVariable = nullptr;
    m_depthMapVariable = nullptr;
    m_diffuseVariable = nullptr;
    m_normalVariable = nullptr;
    m_specularVariable = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::getScalarVariable()
////////////////////////////////////////////////////////////////////////////////
ID3DX11EffectScalarVariable *SoftShadowsRenderer::getScalarVariable(const char *name)
{
    auto variable = getVariable(name);
    if (variable == nullptr)
        return nullptr;

    auto scalar = variable->AsScalar();
    if (scalar == nullptr || !scalar->IsValid())
        return nullptr;

    return scalar;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::getVectorVariable()
////////////////////////////////////////////////////////////////////////////////
ID3DX11EffectVectorVariable *SoftShadowsRenderer::getVectorVariable(const char *name)
{
    auto variable = getVariable(name);
    if (variable == nullptr)
        return nullptr;

    auto vector = variable->AsVector();
    if (vector == nullptr || !vector->IsValid())
        return nullptr;

    return vector;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::getMatrixVariable()
////////////////////////////////////////////////////////////////////////////////
ID3DX11EffectMatrixVariable *SoftShadowsRenderer::getMatrixVariable(const char *name)
{
    auto variable = getVariable(name);
    if (variable == nullptr)
        return nullptr;

    auto matrix = variable->AsMatrix();
    if (matrix == nullptr || !matrix->IsValid())
        return nullptr;

    return matrix;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::getShaderResourceVariable()
////////////////////////////////////////////////////////////////////////////////
ID3DX11EffectShaderResourceVariable *SoftShadowsRenderer::getShaderResourceVariable(const char *name)
{
    auto variable = getVariable(name);
    if (variable == nullptr)
        return nullptr;

    auto resource = variable->AsShaderResource();
    if (resource == nullptr || !resource->IsValid())
        return nullptr;

    return resource;
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::getVariable()
////////////////////////////////////////////////////////////////////////////////
ID3DX11EffectVariable *SoftShadowsRenderer::getVariable(const char *name)
{
    if (!m_effect)
        return nullptr;

    auto variable = m_effect->GetVariableByName(name);
    if (variable == nullptr || !variable->IsValid())
        return nullptr;

    return variable;    
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::drawShadowMap()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::drawShadowMap(ID3D11DeviceContext *immediateContext, ID3DX11EffectPass *pass)
{
    D3DPerfScope perfScope(0xff20A020, L"DrawShadowMap");

    // Draw mesh instances
    {
        D3DPerfScope perfScope(0xff20A020, L"MeshInstances");
        std::for_each(m_meshInstances.begin(), m_meshInstances.end(), [&] (const MeshInstanceList::value_type &instance)
        {
            D3DPerfScope perfScope(0xff20A020, instance->getName());
            instance->draw(
                immediateContext,
                m_vertexLayout.get(),
                pass,
                m_worldVariable);
        });
    }
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::drawScene()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::drawScene(ID3D11DeviceContext *immediateContext, ID3DX11EffectPass *pass)
{
    D3DPerfScope perfScope(0xff20A020, L"DrawScene");

    // Draw mesh instances
    {
        D3DPerfScope perfScope(0xff20A020, L"MeshInstances");
        m_useDiffuseVariable->SetBool(true);
        std::for_each(m_meshInstances.begin(), m_meshInstances.end(), [&] (const MeshInstanceList::value_type &instance)
        {
            if (instance == m_podiumMesh)
                m_useTextureVariable->SetInt(m_useTexture ? 2 : 0);                
            else
                m_useTextureVariable->SetInt(0);

            D3DPerfScope perfScope(0xff20A020, instance->getName());
            instance->draw(
                immediateContext,
                m_vertexLayout.get(),
                pass,
                m_worldVariable,
                m_diffuseVariable,
                m_normalVariable,
                m_specularVariable);
        });
    }

    // Draw ground
    drawGround(immediateContext, pass);
}

////////////////////////////////////////////////////////////////////////////////
// SoftShadowsRenderer::drawGround()
////////////////////////////////////////////////////////////////////////////////
void SoftShadowsRenderer::drawGround(ID3D11DeviceContext *immediateContext, ID3DX11EffectPass *pass)
{
    D3DPerfScope perfScope(0xff20A020, L"DrawGround");

    m_useDiffuseVariable->SetBool(false);
    m_useTextureVariable->SetInt(m_useTexture ? 1 : 0);
    pass->Apply(0, immediateContext);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    auto groundVertexBuffer = m_groundVertexBuffer.get();
    immediateContext->IASetVertexBuffers(0, 1, &groundVertexBuffer, &stride, &offset);
    immediateContext->IASetIndexBuffer(m_groundIndexBuffer.get(), DXGI_FORMAT_R32_UINT, 0 );
    immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    immediateContext->IASetInputLayout(m_vertexLayout.get());
    immediateContext->DrawIndexed(6, 0, 0);
}
