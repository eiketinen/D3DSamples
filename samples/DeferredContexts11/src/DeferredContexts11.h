//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src/DeferredContexts11.h
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

#include <assert.h>

// Direct3D9 includes
#include <d3dx9.h>

// Direct3D11 includes
#include <d3dcommon.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>

// XInput includes
#include <xinput.h>

// HRESULT translation for Direct3D and other APIs
#include <dxerr.h>

#ifndef V
#define V(x)           { hr = (x); }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#include "DeviceManager.h"
#include <process.h>

const UINT    g_iMaxInstances = 200000;    // max instances supported by this sample

// The render path options, if you add to this, make sure to add to g_szRenderStrategyNames as well and ensure the same ordering for the GUI to function properly
enum MT_RENDER_STRATEGY
{
    RS_IMMEDIATE = 0,                        // Traditional rendering, one thread, immediate device context
    RS_IC_INSTANCING,                    // Single threaded immediate context rendering using instancing
    RS_MT_BATCHED_INSTANCES,            // Multiple threads, scene divides among threads, update and render batched together to a deferred context
    RS_MAX
};

extern const WCHAR* g_szRenderStrategyNames[];

#define DC_UNREFERENCED_PARAM(A) {(void)(A);}

//--------------------------------------------------------------------------------------
// Helper functions for querying information about the processors in the current
// system.  ( Copied from the doc page for GetLogicalProcessorInformation() )
//--------------------------------------------------------------------------------------
typedef BOOL (WINAPI* LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
    PDWORD);


//  Helper function to count bits in the processor mask
static DWORD CountBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    DWORD bitSetCount = 0;
    DWORD bitTest = 1 << LSHIFT;
    DWORD i;

    for(i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}

inline int GetPhysicalProcessorCount()
{
    DWORD procCoreCount = 0;    // Return 0 on any failure.  That'll show them.

    LPFN_GLPI Glpi;

    Glpi = (LPFN_GLPI) GetProcAddress(
               GetModuleHandle(TEXT("kernel32")),
               "GetLogicalProcessorInformation");

    if(NULL == Glpi)
    {
        // GetLogicalProcessorInformation is not supported
        return procCoreCount;
    }

    BOOL done = FALSE;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    DWORD returnLength = 0;

    while(!done)
    {
        DWORD rc = Glpi(buffer, &returnLength);

        if(FALSE == rc)
        {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if(buffer)
                    free(buffer);

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                             returnLength);

                if(NULL == buffer)
                {
                    // Allocation failure\n
                    return procCoreCount;
                }
            }
            else
            {
                // Unanticipated error
                return procCoreCount;
            }
        }
        else done = TRUE;
    }

    DWORD byteOffset = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;

    while(byteOffset < returnLength)
    {
        if(ptr->Relationship == RelationProcessorCore)
        {
            if(ptr->ProcessorCore.Flags)
            {
                //  Hyperthreading or SMT is enabled.
                //  Logical processors are on the same core.
                procCoreCount += 1;
            }
            else
            {
                //  Logical processors are on different cores.
                procCoreCount += CountBits(ptr->ProcessorMask);
            }
        }

        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    free(buffer);

    return procCoreCount;
}

