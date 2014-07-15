//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/GPUTimers.cpp
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

#include "GPUTimers.h"
#include <assert.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef SAFE_D3D_CALL
#define SAFE_D3D_CALL(x)     { if (x != S_OK) assert(0); }
#endif

//--------------------------------------------------------------------------------
void GPUTimers::Create(ID3D11Device* pD3DDevice, UINT NumTimers)
{
    m_Timers.resize(NumTimers);

    D3D11_QUERY_DESC queryDesc;
    queryDesc.MiscFlags = 0;

    queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    SAFE_D3D_CALL( pD3DDevice->CreateQuery(&queryDesc, &m_pDisjointTimestampQuery) );
    m_DisjointQueryInFlight = false;

    queryDesc.Query = D3D11_QUERY_TIMESTAMP;
    for (UINT i = 0; i < m_Timers.size(); ++i)
    {
        SAFE_D3D_CALL( pD3DDevice->CreateQuery(&queryDesc, &m_Timers[i].pGPUTimersBegin) );
        SAFE_D3D_CALL( pD3DDevice->CreateQuery(&queryDesc, &m_Timers[i].pGPUTimersEnd) );
        m_Timers[i].TimestampQueryInFlight = false;
    }
}

//--------------------------------------------------------------------------------
void GPUTimers::Release()
{
    SAFE_RELEASE(m_pDisjointTimestampQuery);

    for (UINT i = 0; i < m_Timers.size(); ++i)
    {
        SAFE_RELEASE(m_Timers[i].pGPUTimersBegin);
        SAFE_RELEASE(m_Timers[i].pGPUTimersEnd);
    }

    m_Timers.clear();
}

//--------------------------------------------------------------------------------
void GPUTimers::BeginFrame(ID3D11DeviceContext* pDeviceContext)
{
    for (UINT i = 0; i < m_Timers.size(); ++i)
    {
        m_Timers[i].NumInstancesPerFrame = 0;
    }

    if (!m_DisjointQueryInFlight)
    {
        pDeviceContext->Begin(m_pDisjointTimestampQuery);
    }
}

//--------------------------------------------------------------------------------
void GPUTimers::EndFrame(ID3D11DeviceContext* pDeviceContext)
{
    if (!m_DisjointQueryInFlight)
    {
        pDeviceContext->End(m_pDisjointTimestampQuery);
    }
    m_DisjointQueryInFlight = true;

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointTimestampValue;
    if (pDeviceContext->GetData(m_pDisjointTimestampQuery, &disjointTimestampValue, sizeof(disjointTimestampValue), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK)
    {
        m_DisjointQueryInFlight = false;

        if (!disjointTimestampValue.Disjoint)
        {
            double InvFrequencyMS = 1000.0 / disjointTimestampValue.Frequency;
            for (UINT i = 0; i < m_Timers.size(); ++i)
            {
                if (m_Timers[i].TimestampQueryInFlight)
                {
                    UINT64 TimestampValueBegin;
                    UINT64 TimestampValueEnd;
                    if ((pDeviceContext->GetData(m_Timers[i].pGPUTimersBegin, &TimestampValueBegin, sizeof(UINT64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK) &&
                        (pDeviceContext->GetData(m_Timers[i].pGPUTimersEnd,   &TimestampValueEnd,   sizeof(UINT64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK))
                    {
                        m_Timers[i].TimestampQueryInFlight = false;
                        m_Timers[i].GPUTimeInMS = float(double(TimestampValueEnd - TimestampValueBegin) * InvFrequencyMS);
                    }
                }
                else
                {
                    m_Timers[i].GPUTimeInMS = 0.f;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------
void GPUTimers::StartTimer(ID3D11DeviceContext* pDeviceContext, UINT i)
{
    if (!m_Timers[i].TimestampQueryInFlight)
    {
        pDeviceContext->End(m_Timers[i].pGPUTimersBegin);
    }
}

//--------------------------------------------------------------------------------
void GPUTimers::StopTimer(ID3D11DeviceContext* pDeviceContext, UINT i)
{
    if (!m_Timers[i].TimestampQueryInFlight)
    {
        pDeviceContext->End(m_Timers[i].pGPUTimersEnd);
    }
    m_Timers[i].TimestampQueryInFlight = true;
    m_Timers[i].NumInstancesPerFrame++;
}

//--------------------------------------------------------------------------------
bool GPUTimers::IsValidTimerId(UINT TimerId)
{
    return (TimerId < m_Timers.size());
}

//--------------------------------------------------------------------------------
float GPUTimers::GetGPUTimeInMS(UINT TimerId)
{
    if (!IsValidTimerId(TimerId))
    {
        assert(0);
        return 0.f;
    }

    return m_Timers[TimerId].GPUTimeInMS;
}

//--------------------------------------------------------------------------------
UINT GPUTimers::GetNumInstancesPerFrame(UINT TimerId)
{
    if (!IsValidTimerId(TimerId))
    {
        assert(0);
        return 0;
    }

    return m_Timers[TimerId].NumInstancesPerFrame;
}
