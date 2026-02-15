/*==============================================================================

   Attribute editor drawing [attribute_editor.cpp]
														 Author : Gu Anyi
														 Date   : 2026/02/16
--------------------------------------------------------------------------------

==============================================================================*/

#include "attribute_editor.h"
#include "primitive_tool.h"
#include "imgui/imgui.h"
#include "mesh_object.h"

#include <DirectXMath.h>

using namespace DirectX;

void AttributeEditor::DrawForMesh(MeshObject& obj)
{
	ImGui::Text("Name: %s", obj.name.empty() ? "(unnamed)" : obj.name.c_str());
	ImGui::Text("ID: %u", obj.id);
	ImGui::Separator();

	ImGui::Checkbox("Visible", &obj.visible);
	ImGui::SameLine();
	ImGui::Checkbox("Pickable", &obj.pickable);

	ImGui::Separator();

	// Transform
	ImGui::Text("Transform");
	ImGui::Spacing();

	TransformTRS& trs = obj.transform;

	bool changed = false;

	const float posStep = 0.05f;
	const float rotStep = 0.2f;
	const float sclStep = 0.01f;

	// position
	if (ImGui::DragFloat3("Position", &trs.position.x, posStep))
		changed = true;

	// rotation
	if (ImGui::DragFloat3("Rotation", &trs.rotationQuat.x, rotStep))
		changed = true;

	static bool s_uniformScale = false;
	ImGui::Checkbox("Uniform scale", &s_uniformScale);

	if (!s_uniformScale)
	{
		if (ImGui::DragFloat3("Scale", &trs.scale.x, sclStep))
			changed = true;
	}
	else
	{
		float u = trs.scale.x;
		if (ImGui::DragFloat("Scale", &u, sclStep))
		{
			trs.scale = { u, u, u };
			changed = true;
		}
	}

	ImGui::Spacing();
	ImGui::Separator();

	// reset buttons
	if (ImGui::Button("Reset Position"))
	{
		trs.position = { 0, 0, 0 };
		changed = true;
	}
	//ImGui::SameLine();
	if (ImGui::Button("Reset Rotation"))
	{
		trs.rotationQuat = { 0, 0, 0, 1 };
		changed = true;
	}
	//ImGui::SameLine();
	if (ImGui::Button("Reset Scale"))
	{
		trs.scale = { 1, 1, 1 };
		changed = true;
	}

	if (changed)
	{
		obj.aabbValid = false;
	}
}

void AttributeEditor::DrawForCube(CubeObject& prim)
{
	ImGui::Text("Type: Primitive (Cube)");
	ImGui::Text("Index: %d / Count: %d", PrimitiveTool::GetSelectedIndex(), PrimitiveTool::GetCount());
	ImGui::Separator();

	bool changed = false;

	XMFLOAT3& pos = prim.GetPositionRef();
	const float posStep = 0.05f;

	ImGui::Text("Transform");
	ImGui::Spacing();

	// position
	if (ImGui::DragFloat3("Position", &pos.x, posStep)) changed = true;

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::Button("Reset Position"))
	{
		pos = { 0, 0, 0 };
		changed = true;
	}

	if (changed)
	{
		prim.UpdateAABB();
	}
}
