/*
  Solution to course project #10
  Introduction to programming course
  Faculty of Mathemathics and Informatics of Sofia University
  Winter semester 2025/2026

  @author: Стоян Златев Дрянов
  @idnumber 5MI0600612
  @compiler VC

  ASCII Knight - Main game file for a simple ASCII arena game.

  - Player can move, jump (double jump), and perform directional attacks.
  - Multiple enemy types (walker, jumper, flier, crawler) each with simple AI.
  - Wave system: waves 1-4 spawn normal enemies, wave 5 spawns a boss with
    telegraphed attack patterns (X-laser, + laser, earthquake, lightning, cross sweep).
  - Uses Win32 console API for cursor positioning and color.

  Notes:
  - This file is Windows-only (uses <windows.h>, console APIs and <conio.h>).
  - Rendering writes directly to the console in many places for transient visuals.
*/

#include <iostream>
#include <windows.h>
#include <ctime>
#include <conio.h>
#include <cstdlib>

using namespace std;

// ========== CONSTANTS ==========
// Arena configuration and characters used for rendering and collision.
const int ARENA_WIDTH = 80;
const int ARENA_HEIGHT = 20;
const char WALL_CHAR = '#';
const char PLAYER_CHAR = '@';
const char PLATFORM_CHAR = '=';

// Player physics and limits
const float PLAYER_SPEED = 0.5f;
const float GRAVITY = 0.05f;
const float JUMP_FORCE = -0.9f;
const int MAX_JUMPS = 2;

// Attack timings
const float ATTACK_COOLDOWN = 30.0f;
const float ATTACK_DURATION = 15.0f;

// Damage invulnerability duration after being hit
const float DAMAGE_COOLDOWN = 60.0f;

// Enemy types and behavior tuning constants
const char ENEMY_WALKER_CHAR = 'E';
const float ENEMY_WALKER_SPEED = 0.2f;

const char ENEMY_JUMPER_CHAR = 'J';
const float ENEMY_JUMPER_SPEED = 0.2f;
const float ENEMY_JUMPER_JUMP_FORCE = -1.0f;
const float ENEMY_JUMPER_DETECTION_RANGE = 10.0f;
const float ENEMY_JUMPER_JUMP_COOLDOWN = 90.0f;

const char ENEMY_FLIER_CHAR = 'F';
const float ENEMY_FLIER_SPEED = 0.1f;
const float ENEMY_FLIER_DIVE_SPEED = 0.4f;
const float ENEMY_FLIER_RISE_SPEED = 0.4f;
const float ENEMY_FLIER_DIVE_DURATION = 120.0f;
const float ENEMY_FLIER_RISE_DURATION = 120.0f;
const float ENEMY_FLIER_MIN_HEIGHT = 5.0f;
const float ENEMY_FLIER_MAX_HEIGHT = 15.0f;

const char ENEMY_CRAWLER_CHAR = 'C';
const float ENEMY_CRAWLER_SPEED = 0.6f; // Note: higher crawler speed can cause unwanted behaviour

// Boss configuration - placed near center of arena
const char ENEMY_BOSS_CHAR = 'B';
const int BOSS_Y = ARENA_HEIGHT / 2 - 1;
const int BOSS_X = ARENA_WIDTH / 2 - 1;
const int BOSS_SIZE = 3;
const int BOSS_HP = 5;

// Boss timings (damage cooldown, attack cooldowns, durations)
const float BOSS_DAMAGE_COOLDOWN = 180.0f; // invulnerability duration after being hit
const float BOSS_EARTHQUAKE_INTERVAL = 180.0f;
const float BOSS_ATTACK_COOLDOWN = 90.0f;
const float BOSS_ATTACK_DURATION = 60.0f;
const float BOSS_LIGHTNING_DURATION = 60.0f;
const float BOSS_LASER_DURATION = 90.0f;
const float BOSS_WARNING_DURATION = 60.0f;


// ========== STRUCTS ==========
// Types used throughout the game state.

enum EnemyType {
    ENEMY_WALKER,
    ENEMY_JUMPER,
    ENEMY_FLIER,
    ENEMY_CRAWLER,
    ENEMY_BOSS
};

enum Color {
    COLOR_BLACK = 0,
    COLOR_DARK_BLUE = 1,
    COLOR_DARK_GREEN = 2,
    COLOR_DARK_CYAN = 3,
    COLOR_DARK_RED = 4,
    COLOR_DARK_MAGENTA = 5,
    COLOR_DARK_YELLOW = 6,
    COLOR_GRAY = 7,
    COLOR_DARK_GRAY = 8,
    COLOR_BLUE = 9,
    COLOR_GREEN = 10,
    COLOR_CYAN = 11,
    COLOR_RED = 12,
    COLOR_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15
};

enum AttackDirection {
    ATTACK_NONE = 0,
    ATTACK_UP,
    ATTACK_DOWN,
    ATTACK_LEFT,
    ATTACK_RIGHT
};

enum BossAttackPattern {
    BOSS_PATTERN_NONE = 0,
    BOSS_PATTERN_X_LASER,      // X-shaped lasers 
    BOSS_PATTERN_PLUS_LASER,   // +-shaped lasers 
    BOSS_PATTERN_EARTHQUAKE,   // Ground spikes
    BOSS_PATTERN_LIGHTNING,    // Random lightning strikes
    BOSS_PATTERN_CROSS_SWEEP,  // Sweeping horizontal then vertical
};

// Represents a transient player attack: direction, timer, and small buffer of overwritten
// background characters so we can restore the arena after drawing the attack visual.
struct Attack {
    AttackDirection direction;
    float timer; // countdown until the attack visual ends
    bool active;
    int lastX, lastY; // previous player position where attack was drawn
    AttackDirection lastDirection; // previous attack direction
    char savedChars[3]; // characters overwritten by attack visual (max 3)
    int savedPositions[3][2]; // positions of the overwritten characters
    int numSavedChars; // how many background chars stored
};

// Player state
struct Player {
    float x, y; // position in arena coordinates (float for smooth movement)
    float dy;   // vertical speed
    int hp;
    int jumps; // number of jumps used (for double jump)
    bool grounded;
    int lastX, lastY; // integer last-rendered position for erasing
    float attackCooldown; // cooldown until next attack
    Attack currentAttack; // active attack data
    float damageCooldown; // invulnerability time after taking damage
};

// Generic enemy struct that contains fields used by all enemy types.
// Some fields are only meaningful for certain types (e.g., boss fields).
struct Enemy {
    EnemyType type;
    float x, y;
    float dx, dy;
    bool grounded;
    bool active;
    int lastX, lastY;
    int direction; // 1 = right, -1 = left
    float jumpCooldown;
    float flierTimer;
    bool diving;
    int crawlerState; // 0 = Right, 1 = Up, 2 = Left, 3 = Down
    //boss only
    int hp; 
    float damageCooldown;
    BossAttackPattern currentPattern; 
    bool isAttacking; 
    float attackCooldown; 
    float attackTimer; 
    float attackPhase; 
    bool isWarning; 
};

struct LightningStrike {
    int x, y;
    bool active;
};

// ========== GLOBAL VARIABLES ==========
Player player;
clock_t lastTime;
char arena[ARENA_HEIGHT][ARENA_WIDTH];

Enemy* enemies = nullptr; 
int maxEnemies = 0;
int enemyCount = 0;

Enemy* boss = nullptr; 
int currentWave = 1;
bool EarhquakeActive = false;
float earthquakeTimer = 0;
LightningStrike lightningStrikes[16];

