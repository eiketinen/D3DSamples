//----------------------------------------------------------------------------------
// File:        include\nvidiautils/Camera.h
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

//--------------------------------------------------------------------------------------
// File: DXUTcamera.h
//
// Copyright (c) Microsoft Corporation. All rights reserved
//--------------------------------------------------------------------------------------

#pragma once

#include <d3dx9math.h>

//--------------------------------------------------------------------------------------
class CD3DArcBall
{
public:
                                    CD3DArcBall();

    // Functions to change behavior
    void                            Reset();
    void                            SetTranslationRadius( float fRadiusTranslation )
    {
        m_fRadiusTranslation = fRadiusTranslation;
    }
    void                            SetWindow( int nWidth, int nHeight, float fRadius = 0.9f )
    {
        m_nWidth = nWidth; m_nHeight = nHeight; m_fRadius = fRadius;
        m_vCenter = D3DXVECTOR2( m_nWidth / 2.0f, m_nHeight / 2.0f );
    }
    void                            SetOffset( int nX, int nY )
    {
        m_Offset.x = nX; m_Offset.y = nY;
    }

    // Call these from client and use GetRotationMatrix() to read new rotation matrix
    void                            OnBegin( int nX, int nY );  // start the rotation (pass current mouse position)
    void                            OnMove( int nX, int nY );   // continue the rotation (pass current mouse position)
    void                            OnEnd();                    // end the rotation 

    // Or call this to automatically handle left, middle, right buttons
    LRESULT                         HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    // Functions to get/set state
    const D3DXMATRIX* GetRotationMatrix()
    {
        return D3DXMatrixRotationQuaternion( &m_mRotation, &m_qNow );
    };
    const D3DXMATRIX* GetTranslationMatrix() const
    {
        return &m_mTranslation;
    }
    const D3DXMATRIX* GetTranslationDeltaMatrix() const
    {
        return &m_mTranslationDelta;
    }
    bool                            IsBeingDragged() const
    {
        return m_bDrag;
    }
    D3DXQUATERNION                  GetQuatNow() const
    {
        return m_qNow;
    }
    void                            SetQuatNow( D3DXQUATERNION q )
    {
        m_qNow = q;
    }

    static D3DXQUATERNION WINAPI    QuatFromBallPoints( const D3DXVECTOR3& vFrom, const D3DXVECTOR3& vTo );


protected:
    D3DXMATRIXA16 m_mRotation;         // Matrix for arc ball's orientation
    D3DXMATRIXA16 m_mTranslation;      // Matrix for arc ball's position
    D3DXMATRIXA16 m_mTranslationDelta; // Matrix for arc ball's position

    POINT m_Offset;   // window offset, or upper-left corner of window
    int m_nWidth;   // arc ball's window width
    int m_nHeight;  // arc ball's window height
    D3DXVECTOR2 m_vCenter;  // center of arc ball 
    float m_fRadius;  // arc ball's radius in screen coords
    float m_fRadiusTranslation; // arc ball's radius for translating the target

    D3DXQUATERNION m_qDown;             // Quaternion before button down
    D3DXQUATERNION m_qNow;              // Composite quaternion for current drag
    bool m_bDrag;             // Whether user is dragging arc ball

    POINT m_ptLastMouse;      // position of last mouse POINT
    D3DXVECTOR3 m_vDownPt;           // starting POINT of rotation arc
    D3DXVECTOR3 m_vCurrentPt;        // current POINT of rotation arc

    D3DXVECTOR3                     ScreenToVector( float fScreenPtX, float fScreenPtY );
};


//--------------------------------------------------------------------------------------
// used by CCamera to map WM_KEYDOWN keys
//--------------------------------------------------------------------------------------
enum D3DUtil_CameraKeys
{
    CAM_STRAFE_LEFT = 0,
    CAM_STRAFE_RIGHT,
    CAM_MOVE_FORWARD,
    CAM_MOVE_BACKWARD,
    CAM_MOVE_UP,
    CAM_MOVE_DOWN,
    CAM_RESET,
    CAM_CONTROLDOWN,
    CAM_MAX_KEYS,
    CAM_UNKNOWN     = 0xFF
};

#define KEY_WAS_DOWN_MASK 0x80
#define KEY_IS_DOWN_MASK  0x01

#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_MIDDLE_BUTTON 0x02
#define MOUSE_RIGHT_BUTTON  0x04
#define MOUSE_WHEEL         0x08


