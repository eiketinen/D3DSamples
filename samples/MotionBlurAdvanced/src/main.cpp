//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\src/main.cpp
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
#include "PerfTracker.h"

#include "AntTweakBar.h"
#include "Camera.h"
#include "DeviceManager.h"

#include <d3dcommon.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>
#include <ctime>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////////////////////////

DeviceManager* g_device_manager;

const float CAMERA_CLIP_NEAR =   1.0f;
const float CAMERA_CLIP_FAR = 100.00f;

bool g_bRenderHUD = true;

typedef enum eViewMode {
	VIEW_MODE_COLOR_ONLY = 0,
	VIEW_MODE_DEPTH_ONLY,
	VIEW_MODE_VELOCITY,
	VIEW_MODE_VELOCITY_TILE_MAX,
	VIEW_MODE_VELOCITY_NEIGHBOR_MAX,
	VIEW_MODE_FINAL
};

eViewMode g_view_mode = VIEW_MODE_FINAL;

// Global speed of the fan blades
float g_sailSpeed       = 350.0f;
int   g_sailSpeedPaused = false;

// Globals to define the mation blur reconstruction parameters
float        g_Exposure             = 1.0f;
unsigned int g_K                    =  2;
unsigned int g_S                    = 15;
unsigned int g_MaxSampleTapDistance =  6;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene Controller
////////////////////////////////////////////////////////////////////////////////////////////////////

class SceneController : public IVisualController
{
private:
	struct CBCamera 
	{
		D3DXMATRIX	projection_xform;
		D3DXMATRIX	world_xform_new;
		D3DXMATRIX	world_xform_old;
		D3DXMATRIX	view_xform_new;
		D3DXMATRIX	view_xform_old;
		D3DXVECTOR3	eye_pos;
		FLOAT		half_exposure;
		FLOAT		half_exposure_x_framerate;
		FLOAT		K;
		FLOAT		S;
		FLOAT		max_sample_tap_distance;
	};
	struct CBSceneObject
	{
		D3DXMATRIX	model_xform_new;
		D3DXMATRIX	model_xform_old;
		D3DXMATRIX	model_xform_normal_new;
	};

	CModelViewerCamera*			camera;
	D3DXMATRIX					camera_world_xform_new;
	D3DXMATRIX					camera_view_xform_new;

	ID3D11Buffer*				camera_cb;
	ID3D11Buffer*				model_house_cb;
	ID3D11Buffer*				model_blades_cb;
	FLOAT						model_blades_angle_new;
	FLOAT						model_blades_angle_old;
	D3DXMATRIX					model_blades_xform_new;
	D3DXMATRIX					model_blades_xform_old;
	D3DXMATRIX					model_blades_xform_normal_new;

	ID3D11InputLayout*			scene_layout;
	ID3D11VertexShader*			scene_vs;
	ID3D11PixelShader*			scene_ps;

	ID3D11Texture2D*			scene_tex;
	ID3D11RenderTargetView*		scene_rtv;
	ID3D11ShaderResourceView*	scene_srv;

	ID3D11Texture2D*			scene_depth_tex;
	ID3D11DepthStencilView*		scene_depth_dsv;
	ID3D11ShaderResourceView*	scene_depth_srv;

	ID3D11Texture2D*			velocity_tex;
	ID3D11RenderTargetView*		velocity_rtv;
	ID3D11ShaderResourceView*	velocity_srv;

	ID3D11Texture2D*			velocity_tile_max_tex;
	ID3D11RenderTargetView*		velocity_tile_max_rtv;
	ID3D11ShaderResourceView*	velocity_tile_max_srv;
	ID3D11PixelShader*			velocity_tile_max_ps;

	ID3D11Texture2D*			velocity_neighbor_max_tex;
	ID3D11RenderTargetView*		velocity_neighbor_max_rtv;
	ID3D11ShaderResourceView*	velocity_neighbor_max_srv;
	ID3D11PixelShader*			velocity_neighbor_max_ps;

	ID3D11Buffer*				quad_verts;
	ID3D11InputLayout*			quad_layout;
	ID3D11VertexShader*			quad_vs;
	ID3D11PixelShader*			quad_ps;
	
	ID3D11PixelShader*			depth_ps;
	ID3D11PixelShader*			gather_ps;

	ID3D11RasterizerState*		rs_state;

	ID3D11SamplerState*			samp_point_wrap;
	ID3D11SamplerState*			samp_point_clamp;
	ID3D11SamplerState*			samp_linear_clamp;

	ID3D11DepthStencilState*	ds_state;
	ID3D11DepthStencilState*	ds_state_disabled;
	
	ID3D11BlendState*			blend_state;
	ID3D11BlendState*			blend_state_disabled;

	ID3D11Texture2D*			random_tex;
	ID3D11ShaderResourceView*	random_srv;

	ID3D11ShaderResourceView*	background_srv;

	Scene::RenderList scene;

