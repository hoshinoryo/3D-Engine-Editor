/*==============================================================================

   âeÇÃï`âÊä«óù [shadow_pass.h]
														 Author : Gu Anyi
														 Date   : 2026/02/17

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SHADOW_PASS_H
#define SHADOW_PASS_H

#include <d3d11.h>
#include <DirectXMath.h> 

#include "d3d11_state_guard_util.h"

struct ShadowConstantBuffer
{
	DirectX::XMFLOAT4X4 lightViewProj;
	float shadowBias;
	DirectX::XMFLOAT3 padding;
};

class ShadowPass
{
public:

	bool Initialize(ID3D11Device* device, int shadowMapSize = 2048, UINT shadowSRVSlot = 3);
	void Finalize();

	void Begin(ID3D11DeviceContext* ctx);
	void End(ID3D11DeviceContext* ctx);

	void SetLightViewProj(const DirectX::XMFLOAT4X4& lightViewProj);
	const DirectX::XMFLOAT4X4& GetLightViewProj() const { return m_LightViewProj; }

	void BindShadowMapSRV(ID3D11DeviceContext* ctx) const;
	void PrepareForMainPass(ID3D11DeviceContext* ctx);

	int GetShadowMapSize() const { return m_Size; }
	UINT GetShadowSRVSlot() const { return m_ShadowSRVSlot; }

public:

	struct LightCamera
	{
	public:

		bool Initialize(ID3D11Device* device);
		void Finalize();

		// dirW: light direction
		// targetW: focus point
		// distance: distance between light camera and target
		// orthoW/H: ortho width and height
		// nearZ/farZ: shadow depth
		void UpdateDirectional(
			const DirectX::XMFLOAT3& dirW,
			const DirectX::XMFLOAT3& targetW,
			float distance,
			float orthoW, float orthoH,
			float nearZ, float farZ);

		void BindToVS(ID3D11DeviceContext* ctx, UINT viewSlot = 1, UINT projSlot = 2) const;

		const DirectX::XMFLOAT4X4& GetViewRaw() const { return m_View; }
		const DirectX::XMFLOAT4X4& GetProjRaw() const { return m_Proj; }
		const DirectX::XMFLOAT4X4& GetView() const { return m_ViewT; }
		const DirectX::XMFLOAT4X4& GetProj() const { return m_ProjT; }

		ID3D11Buffer* GetViewCB() const { return m_pVSConstantBufferView; }
		ID3D11Buffer* GetProjCB() const { return m_pVSConstantBufferProj; }

		void Upload();

	private:

		DirectX::XMFLOAT3 m_Position{};
		DirectX::XMFLOAT3 m_Target{};
		DirectX::XMFLOAT4X4 m_View{};
		DirectX::XMFLOAT4X4 m_Proj{};
		DirectX::XMFLOAT4X4 m_ViewT{};
		DirectX::XMFLOAT4X4 m_ProjT{};

		ID3D11Buffer* m_pVSConstantBufferView = nullptr;
		ID3D11Buffer* m_pVSConstantBufferProj = nullptr;
	};

	LightCamera& GetLightCamera() { return m_LightCam; }
	const LightCamera& GetLightCamera() const { return m_LightCam; }

private:

	bool CreateShadowMapResources(ID3D11Device* device);

private:

	int m_Size = 2048;
	UINT m_ShadowSRVSlot = 3;

	DirectX::XMFLOAT4X4 m_LightViewProj{};

	ID3D11Texture2D*          m_ShadowTex = nullptr;
	ID3D11DepthStencilView*   m_ShadowDSV = nullptr; // depth for shadow pass
	ID3D11ShaderResourceView* m_ShadowSRV = nullptr; // sampling depth for main pass
	
	D3D11_VIEWPORT m_Viewport{};

	D3D11StateGuard m_Guard;

	ID3D11ShaderResourceView* m_OldShadowSRV = nullptr;
	ID3D11Buffer* m_pShadowCB = nullptr;

	LightCamera m_LightCam;
};

#endif // SHADOW_PASS