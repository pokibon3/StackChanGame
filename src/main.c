#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "buttons.h"
#include "gamefunc.h"
#include "sound.h"
#include "termGFX.h"
#include "tick.h"

#define FRAME_MS 20

#define G2_PLAYER_X 12
#define G2_PLAYER_W 20
#define G2_PLAYER_H 20
#define G2_MOVE_SPEED 3.0f

#define G2_MAX_BULLETS 8
#define G2_MAX_ENEMIES 6
#define G2_MAX_BOSS_SHOTS 8

#define G2_BULLET_W 3
#define G2_BULLET_H 3
#define G2_BULLET_VX 3.2f

#define G2_ENEMY_MIN_W 8
#define G2_ENEMY_MAX_W 14
#define G2_ENEMY_MIN_H 6
#define G2_ENEMY_MAX_H 14
#define G2_ENEMY_BASE_SPEED 1.6f
#define G2_ENEMY_SCORE_SPEED_COEF 0.8f

#define G2_SPAWN_BASE_INTERVAL_MS 700
#define G2_SPAWN_DEC_PER_STAGE_MS 50
#define G2_SPAWN_MIN_INTERVAL_MS 200
#define G2_SPEEDMUL_PER_STAGE 0.15f
#define G2_EXTRA_ENEMY_PER_STAGE_PC 15
#define G2_EXTRA_ENEMY_MAX_PC 70

#define G2_BOSS_SPAWN_SCORE 3000
#define G2_BOSS_HP 30
#define G2_BOSS_W 24
#define G2_BOSS_H 24
#define G2_BOSS_VY 0.7f
#define G2_BOSS_ENTRY_OFFSET_X 25
#define G2_BOSS_HOLD_X 90
#define G2_BOSS_MIN_Y 10
#define G2_BOSS_MARGIN_BOTTOM 1

#define G2_BOSSSHOT_W 6
#define G2_BOSSSHOT_H 6
#define G2_BOSSSHOT_VX_BASE 1.8f
#define G2_BOSSSHOT_VX_STAGE_COEF 0.15f
#define G2_BOSSSHOT_BASE_INTERVAL_MS 1200
#define G2_BOSSSHOT_DEC_PER_STAGE_MS 40
#define G2_BOSSSHOT_MIN_INTERVAL_MS 800

#define G2_LIVES_MAX 3

#define G2_SPRITE_COUNT (1 + G2_MAX_BULLETS + G2_MAX_ENEMIES + 1 + G2_MAX_BOSS_SHOTS)
#define G2_BG_COLOR 4
#define G2_TEXT_BG_COLOR 4
#define G2_PLAYER_PALLET 0
#define G2_BULLET_PALLET 8
#define G2_ENEMY_PALLET 5
#define G2_BOSS_PALLET 7
#define G2_BOSSSHOT_PALLET 9

typedef struct
{
    uint16_t freq;
    uint16_t ms;
} beep_t;

typedef struct
{
    float x;
    float y;
    float vx;
    uint8_t alive;
} bullet_t;

typedef struct
{
    float x;
    float y;
    uint8_t w;
    uint8_t h;
    uint8_t alive;
} enemy_t;

typedef struct
{
    float x;
    float y;
    float vy;
    uint8_t hp;
    uint8_t alive;
    uint8_t entering;
} boss_t;

typedef struct
{
    float x;
    float y;
    float vx;
    uint8_t alive;
} boss_shot_t;

static const beep_t snd_start = {1200, 90};
static const beep_t snd_cancel = {400, 120};
static const beep_t snd_gameover = {220, 400};
static const beep_t snd_restart = {1000, 100};
static const beep_t snd_shot = {1300, 40};
static const beep_t snd_hit = {900, 70};
static const beep_t snd_boss_hit = {1000, 40};
static const beep_t snd_boss_die = {1600, 120};

