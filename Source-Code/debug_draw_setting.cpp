/*==============================================================================

   Debug draw definitions [debug_draw_setting.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#include "debug_draw_setting.h"

static DebugDrawSettings g_DebugDrawSettings{}; // debug draw settings

DebugDrawSettings& GetDebugDrawSettings()
{
	return g_DebugDrawSettings;
}
