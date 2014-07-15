//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\testing/AutomatedTestingHarness.h
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


#include <vector>

#include "DeferredContexts11.h"

class AutomatedTestHarness
{
public:
    AutomatedTestHarness();
    void InitializeExperiments();

    bool FrameUpdate(float elapsedTime); // returns true if new event triggered
    void FrameRender();
    void WriteOutData();
    bool GetSimulationComplete() {return m_complete;};

    int GetCurrentEventIndex() { return m_currentEvent;}
    int GetNumEvents() {return (int)m_events.size();}

    // UI Variables
    int iStartingInstances;
    int iMaxInstances;
    int instSteps;
    bool bLogIncrement;
    int smoothFrames;
    int maxThreads;
    bool bUnifyVSPSCB;
    bool bVaryShaders;
    bool bVTF;
    std::string szTag;
    int updateThreads;
    bool bVaryOnChange;
    int eventChangeFrames;    // dead frames between events to run before testing.  to smooth spikes

    struct SimulationTestEvent
    {
        DWORD instances;
        MT_RENDER_STRATEGY dcType;
        bool bUnifyVSPSCB;
        bool bVaryShaders;
        bool bVTF;
        unsigned char threads;    // only used in dctype for DC_MT_DEFERRED_PER_MESHINSTANCE
        unsigned char updatethreads;
    };

    struct SimulationTestResult : public SimulationTestEvent
    {
        float ms;
        DWORD frames;
    };

    SimulationTestEvent& GetCurrentEvent()
    {
        if((UINT)m_currentEvent >= m_events.size()) return m_events[m_events.size() - 1];

        return m_events[m_currentEvent];
    }

    bool IsComplete() {return m_complete;}

    std::vector<SimulationTestResult> m_results;
    std::vector<SimulationTestEvent> m_events;

    bool m_complete;

    int m_currentEvent;
    int m_accumFrame;
    float m_accumTime;
    int m_accumEventChange;

    char m_szFilename[MAX_PATH];


};