static const uint8_t player_sprite_data[((G2_PLAYER_W + 3) / 4) * G2_PLAYER_H] = {
    0x01, 0x55, 0x55, 0x55, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x40,
    0x11, 0x00, 0x00, 0x00, 0x10,
    0x10, 0x40, 0x00, 0x00, 0x04,
    0x10, 0x05, 0x55, 0x55, 0x50,
    0x10, 0x10, 0x00, 0x00, 0x04,
    0x10, 0x11, 0x55, 0x55, 0x44,
    0x10, 0x11, 0x55, 0x55, 0x44,
    0x10, 0x11, 0x45, 0x51, 0x44,
    0x10, 0x11, 0x55, 0x55, 0x44,
    0x10, 0x11, 0x55, 0x55, 0x44,
    0x10, 0x11, 0x55, 0x55, 0x44,
    0x10, 0x11, 0x50, 0x05, 0x44,
    0x10, 0x11, 0x55, 0x55, 0x44,
    0x04, 0x10, 0x00, 0x00, 0x04,
    0x01, 0x10, 0x51, 0x45, 0x04,
    0x00, 0x50, 0x00, 0x00, 0x04,
    0x00, 0x05, 0x55, 0x55, 0x50,
    0x00, 0x00, 0x54, 0x15, 0x00,
    0x00, 0x01, 0x54, 0x15, 0x40
};
static const uint8_t enemy_sprite_data[((G2_ENEMY_MAX_W + 3) / 4) * G2_ENEMY_MAX_H] = {
    [0 ... ((((G2_ENEMY_MAX_W + 3) / 4) * G2_ENEMY_MAX_H) - 1)] = 0x55
};
static const uint8_t boss_sprite_data[((G2_BOSS_W + 3) / 4) * G2_BOSS_H] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x54, 0x00, 0x00,
    0x00, 0x00, 0x15, 0x55, 0x40, 0x00,
    0x00, 0x01, 0x55, 0x55, 0x54, 0x00,
    0x00, 0x15, 0x00, 0x00, 0x05, 0x40,
    0x00, 0x54, 0x00, 0x00, 0x01, 0x50,
    0x01, 0x50, 0x00, 0x00, 0x00, 0x54,
    0x05, 0x40, 0xa0, 0xa0, 0x00, 0x55,
    0x15, 0x00, 0xa0, 0xa0, 0x00, 0x15,
    0x15, 0x00, 0x00, 0x00, 0x00, 0x15,
    0x55, 0x02, 0x00, 0x08, 0x00, 0x55,
    0x55, 0x00, 0x00, 0x00, 0x00, 0x15,
    0x55, 0x00, 0xaa, 0xaa, 0x00, 0x15,
    0x55, 0x00, 0x00, 0x00, 0x00, 0x15,
    0x55, 0x02, 0x00, 0x02, 0x00, 0x15,
    0x15, 0x00, 0x80, 0x08, 0x00, 0x15,
    0x15, 0x00, 0x2a, 0xa8, 0x00, 0x55,
    0x05, 0x40, 0x00, 0x00, 0x01, 0x50,
    0x01, 0x50, 0x00, 0x00, 0x00, 0x54,
    0x00, 0x54, 0x00, 0x00, 0x01, 0x50,
    0x00, 0x15, 0x55, 0x55, 0x55, 0x40,
    0x00, 0x01, 0x55, 0x55, 0x54, 0x00,
    0x00, 0x00, 0x01, 0x54, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t bullet_sprite_data[((G2_BULLET_W + 3) / 4) * G2_BULLET_H] = {
    [0 ... ((((G2_BULLET_W + 3) / 4) * G2_BULLET_H) - 1)] = 0x55
};
static const uint8_t boss_shot_sprite_data[((G2_BOSSSHOT_W + 3) / 4) * G2_BOSSSHOT_H] = {
    [0 ... ((((G2_BOSSSHOT_W + 3) / 4) * G2_BOSSSHOT_H) - 1)] = 0x55
};

static float g2_py;
static bullet_t g2_bullets[G2_MAX_BULLETS];
static enemy_t g2_enemies[G2_MAX_ENEMIES];
static boss_t g2_boss;
static boss_shot_t g2_boss_shots[G2_MAX_BOSS_SHOTS];
static uint8_t g2_boss_spawned;
static uint8_t g2_boss_defeated;
static uint8_t g2_clear;
static uint32_t g2_last_spawn;
static uint32_t g2_score;
static uint8_t g2_game_over;
static uint8_t g2_game_over_sound_played;
static uint8_t g2_stage;
static uint16_t g2_spawn_interval;
static float g2_speed_mul;
static uint32_t g2_next_boss_spawn_at;
static uint32_t g2_stage_score_base;
static uint32_t g2_boss_last_shot_ms;
static uint16_t g2_boss_shot_interval;
static uint8_t g2_lives;
static uint8_t g2_hard_over;
static uint32_t g2_input_lock_until;
static uint32_t prng_state = 0x13579bdfUL;

static uint8_t btn_left_prev;
static uint8_t btn_action_prev;

