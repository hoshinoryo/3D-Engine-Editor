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
#include "model_asset.h"
#include "model_renderer.h"
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
#include "gizmo_translate.h"
#include "primitive_tool.h"

#include <DirectXMath.h>

using namespace DirectX;

namespace
{
    // Initialize
    static void InitLighting();
    static void InitShaders(const XMFLOAT3& camPos);
    static void InitSceneAssets();
    static void InitPlayerAndEnv();
    static void InitPasses();

    // Finalize
    static void ShutdownPasses();
    static void ShutdownAssets();

    // Update
    static void UpdatePlayer(double elapsed_time, const XMFLOAT3& camFront);
    static void UpdateCollisionWorld();
    static void UpdateSkydomeFollow(const XMFLOAT3& camPos);

    // Draw
    static void DrawAllMeshObjects(const XMFLOAT3& camPos);
    static void RunPickingPass(const XMMATRIX& view, const XMMATRIX& proj);
    //static void RunOutlineAndGizmo();
    static void RunOutline();
    static void RunGizmo();

    // Input
    static void HandleEditorMouseInput(const CameraBase& cam);

    static void DrawRest(const XMFLOAT3& camPos);
}

// Picking pass
static PickingPass g_PickingPass;
static bool g_PickingReady = false;
// Outline pass
static OutlinePostPass g_OutlinePost;
static bool g_OutlineReady = false;

static ModelAsset* g_modelTest2 = nullptr;
static ModelAsset* g_modelMaterial = nullptr;

static Player g_Player;

AnimationPlayer g_AnimPlayer;

void Game_Initialize()
{
    // Camera
    CameraManager::Initialize(&g_Player);
    const XMFLOAT3& camPos = CameraManager::GetActiveCamera().GetPosition();

    InitLighting();
    InitShaders(camPos);
    InitSceneAssets();
    InitPlayerAndEnv();
    InitPasses();
}

void Game_Finalize()
{
    ShutdownPasses();

    g_Player.Finalize();
    Skydome_Finalize();

    ShutdownAssets();
    
    CameraManager::Finalize();
}

void Game_Update(double elapsed_time)
{
    // ---- CAMERA UPDATE ----
    // DO NOT TOUCH: Update view and proj constant buffer
    CameraManager::Update(elapsed_time);

    CameraBase& cam = CameraManager::GetActiveCamera();
    XMFLOAT3 camPos = cam.GetPosition();
    XMFLOAT3 camFront = cam.GetFront();

    UpdatePlayer(elapsed_time, camFront);
    UpdateCollisionWorld();
    UpdateSkydomeFollow(camPos);
}

void Game_Draw()
{
    // Camera draw
    CameraBase& cam = CameraManager::GetActiveCamera();
    const XMFLOAT3& camPos = cam.GetPosition();
    XMMATRIX view = XMLoadFloat4x4(&cam.GetView());
    XMMATRIX proj = XMLoadFloat4x4(&cam.GetProj());

    Render3D_BeginFrame(cam);
    GizmoTranslate::Begin(view, proj);
    
    // Picking pass
    RunPickingPass(view, proj);
    HandleEditorMouseInput(cam);

    DrawAllMeshObjects(camPos);
    DrawRest(camPos);

    //RunOutlineAndGizmo();
    RunOutline();
    RunGizmo();
    Draw3d_Draw();
}

namespace
{
    static void InitLighting()
    {
        g_LightManager.SetAmbient({ 0.5f, 0.5f, 0.5f, 1.0f });
        XMVECTOR v{ 1.0f, -1.0f, 0.0f, 0.0f };
        v = XMVector3Normalize(v);
        XMFLOAT4 dir;
        XMStoreFloat4(&dir, v);
        g_LightManager.SetDirectionalWorld(dir, { 0.5f, 0.5f, 0.5f, 1.0f });
    }

