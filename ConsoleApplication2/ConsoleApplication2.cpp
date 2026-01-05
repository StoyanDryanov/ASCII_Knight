#include <iostream>
#include <windows.h>
#include <ctime>
#include <conio.h>
#include <cstdlib>

using namespace std;

// CONSTANTS
const int ARENA_WIDTH = 80;
const int ARENA_HEIGHT = 25;
const char WALL_CHAR = '#';

void gotoXY(int x, int y)
{
    COORD coord = {x,y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}



int main()
{
    for (size_t y = 0; y < ARENA_HEIGHT; y++)
    {
        for (size_t x = 0; x < ARENA_WIDTH; x++)
        {
            if (y == 0 || y == ARENA_HEIGHT - 1 || x == 0 || x == ARENA_WIDTH - 1)
            {
				gotoXY(x, y);
				cout << WALL_CHAR;
            }
        }
    }

    return 0;
}