static term_sprite_t player_sprite = {
    .x = 0, .y = 0, .size_x = G2_PLAYER_W, .size_y = G2_PLAYER_H,
    .data = (uint8_t *)player_sprite_data, .pallet = G2_PLAYER_PALLET
};
static term_sprite_t bullet_sprites[G2_MAX_BULLETS] = {
    {.x = 0, .y = 0, .size_x = G2_BULLET_W, .size_y = G2_BULLET_H, .data = (uint8_t *)bullet_sprite_data, .pallet = G2_BULLET_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BULLET_W, .size_y = G2_BULLET_H, .data = (uint8_t *)bullet_sprite_data, .pallet = G2_BULLET_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BULLET_W, .size_y = G2_BULLET_H, .data = (uint8_t *)bullet_sprite_data, .pallet = G2_BULLET_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BULLET_W, .size_y = G2_BULLET_H, .data = (uint8_t *)bullet_sprite_data, .pallet = G2_BULLET_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BULLET_W, .size_y = G2_BULLET_H, .data = (uint8_t *)bullet_sprite_data, .pallet = G2_BULLET_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BULLET_W, .size_y = G2_BULLET_H, .data = (uint8_t *)bullet_sprite_data, .pallet = G2_BULLET_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BULLET_W, .size_y = G2_BULLET_H, .data = (uint8_t *)bullet_sprite_data, .pallet = G2_BULLET_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BULLET_W, .size_y = G2_BULLET_H, .data = (uint8_t *)bullet_sprite_data, .pallet = G2_BULLET_PALLET}
};
static term_sprite_t enemy_sprites[G2_MAX_ENEMIES] = {
    {.x = 0, .y = 0, .size_x = G2_ENEMY_MIN_W, .size_y = G2_ENEMY_MIN_H, .data = (uint8_t *)enemy_sprite_data, .pallet = G2_ENEMY_PALLET},
    {.x = 0, .y = 0, .size_x = G2_ENEMY_MIN_W, .size_y = G2_ENEMY_MIN_H, .data = (uint8_t *)enemy_sprite_data, .pallet = G2_ENEMY_PALLET},
    {.x = 0, .y = 0, .size_x = G2_ENEMY_MIN_W, .size_y = G2_ENEMY_MIN_H, .data = (uint8_t *)enemy_sprite_data, .pallet = G2_ENEMY_PALLET},
    {.x = 0, .y = 0, .size_x = G2_ENEMY_MIN_W, .size_y = G2_ENEMY_MIN_H, .data = (uint8_t *)enemy_sprite_data, .pallet = G2_ENEMY_PALLET},
    {.x = 0, .y = 0, .size_x = G2_ENEMY_MIN_W, .size_y = G2_ENEMY_MIN_H, .data = (uint8_t *)enemy_sprite_data, .pallet = G2_ENEMY_PALLET},
    {.x = 0, .y = 0, .size_x = G2_ENEMY_MIN_W, .size_y = G2_ENEMY_MIN_H, .data = (uint8_t *)enemy_sprite_data, .pallet = G2_ENEMY_PALLET}
};
static term_sprite_t boss_sprite = {
    .x = 0, .y = 0, .size_x = G2_BOSS_W, .size_y = G2_BOSS_H,
    .data = (uint8_t *)boss_sprite_data, .pallet = G2_BOSS_PALLET
};
static term_sprite_t boss_shot_sprites[G2_MAX_BOSS_SHOTS] = {
    {.x = 0, .y = 0, .size_x = G2_BOSSSHOT_W, .size_y = G2_BOSSSHOT_H, .data = (uint8_t *)boss_shot_sprite_data, .pallet = G2_BOSSSHOT_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BOSSSHOT_W, .size_y = G2_BOSSSHOT_H, .data = (uint8_t *)boss_shot_sprite_data, .pallet = G2_BOSSSHOT_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BOSSSHOT_W, .size_y = G2_BOSSSHOT_H, .data = (uint8_t *)boss_shot_sprite_data, .pallet = G2_BOSSSHOT_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BOSSSHOT_W, .size_y = G2_BOSSSHOT_H, .data = (uint8_t *)boss_shot_sprite_data, .pallet = G2_BOSSSHOT_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BOSSSHOT_W, .size_y = G2_BOSSSHOT_H, .data = (uint8_t *)boss_shot_sprite_data, .pallet = G2_BOSSSHOT_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BOSSSHOT_W, .size_y = G2_BOSSSHOT_H, .data = (uint8_t *)boss_shot_sprite_data, .pallet = G2_BOSSSHOT_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BOSSSHOT_W, .size_y = G2_BOSSSHOT_H, .data = (uint8_t *)boss_shot_sprite_data, .pallet = G2_BOSSSHOT_PALLET},
    {.x = 0, .y = 0, .size_x = G2_BOSSSHOT_W, .size_y = G2_BOSSSHOT_H, .data = (uint8_t *)boss_shot_sprite_data, .pallet = G2_BOSSSHOT_PALLET}
};

