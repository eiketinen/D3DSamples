//----------------------------------------------------------------------------------
// File:        DeferredShadingMSAA\src/ObjMeshDX.h
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
#ifndef _MESHLOADER_H_
#define _MESHLOADER_H_
#pragma once

#include "DirectXUtil.h"
#include <vector>

// Vertex format
struct MeshVertex
{
    D3DXVECTOR3 position;
    D3DXVECTOR3 normal;
    D3DXVECTOR2 texcoord;
};

struct MeshFace
{
	int aiIndices[3];
	int iSmoothingGroup;
};

// Used for a hashtable vertex cache when creating the mesh from a .obj file
struct CacheEntry
{
    UINT index;
    CacheEntry* pNext;
};


// Material properties per mesh subset
struct Material
{
    WCHAR   strName[MAX_PATH];

    D3DXVECTOR3 vAmbient;
    D3DXVECTOR3 vDiffuse;
    D3DXVECTOR3 vSpecular;

    int nShininess;
    float fAlpha;

    bool bSpecular;

    WCHAR   strTexture[MAX_PATH];
    ID3D11ShaderResourceView* pTextureRSV;

	Material()
		: pTextureRSV(nullptr)
	{
		ZeroMemory(strTexture, MAX_PATH * sizeof(WCHAR));
	}
};


class ObjMeshDX
{
public:
	ObjMeshDX();
	~ObjMeshDX();

    HRESULT Create(ID3D11Device* pd3dDevice, const WCHAR* strFilename);
    void    Destroy();

    size_t GetNumMaterials() const
    {
		return m_Materials.size();
    }
    Material* GetMaterial(UINT iMaterial)
    {
		return m_Materials[iMaterial];
    }
	size_t GetNumSubsets() const
	{
		return m_SubsetStartIdx.size();
	}
	
	struct MeshConstants
	{
		D3DXMATRIX	LocalToProjected4x4;
		D3DXMATRIX	LocalToWorld4x4;
		D3DXMATRIX	WorldToView4x4;
	};

	struct SubMeshConstants
	{
		D3DXVECTOR3	Diffuse;
		int			Textured;
	};

public:
	void Draw(ID3D11DeviceContext* pDeviceContext, const D3DXMATRIX& worldMatrix, const D3DXMATRIX& viewMatrix, const D3DXMATRIX& WVP, const int diffuseTexSlot = -1) const;

private:
    HRESULT LoadGeometryFromOBJ(const WCHAR* strFilename);
    HRESULT LoadMaterialsFromMTL(const WCHAR* strFileName);
	void	ComputeVertexNormals();
    void    InitMaterial(Material* pMaterial);

    DWORD   AddVertex(UINT hash, MeshVertex* pVertex);
    void    DeleteCache();

	
	inline const DWORD* GetIndexAt(int iNum) const { return m_Indices.data() + 3 * iNum; }
	inline const MeshVertex& GetVertexAt(int iIndex) const { return m_Vertices[iIndex]; }

    std::vector<CacheEntry*> m_VertexCache;   // Hashtable cache for locating duplicate vertices
    std::vector<MeshVertex> m_Vertices;      // Filled and copied to the vertex buffer
    std::vector<DWORD> m_Indices;       // Filled and copied to the index buffer
	std::vector<MeshFace> m_Faces;
	std::vector<DWORD> m_SubsetStartIdx;  // Holds starting index of each subset
	std::vector<DWORD> m_SubsetMtlIdx;  // Holds the material index of each subset
    std::vector<Material*> m_Materials;     // Holds material properties per subset
	int m_iNumSubsets;

	// D3D Buffers
	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	ID3D11Buffer* m_pCBMesh;
	ID3D11Buffer* m_pCBSubMesh;


public:
	static const UINT numElements_layout_Position_Normal_Texture = 3;
	static const D3D11_INPUT_ELEMENT_DESC layout_Position_Normal_Texture[];
};

#endif // _MESHLOADER_H_

