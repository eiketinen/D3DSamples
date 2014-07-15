//----------------------------------------------------------------------------------
// File:        DeferredContexts11\src\utility/SimpleShipController.h
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

#define MAX_FOLLOWING 4

#include "Scene.h"

extern D3DXVECTOR3 g_SceneWorldSize;

class SimpleShipController
{
public:
    SimpleShipController(Scene* pScene, int index, D3DXVECTOR3 initialPosition) :
        m_pScene(pScene),
        m_index(index),
        m_position(initialPosition),
        m_velocity(0, 0, 0),
        m_accel(0, 0, 0)
    {
        m_fTimeToChangeHeading = -1;
        fCustomScale = 0.25f + 0.75f * ((rand() % 1000) / 1000.f);

        int randomColor = rand() % 5;

        switch(randomColor)
        {
        case 0:
            m_color = D3DXVECTOR4(1, 0, 0, 1);
            break;

        case 1:
            m_color = D3DXVECTOR4(0, 1, 0, 1);
            break;

        case 2:
            m_color = D3DXVECTOR4(0, 0, 1, 1);
            break;

        case 3:
            m_color = D3DXVECTOR4(1, 0, 1, 1);
            break;

        case 4:
            m_color = D3DXVECTOR4(1, 1, 0, 1);
            break;
        }

        float colorScale = 0.06125f + 0.06125f * ((rand() % 1000) / 1000.f);
        m_color.x *= colorScale;
        m_color.y *= colorScale;
        m_color.z *= colorScale;
        m_color.w = 1.f;
    }

    void Update(float fTime, float fDeltaTime, float fScale = 1.0f)
    {
        DC_UNREFERENCED_PARAM(fTime);
        //UpdateStationary(fScale);
        UpdateRandomPhysics(fDeltaTime, fCustomScale * fScale);
        m_pScene->SetMeshColorFor(m_index, &m_color);
    }

protected:

    void UpdateStationary(float fScale)
    {
        D3DXMATRIX mWorld;
        D3DXMATRIX mScale;
        D3DXMATRIX mTrans;
        D3DXMatrixTranslation(&mTrans, m_position.x, m_position.y, m_position.z);
        D3DXMatrixScaling(&mScale, fScale, fScale, fScale);
        mWorld = mScale * mTrans ;
        m_pScene->SetWorldMatrixFor(m_index, &mWorld);
    }

    void UpdateRandomPhysics(float fDeltaTime, float fScale)
    {
        D3DXMATRIX mScale;
        D3DXMATRIX mTrans;
        D3DXMATRIX mRot;
        D3DXMATRIX mWorld;

        if(m_fTimeToChangeHeading > 0)
            m_fTimeToChangeHeading -= fDeltaTime;

        const float maxAccel = 50.f;

        if(m_fTimeToChangeHeading < 0)
        {
            m_accel.x = maxAccel * (((float)(rand() % 2000) / 1000.f) - 1.f);
            m_accel.y = maxAccel * (((float)(rand() % 2000) / 1000.f) - 1.f);
            m_accel.z = maxAccel * (((float)(rand() % 2000) / 1000.f) - 1.f);

            // set a new countdown
            m_fTimeToChangeHeading = (float)(rand() % 50 + 1) / 10.f;
        }

        // simple physics
        m_velocity -= m_velocity * (0.05f * fDeltaTime);    // a little drag before added impulse, 5% per second
        m_velocity += m_accel * fDeltaTime;
        D3DXVECTOR3 heading;
        D3DXVec3Normalize(&heading, &m_velocity);

        const float maxSpeed = 100.f;
        const float idleSpeed = 0.75f * maxSpeed;
        const float maxSpeedSq = maxSpeed * maxSpeed;
        const float idleSpeedSq = idleSpeed * idleSpeed;

        float speedSq = D3DXVec3Dot(&m_velocity, &m_velocity);

        if(speedSq > maxSpeedSq)    // going too fast? turn off the accel
        {
            m_accel = D3DXVECTOR3(0, 0, 0);
        }
        else if(speedSq > idleSpeedSq)
        {
            m_accel -= (0.05f * fDeltaTime) * m_accel;    // drag the acceleration once we hit the idel speed
        }

        m_position += m_velocity * fDeltaTime;

        // Keep our ships in the world volume
        if(m_position.y < -(g_SceneWorldSize.y / 2.5f))
            m_accel.y = maxAccel;
        else if(m_position.y > (g_SceneWorldSize.y / 2.f))
            m_accel.y = -maxAccel;

        if(m_position.x < -(g_SceneWorldSize.x / 2.f))
            m_accel.x = maxAccel;
        else if(m_position.x > (g_SceneWorldSize.x / 2.f))
            m_accel.x = -maxAccel;

        if(m_position.z < -(g_SceneWorldSize.z / 2.f))
            m_accel.z = maxAccel;
        else if(m_position.z > (g_SceneWorldSize.z / 2.f))
            m_accel.z = -maxAccel;

        D3DXMatrixScaling(&mScale, fScale, fScale, fScale);
        D3DXMatrixTranslation(&mTrans, m_position.x, m_position.y, m_position.z);

        // always jsut look directly at target position (i.e. direction of impulse)
        D3DXVECTOR3 Up = D3DXVECTOR3(0, 1, 0);
        D3DXMATRIX lookAt;
        D3DXVECTOR3 Origin = D3DXVECTOR3(0, 0, 0);
        D3DXMatrixLookAtLH(&lookAt, &Origin, &heading, &Up);
        D3DXMatrixInverse(&mRot, NULL, &lookAt);

        mWorld = mScale * mRot * mTrans ;

        m_pScene->SetWorldMatrixFor(m_index, &mWorld);

    }

private:

    // simple space ship physics vars
    D3DXVECTOR3 m_velocity;
    D3DXVECTOR3 m_position;
    float fCustomScale;

    D3DXVECTOR4 m_color;

    float m_fTimeToChangeHeading;
    D3DXVECTOR3 m_accel;

    int m_index;
    Scene* m_pScene;

};