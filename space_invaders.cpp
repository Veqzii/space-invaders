/*
 * ═══════════════════════════════════════════════════════════════
 *  SPACE INVADERS — C++17 / SDL2
 *  Author  : veqzii
 *  Build   : g++ space_invaders.cpp -o space_invaders \
 *              $(sdl2-config --cflags --libs) -lSDL2_ttf \
 *              -lSDL2_mixer -std=c++17
 * ═══════════════════════════════════════════════════════════════
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// ── Window ───────────────────────────────────────────────────────
static constexpr int WIN_W  = 900;
static constexpr int WIN_H  = 700;
static constexpr int FPS    = 60;
static constexpr int FRAME_MS = 1000 / FPS;

// ── Colours ──────────────────────────────────────────────────────
struct Col { Uint8 r,g,b,a=255; };
static constexpr Col C_BG       {  5,  5, 15};
static constexpr Col C_WHITE    {255,255,255};
static constexpr Col C_GREEN    { 80,230, 80};
static constexpr Col C_CYAN     { 60,220,220};
static constexpr Col C_RED      {230, 60, 60};
static constexpr Col C_YELLOW   {240,200, 40};
static constexpr Col C_ORANGE   {230,130, 40};
static constexpr Col C_MAGENTA  {200, 80,200};
static constexpr Col C_GREY     {120,120,140};
static constexpr Col C_DARKGREY { 40, 40, 55};
static constexpr Col C_GOLD     {245,166, 35};

static void setCol(SDL_Renderer* r, Col c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

// ── High-Score File ───────────────────────────────────────────────
static constexpr int    HS_COUNT    = 10;
static const char*      HS_FILE     = "highscores.dat";

struct HighScore {
    char  name[4] = "AAA";
    int   score   = 0;
    int   level   = 1;
};

static std::array<HighScore, HS_COUNT> g_scores;

static void loadScores() {
    std::ifstream f(HS_FILE, std::ios::binary);
    if (f) f.read(reinterpret_cast<char*>(g_scores.data()),
                  sizeof(HighScore) * HS_COUNT);
    std::sort(g_scores.begin(), g_scores.end(),
              [](const HighScore& a, const HighScore& b){ return a.score > b.score; });
}
static void saveScores() {
    std::sort(g_scores.begin(), g_scores.end(),
              [](const HighScore& a, const HighScore& b){ return a.score > b.score; });
    std::ofstream f(HS_FILE, std::ios::binary);
    if (f) f.write(reinterpret_cast<const char*>(g_scores.data()),
                   sizeof(HighScore) * HS_COUNT);
}
static int lowestScoreIdx() {
    int idx = 0;
    for (int i = 1; i < HS_COUNT; ++i)
        if (g_scores[i].score < g_scores[idx].score) idx = i;
    return idx;
}
static bool isHighScore(int score) {
    return score > g_scores[lowestScoreIdx()].score;
}

// ── Rendering helpers ─────────────────────────────────────────────
static void fillRect(SDL_Renderer* r, int x,int y,int w,int h) {
    SDL_Rect rc{x,y,w,h};
    SDL_RenderFillRect(r,&rc);
}
static void drawRect(SDL_Renderer* r, int x,int y,int w,int h) {
    SDL_Rect rc{x,y,w,h};
    SDL_RenderDrawRect(r,&rc);
}

static SDL_Texture* makeText(SDL_Renderer* r, TTF_Font* f,
                              const std::string& s, Col c) {
    SDL_Color sc{c.r,c.g,c.b,c.a};
    SDL_Surface* sf = TTF_RenderUTF8_Blended(f, s.c_str(), sc);
    if (!sf) return nullptr;
    SDL_Texture* tx = SDL_CreateTextureFromSurface(r, sf);
    SDL_FreeSurface(sf);
    return tx;
}
static void renderText(SDL_Renderer* r, TTF_Font* f, const std::string& s,
                        Col c, int cx, int cy, bool centred=true) {
    SDL_Texture* tx = makeText(r,f,s,c);
    if (!tx) return;
    int tw,th; SDL_QueryTexture(tx,nullptr,nullptr,&tw,&th);
    int x = centred ? cx-tw/2 : cx;
    int y = centred ? cy-th/2 : cy;
    SDL_Rect dst{x,y,tw,th};
    SDL_RenderCopy(r,tx,nullptr,&dst);
    SDL_DestroyTexture(tx);
}

// ── Pixel-art drawing helpers ─────────────────────────────────────
static void drawPixelArt(SDL_Renderer* r,
                          const std::vector<std::string>& art,
                          int ox, int oy, int px, Col c) {
    setCol(r, c);
    for (int row = 0; row < (int)art.size(); ++row)
        for (int col = 0; col < (int)art[row].size(); ++col)
            if (art[row][col] != ' ')
                fillRect(r, ox + col*px, oy + row*px, px, px);
}

// ── Pixel Art Sprites ─────────────────────────────────────────────
// Invader Type A (squid) — 8×8
static const std::vector<std::string> SPR_ALIEN_A = {
    "  XXXX  ",
    " XXXXXX ",
    "XXXXXXXX",
    "XX XX XX",
    "XXXXXXXX",
    " X XX X ",
    "X      X",
    " X    X ",
};
// Invader Type B (crab) — 11×8
static const std::vector<std::string> SPR_ALIEN_B = {
    "  X     X  ",
    "   X   X   ",
    "  XXXXXXX  ",
    " XX XXX XX ",
    "XXXXXXXXXXX",
    "X XXXXXXX X",
    "X X     X X",
    "   XX XX   ",
};
// Invader Type C (octopus) — 12×8
static const std::vector<std::string> SPR_ALIEN_C = {
    "    XXXX    ",
    "XXXXXXXXXXXX",
    "X XXXXXXXX X",
    "X X XXXX X X",
    "XXXXXXXXXXXX",
    "  XXX  XXX  ",
    " XX  XX  XX ",
    "X          X",
};
// UFO — 16×7
static const std::vector<std::string> SPR_UFO = {
    "    XXXXXXXX    ",
    "  XXXXXXXXXXXX  ",
    " XXXXXXXXXXXXXX ",
    "XX X X X X X XX ",
    " XXXXXXXXXXXXXX ",
    "   XXX    XXX   ",
    "  XX        XX  ",
};
// Player ship — 13×8
static const std::vector<std::string> SPR_PLAYER = {
    "      X      ",
    "     XXX     ",
    "     XXX     ",
    " XXXXXXXXXXX ",
    "XXXXXXXXXXXXX",
    "XXXXXXXXXXXXX",
    "XX         XX",
    "X           X",
};
// Shield block — 22×16
static const std::vector<std::string> SPR_SHIELD = {
    "   XXXXXXXXXXXXXXXX   ",
    "  XXXXXXXXXXXXXXXXXXXX",
    " XXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXX",
    "XXXXXXX       XXXXXXXX",
    "XXXXXX         XXXXXXX",
    "XXXXX           XXXXXX",
    "XXXXX           XXXXXX",
};

// ── Particles ──────────────────────────────────────────────────────
struct Particle {
    float x,y,vx,vy;
    int   life, maxLife;
    Col   color;
    int   size;
};

static std::vector<Particle> g_particles;

static void spawnExplosion(float cx, float cy, Col c, int count=12, int speed=3) {
    for (int i=0; i<count; ++i) {
        float angle = (float)(rand()%360) * 3.14159f / 180.f;
        float spd   = 1.f + (rand()%speed);
        g_particles.push_back({
            cx, cy,
            cosf(angle)*spd, sinf(angle)*spd,
            30 + rand()%30, 60,
            c, 2 + rand()%3
        });
    }
}
static void updateParticles() {
    for (auto& p : g_particles) {
        p.x += p.vx; p.y += p.vy;
        p.vy += 0.08f;
        --p.life;
    }
    g_particles.erase(
        std::remove_if(g_particles.begin(), g_particles.end(),
                       [](const Particle& p){ return p.life<=0; }),
        g_particles.end());
}
static void renderParticles(SDL_Renderer* r) {
    for (auto& p : g_particles) {
        float a = (float)p.life / p.maxLife;
        SDL_SetRenderDrawColor(r, p.color.r, p.color.g, p.color.b,
                               (Uint8)(a*255));
        fillRect(r, (int)p.x, (int)p.y, p.size, p.size);
    }
    SDL_SetRenderDrawColor(r,255,255,255,255);
}

// ── Star field ────────────────────────────────────────────────────
struct Star { int x,y,speed,brightness; };
static std::vector<Star> g_stars;
static void initStars() {
    g_stars.clear();
    for (int i=0; i<120; ++i)
        g_stars.push_back({ rand()%WIN_W, rand()%WIN_H,
                             1 + rand()%3, 80 + rand()%176 });
}
static void updateStars() {
    for (auto& s : g_stars) {
        s.y += s.speed;
        if (s.y >= WIN_H) { s.y=0; s.x=rand()%WIN_W; }
    }
}
static void renderStars(SDL_Renderer* r) {
    for (auto& s : g_stars) {
        SDL_SetRenderDrawColor(r, s.brightness, s.brightness, s.brightness, 255);
        SDL_RenderDrawPoint(r, s.x, s.y);
    }
}

// ── Bullet ────────────────────────────────────────────────────────
struct Bullet {
    float x, y;
    float vy;        // negative = up, positive = down
    bool  active = false;
    bool  player = true;
    int   w=3, h=12;
};

// ── Shield pixel grid ─────────────────────────────────────────────
static constexpr int SHIELD_PX = 3;   // pixels per cell
static constexpr int SHIELD_COLS = 22;
static constexpr int SHIELD_ROWS = 16;

struct Shield {
    float x, y;
    bool  cells[SHIELD_ROWS][SHIELD_COLS];

    void init(float sx, float sy) {
        x=sx; y=sy;
        for (int r=0; r<SHIELD_ROWS; ++r)
            for (int c=0; c<SHIELD_COLS; ++c)
                cells[r][c] = (SPR_SHIELD[r][c]!=' ');
    }
    bool hitTest(float bx,float by,int bw,int bh) {
        // Returns true if bullet overlaps shield
        int left  = (int)(bx-x)/SHIELD_PX;
        int top   = (int)(by-y)/SHIELD_PX;
        int right = (int)(bx+bw-x)/SHIELD_PX;
        int bot   = (int)(by+bh-y)/SHIELD_PX;
        for (int r=std::max(0,top);  r<=std::min(SHIELD_ROWS-1,bot);  ++r)
        for (int c=std::max(0,left); c<=std::min(SHIELD_COLS-1,right);++c)
            if (cells[r][c]) { cells[r][c]=false; return true; }
        return false;
    }
    void damage(float bx,float by,int bw,int bh, int radius=2) {
        int cx=(int)((bx+bw/2-x)/SHIELD_PX);
        int cy=(int)((by+bh/2-y)/SHIELD_PX);
        for (int r=cy-radius; r<=cy+radius; ++r)
        for (int c=cx-radius; c<=cx+radius; ++c)
            if (r>=0&&r<SHIELD_ROWS&&c>=0&&c<SHIELD_COLS)
                if (rand()%3 != 0) cells[r][c]=false;
    }
};

// ── Alien ─────────────────────────────────────────────────────────
struct Alien {
    float x, y;
    bool  alive = true;
    int   type;       // 0=top(squid), 1=mid(crab), 2=bot(octopus)
    int   animFrame=0;
    int   points;
};

// ── UFO ───────────────────────────────────────────────────────────
struct UFO {
    float x, y;
    float vx;
    bool  active=false;
    int   points;
    float spawnTimer=0;
    static constexpr float SPAWN_INTERVAL = 25.f * FPS; // ~25 seconds
};

// ── Game state enum ───────────────────────────────────────────────
enum class GameState {
    TITLE,
    PLAYING,
    LEVEL_CLEAR,
    GAME_OVER,
    HIGH_SCORE_ENTRY,
    HIGH_SCORES,
    PAUSED,
};

// ══════════════════════════════════════════════════════════════════
//  GAME CLASS
// ══════════════════════════════════════════════════════════════════
struct Game {
    // SDL
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font*     fontLg   = nullptr;
    TTF_Font*     fontMd   = nullptr;
    TTF_Font*     fontSm   = nullptr;
    TTF_Font*     fontXs   = nullptr;

    // State
    GameState state = GameState::TITLE;
    bool      running = true;

    // Score / Level
    int  score       = 0;
    int  hiScore     = 0;
    int  level       = 1;
    int  lives       = 3;

    // Level config (scales with level)
    float alienSpeed      = 0.f;
    float alienDropDist   = 16.f;
    float alienFireRate   = 0.f;  // prob per alien per frame
    int   alienBulletMax  = 3;

    // Player
    float playerX = WIN_W/2.f - 32.f;
    float playerY = WIN_H - 80.f;
    float playerSpeed = 5.f;
    int   playerInvTimer = 0;    // invincibility frames after hit

    // Bullets
    static constexpr int MAX_PLAYER_BULLETS = 3;
    static constexpr int MAX_ALIEN_BULLETS  = 8;
    std::vector<Bullet> playerBullets;
    std::vector<Bullet> alienBullets;
    int   shootCooldown = 0;

    // Aliens
    static constexpr int ALIEN_COLS = 11;
    static constexpr int ALIEN_ROWS = 5;
    std::vector<Alien> aliens;
    float alienGridX    = 0.f;   // grid origin
    float alienGridY    = 0.f;
    float alienMoveDir  = 1.f;   // +1 right, -1 left
    int   alienMoveTimer= 0;
    int   alienMovePeriod= 0;    // frames between steps
    bool  alienDrop     = false;
    float alienDropLeft = 0.f;
    int   alienAnimTimer= 0;
    int   aliensAlive   = 0;

    // UFO
    UFO ufo;

    // Shields
    static constexpr int SHIELD_COUNT = 4;
    std::array<Shield, SHIELD_COUNT> shields;

    // Name entry (high score)
    char entryName[4] = "AAA";
    int  entryChar    = 0;   // cursor position 0–2
    int  entryCursor  = 0;   // letter A-Z 0–25

    // Timing
    int  levelClearTimer = 0;
    int  titleBlink      = 0;
    int  frameCount      = 0;

    // Screen flash
    int  flashTimer = 0;
    Col  flashCol   = C_WHITE;

    bool init();
    void cleanup();
    void run();
    void handleEvents();
    void update();
    void render();

    // State handlers
    void startGame();
    void loadLevel();
    void nextLevel();
    void playerDie();
    void gameOver();

    // Update sub-systems
    void updatePlayer();
    void updateAliens();
    void updateBullets();
    void updateUFO();
    void updateCollisions();
    void updateShields();

    // Render sub-systems
    void renderBackground();
    void renderHUD();
    void renderPlayer();
    void renderAliens();
    void renderBullets();
    void renderUFO();
    void renderShields();
    void renderLives();

    // Screen renders
    void renderTitle();
    void renderLevelClear();
    void renderGameOver();
    void renderHighScoreEntry();
    void renderHighScores();
    void renderPaused();

    int  countAlive() const;
    bool anyAlienReachedPlayer() const;
    void getLevelConfig();
};

// ─────────────────────────────────────────────────────────────────
bool Game::init() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    srand((unsigned)time(nullptr));

    window = SDL_CreateWindow("SPACE INVADERS",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, 0);
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return false;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Try common font paths
    auto tryFont = [](const char* path, int pt) -> TTF_Font* {
        return TTF_OpenFont(path, pt);
    };
    const char* boldPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/Library/Fonts/Arial Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
        nullptr
    };
    const char* regPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        nullptr
    };

    for (int i=0; boldPaths[i] && !fontLg; ++i) fontLg = tryFont(boldPaths[i], 36);
    for (int i=0; boldPaths[i] && !fontMd; ++i) fontMd = tryFont(boldPaths[i], 22);
    for (int i=0; regPaths[i]  && !fontSm; ++i) fontSm = tryFont(regPaths[i],  16);
    for (int i=0; regPaths[i]  && !fontXs; ++i) fontXs = tryFont(regPaths[i],  13);

    loadScores();
    hiScore = g_scores[0].score;
    initStars();
    return true;
}

void Game::cleanup() {
    if (fontLg) TTF_CloseFont(fontLg);
    if (fontMd) TTF_CloseFont(fontMd);
    if (fontSm) TTF_CloseFont(fontSm);
    if (fontXs) TTF_CloseFont(fontXs);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

// ── Level difficulty config ───────────────────────────────────────
void Game::getLevelConfig() {
    // Speed: aliens move faster each level, faster when fewer remain
    alienMovePeriod = std::max(4, 28 - (level-1)*2);
    alienFireRate   = 0.0008f + (level-1)*0.0003f;
    alienBulletMax  = std::min(3 + (level-1), 8);
    alienDropDist   = 16.f;
    alienSpeed      = 1.f + (level-1)*0.2f;
}

// ── Start / Load ──────────────────────────────────────────────────
void Game::startGame() {
    score  = 0;
    level  = 1;
    lives  = 3;
    playerBullets.clear();
    alienBullets.clear();
    g_particles.clear();
    loadLevel();
    state = GameState::PLAYING;
}

void Game::loadLevel() {
    getLevelConfig();

    // Grid layout
    const int cellW = 60, cellH = 50;
    const int startX = (WIN_W - ALIEN_COLS*cellW) / 2;
    const int startY = 80 + (level-1)*0;  // keep top fixed

    alienGridX   = (float)startX;
    alienGridY   = (float)startY;
    alienMoveDir = 1.f;
    alienMoveTimer = 0;
    alienDrop    = false;
    alienDropLeft= 0.f;
    alienAnimTimer=0;

    aliens.clear();
    for (int row=0; row<ALIEN_ROWS; ++row) {
        int type = (row == 0) ? 0 : (row <= 2) ? 1 : 2;
        int pts  = (row == 0) ? 30 : (row <= 2) ? 20 : 10;
        for (int col=0; col<ALIEN_COLS; ++col) {
            Alien a;
            a.x     = alienGridX + col*cellW;
            a.y     = alienGridY + row*cellH;
            a.type  = type;
            a.points= pts;
            a.alive = true;
            aliens.push_back(a);
        }
    }
    aliensAlive = ALIEN_COLS * ALIEN_ROWS;

    // Player position
    playerX       = WIN_W/2.f - 26.f;
    playerY       = WIN_H - 80.f;
    playerInvTimer= 0;
    alienBullets.clear();
    playerBullets.clear();

    // UFO
    ufo.active      = false;
    ufo.spawnTimer  = (float)(UFO::SPAWN_INTERVAL + rand()%120);

    // Shields
    const int shieldY   = (int)playerY - 100;
    const int shieldW   = SHIELD_COLS * SHIELD_PX;
    const int gap       = (WIN_W - SHIELD_COUNT * shieldW) / (SHIELD_COUNT+1);
    for (int i=0; i<SHIELD_COUNT; ++i)
        shields[i].init((float)(gap + i*(shieldW+gap)),
                        (float)shieldY);

    flashTimer = 60; flashCol = C_GREEN;
}

// ── Main loop ─────────────────────────────────────────────────────
void Game::run() {
    while (running) {
        Uint32 start = SDL_GetTicks();
        handleEvents();
        update();
        render();
        Uint32 elapsed = SDL_GetTicks() - start;
        if (elapsed < (Uint32)FRAME_MS) SDL_Delay(FRAME_MS - elapsed);
    }
}

// ── Events ────────────────────────────────────────────────────────
void Game::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { running=false; return; }
        if (e.type == SDL_KEYDOWN) {
            auto k = e.key.keysym.sym;

            if (state == GameState::TITLE) {
                if (k==SDLK_RETURN || k==SDLK_SPACE) startGame();
                if (k==SDLK_h) state = GameState::HIGH_SCORES;
                if (k==SDLK_ESCAPE) running=false;
            }
            else if (state == GameState::PLAYING) {
                if (k==SDLK_p) state=GameState::PAUSED;
                if (k==SDLK_ESCAPE) { state=GameState::TITLE; }
                if ((k==SDLK_SPACE||k==SDLK_UP||k==SDLK_z) && shootCooldown<=0) {
                    if ((int)playerBullets.size() < MAX_PLAYER_BULLETS) {
                        playerBullets.push_back({
                            playerX + 26.f, playerY - 5.f,
                            -10.f, 0, true, true });
                        shootCooldown = 8;
                    }
                }
            }
            else if (state == GameState::PAUSED) {
                if (k==SDLK_p||k==SDLK_RETURN) state=GameState::PLAYING;
                if (k==SDLK_ESCAPE) state=GameState::TITLE;
            }
            else if (state == GameState::LEVEL_CLEAR) {
                if (levelClearTimer <= 0) nextLevel();
            }
            else if (state == GameState::GAME_OVER) {
                if (k==SDLK_RETURN||k==SDLK_SPACE) {
                    if (isHighScore(score)) {
                        state=GameState::HIGH_SCORE_ENTRY;
                        entryChar=0; entryCursor=0;
                        entryName[0]='A'; entryName[1]='A';
                        entryName[2]='A'; entryName[3]='\0';
                    } else {
                        state=GameState::TITLE;
                    }
                }
            }
            else if (state == GameState::HIGH_SCORE_ENTRY) {
                if (k==SDLK_UP)    { entryCursor=(entryCursor+1)%26; entryName[entryChar]='A'+entryCursor; }
                if (k==SDLK_DOWN)  { entryCursor=(entryCursor+25)%26; entryName[entryChar]='A'+entryCursor; }
                if (k==SDLK_RIGHT||k==SDLK_RETURN) {
                    if (entryChar<2) { ++entryChar; entryCursor=entryName[entryChar]-'A'; }
                    else {
                        // Save
                        int idx = lowestScoreIdx();
                        g_scores[idx].score=score;
                        g_scores[idx].level=level;
                        strncpy(g_scores[idx].name, entryName, 3);
                        g_scores[idx].name[3]='\0';
                        saveScores();
                        hiScore=g_scores[0].score;
                        state=GameState::HIGH_SCORES;
                    }
                }
                if (k==SDLK_LEFT && entryChar>0) { --entryChar; entryCursor=entryName[entryChar]-'A'; }
            }
            else if (state == GameState::HIGH_SCORES) {
                if (k==SDLK_RETURN||k==SDLK_ESCAPE||k==SDLK_SPACE)
                    state=GameState::TITLE;
            }
        }
    }
}

// ── Update ────────────────────────────────────────────────────────
void Game::update() {
    ++frameCount;
    updateParticles();
    updateStars();
    if (flashTimer>0) --flashTimer;

    if (state==GameState::PLAYING) {
        updatePlayer();
        updateAliens();
        updateBullets();
        updateUFO();
        updateCollisions();
        if (anyAlienReachedPlayer()) { playerDie(); }
        if (aliensAlive==0) {
            state=GameState::LEVEL_CLEAR;
            levelClearTimer=120;
            spawnExplosion(WIN_W/2.f,WIN_H/2.f,C_GOLD,30,5);
        }
    }
    else if (state==GameState::LEVEL_CLEAR) {
        if(--levelClearTimer<=0) nextLevel();
    }
    if (state==GameState::TITLE) titleBlink = (titleBlink+1)%60;
}

void Game::updatePlayer() {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A])
        playerX = std::max(0.f, playerX - playerSpeed);
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        playerX = std::min((float)(WIN_W-52), playerX + playerSpeed);
    if (shootCooldown>0) --shootCooldown;
    if (playerInvTimer>0) --playerInvTimer;
}

void Game::updateAliens() {
    // Animation toggle
    if (++alienAnimTimer >= 30) { alienAnimTimer=0;
        for (auto& a : aliens) if(a.alive) a.animFrame^=1; }

    // Adjust speed based on remaining count
    int alive = countAlive();
    int period = std::max(2, alienMovePeriod * alive / (ALIEN_COLS*ALIEN_ROWS));

    if (alienDrop) {
        // Move down
        for (auto& a : aliens) if(a.alive) a.y += 1.f;
        alienDropLeft -= 1.f;
        if (alienDropLeft <= 0.f) {
            alienDrop = false;
            alienMoveDir = -alienMoveDir;
        }
        return;
    }

    if (++alienMoveTimer < period) return;
    alienMoveTimer = 0;

    // Move horizontally
    float step = alienSpeed * alienMoveDir;
    for (auto& a : aliens) if(a.alive) a.x += step;

    // Check edge collision
    float leftX=99999, rightX=-99999;
    for (auto& a : aliens) if(a.alive) {
        leftX  = std::min(leftX,  a.x);
        rightX = std::max(rightX, a.x + 32.f);
    }
    if (rightX >= WIN_W-10 || leftX <= 10) {
        alienDrop = true;
        alienDropLeft = alienDropDist;
    }

    // Random alien fires
    if (alienBullets.size() < (size_t)alienBulletMax) {
        for (auto& a : aliens) {
            if (!a.alive) continue;
            if ((float)rand()/RAND_MAX < alienFireRate) {
                alienBullets.push_back({ a.x+16.f, a.y+32.f,
                    4.f + level*0.3f, 0, false, false });
                if (alienBullets.size() >= (size_t)alienBulletMax) break;
            }
        }
    }
}

void Game::updateBullets() {
    for (auto& b : playerBullets)  b.y += b.vy;
    for (auto& b : alienBullets)   b.y += b.vy;

    // Remove off-screen
    playerBullets.erase(
        std::remove_if(playerBullets.begin(), playerBullets.end(),
                       [](const Bullet& b){ return b.y<-20; }),
        playerBullets.end());
    alienBullets.erase(
        std::remove_if(alienBullets.begin(), alienBullets.end(),
                       [](const Bullet& b){ return b.y>WIN_H+20; }),
        alienBullets.end());
}

void Game::updateUFO() {
    if (ufo.active) {
        ufo.x += ufo.vx;
        if (ufo.x > WIN_W+60 || ufo.x < -60) ufo.active=false;
    } else {
        ufo.spawnTimer -= 1.f;
        if (ufo.spawnTimer <= 0.f) {
            ufo.active = true;
            ufo.vx     = (rand()%2==0) ? 2.5f : -2.5f;
            ufo.x      = (ufo.vx>0) ? -60.f : WIN_W+10.f;
            ufo.y      = 40.f;
            ufo.points = (rand()%4+1)*50; // 50,100,150,200
            ufo.spawnTimer = (float)(UFO::SPAWN_INTERVAL + rand()%180);
        }
    }
}

void Game::updateCollisions() {
    // Player bullets vs aliens
    for (auto& b : playerBullets) {
        if (b.y < 0) continue;
        for (auto& a : aliens) {
            if (!a.alive) continue;
            if (b.x>=a.x && b.x<=a.x+36 && b.y>=a.y && b.y<=a.y+32) {
                score += a.points;
                hiScore = std::max(hiScore, score);
                a.alive = false;
                --aliensAlive;
                spawnExplosion(a.x+18,a.y+16,
                    a.type==0 ? C_CYAN : a.type==1 ? C_GREEN : C_YELLOW, 16, 4);
                b.y = -100.f; // deactivate bullet
                flashTimer=6; flashCol={255,200,50,60};
                break;
            }
        }
    }

    // Player bullets vs UFO
    if (ufo.active) {
        for (auto& b : playerBullets) {
            if (b.x>=ufo.x && b.x<=ufo.x+64 && b.y>=ufo.y && b.y<=ufo.y+28) {
                score += ufo.points;
                hiScore = std::max(hiScore, score);
                spawnExplosion(ufo.x+32, ufo.y+14, C_RED, 24, 5);
                ufo.active=false;
                b.y=-100.f;
                flashTimer=10; flashCol={255,60,60,80};
            }
        }
    }

    // Player bullets vs shields
    for (auto& b : playerBullets) {
        for (auto& s : shields) {
            if (s.hitTest(b.x,b.y,b.w,b.h)) {
                s.damage(b.x,b.y,b.w,b.h,1);
                b.y=-100.f;
            }
        }
    }

    // Alien bullets vs player
    if (playerInvTimer<=0) {
        for (auto& b : alienBullets) {
            if (b.x>=playerX-4 && b.x<=playerX+56 &&
                b.y>=playerY    && b.y<=playerY+32) {
                b.y=WIN_H+100.f;
                playerDie();
                return;
            }
        }
    }

    // Alien bullets vs shields
    for (auto& b : alienBullets) {
        for (auto& s : shields) {
            if (s.hitTest(b.x,b.y,b.w,b.h)) {
                s.damage(b.x,b.y,b.w,b.h,2);
                b.y=WIN_H+100.f;
            }
        }
    }

    // Aliens vs shields — erode shields as aliens descend
    for (auto& a : aliens) if(a.alive)
        for (auto& s : shields)
            if (a.y+32 >= s.y && a.x+36 >= s.x &&
                a.x <= s.x + SHIELD_COLS*SHIELD_PX)
                s.damage(a.x, a.y+28, 36, 4, 3);
}

bool Game::anyAlienReachedPlayer() const {
    for (auto& a : aliens)
        if (a.alive && a.y + 32 >= playerY) return true;
    return false;
}

int Game::countAlive() const {
    int c=0;
    for (auto& a : aliens) if(a.alive) ++c;
    return c;
}

void Game::playerDie() {
    spawnExplosion(playerX+26,playerY+16,C_GREEN,20,4);
    --lives;
    flashTimer=30; flashCol={230,60,60,100};
    if (lives<=0) { gameOver(); return; }
    playerX       = WIN_W/2.f - 26.f;
    playerInvTimer= 120; // 2 seconds invincibility
    alienBullets.clear();
}

void Game::gameOver() {
    spawnExplosion(WIN_W/2.f,WIN_H/2.f,C_RED,30,6);
    state=GameState::GAME_OVER;
}

void Game::nextLevel() {
    ++level;
    if (level > 20) { // You've won all 20 levels
        state=GameState::GAME_OVER;
        return;
    }
    loadLevel();
    state=GameState::PLAYING;
}

// ═══════════════════════════════════════════════════════════════
//  RENDER
// ═══════════════════════════════════════════════════════════════
void Game::render() {
    // Background
    setCol(renderer, C_BG);
    SDL_RenderClear(renderer);

    renderBackground();

    // Screen flash overlay
    if (flashTimer>0) {
        Col fc = flashCol;
        fc.a = (Uint8)((float)flashTimer/30*flashCol.a);
        SDL_SetRenderDrawColor(renderer,fc.r,fc.g,fc.b,fc.a);
        fillRect(renderer,0,0,WIN_W,WIN_H);
    }

    if (state==GameState::TITLE)             renderTitle();
    else if (state==GameState::PLAYING)      { renderShields(); renderAliens();
                                               renderUFO(); renderBullets();
                                               renderPlayer(); renderParticles(renderer);
                                               renderHUD(); }
    else if (state==GameState::LEVEL_CLEAR)  { renderShields(); renderAliens();
                                               renderUFO(); renderPlayer();
                                               renderParticles(renderer);
                                               renderHUD(); renderLevelClear(); }
    else if (state==GameState::PAUSED)       { renderShields(); renderAliens();
                                               renderUFO(); renderPlayer();
                                               renderHUD(); renderPaused(); }
    else if (state==GameState::GAME_OVER)    { renderParticles(renderer); renderGameOver(); }
    else if (state==GameState::HIGH_SCORE_ENTRY) renderHighScoreEntry();
    else if (state==GameState::HIGH_SCORES)      renderHighScores();

    SDL_RenderPresent(renderer);
}

void Game::renderBackground() {
    renderStars(renderer);
    // Bottom ground line
    setCol(renderer, C_GREEN);
    fillRect(renderer, 0, WIN_H-50, WIN_W, 2);
}

void Game::renderHUD() {
    if (!fontSm) return;
    // Score
    renderText(renderer,fontSm,"SCORE",C_WHITE,10,8,false);
    renderText(renderer,fontMd?fontMd:fontSm,std::to_string(score),C_GREEN,10,26,false);
    // Hi-Score
    renderText(renderer,fontSm,"HI-SCORE",C_WHITE,WIN_W/2,8);
    renderText(renderer,fontMd?fontMd:fontSm,std::to_string(hiScore),C_GOLD,WIN_W/2,26);
    // Level
    renderText(renderer,fontSm,"LEVEL",C_WHITE,WIN_W-80,8,false);
    renderText(renderer,fontMd?fontMd:fontSm,std::to_string(level),C_CYAN,WIN_W-80,26,false);
    // Lives
    renderLives();
    // Controls hint
    renderText(renderer,fontXs?fontXs:fontSm,"[ARROWS] MOVE  [SPACE] FIRE  [P] PAUSE",
               C_GREY, WIN_W/2, WIN_H-20);
}

void Game::renderLives() {
    if (!fontSm) return;
    renderText(renderer,fontSm,"LIVES",C_WHITE,10,WIN_H-45,false);
    for (int i=0; i<lives; ++i) {
        // Mini player ship
        int ox = 70 + i*24, oy = WIN_H-46;
        setCol(renderer,C_GREEN);
        fillRect(renderer,ox+6,oy,4,4);
        fillRect(renderer,ox+2,oy+4,12,4);
        fillRect(renderer,ox,oy+8,16,8);
    }
}

void Game::renderPlayer() {
    if (playerInvTimer>0 && (playerInvTimer/6)%2==1) return; // blink
    drawPixelArt(renderer, SPR_PLAYER,
                 (int)playerX, (int)playerY, 4, C_GREEN);
}

void Game::renderAliens() {
    for (auto& a : aliens) {
        if (!a.alive) continue;
        const auto* spr = (a.type==0) ? &SPR_ALIEN_A
                        : (a.type==1) ? &SPR_ALIEN_B
                                      : &SPR_ALIEN_C;
        Col c = (a.type==0) ? C_CYAN
              : (a.type==1) ? C_GREEN
                            : C_YELLOW;
        // Simple 2-frame animation: shift sprite slightly
        int ox = (a.animFrame==1) ? 2 : 0;
        int pixSize = (a.type==2) ? 3 : 3;
        drawPixelArt(renderer, *spr,
                     (int)a.x + ox, (int)a.y, pixSize, c);
    }
}

void Game::renderUFO() {
    if (!ufo.active) return;
    // Pulsing red
    Uint8 pulse = 180 + (Uint8)(sin(frameCount*0.15f)*75);
    Col uc{pulse, 40, 40, 255};
    drawPixelArt(renderer, SPR_UFO, (int)ufo.x, (int)ufo.y, 3, uc);
    // Points label
    if (fontXs)
        renderText(renderer,fontXs,std::to_string(ufo.points)+"!",
                   C_GOLD,(int)ufo.x+32,(int)ufo.y-10);
}

void Game::renderBullets() {
    setCol(renderer, C_GREEN);
    for (auto& b : playerBullets)
        fillRect(renderer,(int)b.x,(int)b.y,b.w,b.h);

    for (auto& b : alienBullets) {
        // Zig-zag alien bullet
        int xOff = ((int)(b.y/4))%3-1;
        setCol(renderer,C_RED);
        fillRect(renderer,(int)b.x+xOff,(int)b.y,3,10);
    }
}

void Game::renderShields() {
    for (auto& s : shields) {
        setCol(renderer, C_GREEN);
        for (int r=0; r<SHIELD_ROWS; ++r)
        for (int c=0; c<SHIELD_COLS; ++c)
            if (s.cells[r][c])
                fillRect(renderer,
                         (int)s.x+c*SHIELD_PX,
                         (int)s.y+r*SHIELD_PX,
                         SHIELD_PX, SHIELD_PX);
    }
}

// ── Screen renders ────────────────────────────────────────────────
void Game::renderTitle() {
    if (!fontLg) return;
    // Title
    renderText(renderer,fontLg,"SPACE INVADERS",C_GREEN,WIN_W/2,120);
    // Animated alien sprites
    int ox=WIN_W/2-160;
    drawPixelArt(renderer,SPR_ALIEN_C,ox,    190,4,C_YELLOW);
    drawPixelArt(renderer,SPR_ALIEN_B,ox+120,190,4,C_GREEN);
    drawPixelArt(renderer,SPR_ALIEN_A,ox+240,190,4,C_CYAN);
    drawPixelArt(renderer,SPR_UFO,    ox+330,193,3,C_RED);

    if (fontSm) {
        // Score table
        renderText(renderer,fontSm,"= ? =  MYSTERY",C_RED,   WIN_W/2,290);
        renderText(renderer,fontSm,"= A =   30 PTS",C_CYAN,  WIN_W/2,315);
        renderText(renderer,fontSm,"= B =   20 PTS",C_GREEN, WIN_W/2,340);
        renderText(renderer,fontSm,"= C =   10 PTS",C_YELLOW,WIN_W/2,365);
    }

    if (titleBlink < 40 && fontMd)
        renderText(renderer,fontMd,"PRESS SPACE TO PLAY",C_WHITE,WIN_W/2,430);
    if (fontSm) {
        renderText(renderer,fontSm,"[H] HIGH SCORES",C_GREY,WIN_W/2,465);
        renderText(renderer,fontSm,"[ESC] QUIT",C_GREY,WIN_W/2,488);
    }
    if (fontXs)
        renderText(renderer,fontXs,"ARROWS/WASD MOVE  |  SPACE/Z FIRE  |  P PAUSE",
                   C_GREY,WIN_W/2,WIN_H-30);

    // Scrolling hi-score
    if (fontSm)
        renderText(renderer,fontSm,"HI-SCORE: "+std::to_string(hiScore),
                   C_GOLD,WIN_W/2,540);
}

void Game::renderLevelClear() {
    if (!fontLg) return;
    setCol(renderer,{0,0,0,160});
    fillRect(renderer,WIN_W/2-200,WIN_H/2-60,400,120);
    renderText(renderer,fontLg,"LEVEL CLEAR!",C_GOLD,WIN_W/2,WIN_H/2-20);
    if (fontSm)
        renderText(renderer,fontSm,
                   "FLOOR "+ std::to_string(level)+" SCORE "+std::to_string(score),
                   C_GREEN,WIN_W/2,WIN_H/2+28);
}

void Game::renderGameOver() {
    if (!fontLg) return;
    setCol(renderer,{0,0,0,200});
    fillRect(renderer,0,0,WIN_W,WIN_H);
    renderText(renderer,fontLg,"GAME OVER",C_RED,WIN_W/2,WIN_H/2-80);
    if (fontMd) {
        renderText(renderer,fontMd,"SCORE: "+std::to_string(score),C_WHITE,WIN_W/2,WIN_H/2-20);
        renderText(renderer,fontMd,"LEVEL: "+std::to_string(level), C_CYAN, WIN_W/2,WIN_H/2+20);
        if (isHighScore(score))
            renderText(renderer,fontMd,"NEW HIGH SCORE!",C_GOLD,WIN_W/2,WIN_H/2+60);
    }
    if (fontSm)
        renderText(renderer,fontSm,"PRESS SPACE TO CONTINUE",C_GREY,WIN_W/2,WIN_H/2+110);
}

void Game::renderPaused() {
    setCol(renderer,{0,0,0,160});
    fillRect(renderer,WIN_W/2-150,WIN_H/2-50,300,100);
    if (fontLg) renderText(renderer,fontLg,"PAUSED",C_WHITE,WIN_W/2,WIN_H/2-10);
    if (fontSm) renderText(renderer,fontSm,"[P] RESUME  [ESC] QUIT",C_GREY,WIN_W/2,WIN_H/2+35);
}

void Game::renderHighScoreEntry() {
    setCol(renderer,{0,0,0,220});
    fillRect(renderer,0,0,WIN_W,WIN_H);
    if (!fontLg) return;
    renderText(renderer,fontLg,"NEW HIGH SCORE!",C_GOLD,WIN_W/2,120);
    if (fontMd) {
        renderText(renderer,fontMd,"SCORE: "+std::to_string(score),C_WHITE,WIN_W/2,180);
        renderText(renderer,fontMd,"ENTER YOUR NAME:",C_CYAN,WIN_W/2,240);
    }
    // Name display
    for (int i=0; i<3; ++i) {
        int x = WIN_W/2 - 60 + i*60;
        bool active = (i==entryChar);
        // Box
        setCol(renderer, active ? C_GOLD : C_GREY);
        drawRect(renderer,x-18,290,36,44);
        if (fontLg) {
            char tmp[2] = {entryName[i],'\0'};
            renderText(renderer,fontLg,tmp,active?C_GOLD:C_WHITE,x,308);
        }
    }
    if (fontSm) {
        renderText(renderer,fontSm,"UP/DOWN: Change letter",C_GREY,WIN_W/2,360);
        renderText(renderer,fontSm,"RIGHT/ENTER: Next  |  LEFT: Back",C_GREY,WIN_W/2,385);
        renderText(renderer,fontSm,"Press ENTER on last letter to save",C_GREEN,WIN_W/2,415);
    }
}

void Game::renderHighScores() {
    setCol(renderer,{0,0,0,230});
    fillRect(renderer,0,0,WIN_W,WIN_H);
    if (!fontLg) return;
    renderText(renderer,fontLg,"HIGH SCORES",C_GOLD,WIN_W/2,50);
    if (fontSm) {
        renderText(renderer,fontSm,"RANK    NAME    SCORE    LEVEL",C_GREY,WIN_W/2,110);
        setCol(renderer,C_GREY);
        fillRect(renderer,WIN_W/2-220,125,440,2);
        for (int i=0; i<HS_COUNT; ++i) {
            auto& hs = g_scores[i];
            if (hs.score<=0) continue;
            Col c = (i==0)?C_GOLD:(i<3)?C_GREEN:C_WHITE;
            std::string line = "#" + std::to_string(i+1)
                             + "      " + hs.name
                             + "      " + std::to_string(hs.score)
                             + "      LV" + std::to_string(hs.level);
            renderText(renderer,fontSm,line,c,WIN_W/2,145+i*36);
        }
        renderText(renderer,fontSm,"PRESS SPACE/ENTER TO RETURN",C_GREY,WIN_W/2,WIN_H-40);
    }
}

// ═══════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════
int main(int /*argc*/, char** /*argv*/) {
    Game game;
    if (!game.init()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "Error","Failed to initialise SDL2.",nullptr);
        return 1;
    }
    game.run();
    saveScores();
    game.cleanup();
    return 0;
}
