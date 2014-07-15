//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\src/perftracker.h
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
#include <AntTweakBar.h>
#include <d3d11.h>

#include "perftracker_int.h"

namespace PerfTracker {
    class CPUTimer {
    private:
        class SystemInfo {
        public:
            float freq_scale;
            SystemInfo() {
                LARGE_INTEGER f;
                QueryPerformanceFrequency(&f);
                this->freq_scale = 1.f / float(f.QuadPart);
            }
        };
        static SystemInfo system_info;

        LARGE_INTEGER start_time;
        LARGE_INTEGER stop_time;

    public:

        void start() {
            QueryPerformanceCounter(&this->start_time);
        };

        void stop() {
            QueryPerformanceCounter(&this->stop_time);
        };

        float value() {
            float dt = max(0.f, float(stop_time.QuadPart) - float(start_time.QuadPart));
            return dt * CPUTimer::system_info.freq_scale;
        };
    };

    struct PerfMeasurements {
        double cpu_time;
        double gpu_time;
        struct {
            double drawn_vertices;
            double drawn_primitives;
            double shaded_primitives;
            double shaded_fragments;
        } gpu_stats;

        PerfMeasurements() {
            this->cpu_time = 0;
            this->gpu_time = 0;
            this->gpu_stats.drawn_vertices = 0;
            this->gpu_stats.drawn_primitives = 0;
            this->gpu_stats.shaded_primitives = 0;
            this->gpu_stats.shaded_fragments = 0;
        }

        void accumulate(const PerfMeasurements & other) {
            this->cpu_time += other.cpu_time;
            this->gpu_time += other.gpu_time;
            this->gpu_stats.drawn_vertices += other.gpu_stats.drawn_vertices;
            this->gpu_stats.drawn_primitives += other.gpu_stats.drawn_primitives;
            this->gpu_stats.shaded_primitives += other.gpu_stats.shaded_primitives;
            this->gpu_stats.shaded_fragments += other.gpu_stats.shaded_fragments;
        };

        void scale(float scale) {
            this->cpu_time /= scale;
            this->gpu_time /= scale;
            this->gpu_stats.drawn_vertices /= scale;
            this->gpu_stats.drawn_primitives /= scale;
            this->gpu_stats.shaded_primitives /= scale;
            this->gpu_stats.shaded_fragments /= scale;
        }
    };

    struct EventMeasurements {
        UINT id;
        PerfMeasurements data;
    };

    struct FrameMeasurements {
        PerfMeasurements frame_total;
        std::vector<EventMeasurements> events;
    };

    struct EventDesc {
        UINT id;
        char * name;
    };

    void initialize();
    void shutdown();
    void get_results(std::vector<FrameMeasurements> & out_results);
    void ui_setup(EventDesc * events, size_t event_count, const char * dialog_prefs);
    void ui_update(std::vector<PerfTracker::FrameMeasurements> & new_results);
    void ui_toggle_visibility();
}

#define PERF_EVENT_DESC(id) \
    PERF_EVENT_DESC_IMPL(id)

#define PERF_FRAME_BEGIN(ctx) \
    PERF_FRAME_BEGIN_IMPL(ctx)

#define PERF_FRAME_END(ctx) \
    PERF_FRAME_END_IMPL(ctx)

#define PERF_EVENT_SCOPED(ctx, eventname) \
    PERF_EVENT_SCOPED_IMPL(ctx, eventname, __LINE__)

#define PERF_EVENT_BEGIN(ctx, eventname) \
    PERF_EVENT_BEGIN_IMPL(ctx, eventname, __LINE__)

#define PERF_EVENT_END(ctx) \
    PERF_EVENT_END_IMPL(ctx)
