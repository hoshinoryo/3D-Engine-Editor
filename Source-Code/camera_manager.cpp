/*==============================================================================

   Camera management and mode switch [camera_manager.cpp]
                                                         Author : Gu Anyi
                                                         Date   : 2026/01/03
--------------------------------------------------------------------------------

==============================================================================*/

#include "camera_manager.h"
#include "orbit_camera.h"
#include "player_camera.h"
#include "player.h"
#include "guide_overlay.h"

using namespace DirectX;

bool g_IsPlayMode = false;

namespace CameraManager
{
    static OrbitCamera g_OrbitCam;
    static PlayerCamera g_PlayerCam;
    static CameraBase* g_pActiveCam = nullptr;
    static Player* g_pPlayer = nullptr;

    bool Initialize(Player* player)
    {
        g_pPlayer = player;

        if (!g_OrbitCam.Initialize())
        {
            return false;
        }
        if (!g_PlayerCam.Initialize())
        {
            return false;
        }

        if (g_pPlayer)
        {
            g_PlayerCam.SetFollowTarget(&g_pPlayer->GetPosition());
        }

        g_PlayerCam.SetActive(false);
        g_IsPlayMode = false;
        g_pActiveCam = &g_OrbitCam;

        return true;
    }

    void Finalize()
    {
        g_pPlayer = nullptr;
        g_pActiveCam = nullptr;
        g_PlayerCam.Finalize();
        g_OrbitCam.Finalize();
    }

    void Update(double elapsed_time)
    {
        if (g_pActiveCam)
        {
            g_pActiveCam->Update(elapsed_time);
        }
    }

    // Single source of truth
    void SetPlayerMode(bool play)
    {
        if (g_IsPlayMode == play)
            return;

        g_IsPlayMode = play;
        
        // Set camera active and show guide
        if (g_IsPlayMode)
        {
            g_PlayerCam.SetActive(true);
            g_pActiveCam = &g_PlayerCam;

            NotifyEnterPlayMode();
        }
        else
        {
            g_PlayerCam.SetActive(false);
            g_pActiveCam = &g_OrbitCam;

            NotifyEnterEditorMode();
        }
    }

    bool IsPlayMode()
    {
        return g_IsPlayMode;
    }

    void NotifyEnterPlayMode()
    {
        GuideOverlay::OnEnterPlayMode();
    }

    void NotifyEnterEditorMode()
    {
        GuideOverlay::OnEnterEditorMode();
    }

    CameraBase& GetActiveCamera()
    {
        return *g_pActiveCam;
    }

    OrbitCamera& GetOrbitCamera()
    {
        return g_OrbitCam;
    }

    PlayerCamera& GetPlayerCamera()
    {
        return g_PlayerCam;
    }
}
