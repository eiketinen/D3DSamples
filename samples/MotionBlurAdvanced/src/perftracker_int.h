//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\src/perftracker_int.h
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

#include <algorithm>
#include <vector>

#include <stdint.h>

#ifdef DISABLE_PERF_TRACKING
    #define PERF_FRAME_BEGIN_IMPL(ctx) ;
    #define PERF_FRAME_END_IMPL(ctx) ;
    #define PERF_EVENT_SCOPED_IMPL(ctx, eventname) ;
    #define PERF_EVENT_BEGIN_IMPL(ctx, eventname) ;
    #define PERF_EVENT_END_IMPL(ctx) ;
#else

namespace PerfTracker {
    class EventReference {
    public:
        EventReference(UINT h, const char * s);
    };
    class ScopedEvent {
    private:
        ID3D11DeviceContext * ctx;
        UINT handle;
        const char * name;
    public:
        ScopedEvent(ID3D11DeviceContext * c, UINT handle, const char * s);
        ~ScopedEvent();
    };
    void frame_begin(ID3D11DeviceContext * ctx);
    void frame_end(ID3D11DeviceContext * ctx);
    void event_begin(ID3D11DeviceContext * ctx, UINT event_id);
    void event_end(ID3D11DeviceContext * ctx);
}

#define H1(s,i,x)   (x*65599u+(uint8_t)s[(i)<strlen(s)?strlen(s)-1-(i):strlen(s)])
#define H4(s,i,x)   H1(s,i,H1(s,i+1,H1(s,i+2,H1(s,i+3,x))))
#define H16(s,i,x)  H4(s,i,H4(s,i+4,H4(s,i+8,H4(s,i+12,x))))
#define H64(s,i,x)  H16(s,i,H16(s,i+16,H16(s,i+32,H16(s,i+48,x))))
#define H256(s,i,x) H64(s,i,H64(s,i+64,H64(s,i+128,H64(s,i+192,x))))
#define HASH_STRING(s)    ((uint32_t)(H256(s,0,0)^(H256(s,0,0)>>16)))

#define PERF_EVENT_VARNAME(x, y) perfevent_line##x##_##y

#define PERF_FRAME_BEGIN_IMPL(ctx) \
    PerfTracker::frame_begin(ctx);

#define PERF_FRAME_END_IMPL(ctx) \
    PerfTracker::frame_end(ctx);

#define PERF_EVENT_SCOPED_IMPL(ctx, eventname, location) \
    static PerfTracker::EventReference PERF_EVENT_VARNAME(location, ref)(HASH_STRING(eventname), eventname); \
    PerfTracker::ScopedEvent PERF_EVENT_VARNAME(location, var)(ctx, HASH_STRING(eventname), eventname);

#define PERF_EVENT_BEGIN_IMPL(ctx, eventname, location) \
    static PerfTracker::EventReference PERF_EVENT_VARNAME(location, ref)(HASH_STRING(eventname), eventname); \
    PerfTracker::event_begin(ctx, HASH_STRING(eventname));

#define PERF_EVENT_END_IMPL(ctx) \
    PerfTracker::event_end(ctx);

#define PERF_EVENT_DESC_IMPL(id) \
    { HASH_STRING(id), id }

#endif