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

// ========== GLOBAL VARIABLES ==========
struct Player {
    float x, y;
    float dy;
    int hp;
    int jumps;
    bool grounded;
    int lastX, lastY;
};

Player player = { 
    ARENA_WIDTH / 2.0f,
    ARENA_HEIGHT / 2.0f,
    0,
    5,
    false,
    (int)(ARENA_WIDTH / 2),
    (int)(ARENA_HEIGHT / 2),
    };

clock_t lastTime;

char arena[ARENA_HEIGHT][ARENA_WIDTH];

void gotoXY(int x, int y){
    COORD coord = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info = { 100, FALSE };
    SetConsoleCursorInfo(consoleHandle, &info);
}

void initGame(){
	hideCursor();
    srand((unsigned int)time(NULL)); // Seed RNG for different platforms each run

    for (int y = 0; y < ARENA_HEIGHT; y++) {
        for (int x = 0; x < ARENA_WIDTH; x++) {
            if (y == 0 || y == ARENA_HEIGHT - 1 || x == 0 || x == ARENA_WIDTH - 1)
                arena[y][x] = WALL_CHAR;
            else
                arena[y][x] = ' ';
        }
    }

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

    for (int y = 0; y < ARENA_HEIGHT; y++) {
        for (int x = 0; x < ARENA_WIDTH; x++) {
            cout << arena[y][x];
        }
        cout << endl;
    }

    player.lastX = (int)player.x;
    player.lastY = (int)player.y;
    lastTime = clock();
}

void handleInput(float dt) {
    if (_kbhit()) {
        char key = _getch();

        if (key == 'a' && player.x > 1) player.x -= PLAYER_SPEED * dt;
        if (key == 'd' && player.x < ARENA_WIDTH - 2) player.x += PLAYER_SPEED * dt;

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
    }
}

void updatePhysics(float dt) {
    // Apply gravity
    player.dy += GRAVITY * dt;

    // Store old position BEFORE any updates
    float oldY = player.y;

    // Apply vertical velocity
    float newY = player.y + player.dy * dt;

    player.grounded = false;

    int px = (int)player.x;

    // ===== CHECK ALL CELLS BETWEEN OLD AND NEW POSITION =====

    if (player.dy > 0) {  // Falling down
        int startY = (int)oldY;
        int endY = (int)newY;

        bool collided = false;

        // Check each cell we're passing through while falling
        for (int checkY = startY + 1; checkY <= endY; checkY++) {
            if (checkY > 0 && checkY < ARENA_HEIGHT &&
                px > 0 && px < ARENA_WIDTH - 1) {

                if (arena[checkY][px] == PLATFORM_CHAR || arena[checkY][px] == WALL_CHAR) {
                    // Land on top of the platform
                    player.y = (float)(checkY - 1);
                    player.dy = 0;
                    player.grounded = true;
                    player.jumps = 0;
                    collided = true;
                    break;
                }
            }
        }

        if (!collided) {
            player.y = newY;
        }
    }
    else if (player.dy < 0) {  // Jumping up
        int startY = (int)oldY;
        int endY = (int)newY;

        bool collided = false;

		// Check each cell we're passing through while jumping
        for (int checkY = startY; checkY >= endY; checkY--) {
            if (checkY >= 0 && checkY < ARENA_HEIGHT &&
                px > 0 && px < ARENA_WIDTH - 1) {

                if (arena[checkY][px] == PLATFORM_CHAR || arena[checkY][px] == WALL_CHAR) {
                    // Stop below the platform
                    player.y = (float)(checkY + 1);
                    player.dy = 0;
                    collided = true;
                    break;
                }
            }
        }

        if (!collided) {
            player.y = newY;
        }
    }
    else {  // Not moving vertically
        player.y = newY;
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


void render() {
    gotoXY(0, 0);
    string hpStr = " HP: ";
    for (int i = 0; i < player.hp; i++) hpStr += "0-";

    string controls = " (a/d move, w jump, i/j/k/l attack) ";
    string topBorder = "##" + hpStr + controls;

    while (topBorder.length() < ARENA_WIDTH) {
        topBorder += WALL_CHAR;
    }
    cout << topBorder;

    gotoXY(player.lastX, player.lastY);
    cout << ' ';

    player.lastX = (int)player.x;
    player.lastY = (int)player.y;
    gotoXY(player.lastX, player.lastY);
    cout << PLAYER_CHAR;
}

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