static void AppInit(void);
static void PlayBeep(const beep_t *snd);
static float AbsFloat(float value);
static uint32_t NextRandom(void);
static int RandomRange(int min_value, int max_value);
static void PrintLabelValue(int x, int y, uint8_t color, const char *label, uint32_t value);
static void G2_ApplyStageParams(void);
static void G2_Reset(void);
static void G2_NextStage(void);
static void G2_RestartStage(void);
static uint8_t G2_CountAliveEnemies(void);
static void G2_CommitDeath(void);
static void G2_SpawnEnemy(void);
static void G2_Fire(void);
static void G2_BossFire(void);
static void G2_Update(void);
static void G2_Draw(void);

int main(void)
{
    AppInit();
    G2_Reset();
    PlayBeep(&snd_start);

    while (1) {
        uint32_t frame_start = TICK_Get();

        BTN_Task();
        G2_Update();
        G2_Draw();
        if (!g2_game_over && !g2_clear) {
            g2_score += 1U;
        }
        tGFX_Update();

        while ((TICK_Get() - frame_start) < FRAME_MS) {
        }
    }
}

static void AppInit(void)
{
    GameLib_Init();
    tGFX_SetSpritesCount(0);
    GameLib_ClearScreen(G2_BG_COLOR);
    tGFX_Update();
}

static void PlayBeep(const beep_t *snd)
{
    uint8_t ticks_10ms;

    if ((snd == NULL) || (snd->freq == 0U) || (snd->ms == 0U)) {
        return;
    }

    ticks_10ms = (uint8_t)((snd->ms + 9U) / 10U);
    if (ticks_10ms == 0U) {
        ticks_10ms = 1U;
    }
    SND_PlayNow(snd->freq, ticks_10ms);
}

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static uint32_t NextRandom(void)
{
    prng_state = (prng_state * 1664525UL) + 1013904223UL;
    return prng_state;
}

static int RandomRange(int min_value, int max_value)
{
    if (max_value <= min_value) {
        return min_value;
    }
    return min_value + (int)(NextRandom() % (uint32_t)(max_value - min_value));
}

static void PrintLabelValue(int x, int y, uint8_t color, const char *label, uint32_t value)
{
    char buf[11];
    tGFX_SetCursor((uint8_t)x, (uint8_t)y);
    tGFX_Print((char *)label, color, G2_TEXT_BG_COLOR);
    sprintf(buf, "%lu", (unsigned long)value);
    tGFX_SetCursor((uint8_t)(x + (int)strlen(label)), (uint8_t)y);
    tGFX_Print(buf, color, G2_TEXT_BG_COLOR);
}

static void G2_ApplyStageParams(void)
{
    int16_t spawn_candidate;
    int16_t shot_candidate;

    g2_speed_mul = 1.0f + (G2_SPEEDMUL_PER_STAGE * g2_stage);

    spawn_candidate = (int16_t)G2_SPAWN_BASE_INTERVAL_MS - (int16_t)(G2_SPAWN_DEC_PER_STAGE_MS * g2_stage);
    g2_spawn_interval = (spawn_candidate < (int16_t)G2_SPAWN_MIN_INTERVAL_MS) ? G2_SPAWN_MIN_INTERVAL_MS : (uint16_t)spawn_candidate;

    shot_candidate = (int16_t)G2_BOSSSHOT_BASE_INTERVAL_MS - (int16_t)(G2_BOSSSHOT_DEC_PER_STAGE_MS * g2_stage);
    g2_boss_shot_interval = (shot_candidate < (int16_t)G2_BOSSSHOT_MIN_INTERVAL_MS) ? G2_BOSSSHOT_MIN_INTERVAL_MS : (uint16_t)shot_candidate;
}

