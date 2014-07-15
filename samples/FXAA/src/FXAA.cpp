//----------------------------------------------------------------------------------
// File:        FXAA\src/FXAA.cpp
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

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"
#include <vector>

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CModelViewerCamera          g_Camera;                     // A model viewing camera
CModelViewerCamera          g_LightCamera;                // Light camera
CDXUTDialogResourceManager  g_DialogResourceManager;      // Manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;                // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                        // Dialog for standard controls
CDXUTDialog                 g_SampleUI;                   // Dialog for sample specific controls

// Mesh
CDXUTSDKMesh                g_Mesh;

// Direct3D 11 resources
ID3D11VertexShader*         g_pVertexShaderShadow = NULL; // Vertex shader for shadowmap rendering
ID3D11VertexShader*         g_pVertexShader = NULL;       // Vertex shader for main scene rendering
ID3D11VertexShader*         g_pVertexShaderFXAA = NULL;   // Vertex shader for FXAA rendering
ID3D11PixelShader*          g_pPixelShader;               // Pixel shader for main scene rendering
ID3D11PixelShader*          g_pPixelShaderFXAA;           // Pixel shader for FXAA rendering

ID3D11InputLayout*          g_pVertexLayout = NULL;       // Vertex layout object

ID3D11SamplerState*         g_pSamPointMirror = NULL;     // Sampler for rotation texture
ID3D11SamplerState*         g_pSamLinearWrap = NULL;      // Sampler for diffuse texture
ID3D11SamplerState*         g_pSamPointCmpClamp = NULL;   // Comparison sampler for shadowmap
ID3D11SamplerState*         g_pSamBilinear = NULL;        // Sampler for FXAA input texture

// States
ID3D11BlendState*           g_pColorWritesOn  = NULL;
ID3D11BlendState*           g_pColorWritesOff = NULL;
ID3D11RasterizerState*      g_pCullBack  = NULL;
ID3D11RasterizerState*      g_pCullFront = NULL;

// Shadow map resources
const UINT                  g_uShadowMapSize = 2048;

// Shadow map depth stencil texture data
ID3D11Texture2D*            g_pDepthTexture    = NULL;
ID3D11ShaderResourceView*   g_pDepthTextureSRV = NULL; // SRV of the shadow map
ID3D11DepthStencilView*     g_pDepthTextureDSV = NULL; // DSV of the shadow map

// Random rotation texture
const UINT                  g_uRandomRotSize = 16;
ID3D11Texture2D*            g_pRandomRotTexture    = NULL;
ID3D11ShaderResourceView*   g_pRandomRotTextureSRV = NULL;

// FXAA toggle
int                         g_fxaaEnabled = 1;

// extra FXAA resource
ID3D11Texture2D*            g_pProxyTexture = NULL;
ID3D11ShaderResourceView*    g_pProxyTextureSRV = NULL;
ID3D11Texture2D*            g_pCopyResolveTexture = NULL;
ID3D11ShaderResourceView*    g_pCopyResolveTextureSRV = NULL;
ID3D11RenderTargetView*     g_pProxyTextureRTV = NULL;


//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
#pragma pack(push,1)
struct CB_CONSTANTS
{
    D3DXMATRIX  m_mWorldViewProj;
    D3DXMATRIX  m_mWorldViewProjLight;
    D3DXVECTOR4 m_MaterialAmbientColor;
    D3DXVECTOR4 m_MaterialDiffuseColor;
    D3DXVECTOR4 m_vLightPos;
    D3DXVECTOR4 m_LightDiffuse;
    float       m_fInvRandomRotSize;
    float       m_fInvShadowMapSize;
    float       m_fFilterWidth;
    float       m_pad;
};

struct CB_FXAA
{
    D3DXVECTOR4 m_fxaa;
};
#pragma pack(pop)

ID3D11Buffer*               g_pcbConstants = NULL;
ID3D11Buffer*               g_pcbFXAA = NULL;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_CHANGEDEVICE        2
#define IDC_TOGGLEFXAA          3

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext );

void InitApp();
void RenderText();

//--------------------------------------------------------------------------------------
// Shader helper
//--------------------------------------------------------------------------------------

HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    DXUTSetIsInGammaCorrectMode( FALSE );

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"FXAA" );

    DXUTSetMediaSearchPath(L"..\\..\\fxaa\\media");

    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 1280, 720 );

    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent );

    int iY = 30;
    int iYo = 26;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY,        170, 22 );
    g_HUD.AddButton( IDC_CHANGEDEVICE,     L"Change device (F2)", 0, iY += iYo, 170, 22, VK_F2 );
    g_HUD.AddCheckBox( IDC_TOGGLEFXAA,     L"Toggle FXAA (F3)",   0, iY += iYo, 170, 22, true, VK_F3 );

    //g_Camera.SetRotateButtons( true, false, false );
    g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );
    g_LightCamera.SetButtonMasks( MOUSE_RIGHT_BUTTON, 0, 0 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye( 7.0f, 7.0f, -7.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );

    // Setup light camera's view parameters
    D3DXVECTOR3 vecEyeLight( 9.0f, 15.0f, 9.0f );
    D3DXVECTOR3 vecAtLight ( 0.0f, 0.0f, 0.0f );
    g_LightCamera.SetViewParams( &vecEyeLight, &vecAtLight );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


void FxaaIntegrateResource(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
    D3D11_TEXTURE2D_DESC desc;
    ::ZeroMemory (&desc, sizeof (desc));
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Height = pBackBufferSurfaceDesc->Height;
    desc.Width = pBackBufferSurfaceDesc->Width;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels = 1;
    pd3dDevice->CreateTexture2D( &desc, 0, &g_pCopyResolveTexture );
    desc.SampleDesc.Count = pBackBufferSurfaceDesc->SampleDesc.Count;
    desc.SampleDesc.Quality = pBackBufferSurfaceDesc->SampleDesc.Quality;
    desc.MipLevels = 1;
    pd3dDevice->CreateTexture2D( &desc, 0, &g_pProxyTexture );
    pd3dDevice->CreateRenderTargetView( g_pProxyTexture, 0, &g_pProxyTextureRTV );
    pd3dDevice->CreateShaderResourceView( g_pProxyTexture, 0, &g_pProxyTextureSRV);
    pd3dDevice->CreateShaderResourceView( g_pCopyResolveTexture, 0, &g_pCopyResolveTextureSRV);
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    FxaaIntegrateResource(pd3dDevice, pBackBufferSurfaceDesc);


    // Create shadowmap surface and views
    D3D11_TEXTURE2D_DESC surfDesc;
    surfDesc.Width  = g_uShadowMapSize;
    surfDesc.Height = g_uShadowMapSize;
    surfDesc.MipLevels = 1;
    surfDesc.ArraySize = 1;
    surfDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    surfDesc.SampleDesc.Count = 1;
    surfDesc.SampleDesc.Quality = 0;
    surfDesc.Usage = D3D11_USAGE_DEFAULT;
    surfDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    surfDesc.CPUAccessFlags = 0;
    surfDesc.MiscFlags = 0;
    pd3dDevice->CreateTexture2D( &surfDesc, 0, &g_pDepthTexture );

    // Create depth stencil view for shadowmap rendering
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = 0;
    dsvDesc.Texture2D.MipSlice = 0;
    pd3dDevice->CreateDepthStencilView( g_pDepthTexture, &dsvDesc, & g_pDepthTextureDSV );

    // Create shader resource view for shadowmap
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    pd3dDevice->CreateShaderResourceView( g_pDepthTexture, &srvDesc, &g_pDepthTextureSRV );

    // Create random rotation texture surface and views
    surfDesc.Width  = g_uRandomRotSize;
    surfDesc.Height = g_uRandomRotSize;
    surfDesc.MipLevels = 1;
    surfDesc.ArraySize = 1;
    surfDesc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
    surfDesc.SampleDesc.Count = 1;
    surfDesc.SampleDesc.Quality = 0;
    surfDesc.Usage = D3D11_USAGE_DEFAULT;
    surfDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    surfDesc.CPUAccessFlags = 0;
    surfDesc.MiscFlags = 0;

    // Prepare random rotation texture
    DWORD dwData[g_uRandomRotSize * g_uRandomRotSize];

    for (UINT i = 0; i<g_uRandomRotSize * g_uRandomRotSize; i++)
    {
        float ang = (float)rand() / RAND_MAX * D3DX_PI * 2;

        float c = cosf(ang);
        float s = sinf(ang);

        INT x1 = (INT)(127.f * c);
        INT y1 = (INT)(127.f * s);
        INT x2 = (INT)(-127.f * s);
        INT y2 = (INT)(127.f * c);

        dwData[i] = (DWORD)(((x1 & 0xff) << 24) | ((y1 & 0xff) << 16) | ((x2 & 0xff) << 8) | (y2 & 0xff));
    }

    D3D11_SUBRESOURCE_DATA initialData;
    initialData.pSysMem = dwData;
    initialData.SysMemPitch = g_uRandomRotSize * sizeof(DWORD);

    pd3dDevice->CreateTexture2D(&surfDesc, &initialData, &g_pRandomRotTexture );

    // Create shader resource view for the random rotation texture
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    pd3dDevice->CreateShaderResourceView( g_pRandomRotTexture, &srvDesc, &g_pRandomRotTextureSRV );

    // Read the HLSL file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"ModifiedJitteredShadowSampling.hlsl" ) );

    // Compile the shaders
    DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif

    // Create a layout for the object data
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    // Load and compiler shaders
    ID3DBlob* pBlob = NULL;

    if (pd3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
        V_RETURN( D3DX11CompileFromFile( str, NULL, NULL, "ShadowMapVS", "vs_4_0", dwShaderFlags, 0, NULL, &pBlob, NULL, NULL ) )
    else
        V_RETURN( D3DX11CompileFromFile( str, NULL, NULL, "ShadowMapVS", "vs_5_0", dwShaderFlags, 0, NULL, &pBlob, NULL, NULL ) );
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVertexShaderShadow ) );
    SAFE_RELEASE( pBlob );

    if (pd3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
        V_RETURN( D3DX11CompileFromFile( str, NULL, NULL, "RenderSceneVS", "vs_4_0", dwShaderFlags, 0, NULL, &pBlob, NULL, NULL ) )
    else
        V_RETURN( D3DX11CompileFromFile( str, NULL, NULL, "RenderSceneVS", "vs_5_0", dwShaderFlags, 0, NULL, &pBlob, NULL, NULL ) );
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVertexShader ) );
    V_RETURN( pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &g_pVertexLayout ) );
    SAFE_RELEASE( pBlob );

    if (pd3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
        V_RETURN( D3DX11CompileFromFile( str, NULL, NULL, "RenderScenePS", "ps_4_0", dwShaderFlags, 0, NULL, &pBlob, NULL, NULL ) )
    else
        V_RETURN( D3DX11CompileFromFile( str, NULL, NULL, "RenderScenePS", "ps_5_0", dwShaderFlags, 0, NULL, &pBlob, NULL, NULL ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &(g_pPixelShader) ) );
    SAFE_RELEASE( pBlob );
    
    if (pd3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
    {
        V_RETURN( CompileShaderFromFile( L"FXAA.hlsl", "FxaaVS", "vs_4_0", &pBlob ) )
        V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVertexShaderFXAA ) );

        V_RETURN( CompileShaderFromFile( L"FXAA.hlsl", "FxaaPS", "ps_4_0", &pBlob ) )
        V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pPixelShaderFXAA ) );
    }
    else
    {
        V_RETURN( CompileShaderFromFile( L"FXAA.hlsl", "FxaaVS", "vs_5_0", &pBlob ) );
        V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVertexShaderFXAA ) );

        V_RETURN( CompileShaderFromFile( L"FXAA.hlsl", "FxaaPS", "ps_5_0", &pBlob ) )
        V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pPixelShaderFXAA ) );
    }
    SAFE_RELEASE( pBlob );


    // Load the mesh
    V_RETURN( g_Mesh.Create( pd3dDevice, L"Crypt\\Crypt.sdkmesh", true ) );


    // Create sampler objects
    D3D11_SAMPLER_DESC samDesc;

    ZeroMemory( &samDesc, sizeof(samDesc) );
    samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
    samDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
    samDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
    samDesc.MipLODBias = 0.0f;
    samDesc.MaxAnisotropy = 1;
    samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samDesc.BorderColor[0] = samDesc.BorderColor[1] = samDesc.BorderColor[2] = samDesc.BorderColor[3] = 0;
    samDesc.MinLOD = 0;
    samDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN( pd3dDevice->CreateSamplerState( &samDesc, &g_pSamPointMirror ) );

    samDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    V_RETURN( pd3dDevice->CreateSamplerState( &samDesc, &g_pSamBilinear ) );

    samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    V_RETURN( pd3dDevice->CreateSamplerState( &samDesc, &g_pSamLinearWrap ) );

    samDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    samDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
    V_RETURN( pd3dDevice->CreateSamplerState( &samDesc, &g_pSamPointCmpClamp ) );

    // Create blend state objects
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory( &blendDesc, sizeof( blendDesc ) );
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    V_RETURN( pd3dDevice->CreateBlendState( &blendDesc, &g_pColorWritesOn ) );

    blendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
    V_RETURN( pd3dDevice->CreateBlendState( &blendDesc, &g_pColorWritesOff ) );

    // Create rasterizer state objects
    D3D11_RASTERIZER_DESC rastDesc;
    ZeroMemory( &rastDesc, sizeof( rastDesc ) );
    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.AntialiasedLineEnable = FALSE;
    rastDesc.DepthBias = 0;
    rastDesc.DepthBiasClamp = 0;
    rastDesc.DepthClipEnable = TRUE;
    rastDesc.FrontCounterClockwise = FALSE;
    rastDesc.MultisampleEnable = TRUE;
    rastDesc.ScissorEnable = FALSE;
    rastDesc.SlopeScaledDepthBias = 0;
    V_RETURN( pd3dDevice->CreateRasterizerState( &rastDesc, &g_pCullBack ) );

    rastDesc.CullMode = D3D11_CULL_FRONT;
    V_RETURN( pd3dDevice->CreateRasterizerState( &rastDesc, &g_pCullFront ) );

    // Create constant buffer
    D3D11_BUFFER_DESC cbDesc;
    ZeroMemory( &cbDesc, sizeof(cbDesc) );
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    cbDesc.ByteWidth = sizeof( CB_CONSTANTS );
    V_RETURN( pd3dDevice->CreateBuffer( &cbDesc, NULL, &g_pcbConstants ) );

    cbDesc.ByteWidth = sizeof( CB_FXAA );
    V_RETURN( pd3dDevice->CreateBuffer( &cbDesc, NULL, &g_pcbFXAA ) );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 1000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    g_LightCamera.SetProjParams( D3DX_PI / 4, 1.0f, 0.1f, 1000.0f );
    //g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );


    SAFE_RELEASE( g_pProxyTexture );
    SAFE_RELEASE( g_pProxyTextureSRV );
    SAFE_RELEASE( g_pProxyTextureRTV );
    SAFE_RELEASE( g_pCopyResolveTexture );
    SAFE_RELEASE( g_pCopyResolveTextureSRV );
    FxaaIntegrateResource(pd3dDevice, pBackBufferSurfaceDesc);


    return S_OK;
}

