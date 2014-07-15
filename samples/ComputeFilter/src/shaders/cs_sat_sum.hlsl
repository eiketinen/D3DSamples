//----------------------------------------------------------------------------------
// File:        ComputeFilter\src\shaders/cs_sat_sum.hlsl
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
Texture2D<float3>   texInput  : register(t0);
RWTexture2D<float3> texOutput : register(u0);

#define GROUP_SIZE 16
#define WARP_SIZE 32

groupshared float3 sSums[WARP_SIZE*GROUP_SIZE];

[numthreads( WARP_SIZE, GROUP_SIZE, 1 )]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID, uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixelID = uint2(dispatchID.x, dispatchID.y);

    sSums[threadID.x + threadID.y*WARP_SIZE] = texInput[pixelID];
    GroupMemoryBarrierWithGroupSync();

    // XXX TODO: Use a parallel scan here
    if (threadID.x == 0)
    {
        float3 total = float3(0,0,0);
        for (int t=0; t<WARP_SIZE; ++t)
        {
            total += sSums[t + threadID.y*WARP_SIZE];
            sSums[t + threadID.y*WARP_SIZE] = total;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    texOutput[pixelID] = sSums[threadID.x + threadID.y*WARP_SIZE];
}