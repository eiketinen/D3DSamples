//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\src/scene.h
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

#include <d3d11.h>
#include <vector>
#include <map>

struct aiMesh;
struct aiMaterial;

namespace Scene
{
    // Vertex formats
    const UINT VTXLAYOUTDESC_POSITION_FLOAT2 = 1;
    const UINT VTXLAYOUTDESC_POSITION_FLOAT3 = 2;
    const UINT VTXLAYOUTDESC_NBT_NONE = 0;
    const UINT VTXLAYOUTDESC_NBT_FLOAT3 = 1;
    const UINT VTXLAYOUTDESC_TEXCOORD_BYTE1 = 0;
    const UINT VTXLAYOUTDESC_TEXCOORD_BYTE2 = 1;
    const UINT VTXLAYOUTDESC_TEXCOORD_BYTE3 = 2;
    const UINT VTXLAYOUTDESC_TEXCOORD_BYTE4 = 3;
    const UINT VTXLAYOUTDESC_TEXCOORD_FLOAT1 = 4;
    const UINT VTXLAYOUTDESC_TEXCOORD_FLOAT2 = 5;
    const UINT VTXLAYOUTDESC_TEXCOORD_FLOAT3 = 6;
    const UINT VTXLAYOUTDESC_TEXCOORD_FLOAT4 = 7;
    union LayoutDesc
    {
        struct {
            UINT position    : 2;
            UINT normal        : 2;
            UINT tangent    : 2;
            UINT bitangent    : 2;
            UINT tc_count    : 3;
            UINT tc0        : 3;
            UINT tc1        : 3;
            UINT tc2        : 3;
            UINT tc3        : 3;
        } desc;
        UINT key;
    };

    enum MaterialProperty {
        NONE = -1,
        DIFFUSE_TEX,
        SPECULAR_TEX,
        NORMAL_TEX,
        DISPLACEMENT_TEX,
        PROPERTY_COUNT
    };
    typedef std::map<MaterialProperty, void *> MaterialTable;

    class RenderObject
    {
    public:
        RenderObject(ID3D11Device * device, const std::string &file_path, aiMesh * src_mesh, aiMaterial * src_material);
        virtual ~RenderObject();
        void render(ID3D11DeviceContext * context);

    private:
        static const int MAX_VTX_BUFFERS = 16;
        MaterialTable material_properties;

        UINT vtx_count;
        UINT vtx_offset;
        UINT idx_count;
        UINT idx_offset;

        LayoutDesc vtx_layout;
        UINT vtx_buffer_count;
        UINT vtx_strides[MAX_VTX_BUFFERS];
        UINT vtx_offsets[MAX_VTX_BUFFERS];
        ID3D11Buffer *vtx_buffers[MAX_VTX_BUFFERS];

        ID3D11Buffer *idx_buffer;
    };

    typedef std::vector <RenderObject *> RenderList;

	HRESULT load_texture(ID3D11Device * device, const wchar_t * filename, ID3D11ShaderResourceView ** out_srv);
	HRESULT load_model(ID3D11Device * device, const char * filename, RenderList &out_objects);
};