/*
 * DRAGON FURY — Nuke Eater
 * C/SDL2 port of the HTML5 canvas game
 *
 * Dependencies: SDL2, SDL2_ttf, SDL2_gfx (optional), math
 *
 * Build:
 *   gcc dragon_nuke_snake.c -o dragon_nuke_snake \
 *       $(sdl2-config --cflags --libs) -lSDL2_ttf -lm
 *
 * If SDL2_ttf is unavailable, text rendering will be skipped gracefully.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── CONFIG ────────────────────────────────────────────────────────────────── */
#define COLS     30
#define ROWS     24
#define CELL     20
#define W        (COLS * CELL)
#define H        (ROWS * CELL)
#define HUD_H    60   /* pixels above the canvas for score/level */
#define WIN_W    W
#define WIN_H    (H + HUD_H + 40)

/* ── COLORS ─────────────────────────────────────────────────────────────────── */
#define COL(r,g,b,a) ((SDL_Color){(r),(g),(b),(a)})
static const SDL_Color C_BG        = COL(7,  9,  15, 255);
static const SDL_Color C_GRID      = COL(20, 35, 60, 80);
static const SDL_Color C_FIRE1     = COL(255,69, 0,  255);
static const SDL_Color C_FIRE2     = COL(255,140,0,  255);
static const SDL_Color C_FIRE3     = COL(255,215,0,  255);
static const SDL_Color C_GREEN     = COL(0,  255,65, 255);
static const SDL_Color C_RED       = COL(255,0,  51, 255);
static const SDL_Color C_CYAN      = COL(0,  229,255,255);
static const SDL_Color C_NUKE_BODY = COL(26, 42, 26, 255);
static const SDL_Color C_NUKE_OUT  = COL(0,  255,68, 255);
static const SDL_Color C_WHITE     = COL(255,255,255,255);

/* ── LIMITS ─────────────────────────────────────────────────────────────────── */
#define MAX_SNAKE     (COLS * ROWS)
#define MAX_NUKES     3
#define MAX_PARTICLES 512
#define MAX_BULLETS   64
#define MAX_EXPLOSIONS 32

/* ── TYPES ──────────────────────────────────────────────────────────────────── */
typedef struct { int x, y; } Vec2i;
typedef struct { float x, y; } Vec2f;

typedef struct {
    float x, y, vx, vy;
    float life, decay, size;
    SDL_Color color;
} Particle;

typedef struct {
    float x, y, vx, vy;
} Bullet;

typedef struct {
    float x, y, r, maxR, life;
} Explosion;

typedef struct {
    int   x, y;          /* grid position */
    int   hp, maxHp;
    float vx, vy;
    int   moveTimer, moveInterval;
    int   shootTimer, shootInterval;
    Bullet bullets[MAX_BULLETS];
    int   bulletCount;
    int   phase;
    int   flashTimer;
    int   size;
} Boss;

typedef struct {
    int  x, y;
    float pulse;
} Nuke;

/* ── STATE ──────────────────────────────────────────────────────────────────── */
static Vec2i     snake[MAX_SNAKE];
static int       snakeLen;
static Vec2i     dir, nextDir;
static Nuke      nukes[MAX_NUKES];
static int       nukeCount;
static int       score, nukesEaten, level;
static int       speed;        /* ms per snake step */
static int       running, paused;
static Particle  particles[MAX_PARTICLES];
static int       particleCount;
static Explosion explosions[MAX_EXPLOSIONS];
static int       explosionCount;
static Boss      boss;
static int       bossActive, bossPhase;
static int       hiScore;
static Uint32    lastMoveTime;
static int       tick;

/* Game phase: 0=title, 1=playing, 2=dead, 3=win */
static int       gamePhase;

static SDL_Window   *window;
static SDL_Renderer *renderer;
static TTF_Font     *font;
static TTF_Font     *fontSmall;

/* ── HELPERS ────────────────────────────────────────────────────────────────── */
static float fclamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void setColor(SDL_Color c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

static void setColorA(SDL_Color c, Uint8 a) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, a);
}

