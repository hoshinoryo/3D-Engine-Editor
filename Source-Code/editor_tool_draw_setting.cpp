/*==============================================================================

   Editor tool draw definitions [editor_tool_gate.cpp]
														 Author : Gu Anyi
														 Date   : 2026/02/08
--------------------------------------------------------------------------------

==============================================================================*/

#include "editor_tool_draw_setting.h"

static EditorToolDrawSettings g_EditorToolDrawSettings{}; // debug draw settings

EditorToolDrawSettings& GetEditorToolDrawSettings()
{
	return g_EditorToolDrawSettings;
}
