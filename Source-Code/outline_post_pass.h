/*==============================================================================

   Outline drawing [outline_post_pass.h]
														 Author : Gu Anyi
														 Date   : 2026/01/08
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef OUTLINE_POST_PASS_H
#define OUTLINE_POST_PASS_H

#include <d3d11.h>
#include <cstdint>

#include "outline_shader.h"

class OutlinePostPass
{
public:

	bool Initialize(uint32_t width, uint32_t height);
	void Finalize();

	void Resize(uint32_t width, uint32_t height);

	void DrawModel(
		ID3D11ShaderResourceView* idSRV,
		uint32_t selectedId,
		uint32_t thickness,
		const float color[4]
	);

private:

	uint32_t m_Width = 0;
	uint32_t m_Height = 0;

	ID3D11Device*        m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	OutlineShader m_OutlineShader;

};

#endif // OUTLINE_POST_PASS_H

