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

const float ATTACK_COOLDOWN = 30.0f;
const float ATTACK_DURATION = 15.0f;

const float DAMAGE_COOLDOWN = 60.0f;

const char ENEMY_WALKER_CHAR = 'E';
const float ENEMY_WALKER_SPEED = 0.2f;

// ========== STRUCTS ==========
enum EnemyType {
    ENEMY_WALKER,
    ENEMY_JUMPER,
    ENEMY_FLIER,
    ENEMY_CRAWLER,
    ENEMY_BOSS
};

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
	int lastX, lastY; // track last position to clear
	AttackDirection lastDirection; // track last direction to clear
	char savedChars[3]; // characters that was overwritten by the attack display
	int savedPositions[3][2]; // positions of the overwritten characters
	int numSavedChars; // number of characters saved
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
	float damageCooldown;
};

struct Enemy {
    EnemyType type;
    float x, y;
    float dx, dy;
    bool grounded;
    bool active;
    int lastX, lastY;
	int direction; // 1 = right, -1 = left
};

// ========== GLOBAL VARIABLES ==========
Player player;
clock_t lastTime;
char arena[ARENA_HEIGHT][ARENA_WIDTH];

Enemy* enemies = nullptr;
int maxEnemies = 0;
int enemyCount = 0;

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

// ========== ENEMY MANAGMENT ==========
void spawnEnemy(EnemyType type, float x, float y) {
    if (enemyCount >= maxEnemies) {
        int newMax = maxEnemies * 2;
        Enemy* newEnemies = new Enemy[newMax];

        for (int i = 0; i < enemyCount; i++) {
            newEnemies[i] = enemies[i];
        }

        delete[] enemies;
        enemies = newEnemies;
        maxEnemies = newMax;
    }

    Enemy& enemy = enemies[enemyCount];
	enemy.type = type;
    enemy.x = x;
    enemy.y = y;
    enemy.dy = 0;
    enemy.grounded = false;
    enemy.active = true;
    enemy.direction = 1;
    enemy.lastX = (int)x;
    enemy.lastY = (int)y;

    switch (type) {
    case ENEMY_WALKER:
        enemy.dx = ENEMY_WALKER_SPEED;
        enemy.direction = 1;  // Start moving right
        break;
    case ENEMY_JUMPER:
        enemy.dx = 0;
        enemy.direction = 1;
        break;
    case ENEMY_FLIER:
        enemy.dx = 0;
        enemy.direction = 1;
        break;
    case ENEMY_CRAWLER:
        enemy.dx = 0;
        enemy.direction = 1;
        break;
    case ENEMY_BOSS:
        enemy.dx = 0;
        enemy.direction = 1;
        break;
    }

    enemyCount++;
}

void spawnWave(int waveNumber) {
    //spawn walkers
    int numWalkers = 2 + waveNumber;

    for (int i = 0; i < numWalkers; i++) {
        float x, y;
        int attempts = 0;
        bool validSpot = false;

        // Try to find a safe spawn position
        while (!validSpot && attempts < 100) {
            x = (float)(rand() % (ARENA_WIDTH - 4) + 2);
            y = (float)(rand() % (ARENA_HEIGHT - 4) + 2);

            int ix = (int)x;
            int iy = (int)y;

            // Check if spot is empty
            if (arena[iy][ix] != ' ') {
                attempts++;
                continue;
            }

            // Check if too close to player
            float dx = x - player.x;
            float dy = y - player.y;

            float distSq = dx * dx + dy * dy;

            if (distSq < 25.0f) {  // 5 * 5
                attempts++;
                continue;
            }

            // Check if too close to other enemies
            bool tooClose = false;
            for (int j = 0; j < enemyCount; j++) {
                if (!enemies[j].active) continue;

                float edx = x - enemies[j].x;
                float edy = y - enemies[j].y;

                float edistSq = edx * edx + edy * edy;

                if (edistSq < 9.0f) {  // 3 * 3
                    tooClose = true;
                    break;
                }
            }

            if (!tooClose) {
                validSpot = true;
            }
            else {
                attempts++;
            }
        }

        // If we found a valid spot, spawn the enemy
        if (validSpot) {
            spawnEnemy(ENEMY_WALKER, x, y);
        }
    }
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
    player.damageCooldown = 0;
	player.currentAttack.active = false;
	player.currentAttack.direction = ATTACK_NONE;
	player.currentAttack.timer = 0;
	player.currentAttack.lastX = -1;
	player.currentAttack.lastY = -1;
	player.currentAttack.lastDirection = ATTACK_NONE;
	player.currentAttack.numSavedChars = 0;
}

