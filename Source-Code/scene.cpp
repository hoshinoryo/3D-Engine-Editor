/*==============================================================================

　 画面遷移制御 [scene.cpp]
                                                         Author : Youhei Sato
                                                         Date   : 2025/07/11
--------------------------------------------------------------------------------

==============================================================================*/
#include "scene.h"
#include "game.h"

//static Scene g_Scene = SCENE_TITLE; // 今のシーン
static Scene g_Scene = SCENE_GAME; // debugしたいのシーン
static Scene g_SceneNext = g_Scene; // 次のシーン

void Scene_Initialize()
{
    switch (g_Scene)
    {
    case SCENE_GUIDE:
        //Title_Initialize();
        break;
    case SCENE_GAME:
        Game_Initialize();
        break;
    case SCENE_RESULT:
        break;
    }
}

void Scene_Finalize()
{
    switch (g_Scene)
    {
    case SCENE_GUIDE:
        //Title_Finalize();
        break;
    case SCENE_GAME:
        Game_Finalize();
        break;
    case SCENE_RESULT:
        break;
    }
}

void Scene_Update(double elapsed_time)
{
    switch (g_Scene)
    {
    case SCENE_GUIDE:
        //Title_Update(elapsed_time);
        break;
    case SCENE_GAME:
        Game_Update(elapsed_time);
        break;
    case SCENE_RESULT:
        break;
    }
}

void Scene_Draw()
{
    switch (g_Scene)
    {
    case SCENE_GUIDE:
        //Title_Draw();
        break;
    case SCENE_GAME:
        Game_Draw();
        break;
    case SCENE_RESULT:
        break;
    }
}

void Scene_Refresh()
{
    if (g_Scene != g_SceneNext)
    {
        // 現在のシーンの後片付け
        Scene_Finalize();

        g_Scene = g_SceneNext;

        // 次のシーンの初期化
        Scene_Initialize();
    }
}

void Scene_Change(Scene scene)
{
    g_SceneNext = scene;
}
