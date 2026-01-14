/*==============================================================================

   Debug draw definitions [debug_draw_setting.h]
														 Author : Gu Anyi
														 Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef DEBUG_DRAW_SETTING_H
#define DEBUG_DRAW_SETTING_H

#include <cstdint>

enum class DebugDrawCategory : uint32_t
{
	Collision = 1u << 0,
};

// OR operation
inline DebugDrawCategory operator|(DebugDrawCategory a, DebugDrawCategory b)
{
	return (DebugDrawCategory)((uint32_t)a | (uint32_t)b);
}

// check if this flag is active
inline bool HasFlag(uint32_t mask, DebugDrawCategory c)
{
	return (mask & (uint32_t)c) != 0;
}

struct DebugDrawSettings
{
	bool enabled = true; // Main switch
	bool editorOnly = true;
	uint32_t categoryMask = (uint32_t)DebugDrawCategory::Collision;
};

DebugDrawSettings& GetDebugDrawSettings();

#endif // DEBUG_DRAW_SETTING_H