void initGame(){
	hideCursor();
    srand((unsigned int)time(NULL)); // Seed RNG for different platforms each run

	initArena();
	generatePlatforms();
	drawArena();
	initPlayer();
	
	maxEnemies = 10;
    enemies = new Enemy[maxEnemies];
	enemyCount = 0;

	spawnWave(1);

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

    if (key == 'a' && player.x > 2) 
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
void checkAttackCollision() {
    if (!player.currentAttack.active)
        return;

	int px = (int)player.x;
	int py = (int)player.y;


	// Determine attack hit positions based on direction
    int hitX[3], hitY[3];
	int hitCount = 0;

    switch (player.currentAttack.direction) {
        case ATTACK_UP:
            hitX[0] = px - 1; hitY[0] = py - 1;
            hitX[1] = px;     hitY[1] = py - 1;
            hitX[2] = px + 1; hitY[2] = py - 1;
            hitCount = 3;
			break;
        case ATTACK_DOWN:
            hitX[0] = px - 1; hitY[0] = py + 1;
            hitX[1] = px;     hitY[1] = py + 1;
            hitX[2] = px + 1; hitY[2] = py + 1;
			hitCount = 3;
            break;
        case ATTACK_LEFT:
            hitX[0] = px - 1; hitY[0] = py - 1;
            hitX[1] = px - 1; hitY[1] = py;
			hitX[2] = px - 1; hitY[2] = py + 1;
			hitCount = 3;
            break;
        case ATTACK_RIGHT:
            hitX[0] = px + 1; hitY[0] = py - 1;
			hitX[1] = px + 1; hitY[1] = py;
			hitX[2] = px + 1; hitY[2] = py + 1;
            hitCount = 3;
			break;
        default:
            return;
    }

    for (int i = 0; i < enemyCount; i++){
		Enemy& enemy = enemies[i];
        if (!enemy.active)
            continue;

		int ex = (int)enemy.x;
		int ey = (int)enemy.y;

        for (int h = 0; h < hitCount; h++){
            if (ex == hitX[h] && ey == hitY[h]){
				enemy.active = false; // Enemy is hit and deactivated

				gotoXY(enemy.lastX, enemy.lastY);
                cout << ' ';
                break;
            }
        }
    }
}

void checkPlayerEnemyCollision() {
    if(player.damageCooldown > 0) 
        return;

	int px = (int)player.x;
	int py = (int)player.y;

    for (int i = 0; i < enemyCount; i++)
    {
		Enemy& enemy = enemies[i];

        if (!enemy.active) continue;

		int ex = (int)enemy.x;
		int ey = (int)enemy.y;

        if (px == ex && py == ey){
			player.hp--;
			player.damageCooldown = DAMAGE_COOLDOWN;
            break;
        }
    }
}

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

	// Decrease damage cooldown timer
    if (player.damageCooldown > 0) {
        player.damageCooldown -= dt;
        if (player.damageCooldown < 0) {
            player.damageCooldown = 0;
        }
	}
}

