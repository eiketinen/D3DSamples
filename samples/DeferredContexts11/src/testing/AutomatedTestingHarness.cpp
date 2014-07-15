//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\testing/AutomatedTestingHarness.cpp
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
#include "DeferredContexts11.h"
#pragma warning (disable:4996 4995 4127)
#include <string>
#include "resource.h"
#include <vector>
#include <list>
#include "stdio.h"
#include <iostream>
#include <fstream>

#include <cstdio>
#include <ctime>


#include "AutomatedTestingHarness.h"

AutomatedTestHarness::AutomatedTestHarness()
{
    m_currentEvent = 0;
    m_accumFrame = 0;
    m_accumTime = 0.f;
    eventChangeFrames = 4;

    iMaxInstances = 5000;
    instSteps = 10;
    bLogIncrement = false;
    smoothFrames = 100;
    maxThreads = 2;
    bVaryShaders = false;
    bUnifyVSPSCB = false;
    bVTF = true;
    szTag = "Bingo";
    updateThreads = 4;
    iStartingInstances = 1;
    bVaryOnChange = false;
}

void AutomatedTestHarness::InitializeExperiments()
{
    m_complete = false;
    m_events.clear();
    m_results.clear();

    // Create a new output filename

    std::time_t rawtime;
    std::tm* timeinfo;
    char buffer [80];

    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);

    std::strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", timeinfo);
    std::puts(buffer);

    sprintf(m_szFilename, "DCTests_%s_%s.csv", szTag.c_str(), buffer);

    // Create our script of simulation
    for(int dcType = 0; dcType < RS_MAX; dcType++)
    {
        DWORD numInstances = iStartingInstances;
        DWORD instIncr = max(1, iMaxInstances / instSteps); // $ only used for linear increments
        DWORD threads = 1;

        if(bLogIncrement)
        {
            instIncr = 1;   // updated on the fly
        }

        while(1)
        {
            // Load simulation is always linear progression
            SimulationTestEvent e;
            e.instances = numInstances;
            e.dcType = (MT_RENDER_STRATEGY)dcType;
            e.threads = (unsigned char)threads;
            e.bVaryShaders = bVaryShaders;
            e.bUnifyVSPSCB = bUnifyVSPSCB;
            e.bVTF = bVTF;
            e.updatethreads = (unsigned char)updateThreads;    // $$ Do we care to test this changing over time?
            m_events.push_back(e);

            // linear withing a scale, and then log up.  Gets some local trends as well as covers a larger area in a small number of samples.
            if(bLogIncrement && numInstances >= 10 * instIncr)
            {
                instIncr *= 10;
            }

            if(numInstances >= (DWORD)iMaxInstances) // we did the final run, so break out.
            {
                // only MT will use the threads, so all others just run through instances once
                //  otherwise check our threads count versus the requested max
                if((dcType != RS_MT_BATCHED_INSTANCES) || threads >= (DWORD)maxThreads)
                    break;

                threads++;    // up the threads
                numInstances = iStartingInstances;    // reset instances

                if(bLogIncrement)
                {
                    instIncr = 1;   // updated on the fly
                }
                else
                {
                    instIncr = max(1, iMaxInstances / instSteps);
                }
            }
            else
            {
                numInstances += instIncr; // advance

                if(numInstances > (DWORD)iMaxInstances) // clamp to ensure we get a data point on the final target
                    numInstances = iMaxInstances;
            }
        }
    }
}

void AutomatedTestHarness::FrameRender()
{

}

bool AutomatedTestHarness::FrameUpdate(float fElapsedTime)
{
    if(m_complete) return false;

    // A little dead time between events
    if(--m_accumEventChange > 0) return false;

    // each simulation event smooths over a few frames...
    m_accumTime += fElapsedTime;
    m_accumFrame++;

    // limit step by a fixed time
    if(m_accumFrame < smoothFrames) return false;

    SimulationTestResult r;
    r.instances = m_events[m_currentEvent].instances;
    r.dcType = m_events[m_currentEvent].dcType;
    r.ms = m_accumTime / (float)m_accumFrame;    // average time per frame
    r.frames = m_accumFrame;    // probably wont' use this data
    r.bVaryShaders = m_events[m_currentEvent].bVaryShaders;
    r.bUnifyVSPSCB = m_events[m_currentEvent].bUnifyVSPSCB;
    r.bVTF = m_events[m_currentEvent].bVTF;
    r.threads = m_events[m_currentEvent].threads;
    r.updatethreads = m_events[m_currentEvent].updatethreads;
    m_results.push_back(r);

    m_accumFrame = 0;
    m_accumTime = 0.f;
    m_accumEventChange = eventChangeFrames;

    // Go to the next event
    m_currentEvent++;

    if(m_currentEvent == (int)m_events.size())
    {
        WriteOutData();
        m_complete = true;
        return false;
    }

    return true;
}


