//----------------------------------------------------------------------------------
// File:        ComputeFilter\src/main.cpp
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
#include "common_util.h"

#include "scene.h"
#include "sat.h"
#include "PerfTracker.h"

#include "AntTweakBar.h"
#include "Camera.h"
#include "DeviceManager.h"

#include <DirectXMath.h>
#include <d3dcommon.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////////////////////////

DeviceManager * g_device_manager;

const float CAMERA_CLIP_NEAR =   0.50f;
const float CAMERA_CLIP_FAR = 3000.00f;

bool g_bRenderHUD = true;

typedef enum eViewMode {
    VIEW_MODE_DOF = 0,
    VIEW_MODE_BLUR_SIZE
};

eViewMode g_view_mode = VIEW_MODE_DOF;

FLOAT g_focal_distance = 0.35f;
FLOAT g_focal_range    = 0.20f;
FLOAT g_focal_aperture = 0.25f;

FLOAT g_screen_area = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene Controller
////////////////////////////////////////////////////////////////////////////////////////////////////

class SceneController : public IVisualController
{
private:

    CFirstPersonCamera * camera;

    struct CBCamera 
    {
        DirectX::XMMATRIX world_view_proj_xform;
        DirectX::XMFLOAT3 eye_pos;
        FLOAT z_near;
        FLOAT z_far;
        FLOAT dof_near;
        FLOAT dof_far;
        FLOAT aperture;
    };
    ID3D11Buffer * camera_cb;

    struct CBSceneObject
    {
        DirectX::XMMATRIX model_xform;
    };
    ID3D11Buffer * object_cb;

    ID3D11Buffer * quad_verts;

    ID3D11InputLayout * scene_layout;
    ID3D11InputLayout * quad_layout;

    ID3D11VertexShader * vs_scene;
    ID3D11VertexShader * vs_quad;
    ID3D11PixelShader * ps_scene;
    ID3D11PixelShader * ps_post;
    ID3D11PixelShader * ps_post_blursize;
    
    ID3D11RasterizerState * rs_state;

    ID3D11SamplerState * samp_linear_wrap;
    ID3D11SamplerState * samp_linear_clamp;
    ID3D11SamplerState * samp_point_clamp;

    ID3D11DepthStencilState * ds_state;
    ID3D11DepthStencilState * ds_state_disabled;
    
    ID3D11BlendState * blend_state;
    ID3D11BlendState * enabled_blend_state;

    ID3D11Texture2D * scene_tex;
    ID3D11RenderTargetView * scene_rtv;
    ID3D11ShaderResourceView * scene_srv;
    ID3D11Texture2D * scene_depth_tex;
    ID3D11DepthStencilView * scene_depth_dsv;
    ID3D11ShaderResourceView * scene_depth_srv;

    Scene::RenderList scene;

public:
    SceneController(CFirstPersonCamera * cam)
    {
        this->camera = cam;

        this->scene_tex = nullptr;
        this->scene_rtv = nullptr;
        this->scene_srv = nullptr;
        this->scene_depth_tex = nullptr;
        this->scene_depth_dsv = nullptr;
        this->scene_depth_srv = nullptr;
    }