bool gameOver = false;

// ========== UTILITIES / CONSOLE HELPERS ==========

// Move console cursor to (x,y).
void gotoXY(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Hide the blinking cursor for cleaner rendering.
void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info = { 100, FALSE };
    SetConsoleCursorInfo(consoleHandle, &info);
}

// Simple tile collision helper: true for wall or platform tiles.
bool isCollisionTile(char tile) {
    return tile == PLATFORM_CHAR || tile == WALL_CHAR;
}

// Check if a coordinate is within the playable area (ignores outer walls).
bool isInBounds(int x, int y) {
    return x > 0 && x < ARENA_WIDTH - 1 && y > 0 && y < ARENA_HEIGHT;
}

// Set console text color.
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

//Get string length.
int getLength(const char* str) {
    int length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
};

// ========== ENEMY MANAGEMENT ==========

// Ensure capacity and add a new enemy initialized for the type provided with spawn coordinates x,y.
void spawnEnemy(EnemyType type, float x, float y) {
    // Grow array when needed (doubling strategy).
    if (enemyCount >= maxEnemies) {
        int newMax = maxEnemies * 2;
        Enemy* newEnemies = new Enemy[newMax];

        for (int i = 0; i < enemyCount; i++) {
            newEnemies[i] = enemies[i];
        }

        delete[] enemies;
        enemies = newEnemies;
        maxEnemies = newMax;
    }

    Enemy& enemy = enemies[enemyCount];
    enemy.type = type;
    enemy.x = x;
    enemy.y = y;
    enemy.dy = 0;
    enemy.grounded = false;
    enemy.active = true;
    enemy.lastX = (int)x;
    enemy.lastY = (int)y;
    enemy.jumpCooldown = 0;
    enemy.flierTimer = 0;
    enemy.diving = false;

    switch (type) {
    case ENEMY_WALKER:
        enemy.dx = ENEMY_WALKER_SPEED;
        enemy.direction = 1; 
        break;
    case ENEMY_JUMPER:
        enemy.dx = ENEMY_JUMPER_SPEED;
        enemy.direction = 1;
        break;
    case ENEMY_FLIER:
        enemy.dx = ENEMY_FLIER_SPEED;
        enemy.direction = 1;
        enemy.flierTimer = ENEMY_FLIER_RISE_DURATION;
        enemy.diving = false;
        break;
    case ENEMY_CRAWLER:
        enemy.dx = ENEMY_CRAWLER_SPEED;
        enemy.direction = 1;
        enemy.crawlerState = 0;
        break;
    case ENEMY_BOSS:
        enemy.hp = BOSS_HP;
        enemy.damageCooldown = 0;
        enemy.currentPattern = BOSS_PATTERN_NONE;
        enemy.isAttacking = false;
        enemy.attackCooldown = BOSS_ATTACK_COOLDOWN;
        enemy.attackTimer = 0;
        enemy.attackPhase = 0;
        enemy.isWarning = false;
        break;
    }

    enemyCount++;
}

// Try to find a valid spawn position in the arena that:
// - is empty 
// - not too close to the player or other enemies
// Returns true and sets x, y when found.
bool findValidSpawnPosition(float& x, float& y) {
    int attempts = 0;

    while (attempts < 100)
    {
        x = (float)(rand() % (ARENA_WIDTH - 4) + 2);
        y = (float)(rand() % (ARENA_HEIGHT - 4) + 2);

        int ix = (int)x;
        int iy = (int)y;

        if (arena[iy][ix] != ' ') {
            attempts++;
            continue;
        }

        // not too close to player
        float dx = x - player.x;
        float dy = y - player.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < 25.0f) {  // within 5 units
            attempts++;
            continue;
        }

        // not too close to other enemies
        bool tooClose = false;
        for (int j = 0; j < enemyCount; j++) {
            if (!enemies[j].active)
                continue;

            float edx = x - enemies[j].x;
            float edy = y - enemies[j].y;
            float edistSq = edx * edx + edy * edy;

            if (edistSq < 9.0f) {  // within 3 units
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            return true;
        }
        else {
            attempts++;
        }
    }

    return false; // Failed to find a valid spot after attempts
}

// Remove all platform tiles from the arena (used for boss fight wave).
void removePlatforms() {
    for (int y = 1; y < ARENA_HEIGHT - 1; y++)
    {
        for (int x = 1; x < ARENA_WIDTH - 1; x++)
        {
            if (isCollisionTile(arena[y][x])) {
                arena[y][x] = ' ';
                gotoXY(x, y);
                cout << ' ';
            }
        }
    }
}

// Locate the boss in the enemies array and set the 'boss' pointer.
void findBoss() {
    boss = nullptr;
    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].type == ENEMY_BOSS && enemies[i].active) {
            boss = &enemies[i];
            return;
        }
    }
}

// Spawn enemies for a given wave. Wave 5 spawns the boss and removes platforms.
void spawnWave(int waveNumber) {
    // Give player an extra hitpoint per wave due to game difficulty
    if (player.hp < 5)
        player.hp++;

    if (waveNumber == 5)
    {
        spawnEnemy(ENEMY_BOSS, BOSS_X, BOSS_Y);

        findBoss();

        // remove platforms for boss fight
        removePlatforms();

        return;
    }

    // Determine counts based on wave
    int walkers = (waveNumber >= 1) ? waveNumber : 0;
    int jumpers = (waveNumber >= 2) ? waveNumber - 1 : 0;
    int fliers = (waveNumber >= 3) ? waveNumber - 2 : 0;
    int crawlers = (waveNumber >= 4) ? waveNumber - 3 : 0;

    // Spawn each type using valid positions
    for (int i = 0; i < walkers; i++)
    {
        float x, y;
        if (findValidSpawnPosition(x, y)) {
            spawnEnemy(ENEMY_WALKER, x, y);
        }
    }

    for (int i = 0; i < jumpers; i++)
    {
        float x, y;
        if (findValidSpawnPosition(x, y)) {
            spawnEnemy(ENEMY_JUMPER, x, y);
        }
    }

    for (int i = 0; i < fliers; i++)
    {
        float x, y;
        if (findValidSpawnPosition(x, y)) {
            spawnEnemy(ENEMY_FLIER, x, y);
        }
    }

    for (int i = 0; i < crawlers; i++)
    {
        float x, y;
        if (findValidSpawnPosition(x, y)) {
            spawnEnemy(ENEMY_CRAWLER, x, y);
        }
    }
}

// ========== INITIALIZATION ==========
void initArena() {
    for (int y = 0; y < ARENA_HEIGHT; y++) {
        for (int x = 0; x < ARENA_WIDTH; x++) {
            if (y == 0 || y == ARENA_HEIGHT - 1 || x == 0 || x == ARENA_WIDTH - 1)
                arena[y][x] = WALL_CHAR;
            else
                arena[y][x] = ' ';
        }
    }
}

// Generate random platforms
void generatePlatforms() {
    for (int i = 0; i < 4; i++) {
        int pWidth = rand() % 10 + 10; // Width between 10 and 20
        int pX, pY;
        if (i < 2) { // Two on the left side
            pX = rand() % 15 + 5;
        }
        else {     // Two on the right side
            pX = rand() % 15 + 50;
        }
        // Space them out vertically
        pY = (i % 2 == 0) ? (ARENA_HEIGHT * 0.35) : (ARENA_HEIGHT * 0.65);
        pY += (rand() % 3 - 1); // small random vertical offset
        for (int x = 0; x < pWidth; x++) {
            if (pX + x < ARENA_WIDTH - 1)
                arena[pY][pX + x] = PLATFORM_CHAR;
        }
    }
}