static void G2_Reset(void)
{
    memset(g2_bullets, 0, sizeof(g2_bullets));
    memset(g2_enemies, 0, sizeof(g2_enemies));
    memset(g2_boss_shots, 0, sizeof(g2_boss_shots));

    g2_py = 30.0f;
    g2_score = 0U;
    g2_game_over = 0U;
    g2_game_over_sound_played = 0U;
    g2_clear = 0U;
    g2_hard_over = 0U;
    g2_stage = 1U;
    g2_lives = G2_LIVES_MAX;
    g2_boss_spawned = 0U;
    g2_boss_defeated = 0U;
    g2_input_lock_until = 0U;

    G2_ApplyStageParams();

    g2_next_boss_spawn_at = G2_BOSS_SPAWN_SCORE;
    g2_stage_score_base = 0U;
    g2_last_spawn = TICK_Get();
    g2_boss_last_shot_ms = TICK_Get();

    g2_boss.x = 128.0f + G2_BOSS_ENTRY_OFFSET_X;
    g2_boss.y = 12.0f;
    g2_boss.vy = G2_BOSS_VY;
    g2_boss.hp = G2_BOSS_HP;
    g2_boss.alive = 0U;
    g2_boss.entering = 0U;
}

static void G2_NextStage(void)
{
    memset(g2_bullets, 0, sizeof(g2_bullets));
    memset(g2_enemies, 0, sizeof(g2_enemies));
    memset(g2_boss_shots, 0, sizeof(g2_boss_shots));

    g2_stage++;
    g2_clear = 0U;
    g2_game_over = 0U;
    g2_game_over_sound_played = 0U;
    g2_hard_over = 0U;
    G2_ApplyStageParams();

    g2_next_boss_spawn_at = g2_score + G2_BOSS_SPAWN_SCORE;
    g2_stage_score_base = g2_score;
    g2_last_spawn = TICK_Get();
    g2_boss_last_shot_ms = TICK_Get();
    g2_input_lock_until = 0U;

    g2_boss.x = 128.0f + G2_BOSS_ENTRY_OFFSET_X;
    g2_boss.y = 12.0f;
    g2_boss.vy = G2_BOSS_VY;
    g2_boss.hp = G2_BOSS_HP;
    g2_boss.alive = 0U;
    g2_boss.entering = 0U;

    g2_boss_spawned = 0U;
    g2_boss_defeated = 0U;
    PlayBeep(&snd_start);
}

static void G2_RestartStage(void)
{
    uint8_t keep_boss_fight = (g2_boss_spawned && g2_boss.alive) ? 1U : 0U;

    memset(g2_bullets, 0, sizeof(g2_bullets));
    memset(g2_enemies, 0, sizeof(g2_enemies));
    memset(g2_boss_shots, 0, sizeof(g2_boss_shots));

    g2_py = 30.0f;
    g2_game_over = 0U;
    g2_game_over_sound_played = 0U;
    g2_clear = 0U;
    g2_hard_over = 0U;
    g2_input_lock_until = 0U;
    g2_last_spawn = TICK_Get();
    g2_boss_last_shot_ms = TICK_Get();

    if (keep_boss_fight) {
        g2_boss_spawned = 1U;
        g2_boss_defeated = 0U;
    } else {
        g2_boss_spawned = 0U;
        g2_boss_defeated = 0U;
        g2_boss.x = 128.0f + G2_BOSS_ENTRY_OFFSET_X;
        g2_boss.y = 12.0f;
        g2_boss.vy = G2_BOSS_VY;
        g2_boss.hp = G2_BOSS_HP;
        g2_boss.alive = 0U;
        g2_boss.entering = 0U;
    }
}

static uint8_t G2_CountAliveEnemies(void)
{
    uint8_t i;
    uint8_t count = 0U;
    for (i = 0; i < G2_MAX_ENEMIES; ++i) {
        if (g2_enemies[i].alive) {
            count++;
        }
    }
    return count;
}

static void G2_CommitDeath(void)
{
    if (g2_game_over) {
        return;
    }

    g2_game_over = 1U;
    if (!g2_game_over_sound_played) {
        PlayBeep(&snd_gameover);
        g2_game_over_sound_played = 1U;
    }
    if (g2_lives > 0U) {
        g2_lives--;
    }
    g2_hard_over = (g2_lives == 0U) ? 1U : 0U;
    g2_input_lock_until = TICK_Get() + 500U;
    btn_left_prev = 0U;
    btn_action_prev = 0U;
}

static void G2_SpawnEnemy(void)
{
    uint8_t i;
    for (i = 0; i < G2_MAX_ENEMIES; ++i) {
        if (!g2_enemies[i].alive) {
            g2_enemies[i].alive = 1U;
            g2_enemies[i].x = 128.0f + RandomRange(0, 25);
            g2_enemies[i].w = (uint8_t)RandomRange(G2_ENEMY_MIN_W, G2_ENEMY_MAX_W + 1);
            g2_enemies[i].h = (uint8_t)RandomRange(G2_ENEMY_MIN_H, G2_ENEMY_MAX_H + 1);
            g2_enemies[i].y = (float)RandomRange(10, 80 - (int)g2_enemies[i].h);
            return;
        }
    }
}

