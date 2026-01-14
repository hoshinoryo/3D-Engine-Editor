/*==============================================================================

   Mode switching management [mode_management.h]
														 Author : Gu Anyi
														 Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef MODE_MANAGEMENT_H
#define MODE_MANAGEMENT_H

#include "debug_draw_setting.h"

enum class AppMode
{
	Editor,
	Gameplay,
};

AppMode GetAppMode();

#endif // MODE_MANAGEMENT_H