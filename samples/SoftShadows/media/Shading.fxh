//----------------------------------------------------------------------------------
// File:        SoftShadows\media/Shading.fxh
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
float3 g_lightPos;
Texture2D g_rockDiffuse;
Texture2D g_groundDiffuse;
Texture2D g_groundNormal;
bool g_useDiffuse;
int g_UseTexture;
float3 g_podiumCenterWorld;

float2 CubeMapTexCoords(float3 v)
{
    float2 uv;
    if (abs(v.x) > abs(v.y) && abs(v.x) > abs(v.z))
        uv = float2(v.y / abs(v.x), v.z / abs(v.x));
    if (abs(v.y) > abs(v.x) && abs(v.y) > abs(v.z))
        uv = float2(v.x / abs(v.y), v.z / abs(v.y));
    else
        uv = float2(v.x / abs(v.z), v.y / abs(v.z));
    return uv * 0.5 + 0.5;
}

float4 Shade(float3 WorldPos, float3 normal)
{
    float3 vLightDir = normalize(g_lightPos - WorldPos);
    if (g_UseTexture == 1)
    {
        float2 uv = (WorldPos.xz * 0.5 + 0.5) * 2.0;
        float3 diffuse = g_groundDiffuse.Sample( LinearSampler, uv );
        normal = g_groundNormal.Sample( LinearSampler, uv ).xzy * 2.0 - 1.0;
        diffuse *= max(dot(vLightDir, normal), 0);
        diffuse *= pow(dot(vLightDir, normalize(g_lightPos)), 40);
        return float4(diffuse, 1.0);
    }
    else if (g_UseTexture == 2)
    {
        float2 uv = CubeMapTexCoords(normalize(WorldPos.xyz - g_podiumCenterWorld));
        float3 diffuse = g_rockDiffuse.Sample( LinearSampler, uv ) * 1.2;
        diffuse *= max(dot(vLightDir, normal), 0);
        return float4(diffuse, 1.0);
    }
    else
    {
        float3 diffuse = max(dot(vLightDir, normal), 0);
        return float4(g_useDiffuse ? diffuse : 1.0, 1.0);        
    }
}