static void G2_Fire(void)
{
    uint8_t i;
    for (i = 0; i < G2_MAX_BULLETS; ++i) {
        if (!g2_bullets[i].alive) {
            g2_bullets[i].alive = 1U;
            g2_bullets[i].x = (float)(G2_PLAYER_X + 18);
            g2_bullets[i].y = g2_py + 4.0f;
            g2_bullets[i].vx = G2_BULLET_VX;
            PlayBeep(&snd_shot);
            return;
        }
    }
}

static void G2_BossFire(void)
{
    uint8_t i;
    for (i = 0; i < G2_MAX_BOSS_SHOTS; ++i) {
        if (!g2_boss_shots[i].alive) {
            int dy = RandomRange(-10, 11);
            g2_boss_shots[i].alive = 1U;
            g2_boss_shots[i].x = g2_boss.x - 2.0f;
            g2_boss_shots[i].y = g2_boss.y + 9.0f + dy;
            if (g2_boss_shots[i].y < 0.0f) {
                g2_boss_shots[i].y = 0.0f;
            }
            if (g2_boss_shots[i].y > 58.0f) {
                g2_boss_shots[i].y = 58.0f;
            }
            g2_boss_shots[i].vx = -(G2_BOSSSHOT_VX_BASE + (G2_BOSSSHOT_VX_STAGE_COEF * g2_stage));
            return;
        }
    }
}

