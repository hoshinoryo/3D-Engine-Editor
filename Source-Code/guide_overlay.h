/*==============================================================================

   Guide image drawing [guide_overlay.h]
														 Author : Gu Anyi
														 Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef GUIDE_OVERLAY_H
#define GUIDE_OVERLAY_H

namespace GuideOverlay
{
	void Initialize();
	void Finalize();

	// Driven by CameraManager
	void OnEnterPlayMode();
	void OnEnterEditorMode();

	void Draw();
}

#endif // GUIDE_OVERLAY_H
