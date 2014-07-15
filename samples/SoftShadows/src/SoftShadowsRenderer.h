//----------------------------------------------------------------------------------
// File:        SoftShadows\src/SoftShadowsRenderer.h
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

#include "SimpleTexture2D.h"

#include <list>
#include <string>

class NvSimpleMesh;
class CDXUTSDKMesh;
class CBaseCamera;

class SoftShadowsRenderer
{
public:
    SoftShadowsRenderer();
    ~SoftShadowsRenderer();

    class MeshInstance
    {
    public:

        MeshInstance(const wchar_t *name, const D3DMATRIX &worldTransform)
            : m_name(name)
            , m_worldTransform(worldTransform)
        {
        }

        virtual ~MeshInstance() {}

        const wchar_t *getName() const { return m_name.c_str(); }

        void setWorldTransform(const D3DMATRIX &worldTransform) { m_worldTransform = worldTransform; }
        const D3DMATRIX &getWorldTransform() const { return m_worldTransform; }

        D3DXVECTOR3 getWorldCenter() const;

        virtual D3DXVECTOR3 getCenter() const = 0;
        virtual D3DXVECTOR3 getExtents() const = 0;

        virtual HRESULT createInputLayout(ID3D11Device *device, BYTE *iaSig, SIZE_T iaSigSize) { return S_OK; }
        virtual void releaseInputLayout() {}

        virtual void draw(
            ID3D11DeviceContext *immediateContext,
            ID3D11InputLayout *vertexLayout,
            ID3DX11EffectPass *pass,
            ID3DX11EffectMatrixVariable *worldTransformVariable,
            ID3DX11EffectShaderResourceVariable *diffuseVariable = nullptr,
            ID3DX11EffectShaderResourceVariable *normalVariable = nullptr,
            ID3DX11EffectShaderResourceVariable *specularVariable = nullptr) = 0;

        virtual void accumStats(UINT64 &numIndices, UINT64 &numVertices) = 0;

    protected:

        std::wstring m_name;
        D3DXMATRIX m_worldTransform;        
    };

    void onFrameMove(double time, float elapsedTime);

    HRESULT onCreateDevice(ID3D11Device *device, const DXGI_SURFACE_DESC *backBufferSurfaceDesc);
    void onDestroyDevice();

    HRESULT onResizeSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);
    void onReleasingSwapChain();

    void onRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,float fElapsedTime);

    void resolveSceneColor(ID3D11DeviceContext* pd3dImmediateContext);
    void resolveSceneDepth(ID3D11DeviceContext* pd3dImmediateContext);
    void renderPost(ID3D11DeviceContext* pd3dImmediateContext,ID3D11RenderTargetView *pBackBufferRTV, ID3D11DepthStencilView *pBackBufferDST);

    // Camera access
    const CBaseCamera &getCamera() const { return m_camera; }
    CBaseCamera &getCamera() { return m_camera; }

    // Light camera access
    const CBaseCamera &getLightCamera() const { return m_lightCamera; }
    CBaseCamera &getLightCamera() { return m_lightCamera; }

    // Light radius access
    void setLightRadiusWorld(float radius);
    float getLightRadiusWorld() const { return m_lightRadiusWorld; }

    // World width offset access
    void changeWorldWidthOffset(float deltaOffset) { m_worldWidthOffset += deltaOffset; }
    float getWorldWidthOffset() const { return m_worldWidthOffset; }

    // World height offset access
    void changeWorldHeightOffset(float deltaOffset) { m_worldHeightOffset += deltaOffset; }
    float getWorldHeightOffset() const { return m_worldHeightOffset; }

    // PCSS preset access
    enum PcssPreset
    {
        Poisson_25_25 = 0,
        Poisson_32_64,
        Poisson_100_100,
        Poisson_64_128,
        Regular_49_225
    };

    void setPcssPreset(PcssPreset preset) { m_pcssPreset = preset; m_reloadEffect = true; }
    PcssPreset getPcssPreset() const { return m_pcssPreset; }

    // Force reloading the effect
    void reloadEffect() { m_reloadEffect = true; }

    // Shadow technique access
    enum ShadowTechnique
    {
        None = 0,
        PCSS,
        PCF,
        CHS
    };

    void setShadowTechnique(ShadowTechnique technique) { m_shadowTechnique = technique; }
    ShadowTechnique getShadowTechnique() const { return m_shadowTechnique; }

    // Use texture
    void useTexture(bool enable) { m_useTexture = enable; }
    bool useTexture() const { return m_useTexture; }

    // Display texture
    void visualizeDepthTexture(bool enable) { m_visualizeDepthTexture = enable; }
    bool visualizeDepthTexture() const { return m_visualizeDepthTexture; }

    // Scene access
    std::shared_ptr<MeshInstance> addMeshInstance(NvSimpleMesh *mesh, const D3DXMATRIX &worldTransform, const wchar_t *name);
    std::shared_ptr<MeshInstance> addMeshInstance(CDXUTSDKMesh *mesh, const D3DXMATRIX &worldTransform, const wchar_t *name);
    void removeMeshInstance(const std::shared_ptr<MeshInstance> &instance);

    // Scene elements of interest
    void setGroundPlane(float height, float radius)
    {
        m_groundHeight = height;
        m_groundRadius = radius;
    }

    void setKnightMesh(std::shared_ptr<MeshInstance> mesh) { m_knightMesh = mesh; }
    std::shared_ptr<MeshInstance> getKnightMesh() { return m_knightMesh; }

    void setPodiumMesh(std::shared_ptr<MeshInstance> mesh) { m_podiumMesh = mesh; }
    std::shared_ptr<MeshInstance> getPodiumMesh() { return m_podiumMesh; }

    // Stats
    void getSceneStats(UINT64 &numIndices, UINT64 &numVertices, UINT &lightResolution) const;

