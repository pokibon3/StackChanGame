#include <stdio.h>

#include "buttons.h"
#include "gamefunc.h"
#include "sound.h"
#include "st7735.h"
#include "termGFX.h"
#include "tick.h"

#define HUD_ROW 0
#define FIELD_LEFT_CHAR 1
#define FIELD_RIGHT_CHAR (TERM_SIZE_X - 2)
#define FIELD_TOP_CHAR 1
#define FIELD_BOTTOM_CHAR (TERM_SIZE_Y - 1)
#define BRICK_ROWS 3
#define BRICK_COLS 8
#define FRAME_MS 8
#define PADDLE_STEP_MS 4
#define PADDLE_STEP_PX 4
#define START_LIVES 3
#define SOUND_PITCH_SCALE 4
#define SOUND_TIME_SCALE(time) ((uint8_t)(((time) > 1) ? ((time) / 2) : 1))
#define SCALE_FREQ(freq) ((uint16_t)((freq) * SOUND_PITCH_SCALE))

#define FIELD_LEFT_PX (FIELD_LEFT_CHAR * TERM_BLOCK_SIZE)
#define FIELD_RIGHT_PX (((FIELD_RIGHT_CHAR + 1) * TERM_BLOCK_SIZE) - 1)
#define FIELD_TOP_PX (FIELD_TOP_CHAR * TERM_BLOCK_SIZE)
#define FIELD_WIDTH_PX (FIELD_RIGHT_PX - FIELD_LEFT_PX + 1)
#define PADDLE_Y_PX ((FIELD_BOTTOM_CHAR * TERM_BLOCK_SIZE) - 2)
#define PADDLE_WIDTH_PX 16
#define PADDLE_HEIGHT_PX 3
#define BALL_SIZE_PX 3
#define BRICK_SLOT_WIDTH_PX (FIELD_WIDTH_PX / BRICK_COLS)
#define BRICK_SLOT_HEIGHT_PX 6
#define BRICK_WIDTH_PX (BRICK_SLOT_WIDTH_PX - 2)
#define BRICK_HEIGHT_PX 4
#define BALL_START_DX 3
#define BALL_START_DY -3

typedef enum
{
    GAME_PLAYING,
    GAME_ROUND_WAIT,
    GAME_WON,
    GAME_OVER
} game_state_t;

typedef struct
{
    game_state_t state;
    uint32_t round_start_tick;
    uint8_t lives;
    uint8_t score;
    int16_t paddle_x_px;
    int16_t ball_x_px;
    int16_t ball_y_px;
    int8_t ball_dx_px;
    int8_t ball_dy_px;
    uint8_t bricks[BRICK_ROWS][BRICK_COLS];
} breakout_game_t;

static breakout_game_t game;

static term_sprite_t brick_sprites[BRICK_ROWS * BRICK_COLS];

static const uint8_t paddle_sprite_data[(PADDLE_WIDTH_PX / 4) * PADDLE_HEIGHT_PX] = {
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55
};

static const uint8_t ball_sprite_data[((BALL_SIZE_PX + 3) / 4) * BALL_SIZE_PX] = {
    0x54, 0x54, 0x54
};

static const uint8_t brick_sprite_data[((BRICK_WIDTH_PX + 3) / 4) * BRICK_HEIGHT_PX] = {
    0x55, 0x55, 0x55, 0x50,
    0x55, 0x55, 0x55, 0x50,
    0x55, 0x55, 0x55, 0x50,
    0x55, 0x55, 0x55, 0x50
};

static const uint8_t left_border_sprite_data[(LCD_HEIGHT - FIELD_TOP_PX)] = {
    [0 ...(LCD_HEIGHT - FIELD_TOP_PX - 1)] = 0x40
};

static const uint8_t right_border_sprite_data[(LCD_HEIGHT - FIELD_TOP_PX)] = {
    [0 ...(LCD_HEIGHT - FIELD_TOP_PX - 1)] = 0x40
};

static const uint8_t top_border_sprite_data[((FIELD_WIDTH_PX + 2 + 3) / 4)] = {
    [0 ...(((FIELD_WIDTH_PX + 2 + 3) / 4) - 1)] = 0x55
};

static term_sprite_t paddle_sprite = {
    .x = 0,
    .y = PADDLE_Y_PX,
    .size_x = PADDLE_WIDTH_PX,
    .size_y = PADDLE_HEIGHT_PX,
    .data = (uint8_t *)paddle_sprite_data,
    .pallet = 0
};