//--------------------------------------------------------------------------------------
// Simple base camera class that moves and rotates.  The base class
//       records mouse and keyboard input for use by a derived class, and 
//       keeps common state.
//--------------------------------------------------------------------------------------
class CBaseCamera
{
public:
                                CBaseCamera();

    // Call these from client and use Get*Matrix() to read new matrices
    virtual LRESULT             HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    virtual void                FrameMove( float fElapsedTime ) = 0;

    // Functions to change camera matrices
    virtual void                Reset();
    virtual void                SetViewParams( D3DXVECTOR3* pvEyePt, D3DXVECTOR3* pvLookatPt );
    virtual void                SetProjParams( float fFOV, float fAspect, float fNearPlane, float fFarPlane );

    // Functions to change behavior
    virtual void                SetDragRect( RECT& rc )
    {
        m_rcDrag = rc;
    }
    void                        SetInvertPitch( bool bInvertPitch )
    {
        m_bInvertPitch = bInvertPitch;
    }
    void                        SetDrag( bool bMovementDrag, float fTotalDragTimeToZero = 0.25f )
    {
        m_bMovementDrag = bMovementDrag; m_fTotalDragTimeToZero = fTotalDragTimeToZero;
    }
    void                        SetEnableYAxisMovement( bool bEnableYAxisMovement )
    {
        m_bEnableYAxisMovement = bEnableYAxisMovement;
    }
    void                        SetEnablePositionMovement( bool bEnablePositionMovement )
    {
        m_bEnablePositionMovement = bEnablePositionMovement;
    }
    void                        SetClipToBoundary( bool bClipToBoundary, D3DXVECTOR3* pvMinBoundary,
                                                   D3DXVECTOR3* pvMaxBoundary )
    {
        m_bClipToBoundary = bClipToBoundary; if( pvMinBoundary ) m_vMinBoundary = *pvMinBoundary;
        if( pvMaxBoundary ) m_vMaxBoundary = *pvMaxBoundary;
    }
    void                        SetScalers( float fRotationScaler = 0.01f, float fMoveScaler = 5.0f )
    {
        m_fRotationScaler = fRotationScaler; m_fMoveScaler = fMoveScaler;
    }
    void                        SetNumberOfFramesToSmoothMouseData( int nFrames )
    {
        if( nFrames > 0 ) m_fFramesToSmoothMouseData = ( float )nFrames;
    }
    void                        SetResetCursorAfterMove( bool bResetCursorAfterMove )
    {
        m_bResetCursorAfterMove = bResetCursorAfterMove;
    }

    // Functions to get state
    const D3DXMATRIX* GetViewMatrix() const
    {
        return &m_mView;
    }
    const D3DXMATRIX* GetProjMatrix() const
    {
        return &m_mProj;
    }
    const D3DXVECTOR3* GetEyePt() const
    {
        return &m_vEye;
    }
    const D3DXVECTOR3* GetLookAtPt() const
    {
        return &m_vLookAt;
    }
    float                       GetNearClip() const
    {
        return m_fNearPlane;
    }
    float                       GetFarClip() const
    {
        return m_fFarPlane;
    }

    bool                        IsBeingDragged() const
    {
        return ( m_bMouseLButtonDown || m_bMouseMButtonDown || m_bMouseRButtonDown );
    }
    bool                        IsMouseLButtonDown() const
    {
        return m_bMouseLButtonDown;
    }
    bool                        IsMouseMButtonDown() const
    {
        return m_bMouseMButtonDown;
    }
    bool                        IsMouseRButtonDown() const
    {
        return m_bMouseRButtonDown;
    }

protected:
    // Functions to map a WM_KEYDOWN key to a D3DUtil_CameraKeys enum
    virtual D3DUtil_CameraKeys  MapKey( UINT nKey );
    bool                        IsKeyDown( BYTE key ) const
    {
        return( ( key & KEY_IS_DOWN_MASK ) == KEY_IS_DOWN_MASK );
    }
    bool                        WasKeyDown( BYTE key ) const
    {
        return( ( key & KEY_WAS_DOWN_MASK ) == KEY_WAS_DOWN_MASK );
    }