    virtual HRESULT DeviceCreated(ID3D11Device* device)
    {
        HRESULT hr;
        (void)hr;

        {
            D3D11_BUFFER_DESC buffer_desc;
            ZeroMemory(&buffer_desc, sizeof(buffer_desc));
            buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
            buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            buffer_desc.ByteWidth = sizeof(CBCamera);
            _ASSERT((buffer_desc.ByteWidth % 16) == 0);
            device->CreateBuffer(&buffer_desc, nullptr, &this->camera_cb);
        }

        {
            D3D11_BUFFER_DESC buffer_desc;
            ZeroMemory(&buffer_desc, sizeof(buffer_desc));
            buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
            buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            buffer_desc.ByteWidth = sizeof(CBSceneObject);
            _ASSERT((buffer_desc.ByteWidth % 16) == 0);
            device->CreateBuffer(&buffer_desc, nullptr, &this->object_cb);
        }

        {
            DirectX::XMFLOAT2 verts[6];
            verts[0] = DirectX::XMFLOAT2(-1.f, -1.f);
            verts[1] = DirectX::XMFLOAT2(-1.f, 1.f);
            verts[2] = DirectX::XMFLOAT2(1.f, 1.f);
            verts[3] = DirectX::XMFLOAT2(-1.f, -1.f);
            verts[4] = DirectX::XMFLOAT2(1.f, 1.f);
            verts[5] = DirectX::XMFLOAT2(1.f, -1.f);
            D3D11_BUFFER_DESC desc = { 6 * sizeof(DirectX::XMFLOAT2), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0 };
            D3D11_SUBRESOURCE_DATA data = { (void *)verts, sizeof(DirectX::XMFLOAT2), 6 * sizeof(DirectX::XMFLOAT2) };
            hr = device->CreateBuffer(&desc, &data, &this->quad_verts);
            _ASSERT(!FAILED(hr));
        }

        Scene::load_model(device, "..\\..\\computefilter\\media\\sponza\\SponzaNoFlag.x", this->scene);

        void * bytecode;
        UINT bytecode_length;
        get_shader_resource(L"VS_SCENE", &bytecode, bytecode_length);
        device->CreateVertexShader(bytecode, bytecode_length, nullptr, &this->vs_scene);

        D3D11_INPUT_ELEMENT_DESC scene_layout_desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        hr = device->CreateInputLayout(scene_layout_desc, sizeof(scene_layout_desc)/sizeof(D3D11_INPUT_ELEMENT_DESC), bytecode, bytecode_length, &this->scene_layout);

        get_shader_resource(L"PS_SCENE", &bytecode, bytecode_length);
        device->CreatePixelShader(bytecode, bytecode_length, nullptr, &this->ps_scene);

        D3D11_INPUT_ELEMENT_DESC quad_layout_desc[] = {{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 }};
        get_shader_resource(L"VS_QUAD", &bytecode, bytecode_length);
        device->CreateVertexShader(bytecode, bytecode_length, nullptr, &this->vs_quad);
        hr = device->CreateInputLayout(quad_layout_desc, sizeof(quad_layout_desc)/sizeof(D3D11_INPUT_ELEMENT_DESC), bytecode, bytecode_length, &this->quad_layout);

        get_shader_resource(L"PS_POST", &bytecode, bytecode_length);
        device->CreatePixelShader(bytecode, bytecode_length, nullptr, &this->ps_post);

        get_shader_resource(L"PS_POST_BLURSIZE", &bytecode, bytecode_length);
        device->CreatePixelShader(bytecode, bytecode_length, nullptr, &this->ps_post_blursize);
        {
            D3D11_RASTERIZER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.CullMode = D3D11_CULL_NONE;
            desc.FillMode = D3D11_FILL_SOLID;
            desc.AntialiasedLineEnable = FALSE;
            desc.DepthBias = 0;
            desc.DepthBiasClamp = 0;
            desc.DepthClipEnable = TRUE;
            desc.FrontCounterClockwise = FALSE;
            desc.MultisampleEnable = TRUE;
            desc.ScissorEnable = FALSE;
            desc.SlopeScaledDepthBias = 0;
            device->CreateRasterizerState(&desc, &this->rs_state);
        }
        {
            D3D11_SAMPLER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.MipLODBias = 0.0f;
            desc.MaxAnisotropy = 1;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f;
            desc.MinLOD = 0;
            desc.MaxLOD = D3D11_FLOAT32_MAX;
            device->CreateSamplerState(&desc, &this->samp_linear_wrap);
        }
        {
            D3D11_SAMPLER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.MipLODBias = 0.0f;
            desc.MaxAnisotropy = 1;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f;
            desc.MinLOD = 0;
            desc.MaxLOD = D3D11_FLOAT32_MAX;
            device->CreateSamplerState(&desc, &this->samp_linear_clamp);
        }
        {
            D3D11_SAMPLER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.MipLODBias = 0.0f;
            desc.MaxAnisotropy = 1;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f;
            desc.MinLOD = 0;
            desc.MaxLOD = D3D11_FLOAT32_MAX;
            device->CreateSamplerState(&desc, &this->samp_point_clamp);
        }
        {
            D3D11_DEPTH_STENCIL_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.StencilEnable = FALSE;
            desc.DepthEnable = TRUE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
            device->CreateDepthStencilState(&desc, &this->ds_state);
        }
        {
            D3D11_DEPTH_STENCIL_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.StencilEnable = FALSE;
            desc.DepthEnable = FALSE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
            device->CreateDepthStencilState(&desc, &this->ds_state_disabled);
        }
        {
            D3D11_BLEND_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.RenderTarget[0].BlendEnable = FALSE;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            device->CreateBlendState(&desc, &this->blend_state);
        }
        {
            D3D11_BLEND_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.AlphaToCoverageEnable = FALSE;
            desc.IndependentBlendEnable = TRUE;
            desc.RenderTarget[0].BlendEnable = TRUE;
            desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            device->CreateBlendState(&desc, &this->enabled_blend_state);
        }

        return S_OK;
    }