static term_sprite_t ball_sprite = {
    .x = 0,
    .y = 0,
    .size_x = BALL_SIZE_PX,
    .size_y = BALL_SIZE_PX,
    .data = (uint8_t *)ball_sprite_data,
    .pallet = 5
};

static term_sprite_t left_border_sprite = {
    .x = FIELD_LEFT_PX - 1,
    .y = FIELD_TOP_PX,
    .size_x = 1,
    .size_y = LCD_HEIGHT - FIELD_TOP_PX,
    .data = (uint8_t *)left_border_sprite_data,
    .pallet = 0
};

static term_sprite_t right_border_sprite = {
    .x = FIELD_RIGHT_PX + 1,
    .y = FIELD_TOP_PX,
    .size_x = 1,
    .size_y = LCD_HEIGHT - FIELD_TOP_PX,
    .data = (uint8_t *)right_border_sprite_data,
    .pallet = 0
};

static term_sprite_t top_border_sprite = {
    .x = FIELD_LEFT_PX - 1,
    .y = FIELD_TOP_PX,
    .size_x = FIELD_WIDTH_PX + 2,
    .size_y = 1,
    .data = (uint8_t *)top_border_sprite_data,
    .pallet = 0
};

static void AppInit(void);
static void InitBrickSprites(void);
static void GameReset(void);
static void ResetRound(void);
static void FillBricks(void);
static void UpdateGame(void);
static void MovePaddle(uint32_t now);
static void UpdateBall(void);
static void DrawGame(void);
static void DrawBorders(void);
static void DrawBricks(void);
static void DrawStatus(void);
static uint8_t HandleBrickCollision(int16_t x_px, int16_t y_px);
static uint8_t RemainingBricks(void);
static void PlayBounceSound(void);

int main(void)
{
    AppInit();
    GameReset();

    uint8_t action_prev = 0;
    while (1) {
        uint32_t now = TICK_Get();

        BTN_Task();

        if (game.state == GAME_PLAYING) {
            MovePaddle(now);
            UpdateGame();
        } else if (game.state == GAME_ROUND_WAIT) {
            MovePaddle(now);
            game.ball_x_px = game.paddle_x_px + (PADDLE_WIDTH_PX / 2) - (BALL_SIZE_PX / 2);
            if ((now - game.round_start_tick) >= 1000) {
                game.state = GAME_PLAYING;
            }
        } else {
            if (ButtonPressedEdge(BTN_ACTION, &action_prev)) {
                GameReset();
            }
        }

        DrawGame();

        while ((TICK_Get() - now) < FRAME_MS);
    }
}

static void AppInit(void)
{
    GameLib_Init();
    InitBrickSprites();
    tGFX_SetSpritesCount(5 + (BRICK_ROWS * BRICK_COLS));
    LCD_OptimizedFill(0, 0, LCD_WIDTH, LCD_HEIGHT, ST7735_BLACK);
}

static void InitBrickSprites(void)
{
    static const uint8_t brick_palettes[BRICK_ROWS] = {4, 5, 6};
    uint8_t row;
    uint8_t col;

    for (row = 0; row < BRICK_ROWS; ++row) {
        for (col = 0; col < BRICK_COLS; ++col) {
            uint8_t idx = (uint8_t)(row * BRICK_COLS + col);

            brick_sprites[idx].x = FIELD_LEFT_PX + (col * BRICK_SLOT_WIDTH_PX) + ((BRICK_SLOT_WIDTH_PX - BRICK_WIDTH_PX) / 2);
            brick_sprites[idx].y = ((FIELD_TOP_CHAR + 1) * TERM_BLOCK_SIZE) + (row * BRICK_SLOT_HEIGHT_PX) + ((BRICK_SLOT_HEIGHT_PX - BRICK_HEIGHT_PX) / 2);
            brick_sprites[idx].size_x = BRICK_WIDTH_PX;
            brick_sprites[idx].size_y = BRICK_HEIGHT_PX;
            brick_sprites[idx].data = (uint8_t *)brick_sprite_data;
            brick_sprites[idx].pallet = brick_palettes[row];
        }
    }
}

static void GameReset(void)
{
    game.state = GAME_PLAYING;
    game.lives = START_LIVES;
    game.score = 0;
    game.paddle_x_px = FIELD_LEFT_PX + ((FIELD_RIGHT_PX - FIELD_LEFT_PX + 1 - PADDLE_WIDTH_PX) / 2);
    FillBricks();
    ResetRound();
    SND_Unmute();
}

static void ResetRound(void)
{
    game.ball_x_px = game.paddle_x_px + (PADDLE_WIDTH_PX / 2) - (BALL_SIZE_PX / 2);
    game.ball_y_px = PADDLE_Y_PX - BALL_SIZE_PX - 2;
    game.ball_dx_px = BALL_START_DX;
    game.ball_dy_px = BALL_START_DY;
}

