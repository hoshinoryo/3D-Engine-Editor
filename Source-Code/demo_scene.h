/*==============================================================================

   デモシーンのディスプレイ [demo_scene.h]
														 Author : Gu Anyi
														 Date   : 2025/12/19

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef DEMO_SCENE_H
#define DEMO_SCENE_H

#include <d3d11.h>
#include <DirectXMath.h>

void Demo_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Demo_Finalize(void);
void Demo_Draw();

void Demo_UpdateWorldAABB();
void Demo_AddCollidersAABB();

#endif // DEMO_SCENE_H