void checkVerticalCollision(float& y, float& dy, float oldY, float newY, int x,bool& grounded, int& jumps, bool isFalling) {
    int startY = (int)oldY;
    int endY = (int)newY;

    // ===== CHECK ALL CELLS BETWEEN OLD AND NEW POSITION =====
    if (isFalling) {// Falling down
        // Check each cell we're passing through while falling
        for (int checkY = startY + 1; checkY <= endY; checkY++) {
            if (isInBounds(x, checkY) && isCollisionTile(arena[checkY][x])) {
                y = (float)(checkY - 1);
                dy = 0;
                grounded = true;
                jumps = 0;
                return;
            }
        }
    }
	else {// Jumping up
		// Check each cell we're passing through while jumping
        for (int checkY = startY; checkY >= endY; checkY--) {
            if (isInBounds(x, checkY) && isCollisionTile(arena[checkY][x])) {
                y = (float)(checkY + 1);
                dy = 0;
                return;
            }
        }
    }

    // No collision, apply new position
    y = newY;
}

void updateEnemyPhysics(Enemy& enemy, float dt) {
    if (!enemy.active)
        return;

    enemy.dy += GRAVITY * dt;

    float oldY = enemy.y;
    float newY = enemy.y + enemy.dy * dt;
    enemy.grounded = false;

    int ex = (int)enemy.x;
	int imaginaryJumps = 0; // Enemies don't have jumps, but checkVerticalCollision needs it

    if (enemy.dy > 0) {
        checkVerticalCollision(enemy.y, enemy.dy, oldY, newY, ex, enemy.grounded, imaginaryJumps, true);
    }
    else if (enemy.dy < 0) {
        checkVerticalCollision(enemy.y, enemy.dy, oldY, newY, ex, enemy.grounded,imaginaryJumps, false);
    }
    else {
        enemy.y = newY;;
    }
}

void updateWalkerAI(Enemy& enemy, float dt) {
    if(!enemy.grounded) return;

	float newX = enemy.x + enemy.dx * enemy.direction * dt;
	int checkX = (int)newX;
	int checkY = (int)enemy.y;

    if (checkX <= 1 || checkX >= ARENA_WIDTH - 1 || isCollisionTile(arena[checkY][checkX]))
    {
		enemy.direction *= -1; // Reverse direction
        return;
    }

	int groundCheckY = checkY + 1;

    if (groundCheckY < ARENA_HEIGHT && !isCollisionTile(arena[groundCheckY][checkX])) {
		enemy.direction *= -1; // Reverse direction
        return;
    }

	enemy.x = newX;
}

void updateEnemyAI(float dt) {
    for (int i = 0; i < enemyCount; i++)
    {
		Enemy& enemy = enemies[i];
        if (!enemy.active) 
            continue;

        switch (enemy.type)
        {
            case ENEMY_WALKER:
                updateWalkerAI(enemy, dt);
			    break;
            case ENEMY_JUMPER:
                break;
            case ENEMY_FLIER:
                break;
            case ENEMY_CRAWLER:
                break;
            case ENEMY_BOSS:
                break;
            default:
                break;
        }
    }
}

