//----------------------------------------------------------------------------------
// File:        SoftShadows\src/DXUTApp11.cpp
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
#include "stdafx.h"

#include "DXUTApp11.h"

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::DXUTApp11()
////////////////////////////////////////////////////////////////////////////////
DXUTApp11::DXUTApp11()
{
    auto self = static_cast<void *>(this);
    DXUTSetCallbackDeviceChanging(modifyDeviceSettings, self);
    DXUTSetCallbackMsgProc(onWindowsMessage, self);
    DXUTSetCallbackKeyboard(onKeyboardMessage, self);
    DXUTSetCallbackFrameMove(onFrameMove, self);

    DXUTSetCallbackD3D11DeviceAcceptable(isDeviceAcceptable, self);
    DXUTSetCallbackD3D11DeviceCreated(onCreateDevice, self);
    DXUTSetCallbackD3D11SwapChainResized(onResizedSwapChain, self);
    DXUTSetCallbackD3D11FrameRender(onFrameRender, self);
    DXUTSetCallbackD3D11SwapChainReleasing(onReleasingSwapChain, self);
    DXUTSetCallbackD3D11DeviceDestroyed(onDestroyDevice, self);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::~DXUTApp11()
////////////////////////////////////////////////////////////////////////////////
DXUTApp11::~DXUTApp11()
{
    DXUTSetCallbackDeviceChanging(nullptr);
    DXUTSetCallbackMsgProc(nullptr);
    DXUTSetCallbackKeyboard(nullptr);
    DXUTSetCallbackFrameMove(nullptr);

    DXUTSetCallbackD3D11DeviceAcceptable(nullptr);
    DXUTSetCallbackD3D11DeviceCreated(nullptr);
    DXUTSetCallbackD3D11SwapChainResized(nullptr);
    DXUTSetCallbackD3D11FrameRender(nullptr);
    DXUTSetCallbackD3D11SwapChainReleasing(nullptr);
    DXUTSetCallbackD3D11DeviceDestroyed(nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::run()
////////////////////////////////////////////////////////////////////////////////
int DXUTApp11::run()
{
    DXUTMainLoop(); // Enter into the DXUT render loop
    onReleasingSwapChain();
    onDestroyDevice();
    return DXUTGetExitCode();

}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::modifyDeviceSettings()
////////////////////////////////////////////////////////////////////////////////
bool DXUTApp11::modifyDeviceSettings(DXUTDeviceSettings *deviceSettings, void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    return self->modifyDeviceSettings(deviceSettings);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onFrameMove()
////////////////////////////////////////////////////////////////////////////////
void DXUTApp11::onFrameMove(double time, float elapsedTime, void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    self->onFrameMove(time, elapsedTime);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onWindowsMessage()
////////////////////////////////////////////////////////////////////////////////
LRESULT DXUTApp11::onWindowsMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool *stopProcessing, void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    return self->onWindowsMessage(hWnd, uMsg, wParam, lParam, stopProcessing);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onKeyboardMessage()
////////////////////////////////////////////////////////////////////////////////
void DXUTApp11::onKeyboardMessage(UINT nChar, bool keyDown, bool altDown, void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    self->onKeyboardMessage(nChar, keyDown, altDown);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::isDeviceAcceptable()
////////////////////////////////////////////////////////////////////////////////
bool DXUTApp11::isDeviceAcceptable(
    const CD3D11EnumAdapterInfo *adapterInfo,
    UINT output,
    const CD3D11EnumDeviceInfo *deviceInfo,
    DXGI_FORMAT backBufferFormat, bool windowed,
    void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    return self->isDeviceAcceptable(adapterInfo, output, deviceInfo, backBufferFormat, windowed);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onCreateDevice()
////////////////////////////////////////////////////////////////////////////////
HRESULT DXUTApp11::onCreateDevice(
    ID3D11Device *device,
    const DXGI_SURFACE_DESC *backBufferSurfaceDesc,
    void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    return self->onCreateDevice(device, backBufferSurfaceDesc);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onResizedSwapChain()
////////////////////////////////////////////////////////////////////////////////
HRESULT DXUTApp11::onResizedSwapChain(
    ID3D11Device *device,
    IDXGISwapChain *swapChain,
    const DXGI_SURFACE_DESC *backBufferSurfaceDesc,
    void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    return self->onResizedSwapChain(device, swapChain, backBufferSurfaceDesc);
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onReleasingSwapChain()
////////////////////////////////////////////////////////////////////////////////
void DXUTApp11::onReleasingSwapChain(void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    self->onReleasingSwapChain();
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onDestroyDevice()
////////////////////////////////////////////////////////////////////////////////
void DXUTApp11::onDestroyDevice(void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    return self->onDestroyDevice();
}

////////////////////////////////////////////////////////////////////////////////
// DXUTApp11::onFrameRender()
////////////////////////////////////////////////////////////////////////////////
void DXUTApp11::onFrameRender(ID3D11Device *device, ID3D11DeviceContext *deviceContext, double time, float elapsedTime, void *userContext)
{
    auto self = static_cast<DXUTApp11 *>(userContext);
    return self->onFrameRender(device, deviceContext, time, elapsedTime);
}