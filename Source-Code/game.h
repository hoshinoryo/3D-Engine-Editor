/*==============================================================================

Å@ ÉQÅ[ÉÄñ{ëÃ[game.h]
                                                         Author : Youhei Sato
                                                         Date   : 2025/06/27
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef GAME_H
#define GAME_H


void Game_Initialize();
void Game_Finalize();

void Game_Update(double elapsed_time);
void Game_Draw();

void Game_DrawCameraDebugUI();
void Game_DrawLightDebugUI();
void Game_DrawMaterialManager();


#endif // GAME_H