static void FillBricks(void)
{
    uint8_t row;
    uint8_t col;

    for (row = 0; row < BRICK_ROWS; ++row) {
        for (col = 0; col < BRICK_COLS; ++col) {
            game.bricks[row][col] = 1;
        }
    }
}

static void UpdateGame(void)
{
    UpdateBall();

    if (game.state == GAME_PLAYING && RemainingBricks() == 0) {
        game.state = GAME_WON;
        SND_PlaySequence((uint16_t[]){SCALE_FREQ(523), SOUND_TIME_SCALE(1), SCALE_FREQ(659), SOUND_TIME_SCALE(1), SCALE_FREQ(784), SOUND_TIME_SCALE(1), SCALE_FREQ(1046), SOUND_TIME_SCALE(2)}, 3);
    }
}

static void MovePaddle(uint32_t now)
{
    static uint32_t last_move = 0;

    if ((now - last_move) < PADDLE_STEP_MS) {
        return;
    }

    if (BTN_IsPressedRAW(BTN_LEFT) && game.paddle_x_px > FIELD_LEFT_PX) {
        game.paddle_x_px -= PADDLE_STEP_PX;
        if (game.paddle_x_px < FIELD_LEFT_PX) {
            game.paddle_x_px = FIELD_LEFT_PX;
        }
        last_move = now;
    }

    if (BTN_IsPressedRAW(BTN_RIGHT) && (game.paddle_x_px + PADDLE_WIDTH_PX - 1) < FIELD_RIGHT_PX) {
        game.paddle_x_px += PADDLE_STEP_PX;
        if ((game.paddle_x_px + PADDLE_WIDTH_PX - 1) > FIELD_RIGHT_PX) {
            game.paddle_x_px = FIELD_RIGHT_PX - PADDLE_WIDTH_PX + 1;
        }
        last_move = now;
    }
}

static void UpdateBall(void)
{
    static uint8_t ball_frame;
    if (++ball_frame & 1) return;

    int16_t next_x = game.ball_x_px + game.ball_dx_px;
    int16_t next_y = game.ball_y_px + game.ball_dy_px;
    int16_t ball_center_x;
    int16_t ball_center_y;

    if (next_x < FIELD_LEFT_PX || (next_x + BALL_SIZE_PX - 1) > FIELD_RIGHT_PX) {
        game.ball_dx_px = (int8_t)-game.ball_dx_px;
        next_x = game.ball_x_px + game.ball_dx_px;
        PlayBounceSound();
    }

    ball_center_x = next_x + (BALL_SIZE_PX / 2);
    ball_center_y = game.ball_y_px + (BALL_SIZE_PX / 2);
    if (HandleBrickCollision(ball_center_x, ball_center_y)) {
        game.ball_dx_px = (int8_t)-game.ball_dx_px;
        next_x = game.ball_x_px + game.ball_dx_px;
    }

    if (next_y < FIELD_TOP_PX) {
        game.ball_dy_px = (int8_t)-game.ball_dy_px;
        next_y = game.ball_y_px + game.ball_dy_px;
        PlayBounceSound();
    }

    ball_center_x = next_x + (BALL_SIZE_PX / 2);
    ball_center_y = next_y + (BALL_SIZE_PX / 2);
    if (HandleBrickCollision(ball_center_x, ball_center_y)) {
        game.ball_dy_px = (int8_t)-game.ball_dy_px;
        next_y = game.ball_y_px + game.ball_dy_px;
    }

    if (game.ball_dy_px > 0 && (next_y + BALL_SIZE_PX) >= PADDLE_Y_PX) {
        int16_t ball_mid_x = next_x + (BALL_SIZE_PX / 2);

        if (ball_mid_x >= game.paddle_x_px && ball_mid_x < (game.paddle_x_px + PADDLE_WIDTH_PX)) {
            int16_t hit = ball_mid_x - game.paddle_x_px;

            game.ball_dy_px = -2;
            if (hit < (PADDLE_WIDTH_PX / 3)) {
                game.ball_dx_px = -3;
            } else if (hit > ((PADDLE_WIDTH_PX * 2) / 3)) {
                game.ball_dx_px = 3;
            } else {
                game.ball_dx_px = 0;
            }

            next_x = game.ball_x_px + game.ball_dx_px;
            next_y = PADDLE_Y_PX - BALL_SIZE_PX;
            PlayBounceSound();
        } else if (next_y > PADDLE_Y_PX) {
            if (game.lives > 0) {
                game.lives--;
            }

            if (game.lives == 0) {
                game.state = GAME_OVER;
                /* Chopin Funeral March: F Bb F(long) Eb D C Bb(low/long) */
                static const uint16_t funeral_march[] = {
                    SCALE_FREQ(175), SOUND_TIME_SCALE(60),   /* F3  300ms */
                    SCALE_FREQ(233), SOUND_TIME_SCALE(60),   /* Bb3 300ms */
                    SCALE_FREQ(175), SOUND_TIME_SCALE(120),  /* F3  600ms */
                    SCALE_FREQ(156), SOUND_TIME_SCALE(40),   /* Eb3 200ms */
                    SCALE_FREQ(147), SOUND_TIME_SCALE(40),   /* D3  200ms */
                    SCALE_FREQ(131), SOUND_TIME_SCALE(40),   /* C3  200ms */
                    SCALE_FREQ(117), SOUND_TIME_SCALE(200),  /* Bb2 1000ms */
                };
                SND_PlaySequence((uint16_t*)funeral_march, 6);
            } else {
                ResetRound();
                game.state = GAME_ROUND_WAIT;
                game.round_start_tick = TICK_Get();
                SND_PlayNow(SCALE_FREQ(220), 1);
            }
            return;
        }
    }

    game.ball_x_px = next_x;
    game.ball_y_px = next_y;
}