    static void InitShaders(const XMFLOAT3& camPos)
    {
        g_Default3DshaderStatic.UpdateSpecularParams(camPos, 30.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
        g_Default3DshaderSkinned.UpdateSpecularParams(camPos, 30.0f, { 1.0f, 1.0f, 1.0f, 1.0f });

        g_LightManager.SetPointLightCount(1);
        g_LightManager.SetPointLight(0, { 0.0f, 3.0f, -2.0f }, 5.0f, { 1.0f, 0.0f, 0.0f });
    }

    static void InitSceneAssets()
    {
        g_modelTest2 = ModelAsset_Load("resources/oldfurniture/OldFurniturePack_new.fbx", true, 0.03f);
        g_modelMaterial = ModelAsset_Load("resources/materialTestBall.fbx", true, 100.0f);

    }
    static void InitPlayerAndEnv()
    {
        SceneManager::Clear();
        CollisionSystem::ClearColliders();
        //GuideOverlay::Initialize();

        if (g_modelTest2)
        {
            for (uint32_t mi = 0; mi < (uint32_t)g_modelTest2->meshes.size(); ++mi)
            {
                TransformTRS trs;
                uint32_t id = SceneManager::RegisterMeshObject(g_modelTest2, mi, trs, true);
            }
        }

        if (g_modelMaterial)
        {
            for (uint32_t mi = 0; mi < (uint32_t)g_modelMaterial->meshes.size(); ++mi)
            {
                TransformTRS trs;
                trs.position = { -3.0f, 2.0f, -5.0f }; // test position
                uint32_t id = SceneManager::RegisterMeshObject(g_modelMaterial, mi, trs, true);
            }
        }

        // Skeleton import
        //Skeleton_Initialize();

        Skydome_Initialize();

        // Player
        g_Player.Initialize({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
    }

    static void InitPasses()
    {
        g_PickingReady = g_PickingPass.Initialize(Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight());
        g_OutlineReady = g_OutlinePost.Initialize(Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight());
    }

    static void ShutdownPasses()
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
    }

    static void ShutdownAssets()
    {
        ModelAsset_Release(g_modelMaterial);
        ModelAsset_Release(g_modelTest2);
    }

    static void UpdatePlayer(double elapsed_time, const XMFLOAT3& camFront)
    {
        if (CameraManager::IsPlayMode())
        {
            g_Player.Update(elapsed_time, camFront);
        }
        else
        {
            g_Player.UpdateAnimationOnly(elapsed_time);
        }
    }

    // ---- COLLISIONS UPDATE ----
    static void UpdateCollisionWorld()
    {
        SceneManager::UpdateWorldAABBs();
        Demo_UpdateWorldAABB();

        CollisionSystem::ClearColliders();

        // add world AABB into list
        for (const auto& obj : SceneManager::AllObjects())
        {
            if (!obj.visible) continue;
            if (!obj.aabbValid) continue;

            CollisionSystem::AddCollidersAABB(obj.worldAABB);
        }
        Demo_AddColliders();
        PrimitiveTool::AppendColliders();
    }

    static void UpdateSkydomeFollow(const XMFLOAT3& camPos)
    {
        Skydome_SetPosition(camPos);
    }

    static void DrawAllMeshObjects(const XMFLOAT3& camPos)
    {
        // Draw all mesh objects
        for (auto& obj : SceneManager::AllObjects())
        {
            if (!obj.visible || !obj.asset) continue;

            const XMMATRIX instanceWorld = obj.transform.ToMatrix();
            const XMMATRIX nodeToModel = XMLoadFloat4x4(&obj.asset->meshes[obj.meshIndex].nodeToModel);
            const XMMATRIX world = nodeToModel * instanceWorld;

            ModelRenderer_Draw(obj.asset, obj.meshIndex, world, camPos);

            if (DebugDraw_Allow(DebugDrawCategory::Collision) && obj.aabbValid)
            {
                Collision_DebugDraw(obj.worldAABB, { 0.0f, 0.0f, 1.0f, 1.0f });
            }
        }
    }

    static void RunPickingPass(const XMMATRIX& view, const XMMATRIX& proj)
    {
        if (!g_PickingReady) return;

        g_PickingPass.Begin(view, proj);

        for (const auto& obj : SceneManager::AllObjects())
        {
            if (!obj.visible || !obj.pickable || !obj.asset) continue;

            XMMATRIX instanceWorld = obj.transform.ToMatrix();
            XMMATRIX nodeToModel = XMLoadFloat4x4(&obj.asset->meshes[obj.meshIndex].nodeToModel);
            XMMATRIX world = nodeToModel * instanceWorld;

            g_PickingPass.DrawAsset(obj.asset, obj.meshIndex, world, obj.id);
        }

        PrimitiveTool::DrawPicking(g_PickingPass);

        g_PickingPass.End();
    }

    static void RunOutline()
    {
        if (!g_OutlineReady) return;
        if (!g_PickingReady) return;

        const float color[4] = { 0, 0.8f, 0.3f, 1.0f };

        if (MeshObject* selMesh = SceneManager::GetSelectedObject())
        {
            g_OutlinePost.DrawModel(g_PickingPass.GetIdSRV(), selMesh->id, 2, color);
        }
        else if (CubeObject* selPrim = PrimitiveTool::GetSelected())
        {
            uint32_t primId = PrimitiveTool::GetSelectedObjectId();
            g_OutlinePost.DrawModel(g_PickingPass.GetIdSRV(), primId, 2, color);
        }
    }

    static void RunGizmo()
    {
        if (MeshObject* selMesh = SceneManager::GetSelectedObject())
        {
            GizmoTranslate::Draw(*selMesh);
        }
        else if (CubeObject* selPrim = PrimitiveTool::GetSelected())
        {
            GizmoTranslate::DrawExternal(selPrim->GetPosition());
        }
    }

    static void HandleEditorMouseInput(const CameraBase& cam)
    {
        if (!g_PickingReady) return;

        Mouse_State ms;
        Mouse_GetState(&ms);

        static bool prevLeft = false;
        bool left = ms.leftButton;

        // Mouse down
        if (left && !prevLeft)
        {
            bool consumed = false;

            if (CubeObject* selPrim = PrimitiveTool::GetSelected())
            {
                consumed = GizmoTranslate::OnMouseDownExternal(selPrim->GetPositionRef(), ms.x, ms.y);
            }
            else
            {
                consumed = GizmoTranslate::OnMouseDown(ms.x, ms.y);

            }

            if (!consumed)
            {
                if (PrimitiveTool::PickFromMouse(cam, ms.x, ms.y))
                {
                    SceneManager::SetSelectedMeshObject(0);
                }
                else
                {
                    uint32_t pickedId = g_PickingPass.ReadBackId(ms.x, ms.y);
                    SceneManager::SetSelectedMeshObject(pickedId); // bridge from picking pass to attribute editor
                    PrimitiveTool::ClearSelection();
                }
            }
        }

        // Mouse drag
        if (left && prevLeft)
        {
            if (CubeObject* selPrim = PrimitiveTool::GetSelected())
            {
                if (GizmoTranslate::OnMouseDragExternal(selPrim->GetPositionRef(), ms.x, ms.y))
                {
                    selPrim->UpdateAABB();
                }
            }
            else
            {
                GizmoTranslate::OnMouseDrag(ms.x, ms.y);
            }
        }

        // Mouse up
        if (!left && prevLeft)
        {
            GizmoTranslate::OnMouseUp();
        }

        prevLeft = left;
    }

    static void DrawRest(const XMFLOAT3& camPos)
    {
        // Demo scene
        Demo_Draw();

        Skydome_Draw();

        // Light debug draw
        g_LightManager.DebugDrawPointLight();
        g_LightManager.DebugDrawDirectionalLight();

        g_Player.Draw(camPos);

        //Draw3d_Draw();
    }
}