//--------------------------------------------------------------------------------------
// Renders the depth only shadow map
//--------------------------------------------------------------------------------------
void RenderShadowMap( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext )
{
    D3D11_RECT oldRects[1];
    D3D11_VIEWPORT oldVp[1];
    UINT num = 1;
    pd3dImmediateContext->RSGetScissorRects( &num, oldRects );
    num = 1;
    pd3dImmediateContext->RSGetViewports( &num, oldVp );

    D3D11_RECT rects[1] = { { 0, g_uShadowMapSize, 0, g_uShadowMapSize } };
    pd3dImmediateContext->RSSetScissorRects( 1, rects );

    D3D11_VIEWPORT vp[1] = { { 0, 0, (float)g_uShadowMapSize, (float)g_uShadowMapSize, 0.0f, 1.0f } };
    pd3dImmediateContext->RSSetViewports( 1, vp );

    // Set our scene render target & keep original depth buffer
    ID3D11RenderTargetView * pRTVs[2] = { 0, 0 };
    pd3dImmediateContext->OMSetRenderTargets( 1, pRTVs, g_pDepthTextureDSV );
    
    // Clear the render target
    pd3dImmediateContext->ClearDepthStencilView( g_pDepthTextureDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    // Set the shaders
    pd3dImmediateContext->VSSetShader( g_pVertexShaderShadow, NULL, 0 );
    pd3dImmediateContext->PSSetShader( NULL, NULL, 0 );

    pd3dImmediateContext->OMSetBlendState( g_pColorWritesOff, 0, 0xffffffff );
    pd3dImmediateContext->RSSetState( g_pCullFront );

    // Render the scene
    g_Mesh.Render( pd3dImmediateContext, 0 );

    pd3dImmediateContext->OMSetBlendState(g_pColorWritesOn, 0, 0xffffffff);
    pd3dImmediateContext->RSSetState( g_pCullBack );

    // reset the old viewport etc.
    pd3dImmediateContext->RSSetScissorRects( 1, oldRects );
    pd3dImmediateContext->RSSetViewports( 1, oldVp );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }       

    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();

    if(g_fxaaEnabled)
    {
        pRTV = g_pProxyTextureRTV;
    }

    float ClearColor[4] = { sqrt(0.25f), sqrt(0.25f), sqrt(0.5f), 0.0f };
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    D3DXMATRIXA16 mLightWorldView = *g_LightCamera.GetWorldMatrix() * *g_LightCamera.GetViewMatrix();
    D3DXMATRIXA16 mLightWorldViewInv;
    D3DXMatrixInverse( &mLightWorldViewInv, NULL, &mLightWorldView );

    D3DXVECTOR4 vLightPos;
    D3DXVECTOR3 vOrigin(0, 0, 0);
    D3DXVec3Transform( &vLightPos, &vOrigin, &mLightWorldViewInv );

    // Update constant buffers
    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    V( pd3dImmediateContext->Map( g_pcbConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_CONSTANTS* pConstants = ( CB_CONSTANTS* )MappedResource.pData;
    pConstants->m_LightDiffuse = D3DXVECTOR4( 1.f, 1.f, 1.f, 1.f );
    pConstants->m_mWorldViewProj = *g_Camera.GetWorldMatrix() * *g_Camera.GetViewMatrix() * *g_Camera.GetProjMatrix();
    pConstants->m_mWorldViewProjLight = *g_LightCamera.GetWorldMatrix() * *g_LightCamera.GetViewMatrix() * *g_LightCamera.GetProjMatrix();
    pConstants->m_vLightPos = vLightPos;
    pConstants->m_MaterialAmbientColor = D3DXVECTOR4( 0.2f, 0.2f, 0.2f, 1.0f );
    pConstants->m_MaterialDiffuseColor = D3DXVECTOR4( 0.8f, 0.8f, 0.8f, 1.0f );
    pConstants->m_fFilterWidth = 10.0f; // in texels
    pConstants->m_fInvRandomRotSize = 1.0f / g_uRandomRotSize;
    pConstants->m_fInvShadowMapSize = 1.0f / g_uShadowMapSize;
    pd3dImmediateContext->Unmap( g_pcbConstants, 0 );

    pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &g_pcbConstants );
    pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &g_pcbConstants );

    V( pd3dImmediateContext->Map( g_pcbFXAA, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_FXAA* pFXAA = ( CB_FXAA* )MappedResource.pData;
    float frameWidth = (float)DXUTGetDXGIBackBufferSurfaceDesc()->Width;
    float frameHeight = (float)DXUTGetDXGIBackBufferSurfaceDesc()->Height;
    pFXAA->m_fxaa = D3DXVECTOR4(1.0f/frameWidth, 1.0f/frameHeight, 0.0f, 0.0f);
    pd3dImmediateContext->Unmap( g_pcbFXAA, 0 );

    pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &g_pcbFXAA );
    pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &g_pcbFXAA );


    // Set render resources
    pd3dImmediateContext->IASetInputLayout( g_pVertexLayout );

    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    pVB[0] = g_Mesh.GetVB11( 0, 0 );
    Strides[0] = ( UINT )g_Mesh.GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    pd3dImmediateContext->IASetIndexBuffer( g_Mesh.GetIB11( 0 ), g_Mesh.GetIBFormat11( 0 ), 0 );

    // Render shadowmap
    RenderShadowMap( pd3dDevice, pd3dImmediateContext );

    // Rebind to original back buffer and depth buffer
    ID3D11RenderTargetView * pRTVs[2] = { pRTV, NULL };

    if(g_fxaaEnabled)
    {
        pRTVs[0] = g_pProxyTextureRTV;
    }

    pd3dImmediateContext->OMSetRenderTargets( 1, pRTVs, pDSV );
    pd3dImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
    pd3dImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
    
    ID3D11SamplerState * ppSamplerStates[4] = { g_pSamPointMirror, g_pSamLinearWrap, g_pSamPointCmpClamp, g_pSamBilinear };
    pd3dImmediateContext->PSSetSamplers( 0, 4, ppSamplerStates );

    // set the shadow map
    ID3D11ShaderResourceView * ppResources[2] = { g_pDepthTextureSRV, g_pRandomRotTextureSRV };
    pd3dImmediateContext->PSSetShaderResources( 1, 2, ppResources );

    g_Mesh.Render( pd3dImmediateContext );

    // reset shadowmap
    ID3D11ShaderResourceView* pNULL = NULL;
    pd3dImmediateContext->PSSetShaderResources( 1, 1, &pNULL );

    // apply FXAA
    if(g_fxaaEnabled)
    {
        ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
        pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV, 0 );
        pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
        pd3dImmediateContext->VSSetShader( g_pVertexShaderFXAA, NULL, 0 );
        pd3dImmediateContext->PSSetShader( g_pPixelShaderFXAA, NULL, 0 );
        if(DXUTGetDXGIBackBufferSurfaceDesc()->SampleDesc.Count > 1) 
        {
            // resolve first
            pd3dImmediateContext->ResolveSubresource(g_pCopyResolveTexture, 0, g_pProxyTexture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
            pd3dImmediateContext->PSSetShaderResources( 0, 1, &g_pCopyResolveTextureSRV );
            pd3dImmediateContext->Draw( 4, 0 );
        }
        else
        {
            pd3dImmediateContext->PSSetShaderResources( 0, 1, &g_pProxyTextureSRV );
            pd3dImmediateContext->Draw( 4, 0 );
        }
    }

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();

    static DWORD dwTimefirst = GetTickCount();
    if ( GetTickCount() - dwTimefirst > 5000 )
    {    
        OutputDebugString( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
        OutputDebugString( L"\n" );
        dwTimefirst = GetTickCount();
    }
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_SettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    g_Mesh.Destroy();

    SAFE_RELEASE( g_pVertexShaderShadow );
    SAFE_RELEASE( g_pVertexShader );
    SAFE_RELEASE( g_pVertexShaderFXAA );
    SAFE_RELEASE( g_pPixelShader );
    SAFE_RELEASE( g_pPixelShaderFXAA );

    SAFE_RELEASE( g_pVertexLayout );
    SAFE_RELEASE( g_pRandomRotTextureSRV );
    SAFE_RELEASE( g_pRandomRotTexture );
    SAFE_RELEASE( g_pDepthTexture );
    SAFE_RELEASE( g_pDepthTextureDSV );
    SAFE_RELEASE( g_pDepthTextureSRV );
    SAFE_RELEASE( g_pSamPointMirror );
    SAFE_RELEASE( g_pSamLinearWrap );
    SAFE_RELEASE( g_pSamPointCmpClamp );
    SAFE_RELEASE( g_pSamBilinear );
    SAFE_RELEASE( g_pColorWritesOn );
    SAFE_RELEASE( g_pColorWritesOff );
    SAFE_RELEASE( g_pCullBack );
    SAFE_RELEASE( g_pCullFront );

    SAFE_RELEASE( g_pProxyTexture );
    SAFE_RELEASE( g_pProxyTextureSRV );
    SAFE_RELEASE( g_pProxyTextureRTV );
    SAFE_RELEASE( g_pCopyResolveTexture );
    SAFE_RELEASE( g_pCopyResolveTextureSRV );

    // Delete additional render resources here...

    SAFE_RELEASE( g_pcbConstants );
    SAFE_RELEASE( g_pcbFXAA );
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
            pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
    g_LightCamera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );
    g_LightCamera.HandleMessages( hWnd, uMsg, wParam, lParam );
    
    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() );
            break;
        case IDC_TOGGLEFXAA:
        {
            CDXUTCheckBox* pCheckBox = (CDXUTCheckBox*)pControl;
            g_fxaaEnabled = pCheckBox->GetChecked();
            break;
        }
    }
}
