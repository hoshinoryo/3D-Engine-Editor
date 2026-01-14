/*==============================================================================

   ライトの設定とマネジメント [light.h]
														 Author : Gu Anyi
														 Date   : 2025/11/04

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef LIGHT_H
#define LIGHT_H

#include <d3d11.h>
#include <DirectXMath.h>


// Light struct
struct AmbientLightData
{
	DirectX::XMFLOAT4 Color;
};

struct DirectionalLightData
{
	DirectX::XMFLOAT4 Directional;
	DirectX::XMFLOAT4 Color;
};

/*
struct SpecularLightData
{
	DirectX::XMFLOAT3 CameraPosition;
	float Power;
	DirectX::XMFLOAT4 Color;
};
*/

struct PointLightData // for single point light
{
	DirectX::XMFLOAT3 LightPosition;
	float Range;
	DirectX::XMFLOAT4 Color;
};

struct PointLightList
{
	PointLightData point_light[4];
	int count;
	DirectX::XMFLOAT3 point_light_dummy;
};

class LightManager
{
private:

	ID3D11Device* g_pDevice = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;

	ID3D11Buffer* g_pPSConstantBuffer1 = nullptr; // ambient color for pixel shader
	ID3D11Buffer* g_pPSConstantBuffer2 = nullptr; // directional light
	//ID3D11Buffer* g_pPSConstantBuffer3 = nullptr; // specular light
	ID3D11Buffer* g_pPSConstantBuffer4 = nullptr; // point light

	AmbientLightData m_AmbientData{};
	DirectionalLightData m_DirectionalData{};
	//SpecularLightData m_SpecularData{};
	PointLightList m_PointLights{};

public:

	LightManager() = default;
	~LightManager() = default;

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Finalize();

	void SetAmbient(const DirectX::XMFLOAT4& color);
	void SetDirectionalWorld(const DirectX::XMFLOAT4& directional, const DirectX::XMFLOAT4& color);

	void SetPointLightCount(int count);
	void SetPointLight(
		int n,
		const DirectX::XMFLOAT3& position,
		float range,
		const DirectX::XMFLOAT3& color
	);

	void BindAllLightsToPipeline();

	void DebugDraw();
	void DebugDrawPointLight() const;
};

extern LightManager g_LightManager; // Global light management declaration

#endif // LIGHT_H