    void                        ConstrainToBoundary( D3DXVECTOR3* pV );
    void                        UpdateMouseDelta();
    void                        UpdateVelocity( float fElapsedTime );
    void                        GetInput( bool bGetKeyboardInput, bool bGetMouseInput, bool bGetGamepadInput,
                                          bool bResetCursorAfterMove );

    D3DXMATRIX m_mView;              // View matrix 
    D3DXMATRIX m_mProj;              // Projection matrix

    D3DXVECTOR3 m_vGamePadLeftThumb;
    D3DXVECTOR3 m_vGamePadRightThumb;

    int m_cKeysDown;            // Number of camera keys that are down.
    BYTE                        m_aKeys[CAM_MAX_KEYS];  // State of input - KEY_WAS_DOWN_MASK|KEY_IS_DOWN_MASK
    D3DXVECTOR3 m_vKeyboardDirection;   // Direction vector of keyboard input
    POINT m_ptLastMousePosition;  // Last absolute position of mouse cursor
    bool m_bMouseLButtonDown;    // True if left button is down 
    bool m_bMouseMButtonDown;    // True if middle button is down 
    bool m_bMouseRButtonDown;    // True if right button is down 
    int m_nCurrentButtonMask;   // mask of which buttons are down
    int m_nMouseWheelDelta;     // Amount of middle wheel scroll (+/-) 
    D3DXVECTOR2 m_vMouseDelta;          // Mouse relative delta smoothed over a few frames
    float m_fFramesToSmoothMouseData; // Number of frames to smooth mouse data over

    D3DXVECTOR3 m_vDefaultEye;          // Default camera eye position
    D3DXVECTOR3 m_vDefaultLookAt;       // Default LookAt position
    D3DXVECTOR3 m_vEye;                 // Camera eye position
    D3DXVECTOR3 m_vLookAt;              // LookAt position
    float m_fCameraYawAngle;      // Yaw angle of camera
    float m_fCameraPitchAngle;    // Pitch angle of camera

    RECT m_rcDrag;               // Rectangle within which a drag can be initiated.
    D3DXVECTOR3 m_vVelocity;            // Velocity of camera
    bool m_bMovementDrag;        // If true, then camera movement will slow to a stop otherwise movement is instant
    D3DXVECTOR3 m_vVelocityDrag;        // Velocity drag force
    float m_fDragTimer;           // Countdown timer to apply drag
    float m_fTotalDragTimeToZero; // Time it takes for velocity to go from full to 0
    D3DXVECTOR2 m_vRotVelocity;         // Velocity of camera

    float m_fFOV;                 // Field of view
    float m_fAspect;              // Aspect ratio
    float m_fNearPlane;           // Near plane
    float m_fFarPlane;            // Far plane

    float m_fRotationScaler;      // Scaler for rotation
    float m_fMoveScaler;          // Scaler for movement

    bool m_bInvertPitch;         // Invert the pitch axis
    bool m_bEnablePositionMovement; // If true, then the user can translate the camera/model 
    bool m_bEnableYAxisMovement; // If true, then camera can move in the y-axis

    bool m_bClipToBoundary;      // If true, then the camera will be clipped to the boundary
    D3DXVECTOR3 m_vMinBoundary;         // Min POINT in clip boundary
    D3DXVECTOR3 m_vMaxBoundary;         // Max POINT in clip boundary

    bool m_bResetCursorAfterMove;// If true, the class will reset the cursor position so that the cursor always has space to move 
};


//--------------------------------------------------------------------------------------
// Simple first person camera class that moves and rotates.
//       It allows yaw and pitch but not roll.  It uses WM_KEYDOWN and 
//       GetCursorPos() to respond to keyboard and mouse input and updates the 
//       view matrix based on input.  
//--------------------------------------------------------------------------------------
class CFirstPersonCamera : public CBaseCamera
{
public:
                    CFirstPersonCamera();

    // Call these from client and use Get*Matrix() to read new matrices
    virtual void    FrameMove( float fElapsedTime );

    // Functions to change behavior
    void            SetRotateButtons( bool bLeft, bool bMiddle, bool bRight, bool bRotateWithoutButtonDown = false );

    // Functions to get state
    D3DXMATRIX* GetWorldMatrix()
    {
        return &m_mCameraWorld;
    }