	DXGI_SURFACE_DESC			surface_desc;
	double						last_delta_time;
	unsigned int				last_K;

public:
	SceneController(CModelViewerCamera* cam)
	{
		this->camera = cam;

		this->scene_tex = nullptr;
		this->scene_rtv = nullptr;
		this->scene_srv = nullptr;
		this->scene_depth_tex = nullptr;
		this->scene_depth_dsv = nullptr;
		this->scene_depth_srv = nullptr;
		this->velocity_tex = nullptr;
		this->velocity_rtv = nullptr;
		this->velocity_srv = nullptr;
		this->velocity_tile_max_tex = nullptr;
		this->velocity_tile_max_rtv = nullptr;
		this->velocity_tile_max_srv = nullptr;
		this->velocity_neighbor_max_tex = nullptr;
		this->velocity_neighbor_max_rtv = nullptr;
		this->velocity_neighbor_max_srv = nullptr;
		this->random_tex = nullptr;
		this->random_srv = nullptr;
		this->background_srv = nullptr;

		model_blades_angle_new = model_blades_angle_old = 0.0f;
		last_delta_time = 30.0f;
		last_K = g_K;
	}

	HRESULT CompileShaderFromFile(LPCSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
	{
		HRESULT hr = S_OK;
		ID3DBlob* pErrorBlob;
		hr = D3DX11CompileFromFileA(szFileName, NULL, NULL, szEntryPoint, szShaderModel, D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
		if(FAILED(hr))
		{
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			SAFE_RELEASE(pErrorBlob);
			return hr;
		}
		SAFE_RELEASE(pErrorBlob);

		return S_OK;
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

			device->CreateBuffer(&buffer_desc, nullptr, &this->model_house_cb);
			device->CreateBuffer(&buffer_desc, nullptr, &this->model_blades_cb);
		}
		{
			D3DXVECTOR2 verts[6];
			verts[0] = D3DXVECTOR2(-1.f, -1.f);
			verts[1] = D3DXVECTOR2(-1.f,  1.f);
			verts[2] = D3DXVECTOR2( 1.f,  1.f);
			verts[3] = D3DXVECTOR2(-1.f, -1.f);
			verts[4] = D3DXVECTOR2( 1.f,  1.f);
			verts[5] = D3DXVECTOR2( 1.f, -1.f);
			D3D11_BUFFER_DESC desc = { 6 * sizeof(D3DXVECTOR2), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0 };
			D3D11_SUBRESOURCE_DATA data = { (void *)verts, sizeof(D3DXVECTOR2), 6 * sizeof(D3DXVECTOR2) };
			hr = device->CreateBuffer(&desc, &data, &this->quad_verts);
			_ASSERT(!FAILED(hr));
		}

		Scene::load_model(device, "..\\..\\motionbluradvanced\\assets\\models\\house.obj", this->scene);
		Scene::load_model(device, "..\\..\\motionbluradvanced\\assets\\models\\fan.obj", this->scene);

		ID3DBlob* pShaderBuffer = NULL;
		void* ShaderBufferData;
		UINT  ShaderBufferSize;
		{
			CompileShaderFromFile("..\\..\\motionbluradvanced\\assets\\shaders\\vs_scene.hlsl", "main", "vs_5_0", &pShaderBuffer);
			ShaderBufferData = pShaderBuffer->GetBufferPointer();
			ShaderBufferSize = pShaderBuffer->GetBufferSize();
			device->CreateVertexShader(ShaderBufferData, ShaderBufferSize, nullptr, &this->scene_vs);

			D3D11_INPUT_ELEMENT_DESC scene_layout_desc[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			device->CreateInputLayout(scene_layout_desc, sizeof(scene_layout_desc)/sizeof(D3D11_INPUT_ELEMENT_DESC), ShaderBufferData, ShaderBufferSize, &this->scene_layout);

			CompileShaderFromFile("..\\..\\motionbluradvanced\\assets\\shaders\\ps_scene.hlsl", "main", "ps_5_0", &pShaderBuffer);
			ShaderBufferData = pShaderBuffer->GetBufferPointer();
			ShaderBufferSize = pShaderBuffer->GetBufferSize();
			device->CreatePixelShader(ShaderBufferData, ShaderBufferSize, nullptr, &this->scene_ps);
		}
		{
			CompileShaderFromFile("..\\..\\motionbluradvanced\\assets\\shaders\\vs_quad.hlsl", "main", "vs_5_0", &pShaderBuffer);
			ShaderBufferData = pShaderBuffer->GetBufferPointer();
			ShaderBufferSize = pShaderBuffer->GetBufferSize();
			device->CreateVertexShader(ShaderBufferData, ShaderBufferSize, nullptr, &this->quad_vs);

			D3D11_INPUT_ELEMENT_DESC quad_layout_desc[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			hr = device->CreateInputLayout(quad_layout_desc, sizeof(quad_layout_desc)/sizeof(D3D11_INPUT_ELEMENT_DESC), ShaderBufferData, ShaderBufferSize, &this->quad_layout);

			CompileShaderFromFile("..\\..\\motionbluradvanced\\assets\\shaders\\ps_quad.hlsl", "main", "ps_5_0", &pShaderBuffer);
			ShaderBufferData = pShaderBuffer->GetBufferPointer();
			ShaderBufferSize = pShaderBuffer->GetBufferSize();
			device->CreatePixelShader(ShaderBufferData, ShaderBufferSize, nullptr, &this->quad_ps);

			CompileShaderFromFile("..\\..\\motionbluradvanced\\assets\\shaders\\ps_depth.hlsl", "main", "ps_5_0", &pShaderBuffer);
			ShaderBufferData = pShaderBuffer->GetBufferPointer();
			ShaderBufferSize = pShaderBuffer->GetBufferSize();
			device->CreatePixelShader(ShaderBufferData, ShaderBufferSize, nullptr, &this->depth_ps);

			CompileShaderFromFile("..\\..\\motionbluradvanced\\assets\\shaders\\ps_tilemax.hlsl", "main", "ps_5_0", &pShaderBuffer);
			ShaderBufferData = pShaderBuffer->GetBufferPointer();
			ShaderBufferSize = pShaderBuffer->GetBufferSize();
			device->CreatePixelShader(ShaderBufferData, ShaderBufferSize, nullptr, &this->velocity_tile_max_ps);

			CompileShaderFromFile("..\\..\\motionbluradvanced\\assets\\shaders\\ps_neighbormax.hlsl", "main", "ps_5_0", &pShaderBuffer);
			ShaderBufferData = pShaderBuffer->GetBufferPointer();
			ShaderBufferSize = pShaderBuffer->GetBufferSize();
			device->CreatePixelShader(ShaderBufferData, ShaderBufferSize, nullptr, &this->velocity_neighbor_max_ps);

			CompileShaderFromFile("..\\..\\motionbluradvanced\\assets\\shaders\\ps_gather.hlsl", "main", "ps_5_0", &pShaderBuffer);
			ShaderBufferData = pShaderBuffer->GetBufferPointer();
			ShaderBufferSize = pShaderBuffer->GetBufferSize();
			device->CreatePixelShader(ShaderBufferData, ShaderBufferSize, nullptr, &this->gather_ps);
		}
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
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.MipLODBias = 0.0f;
			desc.MaxAnisotropy = 1;
			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f; desc.BorderColor[0] = 0.f;
			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			device->CreateSamplerState(&desc, &this->samp_point_wrap);
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
			device->CreateBlendState(&desc, &this->blend_state);
		}
		{
			D3D11_BLEND_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.RenderTarget[0].BlendEnable = FALSE;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			device->CreateBlendState(&desc, &this->blend_state_disabled);
		}
		{
			// Load the background texture
			Scene::load_texture(device, L"..\\..\\motionbluradvanced\\assets\\models\\sky_cube.dds", &this->background_srv);
		}

		return S_OK;
	}

	virtual void DeviceDestroyed()
	{
		SAFE_RELEASE(this->camera_cb);
		SAFE_RELEASE(this->model_house_cb);
		SAFE_RELEASE(this->model_blades_cb);
		SAFE_RELEASE(this->scene_layout);
		SAFE_RELEASE(this->scene_vs);
		SAFE_RELEASE(this->scene_ps);
		SAFE_RELEASE(this->scene_tex);
		SAFE_RELEASE(this->scene_rtv);
		SAFE_RELEASE(this->scene_srv);
		SAFE_RELEASE(this->scene_depth_tex);
		SAFE_RELEASE(this->scene_depth_dsv);
		SAFE_RELEASE(this->scene_depth_srv);
		SAFE_RELEASE(this->velocity_tex);
		SAFE_RELEASE(this->velocity_rtv);
		SAFE_RELEASE(this->velocity_srv);
		SAFE_RELEASE(this->velocity_tile_max_tex);
		SAFE_RELEASE(this->velocity_tile_max_rtv);
		SAFE_RELEASE(this->velocity_tile_max_srv);
		SAFE_RELEASE(this->velocity_tile_max_ps);
		SAFE_RELEASE(this->velocity_neighbor_max_tex);
		SAFE_RELEASE(this->velocity_neighbor_max_rtv);
		SAFE_RELEASE(this->velocity_neighbor_max_srv);
		SAFE_RELEASE(this->velocity_neighbor_max_ps);
		SAFE_RELEASE(this->quad_verts);
		SAFE_RELEASE(this->quad_layout);
		SAFE_RELEASE(this->quad_vs);
		SAFE_RELEASE(this->quad_ps);
		SAFE_RELEASE(this->depth_ps);
		SAFE_RELEASE(this->gather_ps);
		SAFE_RELEASE(this->rs_state);
		SAFE_RELEASE(this->samp_point_wrap);
		SAFE_RELEASE(this->samp_point_clamp);
		SAFE_RELEASE(this->samp_linear_clamp);
		SAFE_RELEASE(this->ds_state);
		SAFE_RELEASE(this->ds_state_disabled);
		SAFE_RELEASE(this->blend_state);
		SAFE_RELEASE(this->blend_state_disabled);
		SAFE_RELEASE(this->random_tex);
		SAFE_RELEASE(this->random_srv);
		SAFE_RELEASE(this->background_srv);

		for (auto object = this->scene.begin(); object != this->scene.end(); ++object)
		{
			delete (*object);
		}
		this->scene.clear();
	}

	void CreateTextureWithViews(
		ID3D11Device* device,
		const unsigned int surface_width,
		const unsigned int surface_height,
		const DXGI_FORMAT tex_format,
		const DXGI_FORMAT rtv_format,
		const DXGI_FORMAT srv_format,
		ID3D11Texture2D** target_tex,
		ID3D11RenderTargetView** target_rtv,
		ID3D11DepthStencilView** target_dsv,
		ID3D11ShaderResourceView** target_srv)
	{
		HRESULT hr = S_OK;
		
		D3D11_TEXTURE2D_DESC tex_desc;
		ZeroMemory(&tex_desc, sizeof(tex_desc));
		tex_desc.Width = surface_width;
		tex_desc.Height = surface_height;
		tex_desc.MipLevels = 1;
		tex_desc.ArraySize = 1;
		tex_desc.Format = tex_format;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.Usage = D3D11_USAGE_DEFAULT;
		tex_desc.BindFlags = (target_rtv ? D3D11_BIND_RENDER_TARGET : D3D11_BIND_DEPTH_STENCIL) | D3D11_BIND_SHADER_RESOURCE;
		hr = device->CreateTexture2D(&tex_desc, nullptr, target_tex);
		_ASSERT(!FAILED(hr));

		if (target_rtv)
		{
			D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
			ZeroMemory(&rtv_desc, sizeof(rtv_desc));
			rtv_desc.Format = rtv_format;
			rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtv_desc.Texture2D.MipSlice = 0;
			hr = device->CreateRenderTargetView(*target_tex, &rtv_desc, target_rtv);
			_ASSERT(!FAILED(hr));
		}
		else
		{
			_ASSERT(target_dsv);
			D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
			ZeroMemory(&dsv_desc, sizeof(dsv_desc));
			dsv_desc.Format = rtv_format;
			dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsv_desc.Texture2D.MipSlice = 0;
			hr = device->CreateDepthStencilView(*target_tex, &dsv_desc, target_dsv);
			_ASSERT(!FAILED(hr));
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(srv_desc));
		srv_desc.Format = srv_format;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Texture2D.MostDetailedMip = 0;
		hr = device->CreateShaderResourceView(*target_tex, &srv_desc, target_srv);
		_ASSERT(!FAILED(hr));
	}

	// Computes the appropriate max sample tap distance (our implementation needs to work both on small and
	// large resolutions). Based on subjective observations,for 10.1" screens (1920x1136 surface) we need 8
	// texels, and in desktop displays (720p by default) we need 6.
	void ComputeMaxSampleTapDistance(unsigned int width, unsigned int height)
	{
        (void)width;
		g_MaxSampleTapDistance = (2 * height + 1056) / 416;
	}

	void ComputeTiledDimensions(unsigned int width, unsigned int height, unsigned int& widthDividedByK, unsigned int& heightDividedByK) const
	{
		unsigned int k = (g_K > 0) ? g_K : 1;
		widthDividedByK  = width  / k;
		heightDividedByK = height / k;
	}

	virtual void BackBufferResized(ID3D11Device* device, const DXGI_SURFACE_DESC* surface_desc)
	{
		SAFE_RELEASE(this->scene_tex);
		SAFE_RELEASE(this->scene_rtv);
		SAFE_RELEASE(this->scene_srv);
		SAFE_RELEASE(this->scene_depth_tex);
		SAFE_RELEASE(this->scene_depth_dsv);
		SAFE_RELEASE(this->scene_depth_srv);
		SAFE_RELEASE(this->velocity_tex);
		SAFE_RELEASE(this->velocity_rtv);
		SAFE_RELEASE(this->velocity_srv);
		SAFE_RELEASE(this->velocity_tile_max_tex);
		SAFE_RELEASE(this->velocity_tile_max_rtv);
		SAFE_RELEASE(this->velocity_tile_max_srv);
		SAFE_RELEASE(this->velocity_neighbor_max_tex);
		SAFE_RELEASE(this->velocity_neighbor_max_rtv);
		SAFE_RELEASE(this->velocity_neighbor_max_srv);
		SAFE_RELEASE(this->random_tex);
		SAFE_RELEASE(this->random_srv);

		this->surface_desc = *surface_desc;

		// C, Z, V are (width, height); TileMax and NeighborMax are (width/K, height/K)
		unsigned int widthDividedByK, heightDividedByK;
		ComputeTiledDimensions(surface_desc->Width, surface_desc->Height, widthDividedByK, heightDividedByK);
		ComputeMaxSampleTapDistance(surface_desc->Width, surface_desc->Height);

		// Texture to represent a pseudo-random number generator (used in gather pass)
		{
			srand( static_cast<unsigned int>( time(NULL) ) );
			unsigned char* rand_data = new unsigned char[widthDividedByK * heightDividedByK];
			for (unsigned int j = 0; j < heightDividedByK; j++)
			for (unsigned int i = 0; i < widthDividedByK; i++)
			{
				unsigned char value = static_cast<unsigned char>(rand()) & static_cast<unsigned char>(0x00ff);
				rand_data[j * widthDividedByK + i] = value;
			}

			D3D11_TEXTURE2D_DESC tex_desc;
			tex_desc.Width = widthDividedByK;
			tex_desc.Height = heightDividedByK;
			tex_desc.MipLevels = 1;
			tex_desc.ArraySize = 1;
			tex_desc.Format = DXGI_FORMAT_R8_UNORM;
			tex_desc.SampleDesc.Count = 1;
			tex_desc.SampleDesc.Quality = 0;
			tex_desc.Usage = D3D11_USAGE_DEFAULT;
			tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			tex_desc.CPUAccessFlags = 0;
			tex_desc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA tex_data;
			tex_data.pSysMem = rand_data;
			tex_data.SysMemPitch = widthDividedByK;
			tex_data.SysMemSlicePitch = widthDividedByK * heightDividedByK;

			device->CreateTexture2D(&tex_desc, &tex_data, &this->random_tex);
			device->CreateShaderResourceView(this->random_tex, NULL, &this->random_srv);

			delete[] rand_data;
		}

		// C
		CreateTextureWithViews(
			device, surface_desc->Width, surface_desc->Height,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			&this->scene_tex,
			&this->scene_rtv, nullptr,
			&this->scene_srv
		);
		// Z
		CreateTextureWithViews(
			device, surface_desc->Width, surface_desc->Height,
			DXGI_FORMAT_R24G8_TYPELESS,
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
			&this->scene_depth_tex,
			nullptr, &this->scene_depth_dsv,
			&this->scene_depth_srv
		);
		// V
		CreateTextureWithViews(
			device, surface_desc->Width, surface_desc->Height,
			DXGI_FORMAT_R8G8_UNORM,
			DXGI_FORMAT_R8G8_UNORM,
			DXGI_FORMAT_R8G8_UNORM,
			&this->velocity_tex,
			&this->velocity_rtv, nullptr,
			&this->velocity_srv
		);
		// TileMax
		CreateTextureWithViews(
			device, widthDividedByK, heightDividedByK,
			DXGI_FORMAT_R8G8_UNORM,
			DXGI_FORMAT_R8G8_UNORM,
			DXGI_FORMAT_R8G8_UNORM,
			&this->velocity_tile_max_tex,
			&this->velocity_tile_max_rtv, nullptr,
			&this->velocity_tile_max_srv
		);
		// NeighborMax
		CreateTextureWithViews(
			device, widthDividedByK, heightDividedByK,
			DXGI_FORMAT_R8G8_UNORM,
			DXGI_FORMAT_R8G8_UNORM,
			DXGI_FORMAT_R8G8_UNORM,
			&this->velocity_neighbor_max_tex,
			&this->velocity_neighbor_max_rtv, nullptr,
			&this->velocity_neighbor_max_srv
		);

	}

	virtual void Animate(double fElapsedTimeSeconds)
	{
		// If the animation is pasued, we still want to show motion blur on the windmill sails
		if (!g_sailSpeedPaused)
		{
			this->last_delta_time = fElapsedTimeSeconds;
			this->model_blades_angle_old = this->model_blades_angle_new;
			this->model_blades_angle_new = model_blades_angle_old + g_sailSpeed * (float)fElapsedTimeSeconds;
		}
	}

	virtual void Render(ID3D11Device* device, ID3D11DeviceContext* ctx, ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV)
	{
		// We need to resize buffers if K has changed
		if (this->last_K != g_K)
		{
			BackBufferResized(device, &this->surface_desc);
			this->last_K = g_K;
		}

		D3D11_VIEWPORT viewportFull;
		viewportFull.TopLeftX = 0.0f;
		viewportFull.TopLeftY = 0.0f;
		viewportFull.Width    = (float)this->surface_desc.Width;
		viewportFull.Height   = (float)this->surface_desc.Height;
		viewportFull.MinDepth = 0.0f;
		viewportFull.MaxDepth = 1.0f;

		unsigned int widthDividedByK, heightDividedByK;
		ComputeTiledDimensions(this->surface_desc.Width, this->surface_desc.Height, widthDividedByK, heightDividedByK);
		D3D11_VIEWPORT viewportScaled;
		viewportScaled.TopLeftX = 0.0f;
		viewportScaled.TopLeftY = 0.0f;
		viewportScaled.Width    = (float)widthDividedByK;
		viewportScaled.Height   = (float)heightDividedByK;
		viewportScaled.MinDepth = 0.0f;
		viewportScaled.MaxDepth = 1.0f;

		UINT quad_strides = sizeof(D3DXVECTOR2);
		UINT quad_offsets = 0;

		PERF_FRAME_BEGIN(ctx);
		{
			// We'll always render the primary scene, along with all velocity buffers
			{
				PERF_EVENT_SCOPED(ctx, "Render Scene");

				PERF_EVENT_BEGIN(ctx, "Render > Main");

				float clear_color_scene[4]    = { 1.00f, 1.00f, 1.00f, 0.0f };
				float clear_color_velcoity[4] = { 0.50f, 0.50f, 0.50f, 0.0f };
				ctx->ClearRenderTargetView(pRTV, clear_color_scene);
				ctx->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
				ctx->ClearRenderTargetView(this->scene_rtv, clear_color_scene);
				ctx->ClearDepthStencilView(this->scene_depth_dsv, D3D11_CLEAR_DEPTH, 1.0, 0);
				ctx->ClearRenderTargetView(this->velocity_rtv, clear_color_velcoity);
				ctx->ClearRenderTargetView(this->velocity_tile_max_rtv, clear_color_velcoity);
				ctx->ClearRenderTargetView(this->velocity_neighbor_max_rtv, clear_color_velcoity);

				ID3D11Buffer* cbs[3] = {nullptr};

				// Camera parameters
				D3DXMATRIX world_matrix = *(this->camera->GetWorldMatrix());
				D3DXMATRIX view_matrix  = *(this->camera->GetViewMatrix());
				D3DXMATRIX proj_matrix  = *(this->camera->GetProjMatrix());
				{
					D3D11_MAPPED_SUBRESOURCE mapped_resource;
					ctx->Map(this->camera_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
					CBCamera* camera_buffer = (CBCamera *)mapped_resource.pData;

					// Use the last frame's new value as this frame's old value
					camera_buffer->projection_xform = proj_matrix;
					camera_buffer->world_xform_old = this->camera_world_xform_new;
					camera_buffer->world_xform_new = this->camera_world_xform_new = world_matrix;
					camera_buffer->view_xform_old  = this->camera_view_xform_new;
					camera_buffer->view_xform_new  = this->camera_view_xform_new  = view_matrix;
					camera_buffer->eye_pos = *(this->camera->GetEyePt());
					camera_buffer->half_exposure             = 0.5f * g_Exposure;
					camera_buffer->half_exposure_x_framerate = 0.5f * g_Exposure / (float)this->last_delta_time;
					camera_buffer->K = (float)g_K;
					camera_buffer->S = (float)g_S;
					camera_buffer->max_sample_tap_distance = (float)g_MaxSampleTapDistance;

					ctx->Unmap(this->camera_cb, 0);
				}
				// House model parameters
				{
					D3D11_MAPPED_SUBRESOURCE mapped_resource;
					ctx->Map(this->model_house_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
					CBSceneObject* object_buffer = (CBSceneObject *)mapped_resource.pData;

					D3DXMatrixIdentity(&object_buffer->model_xform_new);
					D3DXMatrixIdentity(&object_buffer->model_xform_old);
					D3DXMatrixIdentity(&object_buffer->model_xform_normal_new);
				
					ctx->Unmap(this->model_house_cb, 0);
				}
				// Fan blade model parameters: Update the model parameters to animate the fan blades
				{
					D3D11_MAPPED_SUBRESOURCE mapped_resource;
					ctx->Map(this->model_blades_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
					CBSceneObject* object_buffer = (CBSceneObject *)mapped_resource.pData;

					if (g_sailSpeedPaused)
					{
						object_buffer->model_xform_old        = this->model_blades_xform_old;
						object_buffer->model_xform_new        = this->model_blades_xform_new;
						object_buffer->model_xform_normal_new = this->model_blades_xform_normal_new;
					}
					else
					{
						// Use the last frame's new value as this frame's old value
						object_buffer->model_xform_old = this->model_blades_xform_old = this->model_blades_xform_new;

						// Calculate the new transform value
						D3DXMATRIX TranslateToOrigin;
						D3DXMATRIX TranslateToWorld;
						D3DXMATRIX RotateFanBlades;
						D3DXMATRIX FinalTransform;
						D3DXMatrixTranslation(&TranslateToOrigin, 0.0f, -8.74925041f, -2.67939997f);
						D3DXMatrixTranslation(&TranslateToWorld,  0.0f,  8.74925041f,  2.67939997f);
						D3DXMatrixRotationZ(&RotateFanBlades, (this->model_blades_angle_new / 180.0f) * 3.14159f);
						D3DXMatrixMultiply(&FinalTransform, &TranslateToOrigin, D3DXMatrixMultiply(&FinalTransform, &RotateFanBlades, &TranslateToWorld));

						// Update our cache of the new transform and set it on the model constant data
						object_buffer->model_xform_new = this->model_blades_xform_new = FinalTransform;

						// Separate normal transform for correctness
						D3DXMATRIX Intermediate;
						D3DXMatrixTranspose(&FinalTransform, D3DXMatrixInverse(&Intermediate, nullptr, D3DXMatrixMultiply(&Intermediate, &FinalTransform, &view_matrix)));
						object_buffer->model_xform_normal_new = this->model_blades_xform_normal_new = FinalTransform;
					}
					ctx->Unmap(this->model_blades_cb, 0);
				}

				// Common sampler setup for all shaders
				ID3D11SamplerState* samplers[3];
				samplers[0] = samp_point_wrap;
				samplers[1] = samp_point_clamp;
				samplers[2] = samp_linear_clamp;
				ctx->PSSetSamplers(0, 3, samplers);

				// Render to C and V in one pass (though this would be easy to split)
				ID3D11RenderTargetView* scene_render_targets[2];
				scene_render_targets[0] = this->scene_rtv;
				scene_render_targets[1] = this->velocity_rtv;
				ctx->OMSetRenderTargets(2, scene_render_targets, this->scene_depth_dsv);
				ctx->OMSetBlendState(this->blend_state_disabled, nullptr, 0xFFFFFFFF);
				ctx->RSSetViewports(1, &viewportFull);
				ctx->RSSetState(this->rs_state);

				// Full screen quad to fill the background
				ctx->VSSetShader(this->quad_vs, nullptr, 0);
				ctx->IASetInputLayout(this->quad_layout);
				ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				ctx->IASetVertexBuffers(0, 1, &this->quad_verts, &quad_strides, &quad_offsets);
				ctx->OMSetDepthStencilState(this->ds_state_disabled, 0xFF);
				ctx->PSSetShader(this->quad_ps, nullptr, 0);
				ctx->PSSetShaderResources(0, 1, &this->background_srv);
				ctx->Draw(6, 0);

				// Draw the scene
				ctx->VSSetShader(this->scene_vs, nullptr, 0);
				ctx->IASetInputLayout(this->scene_layout);
				ctx->RSSetState(this->rs_state);
				ctx->PSSetShader(this->scene_ps, nullptr, 0);
				ctx->PSSetConstantBuffers(0, 1, &this->camera_cb);
				ctx->OMSetDepthStencilState(this->ds_state, 0xFF);

				// Render the base/building
				cbs[0] = this->camera_cb;
				cbs[1] = this->model_house_cb;
				ctx->VSSetConstantBuffers(0, 2, cbs);
				this->scene[0]->render(ctx);

				// Update the constant buffers and render the fan blades
				cbs[0] = this->camera_cb;
				cbs[1] = this->model_blades_cb;
				ctx->VSSetConstantBuffers(0, 2, cbs);
				this->scene[1]->render(ctx);

				PERF_EVENT_END(ctx);

				// All passes from this point on use full-screen quads with similar setups
				ctx->VSSetShader(this->quad_vs, nullptr, 0);
				ctx->IASetInputLayout(this->quad_layout);
				ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				ctx->IASetVertexBuffers(0, 1, &this->quad_verts, &quad_strides, &quad_offsets);
				ctx->RSSetState(this->rs_state);
				ctx->OMSetDepthStencilState(this->ds_state_disabled, 0xFF);
				ctx->OMSetBlendState(this->blend_state_disabled, nullptr, 0xFFFFFFFF);

				// Generate the TileMax buffer
				PERF_EVENT_BEGIN(ctx, "Render > TileMax");
				ctx->OMSetRenderTargets(1, &this->velocity_tile_max_rtv, nullptr);
				ctx->RSSetViewports(1, &viewportScaled);
				ctx->PSSetShader(this->velocity_tile_max_ps, nullptr, 0);
				ctx->PSSetShaderResources(0, 1, &this->velocity_srv);
				ctx->Draw(6, 0);
				PERF_EVENT_END(ctx);

				// Generate the NeighborMax buffer
				PERF_EVENT_BEGIN(ctx, "Render > NeighborMax");
				ctx->OMSetRenderTargets(1, &this->velocity_neighbor_max_rtv, nullptr);
				ctx->RSSetViewports(1, &viewportScaled);
				ctx->PSSetShader(this->velocity_neighbor_max_ps, nullptr, 0);
				ctx->PSSetShaderResources(0, 1, &this->velocity_tile_max_srv);
				ctx->Draw(6, 0);
				PERF_EVENT_END(ctx);
			}

			// The final pass
			{
				PERF_EVENT_SCOPED(ctx, "Final Pass");

				ctx->OMSetRenderTargets(1, &pRTV, pDSV);
				ctx->RSSetViewports(1, &viewportFull);
				
				// If requested, perform the final gather and display the results
				if (g_view_mode == VIEW_MODE_FINAL)
				{
					PERF_EVENT_BEGIN(ctx, "Final > Gather");

					ctx->PSSetShader(this->gather_ps, nullptr, 0);

					ID3D11ShaderResourceView* texture_views[5];
					texture_views[0] = this->scene_srv;
					texture_views[1] = this->scene_depth_srv;
					texture_views[2] = this->velocity_srv;
					texture_views[3] = this->velocity_neighbor_max_srv;
					texture_views[4] = this->random_srv;
					ctx->PSSetShaderResources(0, 5, texture_views);
				}
				// Otherwise, display the requested intermediate buffer
				else
				{
					PERF_EVENT_BEGIN(ctx, "Final > Display");
					// Depending on the selected view mode, bind different views
					switch (g_view_mode)
					{
					default:
					case VIEW_MODE_COLOR_ONLY:
						ctx->PSSetShader(this->quad_ps, nullptr, 0);
						ctx->PSSetShaderResources(0, 1, &this->scene_srv);
						break;
					case VIEW_MODE_DEPTH_ONLY:
						ctx->PSSetShader(this->depth_ps, nullptr, 0);
						ctx->PSSetShaderResources(0, 1, &this->scene_depth_srv);
						break;
					case VIEW_MODE_VELOCITY:
						ctx->PSSetShader(this->quad_ps, nullptr, 0);
						ctx->PSSetShaderResources(0, 1, &this->velocity_srv);
						break;
					case VIEW_MODE_VELOCITY_TILE_MAX:
						ctx->PSSetShader(this->quad_ps, nullptr, 0);
						ctx->PSSetShaderResources(0, 1, &this->velocity_tile_max_srv);
						break;
					case VIEW_MODE_VELOCITY_NEIGHBOR_MAX:
						ctx->PSSetShader(this->quad_ps, nullptr, 0);
						ctx->PSSetShaderResources(0, 1, &this->velocity_neighbor_max_srv);
						break;
					}
				}
				ctx->Draw(6, 0);
				PERF_EVENT_END(ctx);
			}

			// Reset RT and SRV state
			ID3D11ShaderResourceView* nullAttach[16] = {nullptr};
			ctx->PSSetShaderResources(0, 16, nullAttach);
			ctx->OMSetRenderTargets(0, nullptr, nullptr);
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
	CModelViewerCamera* camera;
	TwBar* settings_bar;
	float ui_update_time;

public:
	UIController(CModelViewerCamera* cam)
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
			this->ui_update_time = 1.0f;
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
			D3DXVECTOR3 eyePt = D3DXVECTOR3(0.0f, 10.0f, -20.0f);
			D3DXVECTOR3 lookAtPt = D3DXVECTOR3(0.0f, 10.0f, 0.0f);
			this->camera->SetViewParams(&eyePt, &lookAtPt);
			this->camera->SetModelCenter(lookAtPt);
			this->camera->SetButtonMasks(MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_RIGHT_BUTTON);
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
		this->camera->SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

		if (settings_bar) {
		}
	}

	void InitDialogs()
	{
		settings_bar = TwNewBar("Settings");
		TwDefine("Settings color='19 25 19' alpha=128 text=light size='500 250' iconified=true valueswidth=200");

		TwAddVarRW(settings_bar, "Pause Animation", TW_TYPE_BOOLCPP, &g_sailSpeedPaused, "group='' key=SPACE");
		TwAddVarRW(settings_bar, "Sail Speed", TW_TYPE_FLOAT, &g_sailSpeed, "group='' min=0 max=700 step=1.0 keydecr=o keyincr=p");
		TwAddVarRW(settings_bar, "Exposure Fraction", TW_TYPE_FLOAT, &g_Exposure, "group='Reconstruction' min=0.0 max=1.0 step=0.001 keydecr=k keyincr=l");
		TwAddVarRW(settings_bar, "Max Blur Radius", TW_TYPE_UINT32, &g_K, "group='Reconstruction' min=1 max=20 step=1 keydecr=n keyincr=m");
		TwAddVarRW(settings_bar, "Reconstruction Samples", TW_TYPE_UINT32, &g_S, "group='Reconstruction' min=1 max=20 step=2 keydecr=, keyincr=.");
		{
			TwEnumVal enumModeTypeEV[] = {
				{ VIEW_MODE_COLOR_ONLY,            "Color Only" },
				{ VIEW_MODE_DEPTH_ONLY,            "Depth Only" },
				{ VIEW_MODE_VELOCITY,              "Velocity" },
				{ VIEW_MODE_VELOCITY_TILE_MAX,     "Velocity TileMax" },
				{ VIEW_MODE_VELOCITY_NEIGHBOR_MAX, "Velocity NeighborMax" },
				{ VIEW_MODE_FINAL,                 "Gather (final result)" }
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

	CModelViewerCamera eye_camera;
	auto scene_controller = SceneController(&eye_camera);
	g_device_manager->AddControllerToFront(&scene_controller);
	
	auto ui_controller = UIController(&eye_camera);
	g_device_manager->AddControllerToFront(&ui_controller);
	
	DeviceCreationParameters deviceParams;
	deviceParams.swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	deviceParams.swapChainSampleCount = 1;
	deviceParams.startFullscreen = false;
	deviceParams.backBufferWidth = 1280;
	deviceParams.backBufferHeight = 720;

	if(FAILED(g_device_manager->CreateWindowDeviceAndSwapChain(deviceParams, L"NVIDIA Graphics Library : Motion Blur Advanced")))
	{
		MessageBox(nullptr, L"Cannot initialize the D3D11 device with the requested parameters", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	//g_device_manager->SetVsyncEnabled(true);
	PerfTracker::initialize();
	PerfTracker::EventDesc perf_events[] = {
		PERF_EVENT_DESC("Render Scene"),
		PERF_EVENT_DESC("Render > TileMax"),
		PERF_EVENT_DESC("Render > NeighborMax"),
		PERF_EVENT_DESC("Final Pass"),
		PERF_EVENT_DESC("Final > Gather"),
		PERF_EVENT_DESC("Final > Display"),
	};
	PerfTracker::ui_setup(perf_events, sizeof(perf_events)/sizeof(PerfTracker::EventDesc), nullptr);

	g_device_manager->MessageLoop();
	g_device_manager->Shutdown();

	PerfTracker::shutdown();
	delete g_device_manager;

	return 0;
}
