/*==============================================================================

   Translate gizmo API [gizmo_translate.h]
														 Author : Gu Anyi
														 Date   : 2026/02/01
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef GIZMO_TRANSLATE_H
#define GIZMO_TRANSLATE_H

#include <DirectXMath.h>
#include <cstdint>

struct MeshObject;

enum class GizmoAxis
{
	None,
	X,
	Y,
	Z
};

namespace GizmoTranslate
{
	void Begin(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);
	void Draw(const MeshObject& obj);

	bool OnMouseDown(int mouseX, int mouseY);
	void OnMouseDrag(int mouseX, int mouseY);
	void OnMouseUp();

	bool IsActive();

	// for external
	bool OnMouseDownExternal(DirectX::XMFLOAT3& inOutPos, int mouseX, int mouseY);
	bool OnMouseDragExternal(DirectX::XMFLOAT3& inOutPos, int mouseX, int mouseY);
	void DrawExternal(const DirectX::XMFLOAT3& pos);
}

#endif // GIZMO_TRANSLATE_H

