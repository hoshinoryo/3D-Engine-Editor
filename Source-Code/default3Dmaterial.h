/*==============================================================================

   Default 3d material [default3Dmaterial.h]
														 Author : Gu Anyi
														 Date   : 2025/11/18
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef DEFAULT_3D_MATERIAL_H
#define	DEFAULT_3D_MATERIAL_H

#include <DirectXMath.h>
#include <string>

class Default3DShader;

class Default3DMaterial
{
private:

	DirectX::XMFLOAT4 m_BaseColor;
	DirectX::XMFLOAT4 m_SpecularColor;
	float m_SpecularPower;
	bool m_SpecularEnabled;

	// For template map file path
	std::string m_DiffuseMapPath;
	std::string m_NormalMapPath;
	std::string m_SpecularMapPath;

	// Material name
	std::string m_Name;

public:

	Default3DMaterial();

	// ---- Material Attribute ----
	void SetBaseColor(const DirectX::XMFLOAT4& color);
	void SetSpecularColor(const DirectX::XMFLOAT4& color);
	void SetSpecularPower(float power);
	void SetSpecularEnabled(bool enabled);

	const DirectX::XMFLOAT4& GetBaseColor() const { return m_BaseColor; }
	const DirectX::XMFLOAT4& GetSpecularColor() const { return m_SpecularColor; }
	float GetSpecularPower() const { return m_SpecularPower; }
	bool GetSpecularEnabled() const { return m_SpecularEnabled; }

	void SetDiffuseMapPath(const std::string& path) { m_DiffuseMapPath = path; }
	void SetNormalMapPath(const std::string& path) { m_NormalMapPath = path; }
	void SetSpecularMapPath(const std::string& path) { m_SpecularMapPath = path; }

	const std::string& GetDiffuseMapPath() const { return m_DiffuseMapPath; }
	const std::string& GetNormalMapPath() const { return m_NormalMapPath; }
	const std::string& GetSpecularMapPath() const { return m_SpecularMapPath; }

	void SetName(const std::string& name) { m_Name = name; }
	const std::string& GetName() const { return m_Name; }

	// Apply attribution to shader
	void Apply(Default3DShader& shader, const DirectX::XMFLOAT3& cameraPos) const;
	
	// Material management
	void DebugDraw(Default3DShader& shader, const DirectX::XMFLOAT3& cameraPos);
};

extern Default3DMaterial g_DefaultSceneMaterial;

void Default3DMaterial_Register(Default3DMaterial* material);
void Default3DMaterial_Unregister(Default3DMaterial* material);


#endif DEFAULT_3D_MATERIAL_H
