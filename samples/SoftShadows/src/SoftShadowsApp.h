//----------------------------------------------------------------------------------
// File:        SoftShadows\src/SoftShadowsApp.h
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

#include "DXUTApp11.h"
#include "SoftShadowsRenderer.h"

#include <memory>

class SoftShadowsApp
    : public DXUTApp11
{
public:

    SoftShadowsApp();
    virtual ~SoftShadowsApp();

    virtual bool modifyDeviceSettings(DXUTDeviceSettings *pDeviceSettings) override;
    virtual void onFrameMove(double time, float elapsedTime) override;
    virtual LRESULT onWindowsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool *stopProcessing) override;
    virtual void onKeyboardMessage(UINT nChar, bool keyDown, bool altDown) override;
    
    virtual HRESULT onCreateDevice(
        ID3D11Device *device,
        const DXGI_SURFACE_DESC *backBufferSurfaceDesc) override;

    virtual HRESULT onResizedSwapChain(
        ID3D11Device *device,
        IDXGISwapChain *swapChain,
        const DXGI_SURFACE_DESC *backBufferSurfaceDesc) override;

    virtual void onReleasingSwapChain() override;
    virtual void onDestroyDevice() override;
    virtual void onFrameRender(ID3D11Device *device, ID3D11DeviceContext *context, double time, float elapsedTime) override;

    virtual void onGuiEvent(UINT eventId, int controlId, CDXUTControl *control);
    virtual void onSampleGuiEvent(UINT eventId, int controlId, CDXUTControl *control);

private:

    void renderText();

    static void CALLBACK onGuiEvent(UINT eventId, int controlId, CDXUTControl *control, void *userContext);
    static void CALLBACK onSampleGuiEvent(UINT eventId, int controlId, CDXUTControl *control, void *userContext);

    CDXUTDialogResourceManager m_dialogResourceManager; // manager for shared resources of dialogs
    
    CD3DSettingsDlg             m_d3dSettingsDlg;        // Device settings dialog
    CDXUTDialog                 m_hud;                   // manages the 3D   
    CDXUTDialog                 m_sampleUI;              // dialog for sample specific controls
    
    CDXUTSDKMesh                m_knightMesh;
    CDXUTSDKMesh                m_podiumMesh;

    SoftShadowsRenderer            m_renderer;

    unique_ref_ptr<ID3D11Debug>::type m_debug;

    // Direct3D9 resources
    std::unique_ptr<CDXUTTextHelper> m_textHelper;

    bool m_showHelp; // If true, it renders the UI control text
    bool m_drawUI;
};