/* Draw filled circle via scanlines */
static void fillCircle(int cx, int cy, int r, SDL_Color col) {
    setColor(col);
    for (int dy = -r; dy <= r; dy++) {
        int dx = (int)sqrtf((float)(r*r - dy*dy));
        SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void strokeCircle(int cx, int cy, int r, SDL_Color col) {
    setColor(col);
    int x = r, y = 0, err = 0;
    while (x >= y) {
        SDL_RenderDrawPoint(renderer, cx+x, cy+y);
        SDL_RenderDrawPoint(renderer, cx+y, cy+x);
        SDL_RenderDrawPoint(renderer, cx-y, cy+x);
        SDL_RenderDrawPoint(renderer, cx-x, cy+y);
        SDL_RenderDrawPoint(renderer, cx-x, cy-y);
        SDL_RenderDrawPoint(renderer, cx-y, cy-x);
        SDL_RenderDrawPoint(renderer, cx+y, cy-x);
        SDL_RenderDrawPoint(renderer, cx+x, cy-y);
        y++;
        err += 2*y+1;
        if (err > 2*x) { err -= 2*x-1; x--; }
    }
}

static void fillHexagon(int cx, int cy, int r, SDL_Color col) {
    /* render as a filled polygon via horizontal scanlines */
    float pts[6][2];
    for (int i = 0; i < 6; i++) {
        float angle = (float)i / 6.0f * (float)M_PI * 2.0f - (float)M_PI / 6.0f;
        pts[i][0] = cx + r * cosf(angle);
        pts[i][1] = cy + r * sinf(angle);
    }
    int minY = cy - r - 1, maxY = cy + r + 1;
    setColor(col);
    for (int y = minY; y <= maxY; y++) {
        float xs[2]; int nc = 0;
        for (int i = 0; i < 6 && nc < 2; i++) {
            int j = (i+1) % 6;
            float y0 = pts[i][1], y1 = pts[j][1];
            if ((y0 <= y && y < y1) || (y1 <= y && y < y0)) {
                float t = ((float)y - y0) / (y1 - y0);
                xs[nc++] = pts[i][0] + t * (pts[j][0] - pts[i][0]);
            }
        }
        if (nc == 2) {
            int x0 = (int)fminf(xs[0], xs[1]);
            int x1 = (int)fmaxf(xs[0], xs[1]);
            SDL_RenderDrawLine(renderer, x0, y, x1, y);
        }
    }
}

static void strokeHexagon(int cx, int cy, int r, SDL_Color col) {
    setColor(col);
    float prev_x = cx + r * cosf(-(float)M_PI / 6.0f);
    float prev_y = cy + r * sinf(-(float)M_PI / 6.0f);
    for (int i = 1; i <= 6; i++) {
        float angle = (float)i / 6.0f * (float)M_PI * 2.0f - (float)M_PI / 6.0f;
        float nx = cx + r * cosf(angle), ny = cy + r * sinf(angle);
        SDL_RenderDrawLine(renderer, (int)prev_x, (int)prev_y, (int)nx, (int)ny);
        prev_x = nx; prev_y = ny;
    }
}

/* Draw a filled rounded rect */
static void fillRoundRect(int x, int y, int w, int h, int r, SDL_Color col) {
    setColor(col);
    SDL_Rect inner = { x+r, y, w-2*r, h };
    SDL_RenderFillRect(renderer, &inner);
    SDL_Rect left  = { x, y+r, r, h-2*r };
    SDL_Rect right = { x+w-r, y+r, r, h-2*r };
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
    fillCircle(x+r,   y+r,   r, col);
    fillCircle(x+w-r, y+r,   r, col);
    fillCircle(x+r,   y+h-r, r, col);
    fillCircle(x+w-r, y+h-r, r, col);
}

/* Lerp a single channel */
static Uint8 lerpU8(Uint8 a, Uint8 b, float t) {
    return (Uint8)(a + (b - a) * t);
}

static SDL_Color lerpColor(SDL_Color a, SDL_Color b, float t) {
    t = fclamp(t, 0.f, 1.f);
    return COL(lerpU8(a.r,b.r,t), lerpU8(a.g,b.g,t), lerpU8(a.b,b.b,t), 255);
}

static void renderText(const char *text, int x, int y, SDL_Color col, TTF_Font *f) {
    if (!f) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, text, col);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_Rect dst = { x, y, surf->w, surf->h };
        SDL_RenderCopy(renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

static void renderTextCentered(const char *text, int cx, int y, SDL_Color col, TTF_Font *f) {
    if (!f) return;
    int w, h;
    TTF_SizeUTF8(f, text, &w, &h);
    renderText(text, cx - w/2, y, col, f);
}

/* ── PARTICLE SYSTEM ─────────────────────────────────────────────────────────── */
static void spawnParticles(int gx, int gy, SDL_Color col, int count, float spread) {
    for (int i = 0; i < count && particleCount < MAX_PARTICLES; i++) {
        float angle = ((float)rand() / RAND_MAX) * (float)M_PI * 2.0f;
        float spd   = 1.0f + ((float)rand() / RAND_MAX) * spread / 20.0f;
        Particle *p  = &particles[particleCount++];
        p->x     = gx * CELL + CELL/2.0f;
        p->y     = gy * CELL + CELL/2.0f + HUD_H;
        p->vx    = cosf(angle) * spd;
        p->vy    = sinf(angle) * spd;
        p->life  = 1.0f;
        p->decay = 0.03f + ((float)rand() / RAND_MAX) * 0.04f;
        p->color = col;
        p->size  = 2.0f + ((float)rand() / RAND_MAX) * 3.0f;
    }
}

static void spawnExplosion(int gx, int gy) {
    if (explosionCount >= MAX_EXPLOSIONS) return;
    Explosion *e = &explosions[explosionCount++];
    e->x    = gx * CELL + CELL/2.0f;
    e->y    = gy * CELL + CELL/2.0f + HUD_H;
    e->r    = 0;
    e->maxR = CELL * 2.5f;
    e->life = 1.0f;
}

/* ── BOSS ───────────────────────────────────────────────────────────────────── */
static void initBoss(void) {
    memset(&boss, 0, sizeof(boss));
    boss.x            = COLS / 2;
    boss.y            = ROWS / 2;
    boss.hp = boss.maxHp = 20;
    boss.vx           = 1; boss.vy = 0;
    boss.moveInterval = 18;
    boss.shootInterval= 40;
    boss.phase        = 1;
    boss.size         = 2;
}

static void triggerDeath(void);

static void updateBoss(void) {
    if (!bossActive) return;
    if (boss.flashTimer > 0) boss.flashTimer--;

    /* Movement: chase snake head every moveInterval ticks */
    boss.moveTimer++;
    if (boss.moveTimer >= boss.moveInterval) {
        boss.moveTimer = 0;
        int dx = snake[0].x - boss.x, dy = snake[0].y - boss.y;
        if (abs(dx) > abs(dy)) { boss.vx = (dx > 0) ? 1 : -1; boss.vy = 0; }
        else                   { boss.vx = 0; boss.vy = (dy > 0) ? 1 : -1; }
        boss.x = SDL_clamp(boss.x + (int)boss.vx, 1, COLS-2);
        boss.y = SDL_clamp(boss.y + (int)boss.vy, 1, ROWS-2);
    }

    /* Shooting */
    boss.shootTimer++;
    if (boss.shootTimer >= boss.shootInterval) {
        boss.shootTimer = 0;
        float dx = snake[0].x - boss.x, dy = snake[0].y - boss.y;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist < 0.001f) dist = 1.0f;
        float spd = (boss.phase >= 2) ? 2.5f : 1.8f;
        if (boss.bulletCount < MAX_BULLETS) {
            Bullet *b = &boss.bullets[boss.bulletCount++];
            b->x = boss.x * CELL + CELL/2.0f;
            b->y = boss.y * CELL + CELL/2.0f;
            b->vx = (dx/dist) * spd;
            b->vy = (dy/dist) * spd;
        }
        if (boss.phase >= 2 && boss.bulletCount < MAX_BULLETS) {
            Bullet *b2 = &boss.bullets[boss.bulletCount++];
            b2->x  = boss.x * CELL + CELL/2.0f;
            b2->y  = boss.y * CELL + CELL/2.0f;
            b2->vx = (-dy/dist) * spd * 0.7f;
            b2->vy = ( dx/dist) * spd * 0.7f;
        }
    }

    /* Update bullets */
    for (int i = boss.bulletCount - 1; i >= 0; i--) {
        Bullet *b = &boss.bullets[i];
        b->x += b->vx; b->y += b->vy;
        if (b->x < 0 || b->x > W || b->y < 0 || b->y > H) {
            boss.bullets[i] = boss.bullets[--boss.bulletCount];
            continue;
        }
        int bCol = (int)(b->x / CELL), bRow = (int)(b->y / CELL);
        for (int s = 0; s < snakeLen; s++) {
            if (snake[s].x == bCol && snake[s].y == bRow) {
                triggerDeath(); return;
            }
        }
    }

    /* Collision: snake head vs boss body area */
    for (int dx = -boss.size+1; dx < boss.size; dx++) {
        for (int dy2 = -boss.size+1; dy2 < boss.size; dy2++) {
            if (snake[0].x == boss.x + dx && snake[0].y == boss.y + dy2) {
                triggerDeath(); return;
            }
        }
    }
}

static void triggerWin(void);

static void damageBoss(void) {
    boss.hp--;
    boss.flashTimer = 10;
    spawnParticles(boss.x, boss.y, C_RED, 8, 40);

    if (boss.hp <= boss.maxHp / 2 && boss.phase < 2) {
        boss.phase = 2;
        boss.moveInterval  = 12;
        boss.shootInterval = 28;
        spawnParticles(boss.x, boss.y, C_FIRE2, 20, 80);
    }

    if (boss.hp <= 0) {
        spawnExplosion(boss.x, boss.y);
        spawnParticles(boss.x, boss.y, C_FIRE3, 30, 120);
        bossActive = 0; bossPhase = 0;
        score += 500;
        SDL_Delay(800);
        triggerWin();
    }
}

/* ── NUKE SPAWN ──────────────────────────────────────────────────────────────── */
static int isOccupied(int x, int y) {
    for (int i = 0; i < snakeLen; i++)
        if (snake[i].x == x && snake[i].y == y) return 1;
    for (int i = 0; i < nukeCount; i++)
        if (nukes[i].x == x && nukes[i].y == y) return 1;
    return 0;
}

static void spawnNuke(void) {
    if (nukeCount >= MAX_NUKES) return;
    int tries = 0; Vec2i pos;
    do {
        pos.x = rand() % COLS; pos.y = rand() % ROWS;
        tries++;
    } while (tries < 100 && isOccupied(pos.x, pos.y));
    nukes[nukeCount].x = pos.x; nukes[nukeCount].y = pos.y;
    nukes[nukeCount].pulse = ((float)rand()/RAND_MAX) * (float)M_PI * 2.0f;
    nukeCount++;
}

/* ── INIT ───────────────────────────────────────────────────────────────────── */
static void initGame(void) {
    int sx = COLS/2, sy = ROWS/2;
    snakeLen = 3;
    snake[0] = (Vec2i){sx,   sy};
    snake[1] = (Vec2i){sx-1, sy};
    snake[2] = (Vec2i){sx-2, sy};
    dir = nextDir = (Vec2i){1, 0};
    nukeCount = 0;
    score = 0; nukesEaten = 0; level = 1;
    speed = 130;
    running = 1; paused = 0;
    particleCount = 0; explosionCount = 0;
    bossActive = 0; bossPhase = 0;
    lastMoveTime = SDL_GetTicks();
    tick = 0;
    spawnNuke(); spawnNuke();
}

/* ── MOVE ───────────────────────────────────────────────────────────────────── */
static void triggerDeath(void) {
    running = 0;
    if (score > hiScore) hiScore = score;
    for (int i = 0; i < snakeLen; i++)
        spawnParticles(snake[i].x, snake[i].y, C_FIRE1, 6, 30);
    gamePhase = 2;
}

static void triggerWin(void) {
    running = 0;
    if (score > hiScore) hiScore = score;
    gamePhase = 3;
}

static void triggerBossPhase(void) {
    bossPhase = 1; bossActive = 1;
    initBoss();
    nukeCount = 0;
    spawnNuke();
    spawnParticles(COLS/2, ROWS/2, C_RED, 30, 100);
}

static void moveSnake(void) {
    if (!running || paused) return;
    dir = nextDir;

    Vec2i head = { snake[0].x + dir.x, snake[0].y + dir.y };

    /* Wall */
    if (head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS) {
        triggerDeath(); return;
    }
    /* Self */
    for (int i = 0; i < snakeLen; i++) {
        if (snake[i].x == head.x && snake[i].y == head.y) { triggerDeath(); return; }
    }

    /* Push head */
    memmove(&snake[1], &snake[0], snakeLen * sizeof(Vec2i));
    snake[0] = head;
    snakeLen++;

    /* Nuke eaten? */
    int ate = -1;
    for (int i = 0; i < nukeCount; i++) {
        if (nukes[i].x == head.x && nukes[i].y == head.y) { ate = i; break; }
    }

    if (ate >= 0) {
        nukes[ate] = nukes[--nukeCount];
        nukesEaten++;
        score += level * 10;
        spawnParticles(head.x, head.y, C_FIRE3, 14, 50);
        spawnExplosion(head.x, head.y);
        spawnNuke();

        /* Level up */
        if (nukesEaten % 8 == 0) {
            level++;
            speed = (speed - 12 < 60) ? 60 : speed - 12;
        }

        /* Boss trigger */
        if (nukesEaten >= 20 && !bossPhase && !bossActive)
            triggerBossPhase();

        /* Boss damage from head near boss */
        if (bossActive) {
            int bdx = abs(head.x - boss.x), bdy = abs(head.y - boss.y);
            if (bdx < 2 && bdy < 2) damageBoss();
        }
    } else {
        snakeLen--;   /* no food: remove tail */
    }

    if (bossActive) {
        /* Additional damage check */
        int bdx = abs(head.x - boss.x), bdy = abs(head.y - boss.y);
        if (bdx < 2 && bdy < 2 && ate >= 0) damageBoss();
        updateBoss();
    }
}

/* ── DRAW ───────────────────────────────────────────────────────────────────── */
static void drawBackground(void) {
    setColor(C_BG);
    SDL_RenderClear(renderer);
}

static void drawGrid(void) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    setColorA(C_GRID, 80);
    for (int x = 0; x <= COLS; x++)
        SDL_RenderDrawLine(renderer, x*CELL, HUD_H, x*CELL, HUD_H+H);
    for (int y = 0; y <= ROWS; y++)
        SDL_RenderDrawLine(renderer, 0, HUD_H+y*CELL, W, HUD_H+y*CELL);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void drawNukes(void) {
    for (int i = 0; i < nukeCount; i++) {
        Nuke *n = &nukes[i];
        n->pulse += 0.08f;
        int cx = n->x * CELL + CELL/2;
        int cy = n->y * CELL + CELL/2 + HUD_H;
        float glow = 0.6f + 0.4f * sinf(n->pulse);
        int r = CELL/2 - 2;

        /* Outer glow */
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 255, 136, (Uint8)(60 * glow));
        fillCircle(cx, cy, r+5, COL(0,255,136,(Uint8)(60*glow)));
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        /* Body */
        fillCircle(cx, cy, r, C_NUKE_BODY);
        strokeCircle(cx, cy, r, C_NUKE_OUT);

        /* Radioactive wedges */
        for (int w = 0; w < 3; w++) {
            float baseAngle = w * (float)M_PI * 2.0f / 3.0f + n->pulse * 0.1f;
            /* approximate wedge as small triangle */
            float a0 = baseAngle + 0.3f, a1 = baseAngle + (float)M_PI*2.0f/3.0f - 0.3f;
            int wr = r - 3;
            SDL_Point pts[4] = {
                {cx, cy},
                {(int)(cx + wr*cosf(a0)), (int)(cy + wr*sinf(a0))},
                {(int)(cx + wr*cosf((a0+a1)*0.5f)), (int)(cy + wr*sinf((a0+a1)*0.5f))},
                {(int)(cx + wr*cosf(a1)), (int)(cy + wr*sinf(a1))}
            };
            SDL_SetRenderDrawColor(renderer, 0, 224, 96, 255);
            SDL_RenderDrawLines(renderer, pts, 4);
        }

        /* Center dot */
        fillCircle(cx, cy, 3, COL(10,26,10,255));
        fillCircle(cx, cy, 2, COL(0,255,102,255));
    }
}

static void drawDragonHead(int gx, int gy, Vec2i d) {
    int cx = gx * CELL + CELL/2;
    int cy = gy * CELL + CELL/2 + HUD_H;
    int s  = CELL/2 - 1;
    float pulse = 0.9f + 0.1f * sinf(tick * 0.2f);

    /* Head body */
    fillCircle(cx, cy, s, COL(255, 85, 0, 255));

    /* Snout direction offset */
    int sx2 = (int)(d.x * s * 0.6f), sy2 = (int)(d.y * s * 0.6f);
    fillCircle(cx + sx2, cy + sy2, (int)(s * 0.45f), COL(204, 51, 0, 255));

    /* Eyes — offset perpendicular to movement */
    int ex = cx + (int)(d.x * s * 0.2f) - (int)(d.y * s * 0.3f);
    int ey = cy + (int)(d.y * s * 0.2f) + (int)(d.x * s * 0.3f);
    fillCircle(ex, ey, (int)(s*0.18f), COL(255,238,0,255));
    fillCircle(ex + (int)(d.x*s*0.04f), ey + (int)(d.y*s*0.04f), (int)(s*0.08f), COL(17,17,17,255));

    /* Fire breath */
    float fLen = (8 + bossPhase * 4) * pulse;
    int steps = (int)fLen;
    for (int step = 0; step < steps; step++) {
        float t  = (float)step / (float)steps;
        float ff = 1.0f - t;
        Uint8 a  = (Uint8)(200 * ff);
        Uint8 r  = (Uint8)(255);
        Uint8 g  = (Uint8)(200 * (1-t) + 80 * t);
        Uint8 b  = 0;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        int fx = cx + (int)(d.x * (s + step));
        int fy = cy + (int)(d.y * (s + step));
        SDL_RenderDrawPoint(renderer, fx,   fy);
        SDL_RenderDrawPoint(renderer, fx+1, fy);
        SDL_RenderDrawPoint(renderer, fx,   fy+1);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

static void drawSnake(void) {
    for (int i = snakeLen - 1; i >= 0; i--) {
        int gx = snake[i].x, gy = snake[i].y;
        int x  = gx * CELL, y  = gy * CELL + HUD_H;
        float t = (float)i / (float)snakeLen;

        if (i == 0) {
            drawDragonHead(gx, gy, dir);
        } else {
            int margin = 2;
            int cx     = x + CELL/2, cy = y + CELL/2;
            SDL_Color fireColor  = lerpColor(COL(255,102,0,255), COL(204,34,0,255), t);
            SDL_Color innerColor = lerpColor(COL(255,51, 0,255), COL(122,16,0,255), t);

            fillRoundRect(x+margin, y+margin, CELL-margin*2, CELL-margin*2, 4, fireColor);
            int ir = (int)((CELL-margin*2)/2 * 0.6f);
            fillCircle(cx, cy, ir, innerColor);

            if (i % 2 == 0) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 150, 0, 64);
                int sr = (int)((CELL-margin*2)/2 * 0.35f);
                fillCircle(cx, cy-2, sr, COL(255,150,0,64));
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        }
    }
}

static void drawBoss(void) {
    if (!bossActive) return;
    int cx = boss.x * CELL + CELL;
    int cy = boss.y * CELL + CELL + HUD_H;
    float t   = tick * 0.05f;
    int flash = boss.flashTimer > 0 && ((boss.flashTimer / 3) % 2 == 0);
    int bR    = (int)(CELL * 1.6f);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Aura */
    Uint8 auraA = (Uint8)((0.15f + 0.1f * sinf(t * 2)) * 255);
    SDL_SetRenderDrawColor(renderer, 255, 0, 51, auraA);
    fillCircle(cx, cy, (int)(CELL * 2.5f), COL(255,0,51,auraA));

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    /* Body */
    SDL_Color bodyCol = flash ? C_WHITE : (boss.phase >= 2 ? COL(139,0,0,255) : COL(58,0,32,255));
    SDL_Color strokeCol = flash ? C_WHITE : C_RED;
    fillHexagon(cx, cy, bR, bodyCol);
    strokeHexagon(cx, cy, bR, strokeCol);

    /* Core */
    SDL_Color coreCol = flash ? COL(255,136,136,255) : C_RED;
    fillHexagon(cx, cy, (int)(bR * 0.55f), coreCol);

    /* Rotating arc lines (approximate) */
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 102, 0, 153);
        int ar = (int)(bR * 0.8f);
        for (int seg = 0; seg < 2; seg++) {
            float baseAngle = t + seg * (float)M_PI * 2.0f / 3.0f;
            for (int step = 0; step < 20; step++) {
                float a  = baseAngle + step * (float)M_PI * 4.0f / 3.0f / 20;
                float a2 = baseAngle + (step+1) * (float)M_PI * 4.0f / 3.0f / 20;
                SDL_RenderDrawLine(renderer,
                    cx + (int)(ar*cosf(a)),  cy + (int)(ar*sinf(a)),
                    cx + (int)(ar*cosf(a2)), cy + (int)(ar*sinf(a2)));
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    /* Eye */
    fillCircle(cx, cy, (int)(bR * 0.25f), C_WHITE);
    float aimAngle = atan2f((float)(snake[0].y*CELL - cy), (float)(snake[0].x*CELL - cx));
    int ex = cx + (int)(cosf(aimAngle) * bR * 0.1f);
    int ey = cy + (int)(sinf(aimAngle) * bR * 0.1f);
    fillCircle(ex, ey, (int)(bR * 0.14f), C_RED);

    /* HP text */
    {
        char buf[8]; snprintf(buf, sizeof(buf), "%d", boss.hp);
        renderTextCentered(buf, cx, cy - bR - 20, COL(255,136,136,255), fontSmall);
    }

    /* Bullets */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < boss.bulletCount; i++) {
        Bullet *b = &boss.bullets[i];
        SDL_SetRenderDrawColor(renderer, 255, 0, 85, 230);
        fillCircle((int)b->x, (int)b->y + HUD_H, 4, COL(255,0,85,230));
        SDL_SetRenderDrawColor(renderer, 255, 68, 136, 77);
        fillCircle((int)b->x, (int)b->y + HUD_H, 7, COL(255,68,136,77));
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void drawParticles(void) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = particleCount - 1; i >= 0; i--) {
        Particle *p = &particles[i];
        p->x += p->vx; p->y += p->vy;
        p->vy += 0.05f;
        p->life -= p->decay;
        if (p->life <= 0) { particles[i] = particles[--particleCount]; continue; }
        Uint8 a = (Uint8)(p->life * 255);
        int   r = (int)(p->size * p->life);
        if (r < 1) r = 1;
        SDL_SetRenderDrawColor(renderer, p->color.r, p->color.g, p->color.b, a);
        fillCircle((int)p->x, (int)p->y, r, COL(p->color.r,p->color.g,p->color.b,a));
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void drawExplosions(void) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = explosionCount - 1; i >= 0; i--) {
        Explosion *e = &explosions[i];
        e->r += (e->maxR - e->r) * 0.2f;
        e->life -= 0.06f;
        if (e->life <= 0) { explosions[i] = explosions[--explosionCount]; continue; }
        Uint8 strokeA = (Uint8)(e->life * 0.4f * 255);
        Uint8 fillA   = (Uint8)(e->life * 0.15f * 255);
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, strokeA);
        strokeCircle((int)e->x, (int)e->y, (int)e->r, COL(255,215,0,strokeA));
        SDL_SetRenderDrawColor(renderer, 255, 136, 0, fillA);
        fillCircle((int)e->x, (int)e->y, (int)e->r, COL(255,136,0,fillA));
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void drawHUD(void) {
    /* Dark HUD bar */
    SDL_SetRenderDrawColor(renderer, 15, 17, 23, 255);
    SDL_Rect hudRect = {0, 0, WIN_W, HUD_H};
    SDL_RenderFillRect(renderer, &hudRect);

    char buf[64];
    int gap = WIN_W / 4;

    snprintf(buf, sizeof(buf), "%d", score);
    renderTextCentered("SCORE", gap*0, 6, COL(68,68,85,255), fontSmall);
    renderTextCentered(buf,     gap*0, 24, C_FIRE3, font);

    snprintf(buf, sizeof(buf), "%d", nukesEaten);
    renderTextCentered("NUKES", gap*1, 6, COL(68,68,85,255), fontSmall);
    renderTextCentered(buf,     gap*1, 24, C_GREEN, font);

    snprintf(buf, sizeof(buf), "%d", level);
    renderTextCentered("LEVEL", gap*2, 6, COL(68,68,85,255), fontSmall);
    renderTextCentered(buf,     gap*2, 24, C_CYAN, font);

    snprintf(buf, sizeof(buf), "%d", hiScore);
    renderTextCentered("HI-SCORE", gap*3, 6, COL(68,68,85,255), fontSmall);
    renderTextCentered(buf,        gap*3, 24, C_FIRE3, font);

    /* Boss HP bar */
    if (bossActive) {
        int bw = WIN_W - 20, bh = 8, bx = 10, by = HUD_H - 10;
        SDL_SetRenderDrawColor(renderer, 26, 0, 16, 255);
        SDL_Rect barBg = {bx, by, bw, bh};
        SDL_RenderFillRect(renderer, &barBg);
        int fill = (int)(bw * boss.hp / (float)boss.maxHp);
        SDL_SetRenderDrawColor(renderer, 255, 0, 51, 255);
        SDL_Rect barFill = {bx, by, fill, bh};
        SDL_RenderFillRect(renderer, &barFill);

        renderTextCentered("BOSS", WIN_W/2, by - 14, C_RED, fontSmall);
    }
}

static void drawOverlay(const char *title, const char *sub, SDL_Color titleCol) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 5, 8, 15, 235);
    SDL_Rect ov = {0, HUD_H, WIN_W, H};
    SDL_RenderFillRect(renderer, &ov);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    renderTextCentered(title, WIN_W/2, HUD_H + H/2 - 60, titleCol, font);
    renderTextCentered(sub,   WIN_W/2, HUD_H + H/2 - 10, COL(85,85,102,255), fontSmall);
    renderTextCentered("PRESS ENTER TO START", WIN_W/2, HUD_H + H/2 + 40, C_FIRE2, fontSmall);
}

static void drawPauseOverlay(void) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 5, 8, 15, 178);
    SDL_Rect ov = {0, HUD_H, WIN_W, H};
    SDL_RenderFillRect(renderer, &ov);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    renderTextCentered("-- PAUSED --", WIN_W/2, HUD_H + H/2 - 12, C_FIRE2, font);
}