static void G2_Update(void)
{
    uint8_t move_up = BTN_IsPressed(BTN_UP);
    uint8_t move_down = BTN_IsPressed(BTN_DOWN);
    uint8_t fire_edge = ButtonPressedEdge(BTN_ACTION, &btn_action_prev);
    uint32_t now = TICK_Get();
    uint8_t i;

    if (g2_clear || g2_game_over) {
        if (now < g2_input_lock_until) {
            return;
        }
        if (fire_edge) {
            PlayBeep(g2_clear ? &snd_start : &snd_restart);
            if (g2_clear) {
                G2_NextStage();
            } else {
                if (g2_hard_over) {
                    G2_Reset();
                } else {
                    G2_RestartStage();
                }
            }
        }
        if (ButtonPressedEdge(BTN_LEFT, &btn_left_prev)) {
            PlayBeep(&snd_cancel);
            G2_Reset();
        }
        return;
    }

    if (move_up && !move_down) {
        g2_py -= G2_MOVE_SPEED;
    } else if (move_down && !move_up) {
        g2_py += G2_MOVE_SPEED;
    }

    if (g2_py < 0.0f) {
        g2_py = 0.0f;
    }
    if (g2_py > 60.0f) {
        g2_py = 60.0f;
    }

    if (fire_edge) {
        G2_Fire();
    }

    for (i = 0; i < G2_MAX_BULLETS; ++i) {
        if (g2_bullets[i].alive) {
            g2_bullets[i].x += g2_bullets[i].vx;
            if (g2_bullets[i].x > 160.0f) {
                g2_bullets[i].alive = 0U;
            }
        }
    }

    if (!g2_boss_spawned && !g2_boss_defeated && (g2_score >= g2_next_boss_spawn_at)) {
        g2_boss_spawned = 1U;
        g2_boss.alive = 1U;
        g2_boss.entering = 1U;
        g2_boss.x = 128.0f + G2_BOSS_ENTRY_OFFSET_X;
        g2_boss.y = 12.0f;
        g2_boss.vy = G2_BOSS_VY;
        g2_boss.hp = G2_BOSS_HP;
    }

    if ((!g2_boss_spawned || !g2_boss.alive) && ((now - g2_last_spawn) >= g2_spawn_interval)) {
        int extra_chance;
        g2_last_spawn = now;
        if (G2_CountAliveEnemies() < G2_MAX_ENEMIES) {
            G2_SpawnEnemy();
            extra_chance = G2_EXTRA_ENEMY_PER_STAGE_PC * g2_stage;
            if (extra_chance > G2_EXTRA_ENEMY_MAX_PC) {
                extra_chance = G2_EXTRA_ENEMY_MAX_PC;
            }
            if ((RandomRange(0, 100) < extra_chance) && (G2_CountAliveEnemies() < G2_MAX_ENEMIES)) {
                G2_SpawnEnemy();
            }
        }
    }

    for (i = 0; i < G2_MAX_ENEMIES; ++i) {
        if (g2_enemies[i].alive) {
            float score_phase = 0.0f;
            uint8_t b;
            int px1, py1, px2, py2;
            int ex1, ey1, ex2, ey2;

            if (g2_score > g2_stage_score_base) {
                score_phase = (float)(g2_score - g2_stage_score_base) / (float)G2_BOSS_SPAWN_SCORE;
                if (score_phase > 1.0f) {
                    score_phase = 1.0f;
                }
            }

            g2_enemies[i].x -= (G2_ENEMY_BASE_SPEED * g2_speed_mul) + (G2_ENEMY_SCORE_SPEED_COEF * score_phase);
            if ((g2_enemies[i].x + g2_enemies[i].w) < 0.0f) {
                g2_enemies[i].alive = 0U;
                continue;
            }

            px1 = G2_PLAYER_X;
            py1 = (int)g2_py;
            px2 = px1 + G2_PLAYER_W - 1;
            py2 = py1 + G2_PLAYER_H - 1;
            ex1 = (int)g2_enemies[i].x;
            ey1 = (int)g2_enemies[i].y;
            ex2 = ex1 + g2_enemies[i].w - 1;
            ey2 = ey1 + g2_enemies[i].h - 1;

            if (!((px2 < ex1) || (ex2 < px1) || (py2 < ey1) || (ey2 < py1))) {
                G2_CommitDeath();
                return;
            }

            for (b = 0; b < G2_MAX_BULLETS; ++b) {
                if (g2_bullets[b].alive) {
                    int bx1 = (int)g2_bullets[b].x;
                    int by1 = (int)g2_bullets[b].y;
                    int bx2 = bx1 + G2_BULLET_W - 1;
                    int by2 = by1 + G2_BULLET_H - 1;

                    if (!((bx2 < ex1) || (ex2 < bx1) || (by2 < ey1) || (ey2 < by1))) {
                        g2_bullets[b].alive = 0U;
                        g2_enemies[i].alive = 0U;
                        g2_score += 100U;
                        PlayBeep(&snd_hit);
                        break;
                    }
                }
            }
        }
    }

    if (g2_boss_spawned && g2_boss.alive) {
        uint8_t b;
        int px1 = G2_PLAYER_X;
        int py1 = (int)g2_py;
        int px2 = px1 + G2_PLAYER_W - 1;
        int py2 = py1 + G2_PLAYER_H - 1;
        int bx1;
        int by1;
        int bx2;
        int by2;

        if (g2_boss.entering) {
            g2_boss.x -= 1.2f;
            if (g2_boss.x <= G2_BOSS_HOLD_X) {
                g2_boss.x = (float)G2_BOSS_HOLD_X;
                g2_boss.entering = 0U;
            }
        } else {
            g2_boss.y += g2_boss.vy;
            if (g2_boss.y < G2_BOSS_MIN_Y) {
                g2_boss.y = (float)G2_BOSS_MIN_Y;
                g2_boss.vy = AbsFloat(g2_boss.vy);
            }
            if (g2_boss.y > (79 - G2_BOSS_H - G2_BOSS_MARGIN_BOTTOM)) {
                g2_boss.y = (float)(79 - G2_BOSS_H - G2_BOSS_MARGIN_BOTTOM);
                g2_boss.vy = -AbsFloat(g2_boss.vy);
            }
            if ((now - g2_boss_last_shot_ms) >= g2_boss_shot_interval) {
                g2_boss_last_shot_ms = now;
                G2_BossFire();
                if (g2_stage >= 3U) {
                    G2_BossFire();
                }
            }
        }

        bx1 = (int)g2_boss.x;
        by1 = (int)g2_boss.y;
        bx2 = bx1 + G2_BOSS_W - 1;
        by2 = by1 + G2_BOSS_H - 1;

        if (!((px2 < bx1) || (bx2 < px1) || (py2 < by1) || (by2 < py1))) {
            G2_CommitDeath();
            return;
        }

        for (b = 0; b < G2_MAX_BULLETS; ++b) {
            if (g2_bullets[b].alive) {
                int ex1 = (int)g2_bullets[b].x;
                int ey1 = (int)g2_bullets[b].y;
                int ex2 = ex1 + G2_BULLET_W - 1;
                int ey2 = ey1 + G2_BULLET_H - 1;

                if (!((ex2 < bx1) || (bx2 < ex1) || (ey2 < by1) || (by2 < ey1))) {
                    g2_bullets[b].alive = 0U;
                    if (g2_boss.hp > 0U) {
                        g2_boss.hp--;
                    }
                    g2_score += 120U;
                    PlayBeep(&snd_boss_hit);
                    if (g2_boss.hp == 0U) {
                        g2_boss.alive = 0U;
                        g2_boss_defeated = 1U;
                        g2_clear = 1U;
                        g2_score += 2000U;
                        g2_input_lock_until = TICK_Get() + 500U;
                        PlayBeep(&snd_boss_die);
                    }
                    break;
                }
            }
        }
    }

    for (i = 0; i < G2_MAX_BOSS_SHOTS; ++i) {
        if (g2_boss_shots[i].alive) {
            int px1 = G2_PLAYER_X;
            int py1 = (int)g2_py;
            int px2 = px1 + G2_PLAYER_W - 1;
            int py2 = py1 + G2_PLAYER_H - 1;
            int sx1;
            int sy1;
            int sx2;
            int sy2;

            g2_boss_shots[i].x += g2_boss_shots[i].vx;
            if ((g2_boss_shots[i].x + G2_BOSSSHOT_W) < 0.0f) {
                g2_boss_shots[i].alive = 0U;
                continue;
            }

            sx1 = (int)g2_boss_shots[i].x;
            sy1 = (int)g2_boss_shots[i].y;
            sx2 = sx1 + G2_BOSSSHOT_W - 1;
            sy2 = sy1 + G2_BOSSSHOT_H - 1;

            if (!((px2 < sx1) || (sx2 < px1) || (py2 < sy1) || (sy2 < py1))) {
                G2_CommitDeath();
                return;
            }
        }
    }
}

