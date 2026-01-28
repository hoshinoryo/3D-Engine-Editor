/*==============================================================================

   Model asset drawing functions [model_renderer.h]
														 Author : Gu Anyi
														 Date   : 2026/01/28
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef MODEL_RENDERER_H
#define MODEL_RENDERER_H

#include <DirectXMath.h>

struct ModelAsset;

void ModelRenderer_Initialize();
void ModelRenderer_Finalize();

void ModelRenderer_Draw(
	ModelAsset* asset,
	uint32_t meshIndex,
	const DirectX::XMMATRIX& world,
	const DirectX::XMFLOAT3& cameraPos
);
void ModelRenderer_UnlitDraw(
	ModelAsset* asset,
	uint32_t meshIndex,
	const DirectX::XMMATRIX& world,
	const DirectX::XMFLOAT4& color
);

# endif // MODEL_RENDERER_H