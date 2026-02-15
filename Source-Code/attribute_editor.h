/*==============================================================================

   Attribute editor drawing [attribute_editor.h]
														 Author : Gu Anyi
														 Date   : 2026/02/14
--------------------------------------------------------------------------------

==============================================================================*/

struct MeshObject;
class CubeObject;

namespace AttributeEditor
{
	void DrawForMesh(MeshObject& obj);
	void DrawForCube(CubeObject& prim);
}