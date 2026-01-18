## Overview

A simple console-based 2D arena game implemented in C++ (Windows console APIs). The player (`@`) moves, jumps and performs directional attacks while waves of enemies spawn. Wave 5 spawns a boss (`B`) with multiple attack patterns and visual telegraphs.

Key features:
- Arena with walls and platforms
- Player physics (gravity, jumping, multi-jump)
- Directional attack system with cooldown and short visual effects
- Multiple enemy types with distinct AI: walker, jumper, flier, crawler
- Boss enemy with several attack patterns (X-laser, + laser, earthquake, lightning, cross sweep)
- Wave-based spawning and a simple HUD

---

## Important constants

Top of file defines arena size, characters and gameplay constants. Notable constants:
- `ARENA_WIDTH`, `ARENA_HEIGHT` — arena dimensions
- `PLAYER_SPEED`, `GRAVITY`, `JUMP_FORCE`, `MAX_JUMPS`
- Attack timings: `ATTACK_COOLDOWN`, `ATTACK_DURATION`, `DAMAGE_COOLDOWN`
- Enemy speeds and parameters (e.g., `ENEMY_WALKER_SPEED`, flier timings)
- Boss constants: `BOSS_X`, `BOSS_Y`, `BOSS_SIZE`, `BOSS_HP`, attack durations and cooldowns

Adjusting these values tunes gameplay balance.

---

## Data structures

Structs and enums:

- `enum EnemyType` — identifies enemy kind.
- `enum Color` — console color codes used with `setColor`.
- `enum AttackDirection` — player attack orientation.
- `enum BossAttackPattern` — boss attack types.

- `struct Attack`
  - Represents current player attack state and saved background characters overwritten by the attack visuals.
  - Fields: `direction`, `timer`, `active`, `lastX/lastY`, `savedChars`, `savedPositions`, `numSavedChars`, etc.

- `struct Player`
  - Fields: position/velocity (`x, y, dy`), `hp`, `jumps`, `grounded`, `lastX/lastY` for rendering, `attackCooldown`, `currentAttack`, `damageCooldown`.

- `struct Enemy`
  - General enemy state used for every enemy type, including boss-specific fields: `hp`, `damageCooldown`, `currentPattern`, `isAttacking`, `attackTimer`, `attackPhase`, `isWarning`.

- `struct LightningStrike` — used by boss lightning attack.

Global arrays/pointers:
- `char arena[ARENA_HEIGHT][ARENA_WIDTH]` — arena cells.
- Dynamic `Enemy* enemies` with `maxEnemies`, `enemyCount`.
- `Enemy* boss` pointing to the boss entry (if present).
- `LightningStrike lightningStrikes[10]`.

---

## Core flow

Main entrypoint: `main()`
- Calls `initGame()` which:
  - Hides cursor, seeds RNG, initializes the arena and platforms, draws the initial arena, initializes the player, allocates enemies array and spawns wave 1.
- Game loop:
  - Computes `dt` (delta time scaled to 60 FPS).
  - `handleInput(dt)` — non-blocking keyboard input (`_kbhit`, `_getch`); movement and attack key handling.
  - `updatePhysics(dt)` — updates player gravity/vertical collisions, enemy physics, attack timers, collision checks.
  - `updateWaveSystem()` — advances wave when no enemies remain; wave 5 spawns the boss and removes platforms.
  - `render()` — HUD, enemies, player and attack visuals are drawn to the console.
  - `Sleep(16)` to cap frame pacing.

Game ends when player `hp <= 0` or boss defeated after wave 5; `showEndScreen()` displays result.

---

## Major subsystems and key functions

Initialization
- `initArena()` — fills `arena` and sets walls.
- `generatePlatforms()` — randomly places 4 platforms.
- `drawArena()` — prints the `arena` buffer to console.
- `initPlayer()` — sets initial player state.

Enemy management
- `spawnEnemy(type, x, y)` — ensure capacity and initialize an enemy entry.
- `findValidSpawnPosition(float& x, float& y)` — attempts to locate a spawn tile that's not occupied and not too close to player/other enemies.
- `spawnWave(int waveNumber)` — decides how many of each enemy to spawn for a wave; wave 5 spawns the boss and removes platforms.
- `findBoss()` — locates the boss in `enemies[]`.

