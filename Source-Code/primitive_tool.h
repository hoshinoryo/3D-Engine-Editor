/*==============================================================================

　 プリミティブ生成管理ツール[primitive_tool.h]
														 Author : Gu Anyi
														 Date   : 2026/02/10
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef PRIMITIVE_TOOL_H
#define PRIMITIVE_TOOL_H

#include <vector>
#include <DirectXMath.h>

#include "cube.h"
#include "camera_base.h"

namespace PrimitiveTool
{
	void Initialize(float cubeHalfExtent);
	void Finalize();

	void CreateCube(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& scl = { 1, 1, 1 });
	void DeleteSelected();
	void ClearSelection();

	bool HasSelection();
	CubeObject* GetSelected();

	bool PickFromMouse(const CameraBase& cam, int mouseX, int mouseY);

	void Draw();
	void UpdateAABB();

	void AppendColliders();

	void ToolDraw();
}

#endif // PRIMITIEV_TOOL_H