//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/GPUTimers.h
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

typedef int RenderTimeId;

//--------------------------------------------------------------------------------
struct GPUTimerState
{
    bool TimestampQueryInFlight;
    ID3D11Query* pGPUTimersBegin;
    ID3D11Query* pGPUTimersEnd;
    float GPUTimeInMS;
    UINT NumInstancesPerFrame;
};

//--------------------------------------------------------------------------------
class GPUTimers
{
public:
    void Create(ID3D11Device* pD3DDevice, UINT NumTimers);
    void Release();

    void BeginFrame(ID3D11DeviceContext* pDeviceContext);
    void EndFrame(ID3D11DeviceContext* pDeviceContext);

    void StartTimer(ID3D11DeviceContext* pDeviceContext, UINT TimerId);
    void StopTimer(ID3D11DeviceContext* pDeviceContext, UINT TimerId);

    bool IsValidTimerId(UINT TimerId);
    float GetGPUTimeInMS(UINT TimerId);
    UINT GetNumInstancesPerFrame(UINT TimerId);

protected:
    bool m_DisjointQueryInFlight;
    ID3D11Query* m_pDisjointTimestampQuery;
    std::vector<GPUTimerState> m_Timers;
};

//--------------------------------------------------------------------------------
class GPUTimer
{
public:
    GPUTimer(GPUTimers* pGPUTimers, ID3D11DeviceContext* pDeviceContext, RenderTimeId Id)
        : m_pGPUTimers(pGPUTimers)
        , m_pDeviceContext(pDeviceContext)
        , m_RenderTimeId(Id)
    {
        m_pGPUTimers->StartTimer(m_pDeviceContext, m_RenderTimeId);
    }
    ~GPUTimer()
    {
        m_pGPUTimers->StopTimer(m_pDeviceContext, m_RenderTimeId);
    }

private:
    GPUTimers* m_pGPUTimers;
    ID3D11DeviceContext* m_pDeviceContext;
    RenderTimeId m_RenderTimeId;
};
