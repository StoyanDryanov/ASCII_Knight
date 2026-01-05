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
    int x;
    int y;
};

Player player = { ARENA_WIDTH / 2, ARENA_HEIGHT / 2 };

char arena[ARENA_HEIGHT][ARENA_WIDTH];

void gotoXY(int x, int y)
{
    COORD coord = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void draw(int x, int y, char c) {
    gotoXY(x, y);

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
	int nextX = player.x;
	int nextY = player.y;

    switch(direction) {
	    case 'w': nextY--; break;
	    case 's': nextY++; break;
	    case 'a': nextX--; break;
	    case 'd': nextX++; break;
    }

    if (arena[nextY][nextX] != WALL_CHAR)
    {
		draw(player.x, player.y, ' '); // Erase old position

		player.x = nextX;
		player.y = nextY;

		draw(player.x, player.y, PLAYER_CHAR);
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