void drawArena() {
    for (int y = 0; y < ARENA_HEIGHT; y++) {
        for (int x = 0; x < ARENA_WIDTH; x++) {
            setColor(COLOR_DARK_GRAY);
            cout << arena[y][x];
        }
        cout << endl;
    }
    setColor(COLOR_WHITE);
}

// Initialize player fields and attack state.
void initPlayer() {
    player.x = ARENA_WIDTH / 2.0f;
    player.y = ARENA_HEIGHT / 2.0f;
    player.dy = 0;
    player.hp = 5;
    player.jumps = 0;
    player.grounded = false;
    player.lastX = (int)player.x;
    player.lastY = (int)player.y;
    player.attackCooldown = 0;
    player.damageCooldown = 0;
    player.currentAttack.active = false;
    player.currentAttack.direction = ATTACK_NONE;
    player.currentAttack.timer = 0;
    player.currentAttack.lastX = -1;
    player.currentAttack.lastY = -1;
    player.currentAttack.lastDirection = ATTACK_NONE;
    player.currentAttack.numSavedChars = 0;
}

// Initialize the entire game state: arena, platforms, player and first wave.
void initGame() {
    hideCursor();
    srand((unsigned int)time(NULL)); // Seed RNG

    initArena();
    generatePlatforms();
    drawArena();
    initPlayer();

    maxEnemies = 10;
    enemies = new Enemy[maxEnemies];
    enemyCount = 0;

    spawnWave(1);

    lastTime = clock();
}

// ========== INPUT HANDLING ==========
void handleAttackInput(char key) {
    if (player.attackCooldown > 0 || player.currentAttack.active)
        return;

    AttackDirection direction = ATTACK_NONE;

    if (key == 'i') direction = ATTACK_UP;
    else if (key == 'j') direction = ATTACK_LEFT;
    else if (key == 'k') direction = ATTACK_DOWN;
    else if (key == 'l') direction = ATTACK_RIGHT;

    if (direction != ATTACK_NONE) {
        player.currentAttack.active = true;
        player.currentAttack.direction = direction;
        player.currentAttack.timer = ATTACK_DURATION;
        player.attackCooldown = ATTACK_COOLDOWN;
    }
}

void handleInput(float dt) {
    if (!_kbhit()) return;

    char key = _getch();

    if (key == 'a' && player.x > 2)
        player.x -= PLAYER_SPEED * dt;

    if (key == 'd' && player.x < ARENA_WIDTH - 2)
        player.x += PLAYER_SPEED * dt;

    if (key == 'w') {
        if (player.grounded) {
            player.dy = JUMP_FORCE;
            player.grounded = false;
            player.jumps = 1;
        }
        else if (player.jumps < MAX_JUMPS) {
            player.dy = JUMP_FORCE;
            player.jumps++;
        }
    }

    handleAttackInput(key);
}

// ========== BOSS ATTACK VISUAL FUNCTIONS ==========

// Clear boss attack visuals from the arena but preserve boss and player.
void clearBossAttackVisuals() {
    int centerX = BOSS_X + 1;
    int centerY = BOSS_Y + 1;

    // Clear everything in the arena (except walls and boss area)
    for (int y = 1; y < ARENA_HEIGHT - 1; y++) {
        for (int x = 1; x < ARENA_WIDTH - 1; x++) {
            // Skip boss area
            if (x >= BOSS_X && x <= BOSS_X + 2 && y >= BOSS_Y && y <= BOSS_Y + 2) {
                continue;
            }
            // Skip player position
            if (x == (int)player.x && y == (int)player.y) {
                continue;
            }
            gotoXY(x, y);
            cout << ' ';
        }
    }
}

// Draw the X-shaped laser centered on boss (visual only).
void drawXLaser() {
    int centerX = BOSS_X + 1;
    int centerY = BOSS_Y + 1;

    setColor(COLOR_RED);

    // Draw 3-char wide diagonals outward from boss.
    for (int i = 1; i <= 20; i++) {
        for (int thickness = -1; thickness <= 1; thickness++) {
            // Top-left to bottom-right
            int x1 = centerX - i;
            int y1 = centerY - i + thickness;
            int x2 = centerX + i;
            int y2 = centerY + i + thickness;

            // Top-right to bottom-left
            int x3 = centerX + i;
            int y3 = centerY - i + thickness;
            int x4 = centerX - i;
            int y4 = centerY + i + thickness;

            if (x1 > 0 && x1 < ARENA_WIDTH - 1 && y1 > 0 && y1 < ARENA_HEIGHT - 1) {
                gotoXY(x1, y1); cout << '\\';
            }
            if (x2 > 0 && x2 < ARENA_WIDTH - 1 && y2 > 0 && y2 < ARENA_HEIGHT - 1) {
                gotoXY(x2, y2); cout << '\\';
            }
            if (x3 > 0 && x3 < ARENA_WIDTH - 1 && y3 > 0 && y3 < ARENA_HEIGHT - 1) {
                gotoXY(x3, y3); cout << '/';
            }
            if (x4 > 0 && x4 < ARENA_WIDTH - 1 && y4 > 0 && y4 < ARENA_HEIGHT - 1) {
                gotoXY(x4, y4); cout << '/';
            }
        }
    }

    setColor(COLOR_WHITE);
}

// Draw a plus-shaped laser: horizontal and vertical beams across the arena.
void drawPlusLaser() {
    int centerX = BOSS_X + 1;
    int centerY = BOSS_Y + 1;

    setColor(COLOR_RED);

    // Horizontal beam (thickness = 3)
    for (int x = 1; x < ARENA_WIDTH - 1; x++) {
        for (int thickness = -1; thickness <= 1; thickness++) {
            int yPos = centerY + thickness;
            // Don't draw over the boss area
            if (!(x >= BOSS_X && x <= BOSS_X + 2 && yPos >= BOSS_Y && yPos <= BOSS_Y + 2)) {
                if (yPos > 0 && yPos < ARENA_HEIGHT - 1) {
                    gotoXY(x, yPos); cout << '=';
                }
            }
        }
    }

    // Vertical beam (thickness = 3)
    for (int y = 1; y < ARENA_HEIGHT - 1; y++) {
        for (int thickness = -1; thickness <= 1; thickness++) {
            int xPos = centerX + thickness;
            // Don't draw over the boss itself
            if (!(xPos >= BOSS_X && xPos <= BOSS_X + 2 && y >= BOSS_Y && y <= BOSS_Y + 2)) {
                if (xPos > 0 && xPos < ARENA_WIDTH - 1) {
                    gotoXY(xPos, y); cout << '|';
                }
            }
        }
    }

    setColor(COLOR_WHITE);
}

// Draw earthquake visual on the ground (a row of '^').
void drawEarthquake() {
    int groundY = ARENA_HEIGHT - 2;
    setColor(COLOR_YELLOW);

    for (int x = 1; x < ARENA_WIDTH - 1; x++) {
        gotoXY(x, groundY);
        cout << '^';
    }
    setColor(COLOR_WHITE);
}

// Spawn a handful of lightning strikes at random positions.
void spawnLightning() {
    // Spawn 6-15 random lightning strikes
    int numStrikes = 6 + rand() % 10;

    for (int i = 0; i < numStrikes && i < 16; i++) {
        lightningStrikes[i].x = 2 + rand() % (ARENA_WIDTH - 4);
        lightningStrikes[i].y = 1 + rand() % (ARENA_HEIGHT - 3);
        lightningStrikes[i].active = true;
    }

    // Clear remaining slots
    for (int i = numStrikes; i < 16; i++) {
        lightningStrikes[i].active = false;
    }
}

