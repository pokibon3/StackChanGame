#ifndef GAMELIB_GAMEFUNC_H_
#define GAMELIB_GAMEFUNC_H_

#include "buttons.h"

void GameLib_Init(void);
void GameLib_DrawMessage(const char *line1, const char *line2);
void GameLib_ClearScreen(uint8_t color);
uint8_t ButtonPressedEdge(buttons_names_t button, uint8_t *previous);

#endif /* GAMELIB_GAMEFUNC_H_ */