    virtual void DeviceDestroyed()
    {
        SAT::Cleanup();
        SAFE_RELEASE(this->camera_cb);
        SAFE_RELEASE(this->object_cb);
        SAFE_RELEASE(this->scene_layout);
        SAFE_RELEASE(this->quad_layout);
        SAFE_RELEASE(this->vs_scene);
        SAFE_RELEASE(this->vs_quad);
        SAFE_RELEASE(this->ps_scene);
        SAFE_RELEASE(this->ps_post);
        SAFE_RELEASE(this->ps_post_blursize);
        SAFE_RELEASE(this->rs_state);
        SAFE_RELEASE(this->samp_linear_wrap);
        SAFE_RELEASE(this->samp_linear_clamp);
        SAFE_RELEASE(this->samp_point_clamp);
        SAFE_RELEASE(this->ds_state);
        SAFE_RELEASE(this->ds_state_disabled);
        SAFE_RELEASE(this->blend_state);
        SAFE_RELEASE(this->enabled_blend_state);
        SAFE_RELEASE(this->scene_tex);
        SAFE_RELEASE(this->scene_rtv);
        SAFE_RELEASE(this->scene_srv);
        SAFE_RELEASE(this->scene_depth_tex);
        SAFE_RELEASE(this->scene_depth_dsv);
        SAFE_RELEASE(this->scene_depth_dsv);

        for (auto object = this->scene.begin(); object != this->scene.end(); ++object)
        {
            delete (*object);
        }
        this->scene.clear();
    }

    virtual void BackBufferResized(ID3D11Device* device, const DXGI_SURFACE_DESC* surface_desc)
    {
        HRESULT hr;
        SAFE_RELEASE(this->scene_tex);
        SAFE_RELEASE(this->scene_rtv);
        SAFE_RELEASE(this->scene_srv);
        SAFE_RELEASE(this->scene_depth_tex);
        SAFE_RELEASE(this->scene_depth_dsv);
        SAFE_RELEASE(this->scene_depth_dsv);

        {
            D3D11_TEXTURE2D_DESC tex_desc;
            ZeroMemory(&tex_desc, sizeof(tex_desc));
            tex_desc.Width = surface_desc->Width;
            tex_desc.Height = surface_desc->Height;
            tex_desc.MipLevels = 1;
            tex_desc.ArraySize = 1;
            tex_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            tex_desc.SampleDesc.Count = 1;
            tex_desc.Usage = D3D11_USAGE_DEFAULT;
            tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            hr = device->CreateTexture2D(&tex_desc, nullptr, &this->scene_tex);
            _ASSERT(!FAILED(hr));

            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
            ZeroMemory(&rtv_desc, sizeof(rtv_desc));
            rtv_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice = 0;
            hr = device->CreateRenderTargetView(this->scene_tex, &rtv_desc, &this->scene_rtv);
            _ASSERT(!FAILED(hr));

            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
            ZeroMemory(&srv_desc, sizeof(srv_desc));
            srv_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MipLevels = 1;
            srv_desc.Texture2D.MostDetailedMip = 0;
            hr = device->CreateShaderResourceView(this->scene_tex, &srv_desc, &this->scene_srv);
            _ASSERT(!FAILED(hr));
        }

        {
            D3D11_TEXTURE2D_DESC tex_desc;
            ZeroMemory(&tex_desc, sizeof(tex_desc));
            tex_desc.Width = surface_desc->Width;
            tex_desc.Height = surface_desc->Height;
            tex_desc.MipLevels = 1;
            tex_desc.ArraySize = 1;
            tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
            tex_desc.SampleDesc.Count = 1;
            tex_desc.Usage = D3D11_USAGE_DEFAULT;
            tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            hr = device->CreateTexture2D(&tex_desc, nullptr, &this->scene_depth_tex);
            _ASSERT(!FAILED(hr));

            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
            ZeroMemory(&dsv_desc, sizeof(dsv_desc));
            dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Texture2D.MipSlice = 0;
            hr = device->CreateDepthStencilView(this->scene_depth_tex, &dsv_desc, &this->scene_depth_dsv);
            _ASSERT(!FAILED(hr));

            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
            ZeroMemory(&srv_desc, sizeof(srv_desc));
            srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MipLevels = 1;
            srv_desc.Texture2D.MostDetailedMip = 0;
            hr = device->CreateShaderResourceView(this->scene_depth_tex, &srv_desc, &this->scene_depth_srv);
            _ASSERT(!FAILED(hr));
        }
        SAT::Initialize(device, surface_desc->Width, surface_desc->Height);

        g_screen_area = sqrtf((float) surface_desc->Width * surface_desc->Height);
    }

