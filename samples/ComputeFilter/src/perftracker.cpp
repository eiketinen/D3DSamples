//----------------------------------------------------------------------------------
// File:        ComputeFilter\src/perftracker.cpp
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
#include "common_util.h"
#include "PerfTracker.h"

#include <deque>
#include <map>
#include <string>
#include <vector>
#include <list>

////////////////////////////////////////////////////////////////////////////////
namespace {
////////////////////////////////////////////////////////////////////////////////
const char * TWEAK_DLG_NAME = "PerfTracker";
const size_t PERF_STRING_SIZE = 16;

//------------------------------------------------------------------------------
class EventQuery {
private:
    UINT event_id;
    ID3D11Query * gpu_timestamp_begin;
    ID3D11Query * gpu_timestamp_end;
    ID3D11Query * gpu_pipeline_query;
    PerfTracker::CPUTimer cpu_timer;

public:
    EventQuery(ID3D11Device * device) {
        D3D11_QUERY_DESC timestamp_query_desc;
        timestamp_query_desc.Query = D3D11_QUERY_TIMESTAMP;
        timestamp_query_desc.MiscFlags = 0;
        device->CreateQuery(&timestamp_query_desc, &this->gpu_timestamp_begin);
        device->CreateQuery(&timestamp_query_desc, &this->gpu_timestamp_end);

        D3D11_QUERY_DESC pipeline_query_desc;
        pipeline_query_desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
        pipeline_query_desc.MiscFlags = 0;
        device->CreateQuery(&pipeline_query_desc, &this->gpu_pipeline_query);
    };

    ~EventQuery() {
        SAFE_RELEASE(this->gpu_timestamp_begin);
        SAFE_RELEASE(this->gpu_timestamp_end);
        SAFE_RELEASE(this->gpu_pipeline_query);
    };

    UINT get_id () const {
        return event_id;
    }

    void begin(ID3D11DeviceContext * ctx, UINT id) {
        this->event_id = id;
        ctx->Begin(this->gpu_pipeline_query);
        ctx->End(this->gpu_timestamp_begin);
        this->cpu_timer.start();
    };

    void end(ID3D11DeviceContext * ctx) {
        this->cpu_timer.stop();
        ctx->End(this->gpu_timestamp_end);
        ctx->End(this->gpu_pipeline_query);
    };

    float cpu_time() {
        return 1000.f * this->cpu_timer.value();
    };

