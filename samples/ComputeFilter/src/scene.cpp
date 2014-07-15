//----------------------------------------------------------------------------------
// File:        ComputeFilter\src/scene.cpp
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

#include <assimp\assimp.hpp>
#include <assimp\aiPostProcess.h>
#include <assimp\aiScene.h>
#include <DirectXTex.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace {
////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT load_texture(ID3D11Device * device, const wchar_t * filename, ID3D11ShaderResourceView ** out_srv)
{
    HRESULT hr = S_OK;
    *out_srv = nullptr;

    DWORD flags = DirectX::DDS_FLAGS_NONE;
    DirectX::TexMetadata img_info;
    DirectX::ScratchImage file_image;

    hr = DirectX::LoadFromDDSFile(filename, flags, &img_info, file_image);
    if (FAILED(hr)) return hr;

    hr = DirectX::CreateShaderResourceViewEx(device, file_image.GetImages(), file_image.GetImageCount(), img_info, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0x00, 0x00, true, out_srv);
    if (FAILED(hr)) return hr;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
} namespace Scene {
////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT load_model(ID3D11Device * device, const char * filename, std::vector <RenderObject *> &out_objects)
{
    Assimp::Importer asset_importer;
#if 0
    unsigned int asset_import_flags =    aiProcess_JoinIdenticalVertices |
                                        aiProcess_PreTransformVertices | 
                                        aiProcess_RemoveRedundantMaterials |
                                        aiProcess_GenUVCoords |
                                        aiProcess_TransformUVCoords |
                                        aiProcess_FlipUVs |
                                        aiProcess_OptimizeMeshes |
                                        aiProcess_ImproveCacheLocality;
#else
    unsigned int asset_import_flags =    aiProcess_PreTransformVertices | 
                                        aiProcess_RemoveRedundantMaterials |
                                        aiProcess_FlipUVs;
#endif

    printf("Loading assets from: \"%s\", this may take a little time...\n", filename);
    const aiScene * model = asset_importer.ReadFile(filename, asset_import_flags);

    if (model == nullptr)
    {
        printf("Error importing asset \"%s\": %s\n", filename, asset_importer.GetErrorString());
        return E_FAIL;
    }

    std::string file_path = filename;
    
    int split_pos1 = (int) file_path.rfind('\\');
    int split_pos2 = (int) file_path.rfind('/');
    int split_pos = max(split_pos1, split_pos2);
    if (split_pos != (int) std::string::npos)
    {
        file_path = file_path.substr(0, (size_t) split_pos+1);
    }
    else
    {
        file_path = "";
    }

    printf("Loading materials for discovered meshes...\n");
    for (UINT mesh_idx=0; mesh_idx < model->mNumMeshes; mesh_idx++)
    {
        aiMesh * mesh = model->mMeshes[mesh_idx];
        aiMaterial * material = model->mMaterials[mesh->mMaterialIndex];            
        out_objects.push_back(new RenderObject(device, file_path, mesh, material));
    }

    printf("Cleaning up... ");
    asset_importer.FreeScene();
    printf("done\n");
    printf("Bringing up scene...\n");
    return S_OK;
}

RenderObject::RenderObject(ID3D11Device * device, const std::string &file_path, aiMesh * src_mesh, aiMaterial * src_material)
{
    HRESULT hr;
    this->idx_count = src_mesh->mNumFaces * 3;
    this->idx_offset = 0;
    UINT idx_size = sizeof(UINT);
    UINT idx_buffer_size = idx_size * this->idx_count;
    UINT * source_indices = new UINT[this->idx_count];
    for (UINT face_idx=0; face_idx < src_mesh->mNumFaces; ++face_idx)
    {
        aiFace * face = &src_mesh->mFaces[face_idx];
        source_indices[3*face_idx+0] = face->mIndices[0];
        source_indices[3*face_idx+1] = face->mIndices[1];
        source_indices[3*face_idx+2] = face->mIndices[2];
    }

    D3D11_BUFFER_DESC ib_desc = {
        idx_buffer_size,            // Byte Width
        D3D11_USAGE_DEFAULT,        // Usage
        D3D11_BIND_INDEX_BUFFER,    // Bind Flags
        0,                            // CPU Access
        0,                            // Misc Flags
        0,                            // Structure Byte Stride
    };

    D3D11_SUBRESOURCE_DATA ib_data = {
        (void *) source_indices,    // Source Data
        0,                            // Line Pitch
        0                            // Slice Pitch
    };
    device->CreateBuffer(&ib_desc, &ib_data, &this->idx_buffer);
    delete [] source_indices;

    this->vtx_count = src_mesh->mNumVertices;
    this->vtx_offset = 0;
    void * source_data[MAX_VTX_BUFFERS] = {nullptr};
    UINT source_strides[MAX_VTX_BUFFERS];

    source_data[0] = (void *) src_mesh->mVertices;
    source_strides[0] = sizeof(aiVector3D);
    source_data[1] = (void *) src_mesh->mNormals;
    source_strides[1] = sizeof(aiVector3D);
    source_data[2] = (void *) src_mesh->mTextureCoords[0];
    source_strides[2] = sizeof(aiVector3D);

    this->vtx_layout.key = 0;
    this->vtx_layout.desc.position = VTXLAYOUTDESC_POSITION_FLOAT3;
    this->vtx_layout.desc.normal = VTXLAYOUTDESC_NBT_FLOAT3;
    this->vtx_layout.desc.tc_count = 1;
    this->vtx_layout.desc.tc0 = VTXLAYOUTDESC_TEXCOORD_FLOAT2;

    this->vtx_buffer_count = 3;

    for (UINT idx=0; idx<MAX_VTX_BUFFERS; ++idx)
    {
        this->vtx_buffers[idx] = nullptr;
    }

    for (UINT buffer_idx = 0; buffer_idx < this->vtx_buffer_count; ++buffer_idx)
    {
        UINT element_size = source_strides[buffer_idx];
        UINT buffer_size = element_size * this->vtx_count;
        this->vtx_strides[buffer_idx] = source_strides[buffer_idx];
        this->vtx_offsets[buffer_idx] = 0;
        D3D11_BUFFER_DESC vb_desc = {
            buffer_size,                // Byte Width
            D3D11_USAGE_DEFAULT,        // Usage
            D3D11_BIND_VERTEX_BUFFER,    // Bind Flags
            0,                            // CPU Access
            0,                            // Misc Flags
            0,                            // Structure Byte Stride
        };

        D3D11_SUBRESOURCE_DATA vb_data = {
            source_data[buffer_idx],    // Source Data
            0,                            // Line Pitch
            0                            // Slice Pitch
        };

        device->CreateBuffer(&vb_desc, &vb_data, &this->vtx_buffers[buffer_idx]);
    }

    if (src_material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        aiString texture_filename;
        src_material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_filename);
        std::string full_path = std::string(file_path.c_str()) + std::string(texture_filename.data);
        wchar_t wc_path[MAX_PATH];
        mbstowcs_s(nullptr, wc_path, full_path.c_str(), MAX_PATH);

        ID3D11ShaderResourceView * srv;
        hr = load_texture(device, wc_path, &srv);
        _ASSERT(SUCCEEDED(hr));

        this->material_properties[Scene::DIFFUSE_TEX] = (void *) srv;
    }

    if (src_material->GetTextureCount(aiTextureType_SPECULAR) > 0)
    {
        aiString texture_filename;
        src_material->GetTexture(aiTextureType_SPECULAR, 0, &texture_filename);
        std::string full_path = std::string(file_path.c_str()) + std::string(texture_filename.data);
        wchar_t wc_path[MAX_PATH];
        mbstowcs_s(nullptr, wc_path, full_path.c_str(), MAX_PATH);

        ID3D11ShaderResourceView * srv;
        hr = load_texture(device, wc_path, &srv);
        _ASSERT(SUCCEEDED(hr));

        this->material_properties[Scene::SPECULAR_TEX] = (void *) srv;
    }

    if (src_material->GetTextureCount(aiTextureType_NORMALS) > 0)
    {
        aiString texture_filename;
        src_material->GetTexture(aiTextureType_NORMALS, 0, &texture_filename);
        std::string full_path = std::string(file_path.c_str()) + std::string(texture_filename.data);
        wchar_t wc_path[MAX_PATH];
        mbstowcs_s(nullptr, wc_path, full_path.c_str(), MAX_PATH);

        ID3D11ShaderResourceView * srv;
        hr = load_texture(device, wc_path, &srv);
        _ASSERT(SUCCEEDED(hr));

        this->material_properties[Scene::NORMAL_TEX] = (void *) srv;
    }
}

