#include <iostream>
#include <windows.h>
#include <ctime>
#include <conio.h>
#include <cstdlib>

using namespace std;

// CONSTANTS
const int ARENA_WIDTH = 60;
const int ARENA_HEIGHT = 20;
const char WALL_CHAR = '#';
const char PLAYER_CHAR = '@';
const float GRAVITY = 0.1f;

// GLOBAL VARIABLES
struct Player {
    float x, y;
    float dx, dy;
    int lastX, lastY;
};

Player player = { ARENA_WIDTH / 2.0f, ARENA_HEIGHT / 2.0f, 0, 0, (int)(ARENA_WIDTH / 2), (int)(ARENA_HEIGHT / 2) };

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
    if(_kbhit()) {
		char ch = _getch();

		if (ch == 'a') player.dx = -1.0f;
		if (ch == 'd') player.dx = 1.0f;

		if (ch == 'w') player.dy = -1.5f; // Infinite jumps
	}
	else {
		player.dx *= 0.5f;
    }
}

void updatePhysics() {
    player.dy += GRAVITY;

    float nextX = player.x + player.dx;
    float nextY = player.y + player.dy;

    // Collision with walls
    if (arena[(int)nextY][(int)nextX] == WALL_CHAR) {
        player.dy = 0;
    }
    else {
		player.y = nextY;
    }

    if(arena[(int)player.y][(int)player.x] == WALL_CHAR) {
		player.dx = 0;
    } else {
		player.x = nextX;
    }
}

int main()
{
	initArena();
    drawArena();

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