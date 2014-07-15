//----------------------------------------------------------------------------------
// File:        SoftShadows\src/DXUTApp11.h
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

class DXUTApp11
{
public:

    DXUTApp11();
    virtual ~DXUTApp11();

    int run();

    virtual bool modifyDeviceSettings(DXUTDeviceSettings *pDeviceSettings) { return true; }
    virtual void onFrameMove(double time, float elapsedTime) {}
    virtual LRESULT onWindowsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool *stopProcessing) { return 0; }
    virtual void onKeyboardMessage(UINT nChar, bool keyDown, bool altDown) {}
    
    virtual bool isDeviceAcceptable(
        const CD3D11EnumAdapterInfo *adapterInfo,
        UINT output,
        const CD3D11EnumDeviceInfo *deviceInfo,
        DXGI_FORMAT backBufferFormat,
        bool windowed) { return true; }

    virtual HRESULT onCreateDevice(
        ID3D11Device *device,
        const DXGI_SURFACE_DESC *backBufferSurfaceDesc) { return S_OK; }

    virtual HRESULT onResizedSwapChain(
        ID3D11Device *device,
        IDXGISwapChain *swapChain,
        const DXGI_SURFACE_DESC *backBufferSurfaceDesc) { return S_OK; }

    virtual void onReleasingSwapChain() {}
    virtual void onDestroyDevice() {}
    virtual void onFrameRender(ID3D11Device *device, ID3D11DeviceContext *context, double time, float elapsedTime) {}

private:

    static bool CALLBACK modifyDeviceSettings(DXUTDeviceSettings *deviceSettings, void *userContext);
    static void CALLBACK onFrameMove(double time, float elapsedTime, void *userContext);
    static LRESULT CALLBACK onWindowsMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool *stopProcessing, void *userContext);
    static void CALLBACK onKeyboardMessage(UINT nChar, bool keyDown, bool altDown, void *userContext);
    
    static bool CALLBACK isDeviceAcceptable(
        const CD3D11EnumAdapterInfo *adapterInfo,
        UINT output,
        const CD3D11EnumDeviceInfo *deviceInfo,
       DXGI_FORMAT backBufferFormat,
       bool windowed,
       void *userContext);

    static HRESULT CALLBACK onCreateDevice(
        ID3D11Device *device,
        const DXGI_SURFACE_DESC *backBufferSurfaceDesc,
        void *userContext);

    static HRESULT CALLBACK onResizedSwapChain(
        ID3D11Device *device,
        IDXGISwapChain *swapChain,
        const DXGI_SURFACE_DESC *backBufferSurfaceDesc,
        void *userContext);

    static void CALLBACK onReleasingSwapChain(void *userContext);
    static void CALLBACK onDestroyDevice(void *userContext);
    static void CALLBACK onFrameRender(ID3D11Device *device, ID3D11DeviceContext *deviceContext, double time, float elapsedTime, void *userContext);
};
