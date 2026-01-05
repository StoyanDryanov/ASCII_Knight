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

void drawArena()
{
    for (int y = 0; y < ARENA_HEIGHT; y++)
    {
        for (int x = 0; x < ARENA_WIDTH; x++)
        {
            if (y == 0 || y == ARENA_HEIGHT - 1 || x == 0 || x == ARENA_WIDTH - 1)
            {
                draw(x, y, WALL_CHAR);
            }
        }
    }
    draw(player.x, player.y, PLAYER_CHAR);
}

void movePlayer(char direction) {
    draw(player.x, player.y, ' ');

    switch (direction) {
    case 'w':
        if (player.y > 1) player.y--;
        break;
    case 's':
        if (player.y < ARENA_HEIGHT - 2) player.y++;
        break;
    case 'a':
        if (player.x > 1) player.x--;
        break;
    case 'd':
        if (player.x < ARENA_WIDTH - 2) player.x++;
        break;
    }

    draw(player.x, player.y, PLAYER_CHAR);
}

int main()
{
    drawArena();

    while (true)
    {
        if (_kbhit()) {
            movePlayer(_getch());
        }
    }

    return 0;
}