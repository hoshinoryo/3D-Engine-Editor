/*==============================================================================

   Editor windows [editor_windows.h]
														 Author : Gu Anyi
														 Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef EDITOR_WINDOWS_H
#define EDITOR_WINDOWS_H

#include <memory>
#include <vector>

#include "editor_ui.h"

namespace EditorWindows
{
	// Resources needed by windows
	void InitializeResources();
	void ShutdownResources();

	// Create default windows
	std::vector<std::unique_ptr<EditorUI::EditorWindow>> CreateDefaultWindows();
}

#endif // EDITOR_WINDOWS_H