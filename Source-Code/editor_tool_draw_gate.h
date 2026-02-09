/*==============================================================================

   Editor tool draw policy gate [editor_tool_draw_gate.h]
														 Author : Gu Anyi
														 Date   : 2026/02/08
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef EDITOR_TOOL_DRAW_GATE_H
#define EDITOR_TOOL_DRAW_GATE_H

#include "editor_tool_draw_setting.h"
#include "mode_management.h"

// 今このカテゴリのデバッグ描画をして良いか
inline bool EditorTool_Allow(EditorToolCategory category)
{
	const auto& s = GetEditorToolDrawSettings();

	if (!s.enabled) return false;
	if (s.editorOnly && GetAppMode() != AppMode::Editor) return false;
	if (!HasToolFlag(s.categoryMask, category)) return false;

	return true;
}

#endif // EDITOR_TOOL_DRAW_GATE_H