private:

    class SimpleMeshInstance;
    class SdkMeshInstance;

    void updateCamera();
    void updateLightCamera();
    void updateLightCamera(const D3DXMATRIX &view);
    void updateLightSize(float frustumWidth, float frustumHeight);

    HRESULT createShadowMap(ID3D11Device *device);
    HRESULT createGeometry(ID3D11Device *device);
    HRESULT createTextures(ID3D11Device *device);
    HRESULT loadEffect(ID3D11Device *device);
    void releaseEffect();

    ID3DX11EffectScalarVariable *getScalarVariable(const char *name);
    ID3DX11EffectVectorVariable *getVectorVariable(const char *name);
    ID3DX11EffectMatrixVariable *getMatrixVariable(const char *name);
    ID3DX11EffectShaderResourceVariable *getShaderResourceVariable(const char *name);
    ID3DX11EffectVariable *getVariable(const char *name);

    void drawShadowMap(ID3D11DeviceContext *immediateContext, ID3DX11EffectPass *pass);
    void drawScene(ID3D11DeviceContext *immediateContext, ID3DX11EffectPass *pass);
    void drawMeshes(ID3D11DeviceContext *immediateContext, ID3DX11EffectPass *pass);
    void drawGround(ID3D11DeviceContext *immediateContext, ID3DX11EffectPass *pass);

    // ---
    // Scene elements
    typedef std::list<std::shared_ptr<MeshInstance>> MeshInstanceList;
    MeshInstanceList m_meshInstances;
    std::shared_ptr<MeshInstance> m_knightMesh;
    std::shared_ptr<MeshInstance> m_podiumMesh;

    // ---
    // Cameras
    CModelViewerCamera m_camera;      // model viewer camera
    CModelViewerCamera m_lightCamera; // light camera

    // ---
    // Viewports
    D3D11_VIEWPORT m_viewport;
    D3D11_VIEWPORT m_lightViewport;

    // ---
    // Light parameters
    float m_lightRadiusWorld;

    // ---
    // Swap chain independent resources (created in OnCreateDevice, released in OnDestroyDevice)
    unique_ref_ptr<ID3D11InputLayout>::type m_vertexLayout;
    std::unique_ptr<SimpleTexture2D>        m_shadowMap;

    // ---
    // Effect resources
    unique_ref_ptr<ID3DX11Effect>::type  m_effect;
    ID3DX11EffectTechnique              *m_technique;

    // Light variables
    ID3DX11EffectMatrixVariable *m_lightViewVariable;
    ID3DX11EffectMatrixVariable *m_lightViewProjVariable;
    ID3DX11EffectMatrixVariable *m_lightViewProjClip2TexVariable;
    ID3DX11EffectScalarVariable *m_lightZNearVariable;
    ID3DX11EffectScalarVariable *m_lightZFarVariable;
    ID3DX11EffectVectorVariable *m_lightPosVariable;
    ID3DX11EffectScalarVariable *m_lightWidthVariable;
    ID3DX11EffectVectorVariable *m_lightRadiusUvVariable;

    // Transform variables
    ID3DX11EffectMatrixVariable *m_viewProjVariable;
    ID3DX11EffectMatrixVariable *m_worldVariable;

    // Misc variables
    ID3DX11EffectScalarVariable *m_useDiffuseVariable;
    ID3DX11EffectScalarVariable *m_useTextureVariable;

    // Texture variables
    ID3DX11EffectShaderResourceVariable *m_depthMapVariable;
    ID3DX11EffectShaderResourceVariable *m_diffuseVariable;
    ID3DX11EffectShaderResourceVariable *m_normalVariable;
    ID3DX11EffectShaderResourceVariable *m_specularVariable;

    // ---
    // Ground plane
    float m_groundHeight;
    float m_groundRadius;
    unique_ref_ptr<ID3D11Buffer>::type m_groundVertexBuffer;
    unique_ref_ptr<ID3D11Buffer>::type m_groundIndexBuffer;

    // ---
    // Texture resources
    std::unique_ptr<SimpleTexture2D> m_rockDiffuse;
    std::unique_ptr<SimpleTexture2D> m_groundDiffuse;
    std::unique_ptr<SimpleTexture2D> m_groundNormal;

    unsigned int m_frameNumber;

    D3DXCOLOR m_backgroundColor;

    float m_worldHeightOffset;
    float m_worldWidthOffset;

    PcssPreset m_pcssPreset;
    ShadowTechnique m_shadowTechnique;

    bool m_reloadEffect;
    bool m_useTexture;
    bool m_visualizeDepthTexture;
};