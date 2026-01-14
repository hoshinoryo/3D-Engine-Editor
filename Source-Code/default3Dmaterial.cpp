/*==============================================================================

   Default 3d material [default3Dmaterial.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/18
--------------------------------------------------------------------------------

==============================================================================*/

#include "default3Dmaterial.h"
#include "default3Dshader.h"

#include "imgui/imgui.h"

#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

Default3DMaterial g_DefaultSceneMaterial;

static std::vector<Default3DMaterial*> s_AllMaterials;
static int s_SelectedMaterialIndex = -1;


Default3DMaterial::Default3DMaterial()
	: m_BaseColor (0.5f, 0.5f, 0.5f, 1.0f)
	, m_SpecularColor (1.0f, 1.0f, 1.0f, 1.0f)
	, m_SpecularPower (30.0f)
	, m_SpecularEnabled(true)
	, m_DiffuseMapPath()
	, m_NormalMapPath()
	, m_SpecularMapPath()
	, m_Name("Material")
{
}

void Default3DMaterial::SetBaseColor(const DirectX::XMFLOAT4& color)
{
	m_BaseColor = color;
}

void Default3DMaterial::SetSpecularColor(const DirectX::XMFLOAT4& color)
{
	m_SpecularColor = color;
}

void Default3DMaterial::SetSpecularPower(float power)
{
	m_SpecularPower = power;
}

void Default3DMaterial::SetSpecularEnabled(bool enabled)
{
	m_SpecularEnabled = enabled;

	/*
	static XMFLOAT4 lastColor = m_SpecularColor;

	if (!m_SpecularEnabled)
	{
		lastColor = m_SpecularColor;
		m_SpecularColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	}
	else
	{
		m_SpecularColor = lastColor;
	}
	*/
}

void Default3DMaterial::Apply(Default3DShader& shader, const DirectX::XMFLOAT3& cameraPos) const
{
	shader.SetColor(m_BaseColor);

	XMFLOAT4 specColor = m_SpecularEnabled
		? m_SpecularColor
		: XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	shader.UpdateSpecularParams(cameraPos, m_SpecularPower, specColor);
}

void Default3DMaterial::DebugDraw(Default3DShader& shader, const DirectX::XMFLOAT3& cameraPos)
{
	(void)shader;
	(void)cameraPos;

	//ImGui::Begin("Material Manager");

	if (s_AllMaterials.empty())
	{
		ImGui::Text("No materials in scene.");
		ImGui::End();
		return;
	}

	ImGui::Columns(2);

	// LEFT : Material list
	ImGui::Text("Materials");
	ImGui::Separator();

	for (int i = 0; i < static_cast<int>(s_AllMaterials.size()); ++i)
	{
		Default3DMaterial* mat = s_AllMaterials[i];
		const std::string& name = mat->GetName();
		const char* label = name.empty() ? "Unnamed Material" : name.c_str();

		bool selected = (i == s_SelectedMaterialIndex);
		if (ImGui::Selectable(label, selected))
		{
			s_SelectedMaterialIndex = i;
		}
	}

	ImGui::NextColumn();

	// RIGHT : Material attribute
	ImGui::Text("Material Properties");
	ImGui::Separator();

	if (s_SelectedMaterialIndex >= 0 &&
		s_SelectedMaterialIndex < static_cast<int>(s_AllMaterials.size()))
	{
		Default3DMaterial* mat = s_AllMaterials[s_SelectedMaterialIndex];

		// Name
		std::string name = mat->GetName();
		char nameBuf[128];
		memset(nameBuf, 0, sizeof(nameBuf));
		strncpy_s(nameBuf, name.c_str(), sizeof(nameBuf) - 1);

		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
		{
			mat->SetName(nameBuf);
		}

		// Base Color
		XMFLOAT4 base = mat->GetBaseColor();
		if (ImGui::ColorEdit4("Base Color", &base.x))
		{
			mat->SetBaseColor(base);
		}

		// Specular Eanbled
		bool specEnabled = mat->GetSpecularEnabled();
		if (ImGui::Checkbox("Enable Specular", &specEnabled))
		{
			mat->SetSpecularEnabled(specEnabled);
		}

		// Specular Power & Color
		float power = mat->GetSpecularPower();
		if (ImGui::SliderFloat("Specular Power", &power, 0.1f, 50.0f))
		{
			mat->SetSpecularPower(power);
		}
		XMFLOAT4 spec = mat->GetSpecularColor();
		if (ImGui::ColorEdit4("Specular Color", &spec.x))
		{
			mat->SetSpecularColor(spec);
		}

		ImGui::Separator();

		/*
		ImGui::Separator();
		ImGui::Text("Diffuse Map: %s", mat->GetDiffuseMapPath().empty() ? "(none)" : mat->GetDiffuseMapPath());
		ImGui::Text("Normal Map: %s", mat->GetNormalMapPath().empty() ? "(none)" : mat->GetNormalMapPath());
		ImGui::Text("Specular Map: %s", mat->GetSpecularMapPath().empty() ? "(none)" : mat->GetSpecularMapPath());
		*/
	}
	else
	{
		ImGui::Text("No material selected.");
	}

	ImGui::Columns(1);
	//ImGui::End();

	//Apply(shader, cameraPos);
}

void Defaul3DMaterial_Register(Default3DMaterial* material)
{
	if (!material) return;
	s_AllMaterials.push_back(material);

	if (s_SelectedMaterialIndex < 0)
	{
		s_SelectedMaterialIndex = 0;
	}
}

void Defaul3DMaterial_Unregister(Default3DMaterial* material)
{
	if (!material) return;

	auto it = std::find(s_AllMaterials.begin(), s_AllMaterials.end(), material);
	if (it == s_AllMaterials.end()) return;

	int idx = static_cast<int>(it - s_AllMaterials.begin());
	s_AllMaterials.erase(it);

	if (s_AllMaterials.empty())
	{
		s_SelectedMaterialIndex = -1;
	}
	else
	{
		if (s_SelectedMaterialIndex >= static_cast<int>(s_AllMaterials.size()))
		{
			s_SelectedMaterialIndex = static_cast<int>(s_AllMaterials.size()) - 1;
		}
		else if (idx < s_SelectedMaterialIndex)
		{
			--s_SelectedMaterialIndex;
		}
	}
}