/* ── MAIN LOOP ──────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
    }

    window = SDL_CreateWindow(
        "DRAGON FURY — Nuke Eater",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, SDL_WINDOW_SHOWN
    );
    if (!window) { fprintf(stderr, "Window: %s\n", SDL_GetError()); return 1; }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); return 1; }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Try loading a monospace font; fall back to NULL (text skipped) */
    const char *fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf",
        "/System/Library/Fonts/Courier.ttc",
        "C:\\Windows\\Fonts\\cour.ttf",
        NULL
    };
    font = fontSmall = NULL;
    for (int fi = 0; fontPaths[fi] && !font; fi++) {
        font      = TTF_OpenFont(fontPaths[fi], 18);
        fontSmall = TTF_OpenFont(fontPaths[fi], 10);
    }

    gamePhase = 0;  /* title */
    hiScore   = 0;

    int quit = 0;
    SDL_Event ev;

    while (!quit) {
        /* ── EVENTS ── */
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { quit = 1; break; }
            if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode key = ev.key.keysym.sym;

                /* Global: quit */
                if (key == SDLK_ESCAPE) { quit = 1; break; }

                /* Pause */
                if (key == SDLK_p && gamePhase == 1) { paused = !paused; }

                /* Start / Restart */
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    if (gamePhase != 1) { initGame(); gamePhase = 1; }
                }

                /* Direction */
                if (gamePhase == 1 && running) {
                    if ((key == SDLK_LEFT  || key == SDLK_a) && dir.x != 1)  nextDir = (Vec2i){-1,0};
                    if ((key == SDLK_RIGHT || key == SDLK_d) && dir.x != -1) nextDir = (Vec2i){1, 0};
                    if ((key == SDLK_UP    || key == SDLK_w) && dir.y != 1)  nextDir = (Vec2i){0,-1};
                    if ((key == SDLK_DOWN  || key == SDLK_s) && dir.y != -1) nextDir = (Vec2i){0, 1};
                }
            }
        }
        if (quit) break;

        /* ── UPDATE ── */
        if (gamePhase == 1 && running && !paused) {
            Uint32 now = SDL_GetTicks();
            if ((int)(now - lastMoveTime) >= speed) {
                lastMoveTime = now;
                moveSnake();
                tick++;
            }
        }

        /* ── RENDER ── */
        drawBackground();
        drawGrid();

        if (gamePhase >= 1) {
            drawExplosions();
            drawNukes();
            if (bossActive) drawBoss();
            drawSnake();
            drawParticles();
        }

        drawHUD();

        if (gamePhase == 0) {
            drawOverlay("DRAGON FURY",
                        "CONSUME NUKES. GROW POWERFUL.\nSURVIVE THE NUCLEAR GUARDIAN.\n\nARROW KEYS / WASD TO MOVE  |  P TO PAUSE",
                        C_FIRE2);
        } else if (gamePhase == 2) {
            char sub[64];
            snprintf(sub, sizeof(sub), "SCORE: %d  |  NUKES: %d", score, nukesEaten);
            drawOverlay("ANNIHILATED", sub, C_RED);
        } else if (gamePhase == 3) {
            char sub[64];
            snprintf(sub, sizeof(sub), "THE NUCLEAR GUARDIAN FALLS.  SCORE: %d", score);
            drawOverlay("VICTORY", sub, C_GREEN);
        } else if (paused) {
            drawPauseOverlay();
        }

        SDL_RenderPresent(renderer);
    }

    if (font)      TTF_CloseFont(font);
    if (fontSmall) TTF_CloseFont(fontSmall);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