// Draw active lightning strike visuals ('*' with '|' above/below).
void drawLightning() {
    setColor(COLOR_YELLOW);

    for (int i = 0; i < 16; i++) {
        if (!lightningStrikes[i].active) continue;

        int x = lightningStrikes[i].x;
        int y = lightningStrikes[i].y;

        // Draw lightning bolt (3 chars tall)
        if (y > 0) {
            gotoXY(x, y - 1); cout << '|';
        }
        gotoXY(x, y); cout << '*';
        if (y < ARENA_HEIGHT - 2) {
            gotoXY(x, y + 1); cout << '|';
        }
    }

    setColor(COLOR_WHITE);
}

// Draw a sweeping cross (horizontal then vertical) that moves based on phase (0..1).
void drawCrossSweep(float phase) {
    int centerX = BOSS_X + 1;
    int centerY = BOSS_Y + 1;

    setColor(COLOR_CYAN);

    // Phase 0-0.5: horizontal sweep moving upward
    // Phase 0.5-1: vertical sweep moving rightward
    if (phase < 0.5f) {
        // Horizontal line sweeping up
        int y = ARENA_HEIGHT - 2 - (int)(phase * 2.0f * (ARENA_HEIGHT - 3));
        for (int x = 1; x < ARENA_WIDTH - 1; x++) {
            if (y > 0 && y < ARENA_HEIGHT - 1) {
                gotoXY(x, y);
                cout << '-';
            }
        }
    }
    else {
        // Vertical line sweeping right
        int x = 1 + (int)((phase - 0.5f) * 2.0f * (ARENA_WIDTH - 2));
        for (int y = 1; y < ARENA_HEIGHT - 1; y++) {
            if (x > 0 && x < ARENA_WIDTH - 1) {
                gotoXY(x, y);
                cout << '|';
            }
        }
    }

    setColor(COLOR_WHITE);
}

// ========== PHYSICS & COLLISIONS ==========

// Check whether current boss attack hits the player. Applies damage and sets invulnerability.
void checkBossAttackCollision() {
    if (boss == nullptr || !boss->active) return;
    if (!boss->isAttacking) return;
    if (player.damageCooldown > 0) return;

    int px = (int)player.x;
    int py = (int)player.y;
    int centerX = BOSS_X + 1;
    int centerY = BOSS_Y + 1;

    bool hit = false;

    switch (boss->currentPattern) {
    case BOSS_PATTERN_EARTHQUAKE:
        // Player on ground is hit by earthquake
        if (py == ARENA_HEIGHT - 2) {
            hit = true;
        }
        break;

    case BOSS_PATTERN_LIGHTNING:
        // Check if player is in any lightning strike vertical column (3-high)
        for (int i = 0; i < 10; i++) {
            if (!lightningStrikes[i].active) continue;

            int lx = lightningStrikes[i].x;
            int ly = lightningStrikes[i].y;

            if (px == lx && (py == ly - 1 || py == ly || py == ly + 1)) {
                hit = true;
                break;
            }
        }
        break;

    case BOSS_PATTERN_X_LASER:
        // Check all diagonal positions used by X-laser (thickness 3)
        for (int i = 1; i <= 20; i++) {
            for (int thickness = -1; thickness <= 1; thickness++) {
                if ((px == centerX - i && py == centerY - i + thickness) ||
                    (px == centerX + i && py == centerY + i + thickness) ||
                    (px == centerX + i && py == centerY - i + thickness) ||
                    (px == centerX - i && py == centerY + i + thickness)) {
                    hit = true;
                    break;
                }
            }
            if (hit) break;
        }
        break;

    case BOSS_PATTERN_PLUS_LASER:
        // Check horizontal and vertical beams (thickness 3)
        for (int i = 1; i <= 20; i++) {
            for (int thickness = -1; thickness <= 1; thickness++) {
                // Horizontal beam positions
                if ((py == centerY + thickness) &&
                    (px == centerX - i || px == centerX + i)) {
                    hit = true;
                    break;
                }
                // Vertical beam positions
                if ((px == centerX + thickness) &&
                    (py == centerY - i || py == centerY + i)) {
                    hit = true;
                    break;
                }
            }
            if (hit) break;
        }
        break;

    case BOSS_PATTERN_CROSS_SWEEP: {
        float phase = boss->attackPhase;
        if (phase < 0.5f) {
            int y = ARENA_HEIGHT - 2 - (int)(phase * 2.0f * (ARENA_HEIGHT - 3));
            if (py == y) {
                hit = true;
            }
        }
        else {
            int x = 1 + (int)((phase - 0.5f) * 2.0f * (ARENA_WIDTH - 2));
            if (px == x) {
                hit = true;
            }
        }
        break;
    }

    }

    if (hit) {
        player.hp--;
        player.damageCooldown = DAMAGE_COOLDOWN;
    }
}

// Check if player's active attack hits any enemies.
// Deactivates standard enemies and decreases boss HP with invulnerability cooldown.
void checkAttackCollision() {
    if (!player.currentAttack.active)
        return;

    int px = (int)player.x;
    int py = (int)player.y;

    // Map the set of hit positions based on attack direction (3 tiles)
    int hitX[3], hitY[3];
    int hitCount = 0;

    switch (player.currentAttack.direction) {
    case ATTACK_UP:
        hitX[0] = px - 1; hitY[0] = py - 1;
        hitX[1] = px;     hitY[1] = py - 1;
        hitX[2] = px + 1; hitY[2] = py - 1;
        hitCount = 3;
        break;
    case ATTACK_DOWN:
        hitX[0] = px - 1; hitY[0] = py + 1;
        hitX[1] = px;     hitY[1] = py + 1;
        hitX[2] = px + 1; hitY[2] = py + 1;
        hitCount = 3;
        break;
    case ATTACK_LEFT:
        hitX[0] = px - 1; hitY[0] = py - 1;
        hitX[1] = px - 1; hitY[1] = py;
        hitX[2] = px - 1; hitY[2] = py + 1;
        hitCount = 3;
        break;
    case ATTACK_RIGHT:
        hitX[0] = px + 1; hitY[0] = py - 1;
        hitX[1] = px + 1; hitY[1] = py;
        hitX[2] = px + 1; hitY[2] = py + 1;
        hitCount = 3;
        break;
    default:
        return;
    }

    // Check every enemy to see if they overlap any hit tile.
    for (int i = 0; i < enemyCount; i++) {

        Enemy& enemy = enemies[i];
        if (!enemy.active)
            continue;

        int ex = (int)enemy.x;
        int ey = (int)enemy.y;

        // Boss has a 3x3 area and temporary invulnerability via damageCooldown.
        if (enemy.type == ENEMY_BOSS) {
            if (enemy.damageCooldown > 0) {
                continue;  // Boss is temporarily invincible
            }

            int bx = (int)enemy.x;
            int by = (int)enemy.y;

            // If any hit tile overlaps the boss, damage the boss.
            for (int h = 0; h < hitCount; h++) {
                if (hitX[h] >= bx && hitX[h] <= bx + 2 &&
                    hitY[h] >= by && hitY[h] <= by + 2) {

                    enemy.hp--;
                    enemy.damageCooldown = BOSS_DAMAGE_COOLDOWN;

                    if (enemy.hp <= 0) {
                        // Boss defeated: clear boss area and end game (win)
                        enemy.active = false;
                        for (int cy = 0; cy < BOSS_SIZE; cy++) {
                            for (int cx = 0; cx < BOSS_SIZE; cx++) {
                                gotoXY(bx + cx, by + cy);
                                cout << ' ';
                            }
                        }
                        gameOver = true;
                    }
                    return;
                }
            }
            continue;
        }

        
        for (int h = 0; h < hitCount; h++) {
            if (ex == hitX[h] && ey == hitY[h]) {
                enemy.active = false; // Enemy is hit and deactivated

                // Erase the enemy's last known position on screen
                gotoXY(enemy.lastX, enemy.lastY);
                cout << ' ';
                break;
            }
        }
    }
}

