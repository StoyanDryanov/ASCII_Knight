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
const float GRAVITY = 0.05f;
const float JUMP_FORCE = -1.2f;
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
    COORD coord = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info = { 100, FALSE };
    SetConsoleCursorInfo(consoleHandle, &info);
}

void initGame(){

	hideCursor();

    for (int y = 0; y < ARENA_HEIGHT; y++) {
        for (int x = 0; x < ARENA_WIDTH; x++) {
            if (y == 0 || y == ARENA_HEIGHT - 1 || x == 0 || x == ARENA_WIDTH - 1)
                cout << WALL_CHAR;
            else 
                cout << ' ';
        }
		cout << endl;
    }

    lastTime = clock();
}

void handleInput(float dt) {
    if (_kbhit()) {
        char key = _getch();

        if (key == 'a' && player.x > 1) player.x -= 1.0f * dt;
        if (key == 'd' && player.x < ARENA_WIDTH - 2) player.x += 1.0 * dt;

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
    player.dy += GRAVITY * dt;
    player.y += player.dy * dt;

    // Floor collision
    if (player.y >= ARENA_HEIGHT - 2) {
        player.y = (float)ARENA_HEIGHT - 2;
        player.dy = 0;
        player.grounded = true;
        player.jumps = 0;
    }
    // Ceiling collision
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