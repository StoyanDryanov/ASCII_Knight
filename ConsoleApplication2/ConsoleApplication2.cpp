#include <iostream>
#include <windows.h>
#include <ctime>
#include <conio.h>
#include <cstdlib>

using namespace std;

// CONSTANTS
const int ARENA_WIDTH = 100;
const int ARENA_HEIGHT = 30;
const char WALL_CHAR = '#';
const char PLAYER_CHAR = '@';
const float GRAVITY = 0.1f;
const float JUMP_FORCE = -1.5f;

// GLOBAL VARIABLES
struct Player {
    float x, y;
    float dx, dy;
    int lastX, lastY;
	bool isGrounded;
	int jumpCount; // tracks how many jumps have been made
};

Player player = { ARENA_WIDTH / 2.0f, ARENA_HEIGHT / 2.0f, 0, 0, (int)(ARENA_WIDTH / 2), (int)(ARENA_HEIGHT / 2), false, 0 };

char arena[ARENA_HEIGHT][ARENA_WIDTH];

void gotoXY(int x, int y)
{
    COORD coord = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void draw(float x, float y, char c) {
    gotoXY((int)x, (int)y);

    cout << c;
    
    gotoXY(0, ARENA_HEIGHT + 1);// Move cursor out of the way
}

void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

void initArena()
{
    for (int y = 0; y < ARENA_HEIGHT; y++)
    {
        for (int x = 0; x < ARENA_WIDTH; x++)
        {
            if (y == 0 || y == ARENA_HEIGHT - 1 || x == 0 || x == ARENA_WIDTH - 1)
            {
				arena[y][x] = WALL_CHAR;
            }
            else
            {
				arena[y][x] = ' ';
            }
        }
    }
}

void drawArena()
{
    for (int y = 0; y < ARENA_HEIGHT; y++)
    {
        for (int x = 0; x < ARENA_WIDTH; x++)
        {
            if (arena[y][x] == WALL_CHAR)
            {
				draw(x, y, WALL_CHAR);
            }
        }
    }
    draw(player.x, player.y, PLAYER_CHAR);
}

void handleInput() {
	player.dx = 0;

    if(_kbhit()) {
		char ch = _getch();
        GetAsyncKeyState('A');

		if (ch == 'a') player.dx = -1.0;
		if (ch == 'd') player.dx = 1.0;

        if (ch == 'w') {
            if (player.isGrounded) {
                player.dy = JUMP_FORCE;
                player.isGrounded = false;
                player.jumpCount = 1;
            }
            else if (player.jumpCount < 2) {
                player.dy = JUMP_FORCE;
                player.jumpCount++;
            }
        }
	}
}

void updatePhysics() {
    player.dy += GRAVITY;

    float nextY = player.y + player.dy;

	// Boundary to prevent going out of arena
    if (nextY < 0) nextY = 0;
    if (nextY >= ARENA_HEIGHT) nextY = (float)ARENA_HEIGHT - 1;

    // Collision with walls
    if (arena[(int)nextY][(int)player.x] == WALL_CHAR) {
		if (player.dy > 0) // hitting the ground
        {
            player.isGrounded = true;
            player.y = (float)((int)nextY - 1);
            player.dy = 0;
			player.jumpCount = 0; // reset jump count upon landing
        }
		else if (player.dy < 0) // hitting the ceiling
        {
            player.y = (float)((int)nextY - 1);
            player.dy = 0;
        }
    }
    else {
		player.y = nextY;
    }

    float nextX = player.x + player.dx;

	// Boundary to prevent going out of arena
    if (nextX < 0) nextX = 0;
    if (nextX >= ARENA_WIDTH) nextX = (float)ARENA_WIDTH - 1;

    if (arena[(int)player.y][(int)nextX] == WALL_CHAR) {
        player.dx = 0;
    }
    else {
        player.x = nextX;
    }

    if (arena[(int)(player.y + 0.1f)][(int)player.x] == WALL_CHAR) {
        player.isGrounded = true;
        player.dy = 0;
    }
}

int main()
{
	initArena();
    drawArena();
	hideCursor();
    while (true)
    {
        draw((float)player.lastX, (float)player.lastY, ' ');

		handleInput();
        updatePhysics();

        player.lastX = (int)player.x;
        player.lastY = (int)player.y;

        draw(player.x,player.y, PLAYER_CHAR);

		Sleep(33);
    }

    return 0;
}