// Check collisions where player and enemies occupy same tile
void checkPlayerEnemyCollision() {
    if (player.damageCooldown > 0)
        return;

    int px = (int)player.x;
    int py = (int)player.y;

    for (int i = 0; i < enemyCount; i++)
    {
        Enemy& enemy = enemies[i];
        if (!enemy.active) continue;

        int ex = (int)enemy.x;
        int ey = (int)enemy.y;

        // Boss contact
        if (enemy.type == ENEMY_BOSS) {
            int bx = (int)enemy.x;
            int by = (int)enemy.y;

            if (px >= bx && px <= bx + 2 && py >= by && py <= by + 2) {
                player.hp--;
                player.damageCooldown = DAMAGE_COOLDOWN;
                return;
            }
            continue;
        }

        // Standard enemy
        if (px == ex && py == ey) {
            player.hp--;
            player.damageCooldown = DAMAGE_COOLDOWN;
            break;
        }
    }
}

// Update attack timers and cooldowns for player and boss
void updateAttackTimers(float dt) {
    // Player attack cooldown
    if (player.attackCooldown > 0) {
        player.attackCooldown -= dt;
        if (player.attackCooldown < 0) {
            player.attackCooldown = 0;
        }
    }

    // Player's active attack display timer
    if (player.currentAttack.active) {
        player.currentAttack.timer -= dt;
        if (player.currentAttack.timer <= 0) {
            player.currentAttack.active = false;
            player.currentAttack.direction = ATTACK_NONE;
            player.currentAttack.timer = 0;
        }
    }

    // Player damage cooldown (invulnerability)
    if (player.damageCooldown > 0) {
        player.damageCooldown -= dt;
        if (player.damageCooldown < 0) {
            player.damageCooldown = 0;
        }
    }

    // Boss damage cooldown (invulnerability after being hit)
    if (boss != nullptr) {
        if (boss->damageCooldown > 0) {
            boss->damageCooldown -= dt;
            if (boss->damageCooldown < 0) {
                boss->damageCooldown = 0;
            }
        }
    }
}

/*
  Robust vertical collision checking for characters moving between integer rows.
  This prevents tunneling through thin platforms when dy is large: check every
  cell between oldY and newY and snap to just above/below collision tile.

  Parameters:
    y (in/out) - vertical position to update
    dy (in/out) - vertical speed (may be zeroed on collision)
    oldY - previous y before movement
    newY - candidate new y after applying velocity
    x - integer x column to check collisions in
    grounded (in/out) - set to true when collision with ground occurs
    jumps (in/out) - reset jump count on ground contact
    isFalling - true when moving downward, false when moving upward
*/
void checkVerticalCollision(float& y, float& dy, float oldY, float newY, int x, bool& grounded, int& jumps, bool isFalling) {
    int startY = (int)oldY;
    int endY = (int)newY;

    // Check all cells between old and new Y depending on direction.
    if (isFalling) { // moving down
        for (int checkY = startY + 1; checkY <= endY; checkY++) {
            if (isInBounds(x, checkY) && isCollisionTile(arena[checkY][x])) {
                // Snap the entity to the tile above the collision
                y = (float)(checkY - 1);
                dy = 0;
                grounded = true;
                jumps = 0;
                return;
            }
        }
    }
    else { // moving up
        for (int checkY = startY; checkY >= endY; checkY--) {
            if (isInBounds(x, checkY) && isCollisionTile(arena[checkY][x])) {
                // Hit head on collision, snap to just below
                y = (float)(checkY + 1);
                dy = 0;
                return;
            }
        }
    }

    // No collision; accept the newY
    y = newY;
}

// Update vertical physics for a single enemy (gravity, bounds, collisions).
void updateEnemyPhysics(Enemy& enemy, float dt) {
    if (!enemy.active)
        return;

    if (enemy.type == ENEMY_BOSS) return; // Boss is stationary; separate logic

    // Crawler special-case: if touching any collision tile do not apply gravity
    if (enemy.type == ENEMY_CRAWLER) {
        if (isCollisionTile(arena[(int)enemy.y + 1][(int)enemy.x]) ||
            isCollisionTile(arena[(int)enemy.y - 1][(int)enemy.x]) ||
            isCollisionTile(arena[(int)enemy.y][(int)enemy.x + 1]) ||
            isCollisionTile(arena[(int)enemy.y][(int)enemy.x - 1])) {
            enemy.dy = 0;
            return;
        }
        else {
            enemy.dy += GRAVITY * dt;
        }
    }

    // Fliers ignore gravity; others are affected
    if (enemy.type != ENEMY_FLIER) {
        enemy.dy += GRAVITY * dt;
    }

    float oldY = enemy.y;
    float newY = enemy.y + enemy.dy * dt;
    enemy.grounded = false;

    int ex = (int)enemy.x;
    int imaginaryJumps = 0; // unused but required by checkVerticalCollision signature

    if (enemy.dy > 0) {
        checkVerticalCollision(enemy.y, enemy.dy, oldY, newY, ex, enemy.grounded, imaginaryJumps, true);
    }
    else if (enemy.dy < 0) {
        checkVerticalCollision(enemy.y, enemy.dy, oldY, newY, ex, enemy.grounded, imaginaryJumps, false);
    }
    else {
        enemy.y = newY;
    }

    // Floor and ceiling constraints
    if (enemy.y >= ARENA_HEIGHT - 2) {
        enemy.y = (float)(ARENA_HEIGHT - 2);
        enemy.dy = 0;
        enemy.grounded = true;
    }

    if (enemy.y <= 1) {
        enemy.y = 1;
        enemy.dy = 0;
    }
}

// ========== ENEMY AI ==========
/*
  Each enemy type has a simple update routine below:
    - Walker: moves horizontally while on ground, reverses on obstacle or ledge
    - Jumper: moves horizontally and periodically jumps toward the player
    - Flier: flies horizontally and alternates diving and rising phases
    - Crawler: turns 90 degrees when blocked and tries alternate directions
    - Boss: managed separately by updateBossAI()
*/

// Walker: simple ground patrol logic.
void updateWalkerAI(Enemy& enemy, float dt) {
    if (!enemy.grounded) return;

    float newX = enemy.x + enemy.dx * enemy.direction * dt;
    int checkX = (int)newX;
    int checkY = (int)enemy.y;

    if (checkX <= 1 || checkX >= ARENA_WIDTH - 1 || isCollisionTile(arena[checkY][checkX]))
    {
        enemy.direction *= -1; // Reverse direction on obstacle
        return;
    }

    // If there's no ground ahead, turn around to avoid walking off a platform
    int groundCheckY = checkY + 1;

    if (groundCheckY < ARENA_HEIGHT && !isCollisionTile(arena[groundCheckY][checkX])) {
        enemy.direction *= -1; // Reverse direction on ledge
        return;
    }

    enemy.x = newX;
}

