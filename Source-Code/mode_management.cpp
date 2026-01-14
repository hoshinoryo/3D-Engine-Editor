/*==============================================================================

   Mode switching management [mode_management.cpp]
                                                         Author : Gu Anyi
                                                         Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#include "mode_management.h"
#include "camera_manager.h"

AppMode GetAppMode()
{
    return CameraManager::IsPlayMode()
        ? AppMode::Gameplay
        : AppMode::Editor;
}
