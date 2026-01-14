/*==============================================================================

Å@ ÉQÅ[ÉÄñ{ëÃ[game.cpp]
                                                         Author : Gu Anyi
                                                         Date   : 2025/06/27
--------------------------------------------------------------------------------

==============================================================================*/
#include "game.h"
#include "cube.h"
#include "grid.h"
#include "key_logger.h"
#include "sampler.h"
#include "direct3d.h"
#include "light.h"
#include "model.h"
#include "player.h"
#include "skydome.h"
#include "default3Dshader.h"
#include "draw3d.h"
#include "skeleton.h"
#include "render3d.h"
#include "default3Dmaterial.h"
#include "animation.h"
#include "demo_scene.h"
#include "camera_base.h"
#include "camera_manager.h"
#include "player_camera.h"
#include "orbit_camera.h"
#include "picking_pass.h"
#include "outline_post_pass.h"
#include "mouse.h"
#include "scene_manager.h"
#include "collision.h"
#include "debug_draw_gate.h"

#include <DirectXMath.h>

using namespace DirectX;


// Picking pass
static PickingPass g_PickingPass;
static bool g_PickingReady = false;
static uint32_t g_SelectedId = 0;
// Outline pass
static OutlinePostPass g_OutlinePost;
static bool g_OutlineReady = false;


static MODEL* g_modelTest2 = nullptr;
static MODEL* g_modelMaterial = nullptr;

static Player g_Player;

AnimationPlayer g_AnimPlayer;