    const D3DXVECTOR3* GetWorldRight() const
    {
        return ( D3DXVECTOR3* )&m_mCameraWorld._11;
    }
    const D3DXVECTOR3* GetWorldUp() const
    {
        return ( D3DXVECTOR3* )&m_mCameraWorld._21;
    }
    const D3DXVECTOR3* GetWorldAhead() const
    {
        return ( D3DXVECTOR3* )&m_mCameraWorld._31;
    }
    const D3DXVECTOR3* GetEyePt() const
    {
        return ( D3DXVECTOR3* )&m_mCameraWorld._41;
    }

protected:
    D3DXMATRIX m_mCameraWorld;       // World matrix of the camera (inverse of the view matrix)

    int m_nActiveButtonMask;  // Mask to determine which button to enable for rotation
    bool m_bRotateWithoutButtonDown;
};


//--------------------------------------------------------------------------------------
// Simple model viewing camera class that rotates around the object.
//--------------------------------------------------------------------------------------
class CModelViewerCamera : public CBaseCamera
{
public:
                    CModelViewerCamera();

    // Call these from client and use Get*Matrix() to read new matrices
    virtual LRESULT HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    virtual void    FrameMove( float fElapsedTime );


    // Functions to change behavior
    virtual void    SetDragRect( RECT& rc );
    void            Reset();
    void            SetViewParams( D3DXVECTOR3* pvEyePt, D3DXVECTOR3* pvLookatPt );
    void            SetButtonMasks( int nRotateModelButtonMask = MOUSE_LEFT_BUTTON, int nZoomButtonMask = MOUSE_WHEEL,
                                    int nRotateCameraButtonMask = MOUSE_RIGHT_BUTTON )
    {
        m_nRotateModelButtonMask = nRotateModelButtonMask, m_nZoomButtonMask = nZoomButtonMask;
        m_nRotateCameraButtonMask = nRotateCameraButtonMask;
    }
    void            SetAttachCameraToModel( bool bEnable = false )
    {
        m_bAttachCameraToModel = bEnable;
    }
    void            SetWindow( int nWidth, int nHeight, float fArcballRadius=0.9f )
    {
        m_WorldArcBall.SetWindow( nWidth, nHeight, fArcballRadius );
        m_ViewArcBall.SetWindow( nWidth, nHeight, fArcballRadius );
    }
    void            SetRadius( float fDefaultRadius=5.0f, float fMinRadius=1.0f, float fMaxRadius=FLT_MAX )
    {
        m_fDefaultRadius = m_fRadius = fDefaultRadius; m_fMinRadius = fMinRadius; m_fMaxRadius = fMaxRadius;
        m_bDragSinceLastUpdate = true;
    }
    void            SetModelCenter( D3DXVECTOR3 vModelCenter )
    {
        m_vModelCenter = vModelCenter;
    }
    void            SetLimitPitch( bool bLimitPitch )
    {
        m_bLimitPitch = bLimitPitch;
    }
    void            SetViewQuat( D3DXQUATERNION q )
    {
        m_ViewArcBall.SetQuatNow( q ); m_bDragSinceLastUpdate = true;
    }
    void            SetWorldQuat( D3DXQUATERNION q )
    {
        m_WorldArcBall.SetQuatNow( q ); m_bDragSinceLastUpdate = true;
    }

    // Functions to get state
    const D3DXMATRIX* GetWorldMatrix() const
    {
        return &m_mWorld;
    }
    void            SetWorldMatrix( D3DXMATRIX& mWorld )
    {
        m_mWorld = mWorld; m_bDragSinceLastUpdate = true;
    }

protected:
    CD3DArcBall m_WorldArcBall;
    CD3DArcBall m_ViewArcBall;
    D3DXVECTOR3 m_vModelCenter;
    D3DXMATRIX m_mModelLastRot;        // Last arcball rotation matrix for model 
    D3DXMATRIX m_mModelRot;            // Rotation matrix of model
    D3DXMATRIX m_mWorld;               // World matrix of model

    int m_nRotateModelButtonMask;
    int m_nZoomButtonMask;
    int m_nRotateCameraButtonMask;

    bool m_bAttachCameraToModel;
    bool m_bLimitPitch;
    float m_fRadius;              // Distance from the camera to model 
    float m_fDefaultRadius;       // Distance from the camera to model 
    float m_fMinRadius;           // Min radius
    float m_fMaxRadius;           // Max radius
    bool m_bDragSinceLastUpdate; // True if mouse drag has happened since last time FrameMove is called.

    D3DXMATRIX m_mCameraRotLast;

};
