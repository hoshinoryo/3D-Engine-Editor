/*==============================================================================

   skeleton‚Ì•`‰æ [skeleton.h]
														 Author : Gu Anyi
														 Date   : 2025/11/14

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SKELETON_H
#define SKELETON_H

#include <DirectXMath.h>

void Skeleton_Initialize();
void Skeleton_Finalize();

void Skeleton_Draw(const DirectX::XMMATRIX & mtxWorld);

//void BuildNodeWorldMatrices(const aiNode* node, const DirectX::XMMATRIX& parentWorld, std::unordered_map<const aiNode*, DirectX::XMMATRIX>& nodeWorlds);

#endif // SKELETON_H