    float gpu_time(ID3D11DeviceContext * ctx, float frequency) {
        UINT64 gpu_begin_timestamp, gpu_end_timestamp;
        ctx->GetData(this->gpu_timestamp_begin, &gpu_begin_timestamp, sizeof(UINT64), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        ctx->GetData(this->gpu_timestamp_end, &gpu_end_timestamp, sizeof(UINT64), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        return 1000.f * float(gpu_end_timestamp - gpu_begin_timestamp) / frequency;
    };

    void get_measurements(ID3D11DeviceContext * ctx, float gpu_frequency, PerfTracker::PerfMeasurements & out_measurements) {
        out_measurements.cpu_time = this->cpu_time();
        out_measurements.gpu_time = this->gpu_time(ctx, gpu_frequency);

        D3D11_QUERY_DATA_PIPELINE_STATISTICS  pipeline_stats;
        ctx->GetData(this->gpu_pipeline_query, &pipeline_stats, sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        out_measurements.gpu_stats.drawn_vertices = (double) pipeline_stats.IAVertices;
        out_measurements.gpu_stats.drawn_primitives = (double) pipeline_stats.IAPrimitives;
        out_measurements.gpu_stats.shaded_primitives = (double) pipeline_stats.CPrimitives;
        out_measurements.gpu_stats.shaded_fragments = (double) pipeline_stats.PSInvocations;
    };
};

//------------------------------------------------------------------------------

struct FrameQueries {
    EventQuery * total_query;
    ID3D11Query * present_query;
    std::vector <EventQuery *> event_queries;
};

std::deque<FrameQueries> pending_frames;
std::vector<EventQuery *> live_queries;
std::vector<EventQuery *> event_query_pool;
std::vector<ID3D11Query *> disjoint_query_pool;

std::list<PerfTracker::FrameMeasurements> frame_results;

//------------------------------------------------------------------------------

TwBar * tweak_dlg = NULL;
bool tweak_dlg_visible = false;

struct UIEventData {
    std::string name;
    std::string description;
    PerfTracker::PerfMeasurements measurements;
};
std::map<UINT, UIEventData *> tracked_events;

UIEventData * add_perf_event(UINT id, const char * name)
{
    UIEventData * new_event = new UIEventData();
    new_event->name = name;
    new_event->description = name;
    tracked_events[id] = new_event;
    return new_event;
}

void TW_CALL tweakui_get_gpu_event_perf(void * out_var, void * client_data) {
    UIEventData * event_data = (UIEventData *) client_data;
    char * out_string = (char *) out_var;
    _snprintf(out_string, PERF_STRING_SIZE, "%2.3f ms", event_data->measurements.gpu_time);
}

void TW_CALL tweakui_get_cpu_event_perf(void * out_var, void * client_data) {
    UIEventData * event_data = (UIEventData *) client_data;
    char * out_string = (char *) out_var;
    _snprintf(out_string, PERF_STRING_SIZE, "%2.3f ms", event_data->measurements.cpu_time);
}

////////////////////////////////////////////////////////////////////////////////
} namespace PerfTracker {
////////////////////////////////////////////////////////////////////////////////

CPUTimer::SystemInfo CPUTimer::system_info;

EventReference::EventReference(UINT h, const char * s) {
    if (tracked_events.find(h) == ::tracked_events.end()) {
        add_perf_event(h, s);
    } else {
        // if this assert fails, we've got a hash collision :/
        // Since this is debug code, it's easier to just rename one marker
        ASSERT_PRINT(::tracked_events[h]->name.compare(s) == 0, "String Hash Collision @ 0x%08X:\n\"%s\" vs \"%s\"", h, s, ::tracked_events[h]->name.c_str());
    }
}

ScopedEvent::ScopedEvent(ID3D11DeviceContext * c, UINT h, const char * s) {
    this->ctx = c;
    this->handle = h;
    this->name = s;
    event_begin(this->ctx, this->handle);
};

ScopedEvent::~ScopedEvent() {
    event_end(this->ctx);
}

void initialize() {
};

void shutdown() {
    for (auto e = ::live_queries.begin(); e != ::live_queries.end(); ++e) {
        delete *e;
    }
    ::live_queries.clear();
    for (auto f = ::pending_frames.begin(); f != ::pending_frames.end(); ++f) {
        SAFE_RELEASE((*f).present_query);
        for (auto e = (*f).event_queries.begin(); e != (*f).event_queries.end(); ++e) {
            delete (*e);
        }
    }
    ::pending_frames.clear();
    for (auto q = ::disjoint_query_pool.begin(); q != ::disjoint_query_pool.end(); ++q) {
        SAFE_RELEASE((*q));
    }
    ::disjoint_query_pool.clear();
    for (auto e = ::tracked_events.begin(); e != ::tracked_events.end(); ++e) {
        delete (*e).second;
    }
    ::tracked_events.clear();
    ::frame_results.clear();
}

void get_results(std::vector<FrameMeasurements> & out_results) {
    out_results.reserve(::frame_results.size());
    while(frame_results.empty() == false) {
        FrameMeasurements & frame = ::frame_results.front();
        out_results.push_back(frame);
        frame_results.pop_front();
    }
    ::frame_results.clear();
}

void frame_begin(ID3D11DeviceContext * ctx) {
    FrameQueries new_frame;
    if (::disjoint_query_pool.empty()) {
        ID3D11Device * device;
        ctx->GetDevice(&device);
        D3D11_QUERY_DESC query_desc;
        query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        query_desc.MiscFlags = 0;
        device->CreateQuery(&query_desc, &new_frame.present_query);
    } else {
        new_frame.present_query = ::disjoint_query_pool.back();
        ::disjoint_query_pool.pop_back();
    }

    if (::event_query_pool.empty()) {
        ID3D11Device * device;
        ctx->GetDevice(&device);
        new_frame.total_query = new EventQuery(device);
    } else {
        new_frame.total_query = ::event_query_pool.back();
        ::event_query_pool.pop_back();
    }

    if (::pending_frames.empty()) {
        new_frame.event_queries.reserve(::tracked_events.size());
    } else {
        new_frame.event_queries.reserve(::pending_frames.back().event_queries.size());
    }
    ctx->Begin(new_frame.present_query);
    new_frame.total_query->begin(ctx, 0);
    ::pending_frames.push_back(new_frame);
}

void frame_end(ID3D11DeviceContext * ctx) {
    _ASSERT(live_queries.empty());                
    ::pending_frames.back().total_query->end(ctx);
    ctx->End(::pending_frames.back().present_query);
    do {
        FrameQueries & next_frame = ::pending_frames.front();
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT frame_query_data;
        HRESULT hr = ctx->GetData(next_frame.present_query, &frame_query_data, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        float gpu_tick_frequency = (float)frame_query_data.Frequency;
        if (hr == S_OK) {
            if (frame_query_data.Disjoint == FALSE) {
                FrameMeasurements frame_measurements;
                for (auto query = next_frame.event_queries.begin(); query != next_frame.event_queries.end(); ++query) {
                    EventMeasurements event_measurements;
                    event_measurements.id = (*query)->get_id();
                    (*query)->get_measurements(ctx, gpu_tick_frequency, event_measurements.data);
                    frame_measurements.events.push_back(event_measurements);
                }
                next_frame.total_query->get_measurements(ctx, gpu_tick_frequency, frame_measurements.frame_total);
                ::frame_results.push_back(frame_measurements);
            }
            for (auto query = next_frame.event_queries.begin(); query != next_frame.event_queries.end(); ++query) {
                ::event_query_pool.push_back(*query);
            }
            ::event_query_pool.push_back(next_frame.total_query);
            ::disjoint_query_pool.push_back(next_frame.present_query);
            ::pending_frames.pop_front();
        } else {
            break;
        }
    } while (!::pending_frames.empty());
}

void event_begin(ID3D11DeviceContext * ctx, UINT event_id) {
    EventQuery * new_event;
    if (::event_query_pool.empty()) {
        ID3D11Device * device;
        ctx->GetDevice(&device);
        new_event = new EventQuery(device);
    } else {
        new_event = ::event_query_pool.back();
        ::event_query_pool.pop_back();
    }

    new_event->begin(ctx, event_id);
    ::live_queries.push_back(new_event);
}

void event_end(ID3D11DeviceContext * ctx) {
    EventQuery * curr_event = ::live_queries.back();
    curr_event->end(ctx);
    ::pending_frames.back().event_queries.push_back(curr_event);
    ::live_queries.pop_back();
}


void ui_setup(EventDesc * events, size_t event_count, const char * dialog_format) {
    ::tweak_dlg = TwNewBar(TWEAK_DLG_NAME);
    std::string dialog_defines = std::string(TWEAK_DLG_NAME) + " ";
    dialog_defines += "label='Performance' ";
    dialog_defines += "resizable=false ";
    dialog_defines += "movable=false ";
    dialog_defines += "alwaysbottom=true ";
    dialog_defines += "iconified=true ";
    dialog_defines += "color='72 115 1' ";
    dialog_defines += "alpha=32 ";
    dialog_defines += "text=light ";
    dialog_defines += "valueswidth=100 ";
    if (dialog_format) {
        dialog_defines += dialog_format;
    }
    TwDefine(dialog_defines.c_str());
    ::tweak_dlg_visible = false;

    int bar_size[2] = {400, 24+18*2*((int)event_count+3)};
    TwSetParam(::tweak_dlg, nullptr, "size", TW_PARAM_INT32, 2, bar_size);
    int bar_pos[2] = {8, 16};
    TwSetParam(::tweak_dlg, nullptr, "position", TW_PARAM_INT32, 2, bar_pos);

    for (size_t idx=0; idx<event_count; ++idx) {
        EventDesc & desc = events[idx];
        UIEventData * new_event = add_perf_event(desc.id, desc.name);

        std::string gpu_varname = std::string(desc.name) + "-GPU";
        TwAddVarCB(::tweak_dlg, gpu_varname.c_str(), TW_TYPE_CSSTRING(PERF_STRING_SIZE), nullptr, ::tweakui_get_gpu_event_perf, new_event, "group=GPU");
        TwSetParam(::tweak_dlg, gpu_varname.c_str(), "label", TW_PARAM_CSTRING, 1, desc.name);

        std::string cpu_varname = std::string(desc.name) + "-CPU";
        TwAddVarCB(::tweak_dlg, cpu_varname.c_str(), TW_TYPE_CSSTRING(PERF_STRING_SIZE), nullptr, ::tweakui_get_cpu_event_perf, new_event, "group=CPU");
        TwSetParam(::tweak_dlg, cpu_varname.c_str(), "label", TW_PARAM_CSTRING, 1, desc.name);
    }

    UIEventData * total_frame_event = add_perf_event(0, "Total");

    TwAddSeparator(::tweak_dlg, "GPUSeparator", "group=GPU");
    TwAddVarCB(::tweak_dlg, "GPUTotal", TW_TYPE_CSSTRING(PERF_STRING_SIZE), nullptr, ::tweakui_get_gpu_event_perf, total_frame_event, "group=GPU");
    TwSetParam(::tweak_dlg, "GPUTotal", "label", TW_PARAM_CSTRING, 1, "Total GPU Time");

    TwAddSeparator(::tweak_dlg, "CPUSeparator", "group=CPU");
    TwAddVarCB(::tweak_dlg, "CPUTotal", TW_TYPE_CSSTRING(PERF_STRING_SIZE), nullptr, ::tweakui_get_cpu_event_perf, total_frame_event, "group=CPU");
    TwSetParam(::tweak_dlg, "CPUTotal", "label", TW_PARAM_CSTRING, 1, "Total CPU Time");
}

void ui_update(std::vector<PerfTracker::FrameMeasurements> & new_results) {
    std::map<UINT, PerfMeasurements> frame_events_avg;
    frame_events_avg[0] = PerfMeasurements();
    for (auto f = new_results.begin(); f != new_results.end(); ++f) {
        std::map<UINT, PerfMeasurements> frame_events;
        std::map<UINT, UINT> frame_events_count;
        for (auto e=(*f).events.begin(); e!=(*f).events.end(); ++e) {
            if (frame_events.find((*e).id) == frame_events.end()) {
                frame_events[(*e).id] = (*e).data;
                frame_events_count[(*e).id] = 1;
            } else {
                frame_events[(*e).id].accumulate((*e).data);
                frame_events_count[(*e).id] += 1;
            }
        }

        for (auto e=(*f).events.begin(); e!=(*f).events.end(); ++e) {
            PerfMeasurements frame_avg = frame_events[(*e).id];
            frame_avg.scale((float)frame_events_count[(*e).id]);
            if (frame_events.find((*e).id) == frame_events.end()) {
                frame_events_avg[(*e).id] = frame_avg;
            } else {
                frame_events_avg[(*e).id].accumulate(frame_avg);
            }
        }

        frame_events_avg[0].accumulate((*f).frame_total);
    }

    for (auto e=frame_events_avg.begin(); e!=frame_events_avg.end(); ++e) {
        (*e).second.scale((float)new_results.size());
    }

    for (auto e = ::tracked_events.begin(); e != ::tracked_events.end(); ++e) {
        UINT event_id = (*e).first;
        PerfMeasurements & event_measurements = (*e).second->measurements;
        if (frame_events_avg.find(event_id) == frame_events_avg.end()) {
            event_measurements = PerfMeasurements();
        } else {
            event_measurements = frame_events_avg[event_id];
        }
    }
}

void ui_toggle_visibility() {
    ::tweak_dlg_visible = !::tweak_dlg_visible;
    char * ui_state = ::tweak_dlg_visible ? "false" : "true";
    TwSetParam(::tweak_dlg, nullptr, "iconified", TW_PARAM_CSTRING, 1, ui_state);
}

////////////////////////////////////////////////////////////////////////////////
}
