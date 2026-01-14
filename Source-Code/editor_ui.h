/*==============================================================================

   Editor UI Framework [editor_ui.h]
														 Author : Gu Anyi
														 Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#include <memory>

#include "imgui/imgui.h"


namespace EditorUI
{
	struct Layout
	{
		float padding         = 10.0f;
		float maxHeight       = 0.0f;
		float minWidth        = 0.0f;
		float initWidthNarrow = 0.0f;
		float initWidthWide   = 0.0f;
		float initHeight      = 0.0f;

		ImVec2 displaySize{};
		float fontScale = 1.0f;
	};

	// Window interface
	struct EditorWindow
	{
		virtual ~EditorWindow() = default;
		virtual const char* Name() const = 0;
		virtual void Draw(const Layout& layout) = 0;

		bool enabled = true;

		bool AutoFitEnabled() const { return autoFitFrames > 0; }
		void ConsumeAutoFitFrame()
		{
			if (autoFitFrames > 0) --autoFitFrames;
		}

	protected:

		// For first N frames
		int autoFitFrames = 2;
	};

	// Lifetime
	void Initialize(); // register windows
	void Shutdown();

	// Per-frame
	void BeginFrame();
	void DrawWindows();
}

#endif // EDITOR_UI_H