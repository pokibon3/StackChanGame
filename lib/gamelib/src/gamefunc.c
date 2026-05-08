#include "gamefunc.h"

#include "ch32v00x.h"
#include "sound.h"
#include "st7735.h"
#include "system_ch32v00x.h"
#include "termGFX.h"
#include "tick.h"

static __attribute__((noinline, used)) void InitSystemClock(void);

static __attribute__((noinline, used)) void InitSystemClock(void)
{
    /* 48 MHz requires 1 flash wait state */
    FLASH->ACTLR = FLASH_ACTLR_LATENCY_1;
    /* PLL source = HSI*2, HCLK = SYSCLK */
    RCC->CFGR0 = 0;
    /* Enable PLL (HSI * 2 = 48 MHz) */
    RCC->CTLR |= RCC_PLLON;
    while (!(RCC->CTLR & RCC_PLLRDY));
    /* Switch system clock to PLL */
    RCC->CFGR0 |= RCC_SW_PLL;
    while ((RCC->CFGR0 & RCC_SWS) != 0x08);
}

void GameLib_Init(void)
{
    InitSystemClock();
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    TICK_Init();
    LCD_Init();
    SND_Init();
    BTN_Init();
}

void GameLib_DrawMessage(const char *line1, const char *line2)
{
    tGFX_SetCursor(5, 5);
    tGFX_Print((char *)line1, 15, 0);
    tGFX_SetCursor(3, 6);
    tGFX_Print((char *)line2, 15, 0);
}

void GameLib_ClearScreen(uint8_t color)
{
    uint8_t x;
    uint8_t y;

    for (x = 0; x < TERM_SIZE_X; ++x) {
        for (y = 0; y < TERM_SIZE_Y; ++y) {
            tGFX_SetChar(' ', x, y, 15, color);
        }
    }
}

uint8_t ButtonPressedEdge(buttons_names_t button, uint8_t *previous)
{
    uint8_t current = BTN_IsPressed(button);

    if (current && !(*previous)) {
        *previous = current;
        return 1;
    }

    *previous = current;
    return 0;
}