    virtual void Render(ID3D11Device*, ID3D11DeviceContext* ctx, ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV)
    {
        PERF_FRAME_BEGIN(ctx);
        {
            PERF_EVENT_SCOPED(ctx, "Render Scene");
            float clear_color[4] = { 0.30f, 0.50f, 0.80f, 0.0f };
            ctx->ClearRenderTargetView( pRTV, clear_color);
            ctx->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

            UINT numViewports = 1;
            D3D11_VIEWPORT viewport;
            ctx->RSGetViewports(&numViewports, &viewport);

            ctx->ClearRenderTargetView(this->scene_rtv, clear_color);
            ctx->ClearDepthStencilView( this->scene_depth_dsv, D3D11_CLEAR_DEPTH, 1.0, 0 );
            ctx->RSSetViewports(1, &viewport);
            ctx->OMSetRenderTargets(1, &this->scene_rtv, this->scene_depth_dsv);
        
            ID3D11Buffer * cbs[16] = {nullptr};
            {
                D3DXMATRIX view_matrix = *(this->camera->GetViewMatrix());
                D3DXMATRIX proj_matrix = *(this->camera->GetProjMatrix());
                DirectX::XMMATRIX view_xform((float *) view_matrix.m);
                DirectX::XMMATRIX proj_xform((float *) proj_matrix.m);
                D3D11_MAPPED_SUBRESOURCE mapped_resource;
                ctx->Map(this->camera_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
                CBCamera * camera_buffer = (CBCamera *)mapped_resource.pData;

                float z_near = this->camera->GetNearClip();
                float z_far = this->camera->GetFarClip();
                float z_range = z_far - z_near;

                camera_buffer->world_view_proj_xform = view_xform * proj_xform;
                camera_buffer->eye_pos = *((DirectX::XMFLOAT3 *) this->camera->GetEyePt());
                camera_buffer->z_near = z_near;
                camera_buffer->z_far = z_far;
                camera_buffer->dof_near = z_near+z_range*max(0.f, g_focal_distance-g_focal_range/3.0f);
                camera_buffer->dof_far = z_near+z_range*min(1.f, g_focal_distance+2.f*g_focal_range/3.0f);
                camera_buffer->aperture = g_focal_aperture * g_screen_area / 100.f;

                ctx->Unmap(this->camera_cb, 0);
            }
            cbs[0] = this->camera_cb;

            {
                D3D11_MAPPED_SUBRESOURCE mapped_resource;
                ctx->Map(this->object_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
                CBSceneObject * object_buffer = (CBSceneObject *)mapped_resource.pData;
                object_buffer->model_xform = DirectX::XMMatrixIdentity();
                ctx->Unmap(this->object_cb, 0);
            }
            cbs[1] = this->object_cb;

            ctx->VSSetConstantBuffers(0, 2, cbs);
            ctx->VSSetShader(this->vs_scene, nullptr, 0);
            ctx->IASetInputLayout(this->scene_layout);
            ctx->RSSetState(this->rs_state);
            ctx->PSSetShader(this->ps_scene, nullptr, 0);
            ctx->PSSetSamplers(0, 1, &this->samp_linear_wrap);
            ctx->OMSetDepthStencilState(this->ds_state, 0xFF);
            ctx->OMSetBlendState(this->blend_state, nullptr, 0xFFFFFFFF);

            for (auto object = this->scene.begin(); object != this->scene.end(); ++object)
            {
                (*object)->render(ctx);
            }
            ctx->OMSetRenderTargets(1, &pRTV, nullptr);
        }
        
        {    
            PERF_EVENT_SCOPED(ctx, "DoF");
            SAT::GenerateSAT(ctx, this->scene_srv);

            PERF_EVENT_BEGIN(ctx, "DoF > Post Process");
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            UINT quad_strides = sizeof(DirectX::XMFLOAT2);
            UINT quad_offsets = 0;
            ctx->IASetVertexBuffers(0, 1, &this->quad_verts, &quad_strides, &quad_offsets);
            ctx->VSSetShader(this->vs_quad, nullptr, 0);
            ctx->IASetInputLayout(this->quad_layout);
            if (g_view_mode == VIEW_MODE_BLUR_SIZE)
            {
                ctx->PSSetShader(this->ps_post_blursize, nullptr, 0);
            }
            else
            {
                ctx->PSSetShader(this->ps_post, nullptr, 0);
            }

            ID3D11ShaderResourceView * srvs[16] = {nullptr};
            srvs[0] = this->scene_srv;
            srvs[1] = SAT::GetSAT();
            srvs[2] = this->scene_depth_srv;
            ctx->PSSetShaderResources(0, 3, srvs);
            ID3D11SamplerState * samplers[16] = {nullptr};
            samplers[0] = this->samp_point_clamp;
            samplers[1] = this->samp_linear_clamp;
            ctx->PSSetConstantBuffers(0, 1, &this->camera_cb);
            ctx->PSSetSamplers(0, 2, samplers);
            ctx->OMSetDepthStencilState(this->ds_state_disabled, 0xFF);
            ctx->OMSetBlendState(this->blend_state, nullptr, 0xFFFFFFFF);
            ctx->Draw(6, 0);
            PERF_EVENT_END(ctx);
        }
        PERF_FRAME_END(ctx);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// UI Controller
////////////////////////////////////////////////////////////////////////////////////////////////////

class UIController : public IVisualController
{
private:
    CFirstPersonCamera * camera;
    TwBar * settings_bar;
    float ui_update_time;

public:
    UIController(CFirstPersonCamera * cam)
    {
        this->camera = cam;
    }

    virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
    {
        (void) hWnd;
        (void) uMsg;
        (void) wParam;
        (void) lParam;

        if(TwEventWin(hWnd, uMsg, wParam, lParam))
        {
            return 0; // Event has been handled by AntTweakBar
        }

        this->camera->HandleMessages( hWnd, uMsg, wParam, lParam );

        if(uMsg == WM_KEYDOWN)
        {        
            int iKeyPressed = static_cast<int>(wParam);
            switch( iKeyPressed )
            {
            case VK_TAB:
                g_bRenderHUD = !g_bRenderHUD;
                return 0;
                break;

            case VK_F1:
                PerfTracker::ui_toggle_visibility();
                break;

            default:
                break;
            }
        }
        return 1; 
    }

    virtual void Animate(double fElapsedTimeSeconds)
    {
        this->camera->FrameMove( (float)fElapsedTimeSeconds );

        this->ui_update_time -= (float) fElapsedTimeSeconds;
        if (this->ui_update_time <= 0)
        {
            this->ui_update_time = 5.0f;
            std::vector<PerfTracker::FrameMeasurements> perf_measurements;
            PerfTracker::get_results(perf_measurements);
            PerfTracker::ui_update(perf_measurements);
        }
    }

    virtual void Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView*, ID3D11DepthStencilView*) 
    { 
        if (g_bRenderHUD)
        {
            TwBeginText(2, 0, 0, 0);
            char msg[256];
            double averageTime = g_device_manager->GetAverageFrameTime();
            double fps = (averageTime > 0) ? 1.0 / averageTime : 0.0;
            sprintf_s(msg, "%.1f FPS", fps);
            TwAddTextLine(msg, 0xFF9BD839, 0xFF000000);
            TwEndText();

            TwDraw();
        }
    }

    virtual HRESULT DeviceCreated(ID3D11Device* pDevice) 
    { 
        TwInit(TW_DIRECT3D11, pDevice);        
        TwDefine("GLOBAL fontstyle=fixed contained=true");
        InitDialogs();

        static bool s_bFirstTime = true;
        if (s_bFirstTime)
        {
            s_bFirstTime = false;
            D3DXVECTOR3 eyePt = D3DXVECTOR3(1200.0f, -350.0f, 0.0f);
            D3DXVECTOR3 lookAtPt = D3DXVECTOR3(0.0f, -500.0f, 0.0f);
            this->camera->SetViewParams(&eyePt, &lookAtPt);
            this->camera->SetScalers(0.005f, 500.0f);
            this->camera->SetRotateButtons(true, false, true, false);
        }

        return S_OK;
    }

    virtual void DeviceDestroyed() 
    { 
        TwTerminate();
    }

    virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc) 
    {
        (void)pDevice;
        TwWindowSize(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

        // Setup the camera's projection parameters    
        float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
        this->camera->SetProjParams( D3DX_PI / 4, fAspectRatio, CAMERA_CLIP_NEAR, CAMERA_CLIP_FAR );

        if (settings_bar) {
        }
    }

    void InitDialogs()
    {
        settings_bar = TwNewBar("Settings");
        TwDefine("Settings color='72 115 1' alpha=32 text=light valueswidth=100 iconified=true resizable=false");
        int barSize[2] = {400, 200};
        TwSetParam(settings_bar, nullptr, "size", TW_PARAM_INT32, 2, barSize);

        TwAddVarRW(settings_bar, "Focus Distance", TW_TYPE_FLOAT, &g_focal_distance, "group='Depth of Field' min=0 max=1 step=0.0025 keydecr=o keyincr=p");
        TwAddVarRW(settings_bar, "Focus Range", TW_TYPE_FLOAT, &g_focal_range, "group='Depth of Field' min=0.0 max=0.5 step=0.0025 keydecr=k keyincr=l");
        TwAddVarRW(settings_bar, "Aperture", TW_TYPE_FLOAT, &g_focal_aperture, "group='Depth of Field' min=0.01 max=1.00 step=0.01 keydecr=n keyincr=m");

        {
            TwEnumVal enumModeTypeEV[] = {
                { VIEW_MODE_DOF, "SAT Blur" },
                { VIEW_MODE_BLUR_SIZE, "Blur Size" }
            };
            TwType enumModeType = TwDefineEnum("RenderMode", enumModeTypeEV, sizeof(enumModeTypeEV) / sizeof(enumModeTypeEV[0]));
            TwAddVarRW(settings_bar, "View Mode", enumModeType, &g_view_mode, "keyIncr=v keyDecr=V");
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
////////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

    AllocConsole();
    freopen("CONIN$", "r",stdin);
    freopen("CONOUT$", "w",stdout);
    freopen("CONOUT$", "w",stderr);
#endif

    g_device_manager = new DeviceManager();

    CFirstPersonCamera eye_camera;
    auto scene_controller = SceneController(&eye_camera);
    g_device_manager->AddControllerToFront(&scene_controller);
    
    auto ui_controller = UIController(&eye_camera);
    g_device_manager->AddControllerToFront(&ui_controller);
    
    DeviceCreationParameters deviceParams;
    deviceParams.swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    deviceParams.swapChainSampleCount = 1;
    deviceParams.startFullscreen = false;
    deviceParams.backBufferWidth = 1280;
    deviceParams.backBufferHeight = 800;

    if(FAILED(g_device_manager->CreateWindowDeviceAndSwapChain(deviceParams, L"NVIDIA Graphics Library : Compute-Based Image Processing")))
    {
        MessageBox(nullptr, L"Cannot initialize the D3D11 device with the requested parameters", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    PerfTracker::initialize();
    PerfTracker::EventDesc perf_events[] = {
        PERF_EVENT_DESC("Render Scene"),
        PERF_EVENT_DESC("DoF"),
        PERF_EVENT_DESC("DoF > SAT"),
        PERF_EVENT_DESC("DoF > SAT > Translate"),
        PERF_EVENT_DESC("DoF > SAT > Sum"),
        PERF_EVENT_DESC("DoF > SAT > Offset"),
        PERF_EVENT_DESC("DoF > Post Process")
    };
    PerfTracker::ui_setup(perf_events, sizeof(perf_events)/sizeof(PerfTracker::EventDesc), nullptr);

    g_device_manager->MessageLoop();
    g_device_manager->Shutdown();

    PerfTracker::shutdown();
    delete g_device_manager;

    return 0;
}
