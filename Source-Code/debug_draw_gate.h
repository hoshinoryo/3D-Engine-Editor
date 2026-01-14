/*==============================================================================

   Debug draw policy gate [debug_draw_gate.h]
														 Author : Gu Anyi
														 Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef DEBUG_DRAW_GATE_H
#define DEBUG_DRAW_GATE_H

#include "debug_draw_setting.h"
#include "mode_management.h"

// 今このカテゴリのデバッグ描画をして良いか
inline bool DebugDraw_Allow(DebugDrawCategory category)
{
	const auto& s = GetDebugDrawSettings();

	if (!s.enabled) return false;
	if (s.editorOnly && GetAppMode() != AppMode::Editor) return false;
	if (!HasFlag(s.categoryMask, category)) return false;

	return true;
}

#endif // DEBUG_DRAW_GATE_H