// Jumper: moves horizontally and jumps when player is within detection range.
void updateJumperAI(Enemy& enemy, float dt) {
    if (enemy.jumpCooldown > 0) {
        enemy.jumpCooldown -= dt;
        if (enemy.jumpCooldown < 0)
            enemy.jumpCooldown = 0;
    }

    float dx = player.x - enemy.x;
    float dy = player.y - enemy.y;
    float distance = sqrtf(dx * dx + dy * dy);

    float newX = enemy.x + enemy.dx * enemy.direction * dt;
    int checkX = (int)newX;
    int checkY = (int)enemy.y;

    if (checkX <= 1 || checkX >= ARENA_WIDTH - 1 || isCollisionTile(arena[checkY][checkX]))
    {
        enemy.direction *= -1; // Reverse direction on obstacle
    }
    else {
        enemy.x = newX;
    }

    // If on ground and player is near, perform a jump with cooldown afterwards
    if (enemy.grounded && distance <= ENEMY_JUMPER_DETECTION_RANGE && enemy.jumpCooldown <= 0) {
        enemy.dy = ENEMY_JUMPER_JUMP_FORCE;
        enemy.grounded = false;
        enemy.jumpCooldown = ENEMY_JUMPER_JUMP_COOLDOWN;
    }
}

// Flier: alternates diving and rising while moving horizontally.
void updateFlierAI(Enemy& enemy, float dt) {
    enemy.flierTimer -= dt;

    if (enemy.flierTimer <= 0) {
        if (enemy.diving) {
            enemy.diving = false;
            enemy.flierTimer = ENEMY_FLIER_RISE_DURATION;
        }
        else {
            enemy.diving = true;
            enemy.flierTimer = ENEMY_FLIER_DIVE_DURATION;
        }
    }

    float newX = enemy.x + enemy.dx * enemy.direction * dt;
    int checkX = (int)newX;
    int checkY = (int)enemy.y;

    if (checkX <= 1 || checkX >= ARENA_WIDTH - 1 || isCollisionTile(arena[checkY][checkX])) {
        enemy.direction *= -1; // Reverse direction on obstacle
    }
    else {
        enemy.x = newX;
    }

    // Vertical movement differs by diving state
    if (enemy.diving) {
        float newY = enemy.y + ENEMY_FLIER_DIVE_SPEED * dt;
        int nextY = (int)newY;

        if (nextY < ARENA_HEIGHT - 2 && !isCollisionTile(arena[nextY][(int)enemy.x])) {
            enemy.y = newY;
        }
    }
    else {
        float newY = enemy.y - ENEMY_FLIER_RISE_SPEED * dt;
        int nextY = (int)newY;

        if (nextY > ENEMY_FLIER_MIN_HEIGHT && !isCollisionTile(arena[nextY][(int)enemy.x])) {
            enemy.y = newY;
        }

        if (enemy.y < ENEMY_FLIER_MIN_HEIGHT) {
            enemy.y = ENEMY_FLIER_MIN_HEIGHT;
        }
    }
}

// Crawler: attempts to rotate when blocked by obstacle.
void updateCrawlerAI(Enemy& enemy, float dt) {
    float moveDist = enemy.dx * enemy.dx * dt;

    // Determine current integer cell and next cell based on crawlerState
    int checkX = (int)enemy.x;
    int checkY = (int)enemy.y;

    int nextX = checkX;
    int nextY = checkY;

    switch (enemy.crawlerState) {
    case 0: // Moving Right
        nextX += 1;
        break;
    case 1: // Moving Up
        nextY -= 1;
        break;
    case 2: // Moving Left
        nextX -= 1;
        break;
    case 3: // Moving Down
        nextY += 1;
        break;
    }

    // If next position is blocked, rotate clockwise and try alternative
    bool blocked = (nextX <= 0 || nextX >= ARENA_WIDTH - 1 ||
        nextY <= 0 || nextY >= ARENA_HEIGHT - 1 ||
        isCollisionTile(arena[nextY][nextX]));

    if (blocked) {
        // Turn 90 degrees clockwise
        enemy.crawlerState = (enemy.crawlerState + 1) % 4;

        // Check the new direction; if still blocked, try counterclockwise as fallback
        int newNextX = checkX;
        int newNextY = checkY;

        switch (enemy.crawlerState) {
        case 0: newNextX += 1; break;
        case 1: newNextY -= 1; break;
        case 2: newNextX -= 1; break;
        case 3: newNextY += 1; break;
        }

        if (newNextX <= 0 || newNextX >= ARENA_WIDTH - 1 ||
            newNextY <= 0 || newNextY >= ARENA_HEIGHT - 1 ||
            isCollisionTile(arena[newNextY][newNextX])) {
            // Try turning 90 degrees counterclockwise if still blocked
            enemy.crawlerState = (enemy.crawlerState + 2) % 4;
        }
    }
    else {
        // Move in the chosen direction
        switch (enemy.crawlerState) {
        case 0: // Right
            enemy.x += moveDist;
            break;
        case 1: // Up
            enemy.y -= moveDist;
            break;
        case 2: // Left
            enemy.x -= moveDist;
            break;
        case 3: // Down
            enemy.y += moveDist;
            break;
        }
    }
}

// Boss logic: handles selecting attack patterns, warning phase, attack timers and phase progression.
void updateBossAI(float dt) {
    if (boss == nullptr || !boss->active) return;

    // Reduce boss attack cooldown
    if (boss->attackCooldown > 0) {
        boss->attackCooldown -= dt;
    }

    // Warning phase: countdown until actual attack starts
    if (boss->isWarning) {
        boss->attackTimer -= dt;

        if (boss->attackTimer <= 0) {
            // Warning done, start the real attack
            boss->isWarning = false;
            boss->isAttacking = true;

            // Set attack duration based on chosen pattern
            if (boss->currentPattern == BOSS_PATTERN_LIGHTNING) {
                boss->attackTimer = BOSS_LIGHTNING_DURATION;
            }
            else if (boss->currentPattern == BOSS_PATTERN_X_LASER ||
                boss->currentPattern == BOSS_PATTERN_PLUS_LASER) {
                boss->attackTimer = BOSS_LASER_DURATION;
            }
            else {
                boss->attackTimer = BOSS_ATTACK_DURATION;
            }

            // For lightning, spawn strikes at actual attack start
            if (boss->currentPattern == BOSS_PATTERN_LIGHTNING) {
                spawnLightning();
            }
        }
    }

    // When attacking, update attack timer and compute normalized attackPhase used for animations
    if (boss->isAttacking && boss->attackTimer > 0) {
        boss->attackTimer -= dt;

        float totalDuration = BOSS_ATTACK_DURATION;
        if (boss->currentPattern == BOSS_PATTERN_LIGHTNING) {
            totalDuration = BOSS_LIGHTNING_DURATION;
        }
        else if (boss->currentPattern == BOSS_PATTERN_X_LASER ||
            boss->currentPattern == BOSS_PATTERN_PLUS_LASER) {
            totalDuration = BOSS_LASER_DURATION;
        }

        boss->attackPhase = 1.0f - (boss->attackTimer / totalDuration);

        if (boss->attackTimer <= 0) {
            // Attack finished: clear visuals and reset flags
            boss->isAttacking = false;
            boss->attackPhase = 0;
            clearBossAttackVisuals();
        }
    }

    // When cooldown elapsed, pick a new random attack and enter the warning phase
    if (boss->attackCooldown <= 0 && !boss->isAttacking && !boss->isWarning) {
        int attackType = rand() % 5;

        boss->isWarning = true;  // show warning indicator
        boss->attackTimer = BOSS_WARNING_DURATION;
        boss->attackPhase = 0;
        boss->attackCooldown = BOSS_ATTACK_COOLDOWN;

        switch (attackType) {
        case 0: boss->currentPattern = BOSS_PATTERN_X_LASER; break;
        case 1: boss->currentPattern = BOSS_PATTERN_PLUS_LASER; break;
        case 2: boss->currentPattern = BOSS_PATTERN_EARTHQUAKE; break;
        case 3: boss->currentPattern = BOSS_PATTERN_LIGHTNING; break;
        case 4: boss->currentPattern = BOSS_PATTERN_CROSS_SWEEP; break;
        }
    }
}

