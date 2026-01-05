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

// GLOBAL VARIABLES
struct Player {
    float x;
    float y;
    float dx;
	float dy;
};

Player player = { ARENA_WIDTH / 2.0f, ARENA_HEIGHT / 2.0f, 0, 0};

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

void movePlayer(char direction) {
    player.dx = 0;
	player.dy = 0;

    switch(direction) {
	    case 'w': player.dy = -1.0f; break;
	    case 'a': player.dx = -1.0f; break;
	    case 'd': player.dx = 1.0f; break;
    }
}

int main()
{
	initArena();
    drawArena();

    while (true)
    {
        if (_kbhit()) {
            movePlayer(_getch());
        }
        
    }

    return 0;
}