void updatePhysics(float dt) {
	updateAttackTimers(dt);

	checkAttackCollision();
	checkPlayerEnemyCollision();

    // Apply gravity
    player.dy += GRAVITY * dt;

    // Store old position BEFORE any updates
    float oldY = player.y;

    // Apply vertical velocity
    float newY = player.y + player.dy * dt;

    player.grounded = false;

    int px = (int)player.x;


    
    if (player.dy > 0){
		checkVerticalCollision(player.y, player.dy,oldY, newY, px,player.grounded,player.jumps, true);
    } else if (player.dy < 0) {
        checkVerticalCollision(player.y, player.dy, oldY, newY, px, player.grounded, player.jumps, false);
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

    for (int i = 0; i < enemyCount; i++) {
        updateEnemyPhysics(enemies[i], dt);
    }

    updateEnemyAI(dt);
}

// ========== RENDERING ==========
void renderEnemies() {
    for (int i = 0; i < enemyCount; i++) {
        Enemy& enemy = enemies[i];
        if (!enemy.active) continue;

        gotoXY(enemy.lastX, enemy.lastY);
        cout << ' ';

        enemy.lastX = (int)enemy.x;
        enemy.lastY = (int)enemy.y;
        gotoXY(enemy.lastX, enemy.lastY);

        switch (enemy.type) {
        case ENEMY_WALKER:
            cout << ENEMY_WALKER_CHAR;
            break;
        case ENEMY_JUMPER:
            cout << 'J';
            break;
        case ENEMY_FLIER:
            cout << 'F';
            break;
        case ENEMY_CRAWLER:
            cout << 'C';
            break;
        case ENEMY_BOSS:
            cout << 'B';
            break;
        default:
            cout << 'X';
            break;
        }
    }
}

// restores the background characters that were saved before drawing the attack
void clearAttack() {
    // loop through all saved characters and restore them to their original positions
    for (int i = 0; i < player.currentAttack.numSavedChars; i++) {
        int x = player.currentAttack.savedPositions[i][0];
        int y = player.currentAttack.savedPositions[i][1];

        gotoXY(x, y);
        cout << player.currentAttack.savedChars[i];// print the original character
    }
	player.currentAttack.numSavedChars = 0; // reset the count
}

// saves the background characters before drawing attack, then draws the attack
void saveAndDrawAttack(int x, int y, const char* str, int len) {
    // save each character that will be overwritten by the attack animation
    for (int i = 0; i < len; i++) {
        if (x + i >= 0 && x + i < ARENA_WIDTH && y >= 0 && y < ARENA_HEIGHT) {
            int idx = player.currentAttack.numSavedChars;
			if (idx < 3) { // ensure we don't exceed the array bounds
				// save the character
                player.currentAttack.savedChars[idx] = arena[y][x + i];
				// save the position of the character
                player.currentAttack.savedPositions[idx][0] = x + i;
                player.currentAttack.savedPositions[idx][1] = y;
                player.currentAttack.numSavedChars++;
            }
        }
    }

	// draw the attack over the saved characters
    gotoXY(x, y);
    cout << str;
}

void renderAttack() {
	// clear previous attack display if needed
    if (player.currentAttack.lastDirection != ATTACK_NONE) {
        // clear if player moved OR if attack is no longer active
        bool playerMoved = (player.currentAttack.lastX != (int)player.x ||
                            player.currentAttack.lastY != (int)player.y);
        if (!player.currentAttack.active || playerMoved)
        {
			clearAttack(); // restore background
            player.currentAttack.lastDirection = ATTACK_NONE;
        }
    }
	
    if (!player.currentAttack.active) return;

	int px = (int)player.x;
	int py = (int)player.y;

	// save current position and direction for next frame
    player.currentAttack.lastX = px;
    player.currentAttack.lastY = py;
	player.currentAttack.lastDirection = player.currentAttack.direction;

	//reset saved characters count
	player.currentAttack.numSavedChars = 0;

    switch (player.currentAttack.direction) {
    case ATTACK_UP:
        saveAndDrawAttack(px - 1, py - 1, "/-\\", 3);
        break;
    case ATTACK_DOWN:
        saveAndDrawAttack(px - 1, py + 1, "\\_/", 3);
        break;
    case ATTACK_LEFT:
        saveAndDrawAttack(px - 1, py - 1, "/", 1);
        saveAndDrawAttack(px - 1, py, "|", 1);
        saveAndDrawAttack(px - 1, py + 1, "\\", 1);
        break;
    case ATTACK_RIGHT:
        saveAndDrawAttack(px + 1, py - 1, "\\", 1);
        saveAndDrawAttack(px + 1, py, "|", 1);
        saveAndDrawAttack(px + 1, py + 1, "/", 1);
        break;
    default:
        break;
    }
}

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

    renderEnemies();

	// ===== Draw Player =====
	// Erase last position
    gotoXY(player.lastX, player.lastY);
    cout << ' ';

    player.lastX = (int)player.x;
    player.lastY = (int)player.y;
    gotoXY(player.lastX, player.lastY);

    if (player.damageCooldown > 0) {
		int flashCycle = (int)(player.damageCooldown / 5) % 2;
        if (flashCycle == 0) {
            cout << PLAYER_CHAR;
		}
    } else {
        cout << PLAYER_CHAR;
    }
    

	renderAttack();
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

	delete[] enemies;
    return 0;
}