/*==============================================================================

   Editor tool draw definitions [editor_tool_gate.h]
														 Author : Gu Anyi
														 Date   : 2026/02/08
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef EDITOR_TOOL_DRAW_SETTING_H
#define EDITOR_TOOL_DRAW_SETTING_H

#include <cstdint>

enum class EditorToolCategory : uint32_t
{
	GizmoTranslate = 1u << 0,
	Picking        = 1u << 1,
	Outline        = 1u << 2,
};

inline EditorToolCategory operator|(EditorToolCategory a, EditorToolCategory b)
{
	return (EditorToolCategory)((uint32_t)a | (uint32_t)b);
}

inline bool HasToolFlag(uint32_t mask, EditorToolCategory c)
{
	return (mask & (uint32_t)c) != 0;
}

struct EditorToolDrawSettings
{
	bool enabled = true; // Main switch
	bool editorOnly = true;
	uint32_t categoryMask =
		(uint32_t)EditorToolCategory::GizmoTranslate |
		(uint32_t)EditorToolCategory::Picking |
		(uint32_t)EditorToolCategory::Outline;
};

EditorToolDrawSettings& GetEditorToolDrawSettings();

#endif // EDITOR_TOOL_DRAW_SETTING_H
