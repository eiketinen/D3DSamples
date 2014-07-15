//----------------------------------------------------------------------------------
// File:        DeferredShadingMSAA\src/ObjMeshDX.cpp
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
#pragma warning(disable: 4995)
#include "ObjMeshDX.h"
#include <fstream>
using namespace std;
#pragma warning(default: 4995)


// Define the input layout
const D3D11_INPUT_ELEMENT_DESC ObjMeshDX::layout_Position_Normal_Texture[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

//--------------------------------------------------------------------------------------
ObjMeshDX::ObjMeshDX()
	: m_pCBMesh(nullptr)
	, m_pCBSubMesh(nullptr)
{
}


//--------------------------------------------------------------------------------------
ObjMeshDX::~ObjMeshDX()
{
    Destroy();
}


//--------------------------------------------------------------------------------------
void ObjMeshDX::Destroy()
{
    for(UINT32 iMaterial = 0; iMaterial < m_Materials.size(); ++iMaterial)
    {
        Material *pMaterial = m_Materials[iMaterial];

        if(pMaterial->pTextureRSV)
        {
            ID3D11Resource* pRes = NULL;
            
            pMaterial->pTextureRSV->GetResource(&pRes);
            SAFE_RELEASE(pRes);
            SAFE_RELEASE(pRes);   // do this twice, because GetResource adds a ref

            SAFE_RELEASE(pMaterial->pTextureRSV);
        }

        SAFE_DELETE(pMaterial);
    }

    m_Materials.clear();
    m_Vertices.clear();
    m_Indices.clear();
	
	SAFE_RELEASE(m_pCBMesh);
	SAFE_RELEASE(m_pCBSubMesh);
}


//--------------------------------------------------------------------------------------
HRESULT ObjMeshDX::Create(ID3D11Device* pDevice, const WCHAR* strFilename)
{
    HRESULT hr;
    // Start clean
    Destroy();
    V_RETURN(LoadGeometryFromOBJ(strFilename));

    // Load material textures
	for(UINT32 iMaterial = 0; iMaterial < m_Materials.size(); ++iMaterial)
    {
        Material *pMaterial = m_Materials[iMaterial];
        if(pMaterial->strTexture[0])
		{
			D3DX11_IMAGE_LOAD_INFO loadInfo;
			D3DX11_IMAGE_INFO srcInfo;
			D3DX11GetImageInfoFromFile(pMaterial->strTexture, NULL, &srcInfo, NULL);

			loadInfo.pSrcInfo = &srcInfo;
			loadInfo.Format = srcInfo.Format;
			loadInfo.MipLevels = 0;
			D3DX11CreateShaderResourceViewFromFile(pDevice, pMaterial->strTexture, &loadInfo, NULL, &pMaterial->pTextureRSV, NULL);
		}
    }

	// Create D3D Buffers
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(MeshVertex) * UINT(m_Vertices.size());
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = m_Vertices.data();
		V_RETURN(pDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer));
	}
	
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(DWORD) * UINT(m_Indices.size());
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = m_Indices.data();
		V_RETURN(pDevice->CreateBuffer(&bd, &InitData, &m_pIndexBuffer));
	}
	
    // Create the constant buffers
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.ByteWidth = sizeof(MeshConstants);
		V_RETURN(pDevice->CreateBuffer(&bd, NULL, &m_pCBMesh));
	}
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.ByteWidth = sizeof(SubMeshConstants);
		V_RETURN(pDevice->CreateBuffer(&bd, NULL, &m_pCBSubMesh));
	}

    return S_OK;
}

