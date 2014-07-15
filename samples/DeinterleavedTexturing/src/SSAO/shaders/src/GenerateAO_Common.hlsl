//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO\shaders\src/GenerateAO_Common.hlsl
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

#include "ConstantBuffers.hlsl"
#include "FullScreenTriangle_VS.hlsl"

#if DEINTERLEAVED_TEXTURING

Texture2DArray<float>   QuarterResDepthTexture  : register(t0);
Texture2D<float4>       FullResNormalTexture    : register(t1);
sampler                 PointClampSampler       : register(s0);

#else

Texture2D<float>        ViewDepthTexture        : register(t0);
sampler                 PointClampSampler       : register(s0);

Texture2DArray<float4>  RandomTexture           : register(t1);
sampler                 PointWrapSampler        : register(s1);

#endif

//----------------------------------------------------------------------------------
float3 UVToView(float2 UV, float ViewDepth)
{
    UV = g_UVToViewA * UV + g_UVToViewB;
    return float3(UV * ViewDepth, ViewDepth);
}

#if DEINTERLEAVED_TEXTURING

float3 FetchQuarterResViewPos(float2 UV)
{
    float ViewDepth = QuarterResDepthTexture.SampleLevel(PointClampSampler, float3(UV,0), 0);
    return UVToView(UV, ViewDepth);
}

#else //DEINTERLEAVED_TEXTURING

float3 FetchViewPos(float2 UV)
{
    float HardwareDepth = ViewDepthTexture.SampleLevel(PointClampSampler, UV, 0);
    return UVToView(UV, HardwareDepth);
}

float3 MinDiff(float3 P, float3 Pr, float3 Pl)
{
    float3 V1 = Pr - P;
    float3 V2 = P - Pl;
    return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

float3 ReconstructNormal(PostProc_VSOut IN, float3 P)
{
    float3 Pr = FetchViewPos(IN.uv + float2(g_InvFullResolution.x, 0));
    float3 Pl = FetchViewPos(IN.uv + float2(-g_InvFullResolution.x, 0));
    float3 Pt = FetchViewPos(IN.uv + float2(0, g_InvFullResolution.y));
    float3 Pb = FetchViewPos(IN.uv + float2(0, -g_InvFullResolution.y));
    return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

#endif //DEINTERLEAVED_TEXTURING

//----------------------------------------------------------------------------------
float Falloff(float DistanceSquare)
{
    // 1 scalar mad instruction
    return DistanceSquare * g_NegInvR2 + 1.0;
}

//----------------------------------------------------------------------------------
// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
//----------------------------------------------------------------------------------
float ComputeAO(float3 P, float3 N, float3 S)
{
    float3 V = S - P;
    float VdotV = dot(V, V);
    float NdotV = dot(N, V) * rsqrt(VdotV);

    // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
    return saturate(NdotV - g_NDotVBias) * saturate(Falloff(VdotV));
}

//----------------------------------------------------------------------------------
float2 RotateDirection(float2 Dir, float2 CosSin)
{
    return float2(Dir.x*CosSin.x - Dir.y*CosSin.y,
                  Dir.x*CosSin.y + Dir.y*CosSin.x);
}

//----------------------------------------------------------------------------------
float4 GetJitter(PostProc_VSOut IN)
{
#if !USE_RANDOM_TEXTURE
    return float4(1,0,1,1);
#else
    #if DEINTERLEAVED_TEXTURING
        // Get the current jitter vector from the per-pass constant buffer
        return g_Jitter;
    #else
        // (cos(Alpha),sin(Alpha),rand1,rand2)
        return RandomTexture.Sample(PointWrapSampler, float3(IN.pos.xy / RANDOM_TEXTURE_WIDTH, 0));
    #endif
#endif
}

//----------------------------------------------------------------------------------
float ComputeCoarseAO(float2 FullResUV, float RadiusPixels, float4 Rand, float3 ViewPosition, float3 ViewNormal)
{
#if DEINTERLEAVED_TEXTURING
    RadiusPixels /= 4.0;
#endif

    // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

    const float Alpha = 2.0 * M_PI / NUM_DIRECTIONS;
    float AO = 0;

    [unroll]
    for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
    {
        float Angle = Alpha * DirectionIndex;

        // Compute normalized 2D direction
        float2 Direction = RotateDirection(float2(cos(Angle), sin(Angle)), Rand.xy);

        // Jitter starting sample within the first step
        float RayPixels = (Rand.z * StepSizePixels + 1.0);

        [unroll]
        for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
        {
#if DEINTERLEAVED_TEXTURING
            float2 SnappedUV = round(RayPixels * Direction) * g_InvQuarterResolution + FullResUV;
            float3 S = FetchQuarterResViewPos(SnappedUV);
#else
            float2 SnappedUV = round(RayPixels * Direction) * g_InvFullResolution + FullResUV;
            float3 S = FetchViewPos(SnappedUV);
#endif

            RayPixels += StepSizePixels;

            AO += ComputeAO(ViewPosition, ViewNormal, S);
        }
    }

    AO *= g_AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
    return saturate(1.0 - AO * 2.0);
}

//----------------------------------------------------------------------------------
struct GenerateAO_Output
{
#if DEINTERLEAVED_TEXTURING
    float AO : SV_TARGET;
#else
    #if ENABLE_BLUR
        float2 AODepth : SV_TARGET;
    #else
        float4 AO : SV_TARGET;
    #endif
#endif
};

//----------------------------------------------------------------------------------
GenerateAO_Output MAIN_FUNCTION(PostProc_VSOut IN)
{
    GenerateAO_Output OUT;

#if DEINTERLEAVED_TEXTURING
    IN.pos.xy = floor(IN.pos.xy) * 4.0 + g_Float2Offset;
    IN.uv = IN.pos.xy * (g_InvQuarterResolution / 4.0);

    float3 ViewPosition = FetchQuarterResViewPos(IN.uv);
    float4 NormalAndAO = FullResNormalTexture.Load(int3(IN.pos.xy,0));
    float3 ViewNormal = NormalAndAO.xyz * 2.0 - 1.0;
#else
    float3 ViewPosition = FetchViewPos(IN.uv);

    // Reconstruct view-space normal from nearest neighbors
    float3 ViewNormal = ReconstructNormal(IN, ViewPosition);
#endif

    // Compute projection of disk of radius g_R into screen space
    float RadiusPixels = g_RadiusToScreen / ViewPosition.z;

    // Get jitter vector for the current full-res pixel
    float4 Rand = GetJitter(IN);

    float AO = ComputeCoarseAO(IN.uv, RadiusPixels, Rand, ViewPosition, ViewNormal);

#if DEINTERLEAVED_TEXTURING
    OUT.AO = AO;
#else
    #if ENABLE_BLUR
        OUT.AODepth = float2(AO, ViewPosition.z);
    #else
        OUT.AO = pow(AO, g_PowExponent);
    #endif
#endif

    return OUT;
}