// Update all active enemies' AI based on type.
void updateEnemyAI(float dt) {
    for (int i = 0; i < enemyCount; i++)
    {
        Enemy& enemy = enemies[i];
        if (!enemy.active)
            continue;

        switch (enemy.type)
        {
        case ENEMY_WALKER:
            updateWalkerAI(enemy, dt);
            break;
        case ENEMY_JUMPER:
            updateJumperAI(enemy, dt);
            break;
        case ENEMY_FLIER:
            updateFlierAI(enemy, dt);
            break;
        case ENEMY_CRAWLER:
            updateCrawlerAI(enemy, dt);
            break;
        case ENEMY_BOSS:
            updateBossAI(dt);
            break;
        default:
            break;
        }
    }
}

// Update overall physics: player gravity, vertical collision and per-enemy physics.
void updatePhysics(float dt) {
    updateAttackTimers(dt);

    // Attack collisions (player attacking enemies) and enemy-player collisions
    checkAttackCollision();
    checkPlayerEnemyCollision();

    // Apply gravity to player
    player.dy += GRAVITY * dt;

    // Store old Y to check collision path
    float oldY = player.y;
    float newY = player.y + player.dy * dt;
    player.grounded = false;

    int px = (int)player.x;

    // Use vertical collision check to avoid tunneling
    if (player.dy > 0) {
        checkVerticalCollision(player.y, player.dy, oldY, newY, px, player.grounded, player.jumps, true);
    }
    else if (player.dy < 0) {
        checkVerticalCollision(player.y, player.dy, oldY, newY, px, player.grounded, player.jumps, false);
    }
    else {
        player.y = newY;
    }

    // Floor collision: snap player to floor and reset jumps when grounded
    if (player.y >= ARENA_HEIGHT - 2) {
        player.y = (float)(ARENA_HEIGHT - 2);
        player.dy = 0;
        player.grounded = true;
        player.jumps = 0;
    }

    // Ceiling collision
    if (player.y <= 1) {
        player.y = 1;
        player.dy = 0;
    }

    // Update enemy vertical physics
    for (int i = 0; i < enemyCount; i++) {
        updateEnemyPhysics(enemies[i], dt);
    }

    updateEnemyAI(dt);
    checkBossAttackCollision();
}

// ========== RENDERING ==========

// Render boss as 3x3 block; flash color when in damage cooldown.
void renderBoss() {
    if (boss == nullptr) return;

    if (boss->damageCooldown > 0) {
        // Flash red/dark-red when invulnerable
        setColor((int)boss->damageCooldown % 10 < 5 ? COLOR_RED : COLOR_DARK_RED);
    }
    else {
        setColor(COLOR_RED);
    }

    int bx = (int)boss->x;
    int by = (int)boss->y;
    for (int y = 0; y < BOSS_SIZE; y++) {
        for (int x = 0; x < BOSS_SIZE; x++) {
            gotoXY(bx + x, by + y);
            cout << ENEMY_BOSS_CHAR;
        }
    }
}

// Draw a small warning indicator above the boss during the warning phase.
void drawWarning() {
    int centerX = BOSS_X + 1;
    int centerY = BOSS_Y + 1;

    setColor(COLOR_YELLOW);
    gotoXY(centerX, centerY - 2);

    switch (boss->currentPattern) {
    case 1: cout << "X"; break;
    case 2: cout << "+"; break;
    case 3: cout << "^"; break;
    case 4: cout << "*"; break;
    case 5: cout << "|"; break;
    default: break;
    }

    setColor(COLOR_WHITE);
}

// Render all active enemies. For boss, additionally render warnings and attacks.
void renderEnemies() {
    for (int i = 0; i < enemyCount; i++) {
        Enemy& enemy = enemies[i];
        if (!enemy.active) continue;

        // Erase previous position
        gotoXY(enemy.lastX, enemy.lastY);
        cout << ' ';

        enemy.lastX = (int)enemy.x;
        enemy.lastY = (int)enemy.y;

        gotoXY(enemy.lastX, enemy.lastY);

        switch (enemy.type) {
        case ENEMY_WALKER:
            setColor(COLOR_GREEN);
            cout << ENEMY_WALKER_CHAR;
            break;
        case ENEMY_JUMPER:
            setColor(COLOR_YELLOW);
            cout << ENEMY_JUMPER_CHAR;
            break;
        case ENEMY_FLIER:
            setColor(COLOR_CYAN);
            cout << ENEMY_FLIER_CHAR;
            break;
        case ENEMY_CRAWLER:
            setColor(COLOR_MAGENTA);
            cout << ENEMY_CRAWLER_CHAR;
            break;
        case ENEMY_BOSS:
            renderBoss();

            // Show warning indicator when appropriate
            if (boss != nullptr && boss->active && boss->isWarning) {
                drawWarning();
            }

            // Draw attack visuals during active attack
            if (boss != nullptr && boss->active && boss->isAttacking) {
                switch (boss->currentPattern) {
                case BOSS_PATTERN_X_LASER:
                    drawXLaser();
                    break;
                case BOSS_PATTERN_PLUS_LASER:
                    drawPlusLaser();
                    break;
                case BOSS_PATTERN_EARTHQUAKE:
                    drawEarthquake();
                    break;
                case BOSS_PATTERN_LIGHTNING:
                    drawLightning();
                    break;
                case BOSS_PATTERN_CROSS_SWEEP:
                    drawCrossSweep(boss->attackPhase);
                    break;
                }
            }
            break;
        default:
            cout << 'X';
            break;
        }
    }
    setColor(COLOR_WHITE);
}

// Restore background characters that were overwritten by the player's attack visuals.
void clearAttack() {
    for (int i = 0; i < player.currentAttack.numSavedChars; i++) {
        int x = player.currentAttack.savedPositions[i][0];
        int y = player.currentAttack.savedPositions[i][1];

        gotoXY(x, y);

        // Draw the stored character in arena color
        setColor(COLOR_DARK_GRAY);
        cout << player.currentAttack.savedChars[i];
    }
    player.currentAttack.numSavedChars = 0; // reset stored count
    setColor(COLOR_WHITE);
}

// Save background characters from the arena and draw the attack string at (x,y).
// The saved characters are used to restore the arena when attack ends.
void saveAndDrawAttack(int x, int y, const char* str, int len) {
    for (int i = 0; i < len; i++) {
        if (x + i >= 0 && x + i < ARENA_WIDTH && y >= 0 && y < ARENA_HEIGHT) {
            int idx = player.currentAttack.numSavedChars;
            if (idx < 3) { // bound check to avoid overwrite
                player.currentAttack.savedChars[idx] = arena[y][x + i];
                player.currentAttack.savedPositions[idx][0] = x + i;
                player.currentAttack.savedPositions[idx][1] = y;
                player.currentAttack.numSavedChars++;
            }
        }
    }

    // Draw the attack string 
    gotoXY(x, y);
    setColor(COLOR_YELLOW);
    cout << str;
    setColor(COLOR_WHITE);
}