void Game_Initialize()
{
    // Camera
    CameraManager::Initialize(&g_Player);
    const XMFLOAT3& camPos = CameraManager::GetActiveCamera().GetPosition();

    // Light initialization
    g_LightManager.SetAmbient({ 0.5f, 0.5f, 0.5f, 1.0f });
    XMVECTOR v{ 1.0f, -1.0f, 0.0f, 0.0f };
    v = XMVector3Normalize(v);
    XMFLOAT4 dir;
    XMStoreFloat4(&dir, v);
    g_LightManager.SetDirectionalWorld(dir, { 0.5f, 0.5f, 0.5f, 1.0f });

    // Shaders
    g_Default3DshaderStatic.UpdateSpecularParams(camPos, 30.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
    g_Default3DshaderSkinned.UpdateSpecularParams(camPos, 30.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
 
    g_LightManager.SetPointLightCount(1);
    g_LightManager.SetPointLight(0, { 0.0f, 3.0f, -2.0f }, 5.0f, { 1.0f, 0.0f, 0.0f });

    // Model import
    g_modelTest2 = ModelLoad("resources/oldfurniture/Chair02.fbx", false, 4.0f);
    g_modelMaterial = ModelLoad("resources/materialTestBall.fbx", true, 1.0f);

    SceneManager::Clear();
    CollisionSystem::ClearStatics();
    //GuideOverlay::Initialize();

    if (g_modelTest2)
    {
        XMMATRIX w1 = XMMatrixTranslation(0, 0, 3);
        SceneManager::RegisterObject(g_modelTest2, w1, true);
        CollisionSystem::AddStaticModel(g_modelTest2, w1);
    }
    if (g_modelMaterial)
    {
        XMMATRIX w2 = XMMatrixTranslation(-3, 2, 5);
        SceneManager::RegisterObject(g_modelMaterial, w2, true);
        CollisionSystem::AddStaticModel(g_modelMaterial, w2);
    }
    
    // Skeleton import
    //Skeleton_Initialize();

    Skydome_Initialize();

    // Player
    g_Player.Initialize({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });

    g_PickingReady = g_PickingPass.Initialize(Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight());
    g_OutlineReady = g_OutlinePost.Initialize(Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight());
}

void Game_Finalize()
{
    if (g_PickingReady)
    {
        g_PickingPass.Finalize();
        g_PickingReady = false;
    }
    if (g_OutlineReady)
    {
        g_OutlinePost.Finalize();
        g_OutlineReady = false;
    }

    g_Player.Finalize();

    Skydome_Finalize();

    ModelRelease(g_modelMaterial);
    ModelRelease(g_modelTest2);
    
    CameraManager::Finalize();
}

void Game_Update(double elapsed_time)
{
    // ---- CAMERA UPDATE ----
    // DO NOT TOUCH: Update view and proj constant buffer
    CameraManager::Update(elapsed_time);
    // -----------------------

    CameraBase& cam = CameraManager::GetActiveCamera();
    XMFLOAT3 camPos = cam.GetPosition();
    XMFLOAT3 camFront = cam.GetFront();

    //g_Player.Update(elapsed_time);
    if (CameraManager::IsPlayMode())
    {
        g_Player.Update(elapsed_time, camFront);
    }
    else
    {
        g_Player.UpdateAnimationOnly(elapsed_time);
    }

    Skydome_SetPosition(camPos);
}

void Game_Draw()
{
    static XMFLOAT3 pos2 = { 0.0f, 0.0f, 3.0f };
    XMMATRIX world2 = XMMatrixTranslationFromVector(XMLoadFloat3(&pos2));
    static XMFLOAT3 pos3 = { 0.0f, 2.0f, -2.0f };
    XMMATRIX world3 = XMMatrixTranslationFromVector(XMLoadFloat3(&pos3));

    // Camera draw
    CameraBase& cam = CameraManager::GetActiveCamera();
    const XMFLOAT3& camPos = cam.GetPosition();
    XMMATRIX view = XMLoadFloat4x4(&cam.GetView());
    XMMATRIX proj = XMLoadFloat4x4(&cam.GetProj());

    Render3D_BeginFrame(cam);

    // Picking drawing setting
    if (g_PickingReady)
    {
        g_PickingPass.Begin(view, proj);

        for (const auto& obj : SceneManager::AllObjects())
        {
            if (!obj.visible || !obj.pickable || !obj.model) continue;
            g_PickingPass.DrawModel(obj.model, obj.world, obj.id);
        }

        g_PickingPass.End();

        // Read back
        Mouse_State ms;
        Mouse_GetState(&ms);

        static bool prevLeft = false;
        bool left = ms.leftButton;

        if (left && !prevLeft)
        {
            g_SelectedId = g_PickingPass.ReadBackId(ms.x, ms.y);
        }

        prevLeft = left;
    }

    // Draw all objects
    for (auto& obj : SceneManager::AllObjects())
    {
        if (!obj.visible || !obj.model) continue;

        ModelDraw(obj.model, obj.world, camPos);

/*
#if defined(DEBUG) || defined(_DEBUG)
        Collision_DebugDraw(obj.model->worldAABB, { 0.0f, 0.0f, 1.0f, 1.0f });
#endif
*/
        if (DebugDraw_Allow(DebugDrawCategory::Collision))
        {
            Collision_DebugDraw(obj.model->worldAABB, { 0.0f, 0.0f, 1.0f, 1.0f });
        }
    }

    // Highlight drawing
    if (g_OutlineReady && g_SelectedId != 0)
    {
        const float color[4] = { 0, 0.8f, 0.3f, 1.0f };
        g_OutlinePost.DrawModel(g_PickingPass.GetIdSRV(), g_SelectedId, 2, color);
    }

    // Demo scene
    Demo_Draw();

    Skydome_Draw();

    //Grid_Draw();

    g_LightManager.DebugDrawPointLight();

    g_Player.Draw(camPos);

    Draw3d_Draw();
}

void Game_DrawCameraDebugUI()
{
    if (!CameraManager::IsPlayMode())
    {
        CameraManager::GetOrbitCamera().DebugDraw();
    }
}

void Game_DrawLightDebugUI()
{
    g_LightManager.DebugDraw();
}


void Game_DrawMaterialManager()
{
    g_DefaultSceneMaterial.DebugDraw(g_Default3DshaderStatic, CameraManager::GetActiveCamera().GetPosition());
}

