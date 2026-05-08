/*
 * buttons.c
 *
 *  Created on: Jun 25, 2023
 *      Author: max
 */

#include <stdlib.h>

#include "debug.h"
#include "ch32v00x.h"
#include "tick.h"

#include "buttons.h"

/*
*					BUTTONS CONNECTIONS
*
*			PD2 -> ACTION
*
*	PA1 -> LEFT		PC4 -> RIGHT
*
*	PC1 -> UP		PC2 -> DOWN
*
*/

volatile button_status_t BUTTONS[BTN_COUNT];

void BTN_Init(void)
{
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

	// PA1 input with pull resistor
	GPIOA->CFGLR &= ~(0b11 << 6);
	GPIOA->CFGLR |= (0b10 << 6);

	// PC1 input with pull resistor (UP)
	GPIOC->CFGLR &= ~(0b11 << 6);
	GPIOC->CFGLR |= (0b10 << 6);

	// PC2 input with pull resistor (DOWN)
	GPIOC->CFGLR &= ~(0b11 << 10);
	GPIOC->CFGLR |= (0b10 << 10);

	// PC4 input with pull resistor
	GPIOC->CFGLR &= ~(0b11 << 18);
	GPIOC->CFGLR |= (0b10 << 18);

	// PD2 input with pull resistor
	GPIOD->CFGLR &= ~(0b11 << 10);
	GPIOD->CFGLR |= (0b10 << 10);

	// Enable pull-ups
	GPIOA->OUTDR |= (1 << 1);
	GPIOC->OUTDR |= (1 << 1);
	GPIOC->OUTDR |= (1 << 2);
	GPIOC->OUTDR |= (1 << 4);
	GPIOD->OUTDR |= (1 << 2);


}

static uint8_t _BTN_CheckState(buttons_names_t button)
{
    switch (button)
    {

        case BTN_ACTION:
        return (GPIOD->INDR >> 2) & 1;

        case BTN_RIGHT:
        return (GPIOC->INDR >> 4) & 1;

        case BTN_UP:
        return (GPIOC->INDR >> 1) & 1;  // PC1

        case BTN_LEFT:
        return (GPIOA->INDR >> 1) & 1;

        case BTN_DOWN:
        return (GPIOC->INDR >> 2) & 1;  // PC2

        default:
        return 0;

    }
}

void BTN_Task(void)
{
	for(size_t i = 0; i < BTN_COUNT; i++)
	{
		switch(BUTTONS[i].state)
		{
			case BTN_STATE_IDLE:

				if(_BTN_CheckState(i) == 0)
				{
					BUTTONS[i].state = BTN_STATE_DEBOUNCE;
					BUTTONS[i].last_tick = TICK_Get();
				}

				break;

			case BTN_STATE_DEBOUNCE:

				if ((TICK_Get() - BUTTONS[i].last_tick) > BTN_DEBOUNCE_TIME)
				{
					if (_BTN_CheckState(i) == 0)
					{
						BUTTONS[i].state = BTN_STATE_PRESSED;
					}
					else
					{
						BUTTONS[i].state = BTN_STATE_IDLE;
					}

				}

				break;

			case BTN_STATE_PRESSED:

				if (_BTN_CheckState(i) == 1)
				{
					BUTTONS[i].state = BTN_STATE_IDLE;
				}

				break;
		}
	}
}

uint8_t BTN_IsPressed(buttons_names_t button)
{
	if(BUTTONS[button].state == BTN_STATE_PRESSED) return 1;
	return 0;
}

uint8_t BTN_IsPressedRAW(buttons_names_t button)
{
	if(!_BTN_CheckState(button)) return 1;
	return 0;
}
