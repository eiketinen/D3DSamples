//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\renderers/IC_Instancing_Renderer.h
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
#pragma  once

#include "RendererBase.h"


/*
    This is a Single threaded immediate context renderer that uses instancing as opposed to single draws

    Pseudo code
    --------------

    For each pass, render all meshes with instancing to that pass.  Will always use VTF to encode mesh instance data

*/
class IC_Instancing_Renderer : public RendererBase
{
public:
    IC_Instancing_Renderer() : RendererBase() {};

    virtual MT_RENDER_STRATEGY GetContextType() {return RS_IC_INSTANCING;}

    // Instancing will *always* use VTF
    virtual void SetUseVTF(bool bUseVTF) { DC_UNREFERENCED_PARAM(bUseVTF); RendererBase::SetUseVTF(true);}

    virtual ID3D11PixelShader* PickAppropriatePixelShader();
    virtual ID3D11VertexShader* PickAppropriateVertexShader();

    virtual void OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime);
    virtual HRESULT RenderPassToContext(ID3D11DeviceContext* pd3dContext, int iRenderPass, int iResourceIndex = 0);

};