static void DrawGame(void)
{
    uint8_t brick_idx;

    paddle_sprite.x = game.paddle_x_px;
    ball_sprite.x = game.ball_x_px;
    ball_sprite.y = game.ball_y_px;

    GameLib_ClearScreen(0);
    DrawBorders();
    DrawBricks();
    DrawStatus();
    tGFX_SetSprite(&paddle_sprite, 0);
    tGFX_SetSprite(&ball_sprite, 1);
    tGFX_SetSprite(&left_border_sprite, 2);
    tGFX_SetSprite(&right_border_sprite, 3);
    tGFX_SetSprite(&top_border_sprite, 4);
    for (brick_idx = 0; brick_idx < (BRICK_ROWS * BRICK_COLS); ++brick_idx) {
        if (game.bricks[brick_idx / BRICK_COLS][brick_idx % BRICK_COLS]) {
            tGFX_SetSprite(&brick_sprites[brick_idx], (uint8_t)(brick_idx + 5));
        } else {
            tGFX_ClearSprite((uint8_t)(brick_idx + 5));
        }
    }

    if (game.state == GAME_WON) {
        GameLib_DrawMessage("YOU WIN!", "ACTION RETRY");
    } else if (game.state == GAME_OVER) {
        GameLib_DrawMessage("GAME OVER", "ACTION RETRY");
    }

    tGFX_Update();
}

static void DrawBorders(void)
{
    /* Border lines are drawn directly to the LCD for crisp 1px edges. */
}

static void DrawBricks(void)
{
    /* Bricks are rendered as sprites to keep a 1px black margin on all sides. */
}

static void DrawStatus(void)
{
    char line[21];

    snprintf(line, sizeof(line), "BK %02u L%u", (unsigned)game.score, (unsigned)game.lives);
    tGFX_SetCursor(0, HUD_ROW);
    tGFX_Print(line, 15, 0);
}

static uint8_t HandleBrickCollision(int16_t x_px, int16_t y_px)
{
    uint8_t row;
    uint8_t col;

    for (row = 0; row < BRICK_ROWS; ++row) {
        for (col = 0; col < BRICK_COLS; ++col) {
            uint8_t idx = (uint8_t)(row * BRICK_COLS + col);
            term_sprite_t *brick = &brick_sprites[idx];

            if (!game.bricks[row][col]) {
                continue;
            }

            if (x_px >= brick->x && x_px < (brick->x + brick->size_x) &&
                y_px >= brick->y && y_px < (brick->y + brick->size_y)) {
                game.bricks[row][col] = 0;
                game.score++;
                SND_PlayNow(SCALE_FREQ(700 + (row * 100)), SOUND_TIME_SCALE(3));
                return 1;
            }
        }
    }
    return 0;
}

static uint8_t RemainingBricks(void)
{
    uint8_t row;
    uint8_t col;

    for (row = 0; row < BRICK_ROWS; ++row) {
        for (col = 0; col < BRICK_COLS; ++col) {
            if (game.bricks[row][col]) {
                return 1;
            }
        }
    }

    return 0;
}

static void PlayBounceSound(void)
{
    SND_PlayNow(SCALE_FREQ(880), SOUND_TIME_SCALE(1));
}
