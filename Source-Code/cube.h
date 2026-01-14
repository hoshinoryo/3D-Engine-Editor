/*==============================================================================

Å@ cubeÇÃï\é¶[cube.h]
														 Author : Gu Anyi
														 Date   : 2025/11/05
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef CUBE_H
#define CUBE_H

#include <d3d11.h>
#include <DirectXMath.h>

#include "collision.h"
#include "aabb_provider.h"

class CubeObject : public IAABBProvider
{
public:

	CubeObject(float halfExtent = 1.0f);

	void SetPosition(const DirectX::XMFLOAT3& pos);
	void SetScale(const DirectX::XMFLOAT3& scl);

	const DirectX::XMFLOAT3& GetPosition() const { return m_Position; }
	const AABB& GetAABB() const override { return m_AABB; }

	void UpdateAABB();
	void Draw() const;

private:

	DirectX::XMFLOAT3 m_Position{};
	DirectX::XMFLOAT3 m_Scale{ 1, 1, 1 };

	float m_HalfExtent = 1.0f;
	AABB m_AABB{};
};


void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, float halfExtent = 1.0f);
void Cube_Finalize(void);
void Cube_DrawMesh(const DirectX::XMMATRIX& mtxWorld);

//void Cube_DrawPyramid(int layers = 5, float cubeSize = 1.0f, float gap = 0.0f);

#endif // CUBE_H