RenderObject::~RenderObject()
{
    SAFE_RELEASE(this->idx_buffer);

    for (int i=0; i<MAX_VTX_BUFFERS; ++i)
    {
        if (this->vtx_buffers[i])
        {
            SAFE_RELEASE(this->vtx_buffers[i]);
        }
    }

    for (auto prop = this->material_properties.begin(); prop != this->material_properties.end(); ++prop)
    {
        ID3D11ShaderResourceView * srv = (ID3D11ShaderResourceView *)(*prop).second;
        SAFE_RELEASE(srv);
    }
}

void RenderObject::render(ID3D11DeviceContext * ctx)
{
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, this->vtx_buffer_count, this->vtx_buffers, this->vtx_strides, this->vtx_offsets);
    ctx->IASetIndexBuffer( this->idx_buffer, DXGI_FORMAT_R32_UINT, this->idx_offset);

    ID3D11ShaderResourceView * srvs[16];
    srvs[0] = (ID3D11ShaderResourceView *) material_properties[Scene::DIFFUSE_TEX];
    srvs[1] = (ID3D11ShaderResourceView *) material_properties[Scene::SPECULAR_TEX];
    srvs[2] = (ID3D11ShaderResourceView *) material_properties[Scene::NORMAL_TEX];
    ctx->PSSetShaderResources(0, 1, srvs);

    ctx->DrawIndexed(this->idx_count, this->idx_offset, this->vtx_offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
}