// Handle rendering and lifecycle of the player's attack visuals: clear old visuals,
// save background under new visuals and draw the new visuals.
void renderAttack() {
    // If a previous attack direction existed, clear its visuals if player moved or attack ended.
    if (player.currentAttack.lastDirection != ATTACK_NONE) {
        // clear if player moved OR if attack is no longer active
        bool playerMoved = (player.currentAttack.lastX != (int)player.x ||
            player.currentAttack.lastY != (int)player.y);
        if (!player.currentAttack.active || playerMoved)
        {
            clearAttack(); // restore background
            player.currentAttack.lastDirection = ATTACK_NONE;
        }
    }

    if (!player.currentAttack.active) return;

    int px = (int)player.x;
    int py = (int)player.y;

    // Save current position and direction for next frame
    player.currentAttack.lastX = px;
    player.currentAttack.lastY = py;
    player.currentAttack.lastDirection = player.currentAttack.direction;

    // Reset saved chars before saving new ones
    player.currentAttack.numSavedChars = 0;

    switch (player.currentAttack.direction) {
    case ATTACK_UP:
        saveAndDrawAttack(px - 1, py - 1, "/-\\", 3);
        break;
    case ATTACK_DOWN:
        saveAndDrawAttack(px - 1, py + 1, "\\_/", 3);
        break;
    case ATTACK_LEFT:
        saveAndDrawAttack(px - 1, py - 1, "/", 1);
        saveAndDrawAttack(px - 1, py, "|", 1);
        saveAndDrawAttack(px - 1, py + 1, "\\", 1);
        break;
    case ATTACK_RIGHT:
        saveAndDrawAttack(px + 1, py - 1, "\\", 1);
        saveAndDrawAttack(px + 1, py, "|", 1);
        saveAndDrawAttack(px + 1, py + 1, "/", 1);
        break;
    default:
        break;
    }
}

// Main render: HUD, enemies, player and attack visuals.
void render() {
    // Draw HUD on top-left
    gotoXY(0, 0);
    setColor(COLOR_DARK_GRAY);
    cout << "##";

    // Player HP display
    setColor(COLOR_RED);
    cout << " HP: ";
    for (int i = 0; i < player.hp; i++) cout << "0-";

    // Boss HP display (if boss exists)
    int bossBarLength = 0;
    if (boss != nullptr && boss->active) {
        const char* bossLabel = " | BOSS: ";
        setColor(COLOR_YELLOW);
        cout << bossLabel;
        setColor(COLOR_RED);
        for (int h = 0; h < boss->hp; h++) {
            cout << "0-";
        }
        // Estimate length used by boss bar for filling top border
        bossBarLength = 9 + (boss->hp * 2);
    }

    // Controls hint
    setColor(COLOR_DARK_CYAN);
    const char* controls = " (a/d move, w jump, i/j/k/l attack) ";
    cout << controls;

    // Fill remaining top border with wall characters
    setColor(COLOR_DARK_GRAY);
    int usedSpace = 2 + 5 + (player.hp * 2) + bossBarLength + getLength(controls);
    int remaining = ARENA_WIDTH - usedSpace;

    for (int i = 0; i < remaining; i++) {
        if (i >= 0) cout << WALL_CHAR;
    }

    setColor(COLOR_WHITE);

    // Render enemies (and boss visuals)
    renderEnemies();

    // Draw player: erase last position, draw new one (with flash when damaged)
    setColor(COLOR_WHITE);
    gotoXY(player.lastX, player.lastY);
    cout << ' ';

    player.lastX = (int)player.x;
    player.lastY = (int)player.y;
    gotoXY(player.lastX, player.lastY);

    if (player.damageCooldown > 0) {
        int flashCycle = (int)(player.damageCooldown / 5) % 2;
        if (flashCycle == 0) {
            setColor(COLOR_DARK_RED);
            cout << PLAYER_CHAR;
        }
    }
    else {
        setColor(COLOR_WHITE);
        cout << PLAYER_CHAR;
    }

    setColor(COLOR_WHITE);
    // Render any active attack visuals after drawing the player
    renderAttack();
}

// ========== MAIN LOOP & END SCREEN ==========.
void showEndScreen(bool win) {
    system("cls"); // Clear the console

    // Vertical spacing
    for (int i = 0; i < 4; i++) cout << endl;

    if (win) {
        setColor(COLOR_GREEN);
        cout << "  __     ______  _    _  __          _______ _   _  " << endl;
        cout << "  \\ \\   / / __ \\| |  | | \\ \\        / /_   _| \\ | | " << endl;
        cout << "   \\ \\_/ / |  | | |  | |  \\ \\  /\\  / /  | | |  \\| | " << endl;
        cout << "    \\   /| |  | | |  | |   \\ \\/  \\/ /   | | | . ` | " << endl;
        cout << "     | | | |__| | |__| |    \\  /\\  /   _| |_| |\\  | " << endl;
        cout << "     |_|  \\____/ \\____/      \\/  \\/   |_____|_| \\_| " << endl;
        cout << endl;
    }
    else {
        setColor(COLOR_RED);
        cout << "   _____          __  __ ______    ______      ________ _____  " << endl;
        cout << "  / ____|   /\\   |  \\/  |  ____|  / __ \\ \\    / /  ____|  __ \\ " << endl;
        cout << " | |  __   /  \\  | \\  / | |__    | |  | \\ \\  / /| |__  | |__) |" << endl;
        cout << " | | |_ | / /\\ \\ | |\\/| |  __|   | |  | |\\ \\/ / |  __| |  _  / " << endl;
        cout << " | |__| |/ ____ \\| |  | | |____  | |__| | \\  /  | |____| | \\ \\ " << endl;
        cout << "  \\_____/_/    \\_\\_|  |_|______|  \\____/   \\/   |______|_|  \\_\\" << endl;
        cout << endl;
    }

    cout << endl << endl;
    setColor(COLOR_GRAY);
    cout << "           Final Wave Reached: " << currentWave << endl;
    cout << "           Press any key to exit the game..." << endl;

    _getch(); // Wait for key press
}

// Update wave progression: when all enemies are defeated advance the wave.
// Wave 5 triggers the boss; after that, defeating the boss ends the game.
void updateWaveSystem() {
    if (gameOver) return;

    bool enemiesRemaining = false;
    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].active) {
            enemiesRemaining = true;
            break;
        }
    }

    // If no enemies left, advance wave or end if beyond boss fight
    if (!enemiesRemaining) {
        currentWave++;
        if (currentWave <= 5) {
            spawnWave(currentWave);
        }
        else {
            gameOver = true;
        }
    }
}

int main()
{
    initGame();

    // Main game loop: run until player HP depleted or game ends by beating the boss.
    while (player.hp > 0 && !gameOver)
    {
        clock_t currentTime = clock();
        // Calculate delta time and scale to ~60 FPS based units used by the game
        float dt = float(currentTime - lastTime) / CLOCKS_PER_SEC * 60.0f;
        lastTime = currentTime;

        handleInput(dt);
        updatePhysics(dt);
        updateWaveSystem();
        render();

        Sleep(16); // small sleep to roughly cap framerate
    }

    showEndScreen(player.hp > 0);

    delete[] enemies;
    return 0;
}