static const char *StrategyName(const int strategy)
{
    switch (strategy) {
    case 0:
        return "Imm";
    case 1:
        return "Ins";
    case 2:
        return "Def";
    default:
        break;
    }

    return "Undefined";
}

/*
    Dump each batch run into the output file.
*/
void AutomatedTestHarness::WriteOutData()
{

    std::string output;

    // You can think of the num instances as Y axis and type of rendering as z axis.
    char szLine[MAX_PATH];

    // first run through all samples and get all the discrete instance counts(rows) and render type and thread count (cols)
    output.append("Method(#RenderThread),");
    std::vector<DWORD> instanceCounts;
    struct TYPE_THREAD {int type; int thread;};
    std::vector<TYPE_THREAD> typeThreads;
    int lastType = -1;
    int lastThreads = -1;

    for(unsigned int i = 0; i < m_results.size(); i++)
    {
        bool bFound = false;

        for(unsigned int j = 0; j < instanceCounts.size(); j++)
        {
            if(instanceCounts[j] == m_results[i].instances)
            {
                bFound = true;
                break;
            }
        }

        if(!bFound)
            instanceCounts.push_back(m_results[i].instances);

        // output our headers and make a list to iterate over later
        if(((int)m_results[i].dcType) != lastType || m_results[i].threads != lastThreads)
        {
            sprintf(szLine, "%s(%d),", StrategyName((int)m_results[i].dcType), (int)m_results[i].threads);
            output.append(szLine);
            lastType = m_results[i].dcType;
            lastThreads = m_results[i].threads;
            TYPE_THREAD localTT;
            localTT.type = lastType;
            localTT.thread = lastThreads;
            typeThreads.push_back(localTT);
        }
    }



    // one row for each instance count and columns are render strategies and thread counts
    for(unsigned int iInstanceCountIndex = 0; iInstanceCountIndex < instanceCounts.size(); iInstanceCountIndex++)
    {
        // Prep each line with current instance count
        sprintf(szLine, "\n%d,", (int)instanceCounts[iInstanceCountIndex]);
        output.append(szLine);

        // then add in columns for all instance counts...
        for(unsigned int iTypeThreadIndex = 0; iTypeThreadIndex < typeThreads.size(); iTypeThreadIndex++)
        {
            unsigned int iResultIndexBase = 0;

            for(unsigned int iCurrentResult = iResultIndexBase; iCurrentResult < m_results.size(); iCurrentResult++)
            {
                if(typeThreads[iTypeThreadIndex].type == m_results[iCurrentResult].dcType)
                {
                    if(typeThreads[iTypeThreadIndex].thread == m_results[iCurrentResult].threads)
                    {
                        iResultIndexBase = iCurrentResult;    // save off this base as the progression is the same through results and typeThreads structs

                        if(instanceCounts[iInstanceCountIndex] == m_results[iCurrentResult].instances)
                        {
                            // calculate and output frame time
                            sprintf(szLine, "%f,", m_results[iCurrentResult].ms);
                            output.append(szLine);
                            break;
                        }
                    }
                }
            }
        }
    }

    FILE* pFile;
    pFile = fopen(m_szFilename, "wt");

    if(pFile != NULL)
    {
        // A header with config info
        char line[260];
        sprintf_s(line, "%s,%d\n", "iStartingInstances", iStartingInstances);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "iMaxInstances", iMaxInstances);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "instSteps", instSteps);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "bLogIncrement", bLogIncrement);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "smoothFrames", smoothFrames);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "maxThreads", maxThreads);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "updateThreads", updateThreads);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "bVaryOnChange", bVaryOnChange);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "bVTF", bVTF);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "bVaryShaders", bVaryShaders);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "bUnifyVSPSCB", bUnifyVSPSCB);
        fputs(line, pFile);
        sprintf_s(line, "%s,%d\n", "eventChangeFrames", eventChangeFrames);
        fputs(line, pFile);

        // then the results
        fputs(output.c_str(), pFile);
        fclose(pFile);
    }
}

