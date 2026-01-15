#include <iostream>
#include <windows.h>
#include <ctime>
#include <conio.h>
#include <cstdlib>

using namespace std;

// ========== CONSTANTS ==========
const int ARENA_WIDTH = 80;
const int ARENA_HEIGHT = 20;
const char WALL_CHAR = '#';
const char PLAYER_CHAR = '@';
const char PLATFORM_CHAR = '=';
const float PLAYER_SPEED = 0.5f;
const float GRAVITY = 0.05f;
const float JUMP_FORCE = -0.8f;
const int MAX_JUMPS = 2;

const float ATTACK_COOLDOWN = 0.5f;
const float ATTACK_DURATION = 0.3f;

// ========== STRUCTS ==========
enum AttackDirection {
	ATTACK_NONE = 0,
	ATTACK_UP,
	ATTACK_DOWN,
	ATTACK_LEFT,
	ATTACK_RIGHT
};

struct Attack {
	AttackDirection direction;
	float timer; // countdown timer until ATTACK_DURATION reaches 0
    bool active;
};

struct Player {
    float x, y;
    float dy;
    int hp;
    int jumps;
    bool grounded;
    int lastX, lastY;
	float attackCooldown;
	Attack currentAttack;
};



// ========== GLOBAL VARIABLES ==========
Player player;
clock_t lastTime;
char arena[ARENA_HEIGHT][ARENA_WIDTH];

// ========== FUTILITY FUNCTIONS ==========
void gotoXY(int x, int y){
    COORD coord = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info = { 100, FALSE };
    SetConsoleCursorInfo(consoleHandle, &info);
}

bool isCollisionTile(char tile) {
    return tile == PLATFORM_CHAR || tile == WALL_CHAR;
}

bool isInBounds(int x, int y) {
    return x > 0 && x < ARENA_WIDTH - 1 && y > 0 && y < ARENA_HEIGHT;
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
        pY += (rand() % 3 - 1); // Add a small random height offset
        for (int x = 0; x < pWidth; x++) {
            if (pX + x < ARENA_WIDTH - 1)
                arena[pY][pX + x] = PLATFORM_CHAR;
        }
    }
}

void drawArena() {
    for (int y = 0; y < ARENA_HEIGHT; y++) {
        for (int x = 0; x < ARENA_WIDTH; x++) {
            cout << arena[y][x];
        }
        cout << endl;
    }
}

void initPlayer(){
    player.x = ARENA_WIDTH / 2.0f;
    player.y = ARENA_HEIGHT / 2.0f;
    player.dy = 0;
    player.hp = 5;
    player.jumps = 0;
    player.grounded = false;
    player.lastX = (int)player.x;
    player.lastY = (int)player.y;
	player.attackCooldown = 0;
	player.currentAttack.active = false;
	player.currentAttack.direction = ATTACK_NONE;
	player.currentAttack.timer = 0;
}

void initGame(){
	hideCursor();
    srand((unsigned int)time(NULL)); // Seed RNG for different platforms each run

	initArena();
	generatePlatforms();
	drawArena();
	initPlayer();

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

    if (key == 'a' && player.x > 1) 
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
    

	handleAttackInput(key);// Handle attack input
}

// ========== PHYSICS ==========
void updateAttackTimers(float dt) {
	// Decrease attack cooldown timer
    if (player.attackCooldown > 0) {
		player.attackCooldown -= dt;

        if (player.attackCooldown < 0) {
			player.attackCooldown = 0;
        }
    }

	// Decrease current attack display timer
	if (player.currentAttack.active) {
        player.currentAttack.timer -= dt;
        if (player.currentAttack.timer <= 0) {
            player.currentAttack.active = false;
            player.currentAttack.direction = ATTACK_NONE;
            player.currentAttack.timer = 0;
		}
    }
}

void checkVerticalCollision(float oldY, float newY, int px, bool isFalling) {
    int startY = (int)oldY;
    int endY = (int)newY;

    // ===== CHECK ALL CELLS BETWEEN OLD AND NEW POSITION =====
    if (isFalling) {// Falling down
        // Check each cell we're passing through while falling
        for (int checkY = startY + 1; checkY <= endY; checkY++) {
            if (isInBounds(px, checkY) && isCollisionTile(arena[checkY][px])) {
                player.y = (float)(checkY - 1);
                player.dy = 0;
                player.grounded = true;
                player.jumps = 0;
                return;
            }
        }
    }
	else {// Jumping up
		// Check each cell we're passing through while jumping
        for (int checkY = startY; checkY >= endY; checkY--) {
            if (isInBounds(px, checkY) && isCollisionTile(arena[checkY][px])) {
                player.y = (float)(checkY + 1);
                player.dy = 0;
                return;
            }
        }
    }

    // No collision, apply new position
    player.y = newY;
}

void updatePhysics(float dt) {
	updateAttackTimers(dt);

    // Apply gravity
    player.dy += GRAVITY * dt;

    // Store old position BEFORE any updates
    float oldY = player.y;

    // Apply vertical velocity
    float newY = player.y + player.dy * dt;

    player.grounded = false;

    int px = (int)player.x;


    
    if (player.dy > 0){
		checkVerticalCollision(oldY, newY, px, true);
    } else if (player.dy < 0) {
		checkVerticalCollision(oldY, newY, px, false);
    } else {
		player.y = newY;;
    }

    // ===== Floor collision =====
    if (player.y >= ARENA_HEIGHT - 2) {
        player.y = (float)(ARENA_HEIGHT - 2);
        player.dy = 0;
        player.grounded = true;
        player.jumps = 0;
    }

    // ===== Ceiling collision =====
    if (player.y <= 1) {
        player.y = 1;
        player.dy = 0;
    }
}

// ========== RENDERING ==========
void render() {
	// ===== Draw HUD =====
    gotoXY(0, 0);
    string hpStr = " HP: ";
    for (int i = 0; i < player.hp; i++) hpStr += "0-";

    string controls = " (a/d move, w jump, i/j/k/l attack) ";
    string topBorder = "##" + hpStr + controls;

    while (topBorder.length() < ARENA_WIDTH) {
        topBorder += WALL_CHAR;
    }
    cout << topBorder;

	// ===== Draw Player =====
	// Erase last position
    gotoXY(player.lastX, player.lastY);
    cout << ' ';

    player.lastX = (int)player.x;
    player.lastY = (int)player.y;
    gotoXY(player.lastX, player.lastY);
    cout << PLAYER_CHAR;
}

// ========== MAIN LOOP ==========
int main()
{
	initGame();

    while (player.hp > 0)
    {
        clock_t currentTime = clock();
        float dt = float(currentTime - lastTime) / CLOCKS_PER_SEC * 60.0f;
		lastTime = currentTime;

		handleInput(dt);
        updatePhysics(dt);
		render();

		Sleep(16);
    }

    return 0;
}