Input
- `handleInput(float dt)` and `handleAttackInput(char key)` — handle movement (`a/d`), jump (`w`) and attacks (`i/j/k/l`).

Physics & collision
- `updatePhysics(float dt)` — updates timers, applies gravity, checks collisions, updates enemies.
- `checkVerticalCollision(...)` — robust vertical collision detection that checks cells between old and new Y positions (handles tunneling).
- `checkAttackCollision()` — resolves player attack hits against enemies and boss (boss has invincibility cooldown).
- `checkPlayerEnemyCollision()` — player takes damage on enemy contact, handles boss area as 3x3.

Enemy AI / movement
- `updateEnemyPhysics(Enemy&, dt)` — vertical movement and bounds.
- `updateWalkerAI`, `updateJumperAI`, `updateFlierAI`, `updateCrawlerAI` — type-specific behavior.
- `updateBossAI(dt)` — boss attack selection, warning phase, actual attack timing, spawn lightning.

Boss attacks & visuals
- Visual drawing helpers for boss attacks:
  - `drawXLaser()`, `drawPlusLaser()`, `drawEarthquake()`, `drawLightning()`, `drawCrossSweep(float phase)`.
  - `clearBossAttackVisuals()` clears arena non-wall non-boss area (preserves player).
  - `spawnLightning()` and `drawLightning()` manage lightning strike positions and rendering.
- Damage checks:
  - `checkBossAttackCollision()` tests player position against current boss attack visuals/patterns and decrements player HP / applies damage cooldown.

Rendering
- `render()` draws HUD, enemies via `renderEnemies()` and the player.
- `renderBoss()` renders the 3x3 boss and visual feedback for damage cooldown.
- Attack visuals:
  - `renderAttack()` uses `saveAndDrawAttack()` and `clearAttack()` to temporarily overwrite and restore arena characters to show attack shapes without permanently changing `arena`.

Utilities
- `gotoXY(int x, int y)` — sets console cursor position using WinAPI.
- `setColor(int color)` — sets text color via WinAPI.
- `hideCursor()` — hides console caret.

---

## Controls / How to play

- Movement: `a` (left), `d` (right)
- Jump: `w` (supports double-jump)
- Attacks: `i` (up), `k` (down), `j` (left), `l` (right)
- Objective: survive waves and defeat boss at wave 5.

---

## Build and run (Visual Studio)

1. Open the solution/project in Visual Studio.
2. Ensure project configuration targets Windows and uses a console subsystem.
3. Build and Run (F5 or Ctrl+F5).
4. Note: Code relies on Windows-specific headers (`windows.h`, console APIs) and `_kbhit`/`_getch` from `<conio.h>`.

---

## Notes, limitations and TODOs

Known behavior / implementation notes:
- `arena` characters are used for collision checks and saved/restored for short attack visuals. Some rendering functions write directly to the console without updating `arena` (intended for transient visuals).
- The crawler logic moves in discrete 1-cell steps when checking collisions; crawler speed is scaled but high speeds may cause edge cases.
- The flier `diving` logic checks collision before moving vertically; flies may hover at `ENEMY_FLIER_MIN_HEIGHT`.
- Boss `clearBossAttackVisuals()` clears the arena except the boss and player. This might remove platform graphics during boss fight (intended).
- Attack save buffer in `Attack` supports max 3 saved characters; multi-part attack visuals use up to this limit.
- Timing values (attack durations, cooldowns) are float-tuned for the game's internal dt scaling — modifying dt calculation or Sleep may require retuning constants.

---

## Where to look in code

Quick navigation (function names are present in `ConsoleApplication2.cpp`):
- Initialization: `initGame()`, `initArena()`, `generatePlatforms()`
- Input: `handleInput()`, `handleAttackInput()`
- Physics & collision: `updatePhysics()`, `checkVerticalCollision()`, `checkAttackCollision()`, `checkPlayerEnemyCollision()`
- Enemy logic: `spawnEnemy()`, `spawnWave()`, `updateEnemyAI()` and per-type `update*AI()` functions
- Boss visuals: `drawXLaser()`, `drawPlusLaser()`, `drawEarthquake()`, `drawLightning()`, `drawCrossSweep()`, `clearBossAttackVisuals()`
- Rendering: `render()`, `renderEnemies()`, `renderBoss()`, `renderAttack()`
- Game loop & termination: `main()`, `updateWaveSystem()`, `showEndScreen()`
