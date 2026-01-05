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

struct Player {
    int x;
    int y;
};

void gotoXY(int x, int y)
{
    COORD coord = {x,y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void draw(int x, int y, char c) {
    gotoXY(x, y);
	cout << c;
	gotoXY(0, ARENA_HEIGHT); // Move cursor out of the way
}


int main()
{
    for (size_t y = 0; y < ARENA_HEIGHT; y++)
    {
        for (size_t x = 0; x < ARENA_WIDTH; x++)
        {
            if (y == 0 || y == ARENA_HEIGHT - 1 || x == 0 || x == ARENA_WIDTH - 1)
            {
				draw(x, y, WALL_CHAR);
            }
        }
    }

	Player player = { ARENA_WIDTH / 2, ARENA_HEIGHT / 2 };

	draw(player.x, player.y, PLAYER_CHAR);

    return 0;
}