//--------------------------------------------------------------------------------------
void ObjMeshDX::Draw(ID3D11DeviceContext* pDeviceContext, const D3DXMATRIX& worldMatrix, const D3DXMATRIX& viewMatrix, const D3DXMATRIX& WVP, const int diffuseTexSlot) const
{
    // Set primitive topology
    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// Set vertex buffer
    UINT stride = sizeof(MeshVertex);
    UINT offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	// Set index buffer
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	
	// Set constant buffer for mesh
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	pDeviceContext->Map(m_pCBMesh, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	MeshConstants* pVSPerObject = (MeshConstants*)MappedResource.pData;
	D3DXMatrixTranspose(&pVSPerObject->LocalToProjected4x4, &WVP);
	D3DXMatrixTranspose(&pVSPerObject->LocalToWorld4x4, &worldMatrix);
	D3DXMatrixTranspose(&pVSPerObject->WorldToView4x4, &viewMatrix);
	pDeviceContext->Unmap(m_pCBMesh, 0);
		
	pDeviceContext->VSSetConstantBuffers(0, 1, &m_pCBMesh);
	pDeviceContext->PSSetConstantBuffers(0, 1, &m_pCBMesh);

	for(auto i = 0; i < m_iNumSubsets; i++)
	{
		// Set constant buffer for sub-mesh
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		pDeviceContext->Map(m_pCBSubMesh, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		SubMeshConstants* pVSPerObject = (SubMeshConstants*)MappedResource.pData;
		pVSPerObject->Diffuse = m_Materials[m_SubsetMtlIdx[i]]->vDiffuse;
		pVSPerObject->Textured = m_Materials[m_SubsetMtlIdx[i]]->pTextureRSV != nullptr;
		pDeviceContext->Unmap(m_pCBSubMesh, 0);
		if(diffuseTexSlot >= 0 && m_Materials[m_SubsetMtlIdx[i]]->pTextureRSV)
		{
			pDeviceContext->PSSetShaderResources(diffuseTexSlot, 1, &m_Materials[m_SubsetMtlIdx[i]]->pTextureRSV);
		}

		pDeviceContext->VSSetConstantBuffers(1, 1, &m_pCBSubMesh);
		pDeviceContext->PSSetConstantBuffers(1, 1, &m_pCBSubMesh);

		// Render
		auto startIdx = m_SubsetStartIdx[i];
		auto count = m_SubsetStartIdx[i + 1] - m_SubsetStartIdx[i];
		pDeviceContext->DrawIndexed(count, startIdx, 0);
	}
}

//--------------------------------------------------------------------------------------
HRESULT ObjMeshDX::LoadGeometryFromOBJ(const WCHAR* strFileName)
{
    WCHAR strMaterialFilename[MAX_PATH] = {0};
    HRESULT hr;

    // Create temporary storage for the input data. Once the data has been loaded into
    // a reasonable format we can create a D3DXMesh object and load it with the mesh data.
    std::vector<D3DXVECTOR3> Positions;
    std::vector<D3DXVECTOR2> TexCoords;
    std::vector<D3DXVECTOR3> Normals;
	int iSmoothingGroup = 0;

    // The first subset uses the default material
    Material* pMaterial = new Material();
    if(pMaterial == NULL)
        return E_OUTOFMEMORY;

    InitMaterial(pMaterial);
    wcscpy_s(pMaterial->strName, MAX_PATH - 1, L"default");
    m_Materials.push_back(pMaterial);
	
	m_iNumSubsets = 0;
    auto dwCurSubset = 0;

    // File input
    WCHAR strCommand[256] = {0};
    wifstream InFile(strFileName);
    if(!InFile)
		return E_FAIL;
	
	D3DXVECTOR3 vMin = D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX);
	D3DXVECTOR3 vMax = D3DXVECTOR3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for(; ;)
    {
        InFile >> strCommand;
        if(!InFile)
            break;

        if(0 == wcscmp(strCommand, L"#"))
        {
            // Comment
        }
        else if(0 == wcscmp(strCommand, L"v"))
        {
            // Vertex Position
            float x, y, z;
            InFile >> x >> y >> z;
            Positions.push_back(D3DXVECTOR3(x, y, z));
			
			if(x > vMax.x)
				vMax.x = x;
			if(y > vMax.y)
				vMax.y = y;
			if(z > vMax.z)
				vMax.z = z;
			if(x < vMin.x)
				vMin.x = x;
			if(y < vMin.y)
				vMin.y = y;
			if(z < vMin.z)
				vMin.z = z;
        }
        else if(0 == wcscmp(strCommand, L"vt"))
        {
            // Vertex TexCoord
            float u, v;
            InFile >> u >> v;
            TexCoords.push_back(D3DXVECTOR2(u, v));
        }
        else if(0 == wcscmp(strCommand, L"vn"))
        {
            // Vertex Normal
            float x, y, z;
            InFile >> x >> y >> z;
            Normals.push_back(D3DXVECTOR3(x, y, z));
        }
        else if(0 == wcscmp(strCommand, L"f"))
        {
            // Face
            UINT iPosition = 0, iTexCoord, iNormal;
            MeshVertex vertex;
			MeshFace Face, quadFace;

			auto ReadNextVertex = [&]() -> DWORD
			{
				ZeroMemory(&vertex, sizeof(MeshVertex));

                // OBJ format uses 1-based arrays
                InFile >> iPosition;
                vertex.position = Positions[iPosition - 1];

                if('/' == InFile.peek())
                {
                    InFile.ignore();

                    if('/' != InFile.peek())
                    {
                        // Optional texture coordinate
                        InFile >> iTexCoord;
                        vertex.texcoord = TexCoords[iTexCoord - 1];
                    }

                    if('/' == InFile.peek())
                    {
                        InFile.ignore();

                        // Optional vertex normal
                        InFile >> iNormal;
                        vertex.normal = Normals[iNormal - 1];
                    }
                }

				DWORD index = AddVertex(iPosition, &vertex);
				return index;
			};

			UINT iFace;
            for(iFace = 0; iFace < 3; iFace++)
            {
				auto index = ReadNextVertex();
				m_Indices.push_back(index);
				Face.aiIndices[iFace] = index;
            }

			Face.iSmoothingGroup = iSmoothingGroup;
			m_Faces.push_back(Face);
			
			bool bQuad = false;
			while((InFile.peek() < '0' || InFile.peek() > '9') && InFile.peek() != '\n')
			{
				InFile.ignore();
			}
			if(InFile.peek() >= '0' && InFile.peek() <= '9')
				bQuad = true;
			else
				bQuad = false;

			if(bQuad)
			{
				auto index = ReadNextVertex();

				// Triangularize quad
				{
					quadFace.aiIndices[0] = index;
					quadFace.aiIndices[1] = Face.aiIndices[0];
					quadFace.aiIndices[2] = Face.aiIndices[2];

					m_Indices.push_back(quadFace.aiIndices[0]);
					m_Indices.push_back(quadFace.aiIndices[1]);
					m_Indices.push_back(quadFace.aiIndices[2]);
				}

				quadFace.iSmoothingGroup = iSmoothingGroup;
				m_Faces.push_back(quadFace);
			}
        }
		else if(0 == wcscmp(strCommand, L"s")) // Handle smoothing group for normal computation
		{
			InFile.ignore();

			if(InFile.peek() >= '0' && InFile.peek() <= '9')
				InFile >> iSmoothingGroup;
			else
				iSmoothingGroup = 0;
		}
        else if(0 == wcscmp(strCommand, L"mtllib"))
        {
            // Material library
            InFile >> strMaterialFilename;
        }
        else if(0 == wcscmp(strCommand, L"usemtl"))
        {
            // Material
            WCHAR strName[MAX_PATH] = {0};
            InFile >> strName;

			bool bFound = false;
			for(UINT32 iMaterial = 0; iMaterial < m_Materials.size(); iMaterial++)
			{
				Material* pCurMaterial = m_Materials[iMaterial];
				if(0 == wcscmp(pCurMaterial->strName, strName))
				{
					bFound = true;
                    dwCurSubset = iMaterial;
                    break;
                }
            }

            if(!bFound)
            {
                pMaterial = new Material();
                if(pMaterial == NULL)
                    return E_OUTOFMEMORY;

                dwCurSubset = static_cast<int>(m_Materials.size());

                InitMaterial(pMaterial);
                wcscpy_s(pMaterial->strName, MAX_PATH - 1, strName);

                m_Materials.push_back(pMaterial);
            }

			m_SubsetStartIdx.push_back(DWORD(m_Indices.size()));
			m_SubsetMtlIdx.push_back(dwCurSubset);
			m_iNumSubsets++;
        }
        else
        {
            // Unimplemented or unrecognized command
        }

        InFile.ignore(1000, '\n');
    }

	// Correct subsets index
	if(m_iNumSubsets == 0)
	{
		m_SubsetStartIdx.push_back(0);
		m_iNumSubsets = 1;
		m_SubsetMtlIdx.push_back(0);
	}

	m_SubsetStartIdx.push_back(DWORD(m_Indices.size()));
	
	ComputeVertexNormals();

    // Cleanup
    InFile.close();
    DeleteCache();

	D3DXVECTOR3 vMid = 0.5f * (vMax + vMin);
	for(UINT32 i = 0; i < m_Vertices.size(); i++)
		m_Vertices[i].position -= vMid;


    // If an associated material file was found, read that in as well.
    if(strMaterialFilename[0])
    {
		auto idx = wcsrchr(strFileName, L'/') - strFileName + 1;
		WCHAR strMtlPath[MAX_PATH] = {0};
		wcsncpy(strMtlPath, strFileName, idx);
		wcscat_s(strMtlPath, MAX_PATH, strMaterialFilename);
        V_RETURN(LoadMaterialsFromMTL(strMtlPath));
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
void ObjMeshDX::ComputeVertexNormals()
{
	// First compute per face normals
	vector<D3DXVECTOR3> vFaceNormals;
	vFaceNormals.resize(m_Faces.size());
	for(UINT32 i = 0; i < m_Faces.size(); i++)
	{
		const D3DXVECTOR3& pt1 = GetVertexAt(m_Faces[i].aiIndices[0]).position;
		const D3DXVECTOR3& pt2 = GetVertexAt(m_Faces[i].aiIndices[1]).position;
		const D3DXVECTOR3& pt3 = GetVertexAt(m_Faces[i].aiIndices[2]).position;

		D3DXVECTOR3 vEdge1 = pt2 - pt1;
		D3DXVECTOR3 vEdge2 = pt3 - pt1;

		D3DXVECTOR3 vNormal;
		D3DXVec3Cross(&vNormal, &vEdge1, &vEdge2);
		D3DXVec3Normalize(&vNormal, &vNormal);
		vFaceNormals[i] = vNormal;
	}

	struct VertexFace
	{
		int iCount;
		vector<int> List;
		VertexFace()
			: iCount(0) {}
	};
	vector<VertexFace> VertexFaceList;
	VertexFaceList.resize(m_Vertices.size());
	for(UINT32 i = 0; i < m_Faces.size(); i++)
	{
		for(auto j = 0; j < 3; j++)
		{
			VertexFaceList[m_Faces[i].aiIndices[j]].iCount++;
			VertexFaceList[m_Faces[i].aiIndices[j]].List.push_back(i);
		}
	}


	// Compute per vertex normals with smoothing group
	for(UINT32 i = 0; i < m_Faces.size(); i++)
	{
		const MeshFace& face = m_Faces[i];
		for(auto j = 0; j < 3; j++)
		{
			int iFaceCount = 0;
			D3DXVECTOR3 vNormal = D3DXVECTOR3(0, 0, 0);
			for(auto k = 0; k < VertexFaceList[m_Faces[i].aiIndices[j]].iCount; k++)
			{
				int iFaceIdx = VertexFaceList[m_Faces[i].aiIndices[j]].List[k];
				if(face.iSmoothingGroup & m_Faces[iFaceIdx].iSmoothingGroup)
				{
					vNormal += vFaceNormals[iFaceIdx];
					iFaceCount++;
				}
			}

			if(iFaceCount > 0)
				vNormal /= float(iFaceCount);
			else
				vNormal = vFaceNormals[i];
			D3DXVec3Normalize(&vNormal, &vNormal);

			MeshVertex& vert = m_Vertices[face.aiIndices[j]];
			if(vert.normal == D3DXVECTOR3(0, 0, 0))
				vert.normal = vNormal;
			else if(vert.normal != vNormal)
			{
				MeshVertex newVertex = vert;
				newVertex.normal = vNormal;

				auto idx = AddVertex(m_Faces[i].aiIndices[j], &newVertex);
				//m_Faces[i].aiIndices[j] = idx;
				m_Indices[3 * i + j] = idx;
			}
		}
	}
}

//--------------------------------------------------------------------------------------
DWORD ObjMeshDX::AddVertex(UINT hash, MeshVertex* pVertex)
{
    // If this vertex doesn't already exist in the Vertices list, create a new entry.
    // Add the index of the vertex to the Indices list.
    bool bFoundInList = false;
    auto index = 0;

    // Since it's very slow to check every element in the vertex list, a hashtable stores
    // vertex indices according to the vertex position's index as reported by the OBJ file
    if((UINT)m_VertexCache.size() > hash)
    {
        CacheEntry* pEntry = m_VertexCache[hash];
        while(pEntry != NULL)
        {
            MeshVertex* pCacheVertex = m_Vertices.data() + pEntry->index;

            // If this vertex is identical to the vertex already in the list, simply
            // point the index buffer to the existing vertex
            if(0 == memcmp(pVertex, pCacheVertex, sizeof(MeshVertex)))
            {
                bFoundInList = true;
                index = pEntry->index;
                break;
            }

            pEntry = pEntry->pNext;
        }
    }

    // Vertex was not found in the list. Create a new entry, both within the Vertices list
    // and also within the hashtable cache
    if(!bFoundInList)
    {
        // Add to the Vertices list
        index = static_cast<int>(m_Vertices.size());
        m_Vertices.push_back(*pVertex);

        // Add this to the hashtable
        CacheEntry* pNewEntry = new CacheEntry;
        if(pNewEntry == NULL)
            return(DWORD)-1;

        pNewEntry->index = index;
        pNewEntry->pNext = NULL;

        // Grow the cache if needed
        while((UINT)m_VertexCache.size() <= hash)
        {
            m_VertexCache.push_back(NULL);
        }

        // Add to the end of the linked list
        CacheEntry* pCurEntry = m_VertexCache[hash];
        if(pCurEntry == NULL)
        {
            // This is the head element
            m_VertexCache[hash] = pNewEntry;
        }
        else
        {
            // Find the tail
            while(pCurEntry->pNext != NULL)
            {
                pCurEntry = pCurEntry->pNext;
            }

            pCurEntry->pNext = pNewEntry;
        }
    }

    return index;
}


//--------------------------------------------------------------------------------------
void ObjMeshDX::DeleteCache()
{
    // Iterate through all the elements in the cache and subsequent linked lists
    for(UINT32 i = 0; i < m_VertexCache.size(); i++)
    {
        CacheEntry* pEntry = m_VertexCache[i];
        while(pEntry != NULL)
        {
            CacheEntry* pNext = pEntry->pNext;
            SAFE_DELETE(pEntry);
            pEntry = pNext;
        }
    }

    m_VertexCache.clear();
}


//--------------------------------------------------------------------------------------
HRESULT ObjMeshDX::LoadMaterialsFromMTL(const WCHAR* strFileName)
{
	// File input
    WCHAR strCommand[256] = {0};
    wifstream InFile(strFileName);
    if(!InFile)
        return E_FAIL;

    Material* pMaterial = NULL;

    for(; ;)
    {
        InFile >> strCommand;
        if(!InFile)
            break;

        if(0 == wcscmp(strCommand, L"newmtl"))
        {
            // Switching active materials
            WCHAR strName[MAX_PATH] = {0};
            InFile >> strName;

            pMaterial = NULL;
            for(UINT32 i = 0; i < m_Materials.size(); i++)
            {
                Material* pCurMaterial = m_Materials[i];
                if(0 == wcscmp(pCurMaterial->strName, strName))
                {
                    pMaterial = pCurMaterial;
                    break;
                }
            }
        }

        // The rest of the commands rely on an active material
        if(pMaterial == NULL)
            continue;

        if(0 == wcscmp(strCommand, L"#"))
        {
            // Comment
        }
        else if(0 == wcscmp(strCommand, L"Ka"))
        {
            // Ambient color
            float r, g, b;
            InFile >> r >> g >> b;
            pMaterial->vAmbient = D3DXVECTOR3(r, g, b);
        }
        else if(0 == wcscmp(strCommand, L"Kd"))
        {
            // Diffuse color
            float r, g, b;
            InFile >> r >> g >> b;
            pMaterial->vDiffuse = D3DXVECTOR3(r, g, b);
        }
        else if(0 == wcscmp(strCommand, L"Ks"))
        {
            // Specular color
            float r, g, b;
            InFile >> r >> g >> b;
            pMaterial->vSpecular = D3DXVECTOR3(r, g, b);
        }
        else if(0 == wcscmp(strCommand, L"d") ||
                 0 == wcscmp(strCommand, L"Tr"))
        {
            // Alpha
            InFile >> pMaterial->fAlpha;
        }
        else if(0 == wcscmp(strCommand, L"Ns"))
        {
            // Shininess
            int nShininess;
            InFile >> nShininess;
            pMaterial->nShininess = nShininess;
        }
        else if(0 == wcscmp(strCommand, L"illum"))
        {
            // Specular on/off
            int illumination;
            InFile >> illumination;
            pMaterial->bSpecular =(illumination == 2);
        }
        else if(0 == wcscmp(strCommand, L"map_Kd"))
        {
			// Texture
			WCHAR strTexName[MAX_PATH] = {0};
			InFile >> strTexName;

			auto idx = wcsrchr(strFileName, L'/') - strFileName + 1;
			wcsncpy(pMaterial->strTexture, strFileName, idx);
			wcscat_s(pMaterial->strTexture, MAX_PATH, strTexName);
        }

        else
        {
            // Unimplemented or unrecognized command
        }

        InFile.ignore(1000, L'\n');
    }

    InFile.close();

    return S_OK;
}


//--------------------------------------------------------------------------------------
void ObjMeshDX::InitMaterial(Material* pMaterial)
{
    ZeroMemory(pMaterial, sizeof(Material));

    pMaterial->vAmbient = D3DXVECTOR3(0.2f, 0.2f, 0.2f);
    pMaterial->vDiffuse = D3DXVECTOR3(0.8f, 0.8f, 0.8f);
    pMaterial->vSpecular = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
    pMaterial->nShininess = 0;
    pMaterial->fAlpha = 1.0f;
    pMaterial->bSpecular = false;
    pMaterial->pTextureRSV = NULL;
}
