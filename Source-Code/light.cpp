/*==============================================================================

   ライトの設定とマネジメント [light.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/04

--------------------------------------------------------------------------------

==============================================================================*/

#include <cstdio>

#include "light.h"
#include "direct3d.h"
#include "imgui/imgui.h"
#include "draw3d.h"

using namespace DirectX;


LightManager g_LightManager;


void LightManager::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	// Ambient(Slot 1)
	buffer_desc.ByteWidth = sizeof(AmbientLightData); // buffer size
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer1); // ambient light

	// Directional(Slot 2)
	buffer_desc.ByteWidth = sizeof(DirectionalLightData); // directional light buffer size
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer2); // directional light

	/*
	// Specular(Slot 3)
	buffer_desc.ByteWidth = sizeof(SpecularLightData); // specular light buffer size
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer3); // specular light
	*/

	// Point Light List(Slot 4)
	buffer_desc.ByteWidth = sizeof(PointLightList); // specular light buffer size
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer4); // specular light
}

void LightManager::Finalize()
{
	SAFE_RELEASE(g_pPSConstantBuffer4);
	//SAFE_RELEASE(g_pPSConstantBuffer3);
	SAFE_RELEASE(g_pPSConstantBuffer2);
	SAFE_RELEASE(g_pPSConstantBuffer1);
}

void LightManager::SetAmbient(const XMFLOAT4& color)
{
	m_AmbientData.Color = color;
}

void LightManager::SetDirectionalWorld(const XMFLOAT4& directional, const XMFLOAT4& color)
{
	m_DirectionalData.Directional = directional;
	m_DirectionalData.Color = color;
}

/*
void LightManager::SetSpecularWorld(const XMFLOAT3& cameraPos, float power, const XMFLOAT4& color)
{
	m_SpecularData.CameraPosition = cameraPos;
	m_SpecularData.Power = power;
	m_SpecularData.Color = color;
}
*/

void LightManager::SetPointLightCount(int count)
{
	int old_count = m_PointLights.count;

	if (count > 4) count = 4;
	if (count < 0) count = 0;

	m_PointLights.count = count;

	for (int i = old_count; i < m_PointLights.count; i++)
	{
		m_PointLights.point_light[i].LightPosition = XMFLOAT3( 0.0f, 0.0f, 0.0f );
		m_PointLights.point_light[i].Range = 5.0f;
		m_PointLights.point_light[i].Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	}
}

void LightManager::SetPointLight(int n, const XMFLOAT3& position, float range, const XMFLOAT3& color)
{
	if (n >= 0 && n < 4)
	{
		m_PointLights.point_light[n].LightPosition = position;
		m_PointLights.point_light[n].Range = range;
		m_PointLights.point_light[n].Color = { color.x, color.y, color.z, 1.0f };
	}
}

void LightManager::BindAllLightsToPipeline()
{
	// Slot 1: Ambient
	g_pContext->UpdateSubresource(g_pPSConstantBuffer1, 0, nullptr, &m_AmbientData, 0, 0);
	g_pContext->PSSetConstantBuffers(1, 1, &g_pPSConstantBuffer1);

	// Slot 2: Directional
	g_pContext->UpdateSubresource(g_pPSConstantBuffer2, 0, nullptr, &m_DirectionalData, 0, 0);
	g_pContext->PSSetConstantBuffers(2, 1, &g_pPSConstantBuffer2);

	/*
	// Slot 3: Specular
	g_pContext->UpdateSubresource(g_pPSConstantBuffer3, 0, nullptr, &m_SpecularData, 0, 0);
	g_pContext->PSSetConstantBuffers(3, 1, &g_pPSConstantBuffer3);
	*/

	// Slot 4: Point Lights List
	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &m_PointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);
}

void LightManager::DebugDraw()
{
	// --- Ambient Light Control ---
	if (ImGui::CollapsingHeader("Ambient Light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		XMFLOAT4 ambient = m_AmbientData.Color;
		if (ImGui::ColorEdit4("Color##Ambient", &ambient.x))
		{
			m_AmbientData.Color = ambient;
		}
	}

	// --- Directional Light Control ---
	if (ImGui::CollapsingHeader("Directional Light",ImGuiTreeNodeFlags_DefaultOpen))
	{
		XMFLOAT4 color = m_DirectionalData.Color;
		if (ImGui::ColorEdit4("Color##Directional", &color.x))
		{
			m_DirectionalData.Color = color;
		}

		XMFLOAT4 dir = m_DirectionalData.Directional;
		if (ImGui::SliderFloat3("Direction##Dir", &dir.x, -1.0f, 1.0f))
		{
			m_DirectionalData.Directional = dir;

			XMVECTOR v = XMLoadFloat4(&m_DirectionalData.Directional);
			v = XMVector3Normalize(v);
			XMStoreFloat4(&m_DirectionalData.Directional, v);
		}
	}

	// --- Point Light Control ---
	if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::InputInt("Count", &m_PointLights.count);

		if (m_PointLights.count > 4) m_PointLights.count = 4;
		if (m_PointLights.count < 0) m_PointLights.count = 0;

		for (int i = 0; i < m_PointLights.count; i++)
		{
			ImGui::PushID(i);

			char light_label[32];
			std::snprintf(light_label, 32, "Point Light %d", i+1);

			if (ImGui::CollapsingHeader(light_label, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("Position", (float*)&m_PointLights.point_light[i].LightPosition, 0.1f);
				ImGui::SliderFloat("Range", &m_PointLights.point_light[i].Range, 0.1f, 20.0f);
				ImGui::ColorEdit3("Color", &m_PointLights.point_light[i].Color.x);
			}
			m_PointLights.point_light[i].Color.w = 1.0f;

			ImGui::PopID();
		}
	}
}

void LightManager::DebugDrawPointLight() const
{
	for (int i = 0; i < m_PointLights.count; ++i)
	{
		const auto& pl = m_PointLights.point_light[i];

		float radius = pl.Range * 0.03f;

		XMFLOAT4 sphereColor = pl.Color;

		Draw3d_MakeWireSphere(pl.LightPosition, radius, sphereColor);
	}
}