static void G2_Draw(void)
{
    uint8_t i;
    uint8_t sprite_idx;

    tGFX_SetSpritesCount(G2_SPRITE_COUNT);
    GameLib_ClearScreen(G2_BG_COLOR);
    for (sprite_idx = 0; sprite_idx < G2_SPRITE_COUNT; ++sprite_idx) {
        tGFX_ClearSprite(sprite_idx);
    }

    player_sprite.x = G2_PLAYER_X;
    player_sprite.y = (int16_t)g2_py;
    tGFX_SetSprite(&player_sprite, 0);

    sprite_idx = 1;
    for (i = 0; i < G2_MAX_BULLETS; ++i, ++sprite_idx) {
        if (g2_bullets[i].alive) {
            bullet_sprites[i].x = (int16_t)g2_bullets[i].x;
            bullet_sprites[i].y = (int16_t)g2_bullets[i].y;
            tGFX_SetSprite(&bullet_sprites[i], sprite_idx);
        }
    }

    for (i = 0; i < G2_MAX_ENEMIES; ++i, ++sprite_idx) {
        if (g2_enemies[i].alive) {
            enemy_sprites[i].x = (int16_t)g2_enemies[i].x;
            enemy_sprites[i].y = (int16_t)g2_enemies[i].y;
            enemy_sprites[i].size_x = g2_enemies[i].w;
            enemy_sprites[i].size_y = g2_enemies[i].h;
            tGFX_SetSprite(&enemy_sprites[i], sprite_idx);
        }
    }

    if (g2_boss_spawned && g2_boss.alive) {
        boss_sprite.x = (int16_t)g2_boss.x;
        boss_sprite.y = (int16_t)g2_boss.y;
        tGFX_SetSprite(&boss_sprite, sprite_idx);
    }
    sprite_idx++;

    for (i = 0; i < G2_MAX_BOSS_SHOTS; ++i, ++sprite_idx) {
        if (g2_boss_shots[i].alive) {
            boss_shot_sprites[i].x = (int16_t)g2_boss_shots[i].x;
            boss_shot_sprites[i].y = (int16_t)g2_boss_shots[i].y;
            tGFX_SetSprite(&boss_shot_sprites[i], sprite_idx);
        }
    }

    PrintLabelValue(1, 0, 14, "ST:", g2_stage);
    PrintLabelValue(6, 0, 11, "SC:", g2_score);
    PrintLabelValue(15, 0, 10, "L:", g2_lives);
    if (g2_boss_spawned && g2_boss.alive) {
        PrintLabelValue(1, 1, 9, "HP:", g2_boss.hp);
    }
    if (g2_clear) {
        tGFX_SetCursor(5, 7);
        tGFX_Print("GAME CLEAR", 10, G2_TEXT_BG_COLOR);
    } else if (g2_game_over) {
        if (g2_hard_over) {
            tGFX_SetCursor(5, 7);
            tGFX_Print("GAME OVER", 9, G2_TEXT_BG_COLOR);
        } else {
            tGFX_SetCursor(7, 7);
            tGFX_Print("Miss!", 9, G2_TEXT_BG_COLOR);
        }
    }
}
