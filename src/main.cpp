#include <M5Stack.h>

//#define SOUND_ON

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262

#define RACKETSIZE 8
#define RACKETLINE 37
#define SCRNWIDTH 30
#define SCRNHEIGHT 40

#define BLOCKWIDTH 3
#define BLOCKHEIGHT 2
#define BLOCKTOP 3
#define BLOCKBOTTOM 12
#define BLOCKLINES ((BLOCKBOTTOM - BLOCKTOP + 1) / BLOCKHEIGHT)
#define NBLOCKS (BLOCKLINES * SCRNWIDTH / BLOCKWIDTH)

#define GAMEPORT A5
#define SOUNDPORT 12

boolean exist[NBLOCKS];
boolean demo_mode;

void BlocksInit()
{
    int8_t i;

    for (i = 0; i < NBLOCKS; i++)
    {
        exist[i] = true;
    }
}

uint8_t BlocksLeft()
{
    int8_t i;
    int8_t leftBlocks;

    leftBlocks = 0;
    for (i = 0; i < NBLOCKS; i++)
    {
        if (exist[i])
            leftBlocks++;
    }
    return leftBlocks;
}

#define TOSCRN(v) (v * 6)
uint16_t colors[BLOCKLINES] = {RED, GREEN, BLUE, YELLOW, MAGENTA};

void BlocksDrawAll()
{
    int8_t x, y;
    int8_t w, h;
    int8_t c;
    int8_t i;

    w = TOSCRN(BLOCKWIDTH);
    h = TOSCRN(BLOCKHEIGHT);
    c = 0;
    i = 0;
    for (y = BLOCKTOP; y <= BLOCKBOTTOM; y += BLOCKHEIGHT)
    {
        for (x = 0; x < SCRNWIDTH; x += BLOCKWIDTH)
        {
            if (exist[i++])
            {
                M5.Lcd.fillRect(80 + TOSCRN(x), TOSCRN(y), w, h, colors[c]);
            }
        }
        c++;
    }
    M5.Lcd.drawFastVLine(79,0,240,0x4268);
    M5.Lcd.drawFastVLine(260,0,240,0x4268);
}

void BlocksEraseOne(int8_t block)
{
    int8_t x, y;
    int8_t w, h;
    int8_t rows;

    w = TOSCRN(BLOCKWIDTH);
    h = TOSCRN(BLOCKHEIGHT);
    rows = SCRNWIDTH / BLOCKWIDTH;
    x = (block % rows) * BLOCKWIDTH;
    y = (block / rows) * BLOCKHEIGHT + BLOCKTOP;
    M5.Lcd.fillRect(80 + TOSCRN(x), TOSCRN(y), w, h, BLACK);
}

int8_t BlocksFind(uint8_t x, uint8_t y)
{
    if (x > SCRNWIDTH)
    {
        return -1;
    }
    if ((y < BLOCKTOP) || (y > BLOCKBOTTOM))
    {
        return -1;
    }
    uint8_t rows = SCRNWIDTH / BLOCKWIDTH;
    return (y - BLOCKTOP) / BLOCKHEIGHT * rows + x / BLOCKWIDTH;
}

int8_t BlocksHit(uint8_t x, uint8_t y, int8_t *vx, int8_t *vy)
{
    int8_t block;

    block = BlocksFind(x + *vx, y);
    if ((block >= 0) && exist[block])
    {
        *vx = -*vx;
        return block;
    }
    block = BlocksFind(x, y + *vy);
    if ((block >= 0) && exist[block])
    {
        *vy = -*vy;
        return block;
    }
    block = BlocksFind(x + *vx, y + *vy);
    if ((block >= 0) && exist[block])
    {
        *vx = -*vx;
        *vy = -*vy;
        return block;
    }
    return -1;
}

typedef struct
{
    uint8_t x, y;
    int8_t vx, vy;
} Ball;

#define BallInit(ball) ball = {.x = SCRNWIDTH - 1, .y = BLOCKBOTTOM + BLOCKHEIGHT, .vx = -1, .vy = 1}
#define BallDraw(ball, tft, color) tft.fillRect(80 + ball.x *6, ball.y * 6, 6, 6, color)
#define RacketDraw(racket, tft, color) tft.fillRect(80 + racket * 6, (RACKETLINE + 1) * 6, RACKETSIZE * 6, 6, color)

#define GAME_ONGOING 1
#define GAME_RESTART 0

uint8_t gameStatus;

int16_t racket = (SCRNWIDTH - RACKETSIZE) >> 1;
Ball ball;

void game_move_racket()
{
    RacketDraw(racket, M5.Lcd, BLACK);

    if (M5.BtnC.isPressed())
    {
        racket += 2;
        if (racket > SCRNWIDTH - RACKETSIZE)
        {
            racket = SCRNWIDTH - RACKETSIZE;
        }
    }
    if (M5.BtnA.isPressed())
    {
        if (racket > 0)
        {
            racket -= min(racket, 2);
        }
    }

    if(demo_mode) {
        racket = ball.x - (RACKETSIZE / 2);
        if (M5.BtnA.isPressed() || M5.BtnC.isPressed()) {
            demo_mode = false;
        }
    }
    
    racket = constrain(racket, 0, SCRNWIDTH - RACKETSIZE);
    RacketDraw(racket, M5.Lcd, WHITE);
}

void setup(void)
{
    M5.begin();
    gameStatus = GAME_RESTART;
    BallInit(ball);
    M5.update();
    demo_mode = M5.BtnB.isPressed();
}

boolean game_restart()
{
    if ((ball.y > BLOCKBOTTOM) && (ball.vy > 0))
    {
        BlocksInit();
        BlocksDrawAll();
        return true;
    }
    else
    {
        return false;
    }
}

boolean game_ongoing()
{
    int8_t block;
    boolean blockErased;

    blockErased = false;
    do
    {
        block = BlocksHit(ball.x, ball.y, &ball.vx, &ball.vy);
        if (block >= 0)
        {
            exist[block] = false;
            BlocksEraseOne(block);
            blockErased = true;
        }
    } while (block > 0);
    return blockErased;
}

void loop()
{
    M5.update();
    switch (gameStatus)
    {
    case GAME_RESTART:
        if (game_restart())
        {
            gameStatus = GAME_ONGOING;
        }
        break;
    case GAME_ONGOING:
        if (game_ongoing())
#ifdef SOUND_ON
            M5.Speaker.tone(NOTE_C4, 40);
#endif
        if (BlocksLeft() == 0)
        {
            gameStatus = GAME_RESTART;
        }
        break;
    }

    BallDraw(ball, M5.Lcd, BLACK);
    ball.x += ball.vx;
    if (ball.x >= SCRNWIDTH - 1 || ball.x < 1)
    {
        ball.vx = -ball.vx;
    }
    ball.y += ball.vy;
    if (ball.y == RACKETLINE)
    {
        if (ball.x >= racket && ball.x < racket + RACKETSIZE)
        {
            ball.vy = -ball.vy;
#ifdef SOUND_ON
            M5.Speaker.tone(NOTE_C3, 80);
#endif
        }
    }
    if (ball.y == 0)
    {
        ball.vy = -ball.vy;
    }
    if (ball.y > SCRNHEIGHT)
    {
        BallInit(ball);
    }
    BallDraw(ball, M5.Lcd, YELLOW);

    game_move_racket();

    delay(32);
}
