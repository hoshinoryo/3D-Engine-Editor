/*==============================================================================

   ‹ó‚Ì•`‰æ [skydome.h]
														 Author : Gu Anyi
														 Date   : 2025/11/21

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SKYDOME_H
#define SKYDOME_H

#include <DirectXMath.h>

void Skydome_Initialize();
void Skydome_Finalize();
void Skydome_SetPosition(const DirectX::XMFLOAT3& position);
void Skydome_Draw();

#endif // SKYDOME_H
