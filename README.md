#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <conio.h>
#include <Windows.h>
#include <string.h>

#define GAME_WIDTH 50
#define GAME_HEIGHT 20
#define MAX_TAIL 200
#define CONSOLE_WIDTH 60
#define CONSOLE_HEIGHT 25

typedef enum { STOP = 0, LEFT, RIGHT, UP, DOWN } Direction;

bool gameOver;
Direction dir;
int snakeX, snakeY, foodX, foodY, score;
int tailX[MAX_TAIL], tailY[MAX_TAIL], snakeLen;
int bonusX, bonusY, bonusTimer;
bool bonusExist;
int gameSpeed, foodEatCount;
HANDLE hConsole;
CHAR_INFO backBuffer[CONSOLE_HEIGHT][CONSOLE_WIDTH];

void SetChar(int x, int y, char ch, WORD color) {
    if (x >= 0 && x < CONSOLE_WIDTH && y >= 0 && y < CONSOLE_HEIGHT) {
        backBuffer[y][x].Char.AsciiChar = ch;
        backBuffer[y][x].Attributes = color;
    }
}

void InitConsole() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowSize = {0, 0, CONSOLE_WIDTH-1, CONSOLE_HEIGHT-1};
    COORD bufferSize = {CONSOLE_WIDTH, CONSOLE_HEIGHT};
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    CONSOLE_CURSOR_INFO cursorInfo = {1, 0};
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    for (int y = 0; y < CONSOLE_HEIGHT; y++)
        for (int x = 0; x < CONSOLE_WIDTH; x++) {
            backBuffer[y][x].Char.AsciiChar = ' ';
            backBuffer[y][x].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
}

void Setup() {
    gameOver = false; dir = STOP;
    snakeX = GAME_WIDTH / 2; snakeY = GAME_HEIGHT / 2;
    for (int i = 0; i < 3; i++) { tailX[i] = snakeX - i - 1; tailY[i] = snakeY; }
    snakeLen = 3; score = 0; gameSpeed = 150; foodEatCount = 0;
    bonusExist = false; bonusTimer = 0;
    srand((unsigned int)time(NULL));
    bool valid;
    do {
        foodX = rand() % GAME_WIDTH; foodY = rand() % GAME_HEIGHT;
        valid = true;
        if (foodX == snakeX && foodY == snakeY) valid = false;
        for (int k = 0; k < snakeLen; k++)
            if (tailX[k] == foodX && tailY[k] == foodY) valid = false;
    } while (!valid);
}

void DrawStatusBar() {
    char status[128];
    snprintf(status, sizeof(status), "Score: %-4d | Speed: %-3dms | X:Exit", score, gameSpeed);
    int len = (int)strlen(status);
    for (int x = 0; x < CONSOLE_WIDTH; x++) {
        if (x < len)
            SetChar(x, 0, status[x], FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        else
            SetChar(x, 0, ' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}

void DrawGameArea() {
    for (int y = 1; y <= GAME_HEIGHT + 1; y++)
        for (int x = 0; x < CONSOLE_WIDTH; x++)
            SetChar(x, y, ' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    int ox = (CONSOLE_WIDTH - (GAME_WIDTH + 2)) / 2, oy = 1;
    for (int x = 0; x < GAME_WIDTH + 2; x++) {
        SetChar(ox + x, oy, '-', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        SetChar(ox + x, oy + GAME_HEIGHT + 1, '-', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    for (int y = 1; y <= GAME_HEIGHT; y++) {
        SetChar(ox, oy + y, '|', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        SetChar(ox + GAME_WIDTH + 1, oy + y, '|', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    for (int i = 0; i < snakeLen; i++)
        SetChar(ox + tailX[i] + 1, oy + tailY[i] + 1, 'o', FOREGROUND_GREEN);
    SetChar(ox + snakeX + 1, oy + snakeY + 1, 'O', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    SetChar(ox + foodX + 1, oy + foodY + 1, 'F', FOREGROUND_RED | FOREGROUND_INTENSITY);
    if (bonusExist)
        SetChar(ox + bonusX + 1, oy + bonusY + 1, '*', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void Draw() {
    for (int y = 0; y < CONSOLE_HEIGHT; y++)
        for (int x = 0; x < CONSOLE_WIDTH; x++) {
            backBuffer[y][x].Char.AsciiChar = ' ';
            backBuffer[y][x].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    DrawStatusBar(); DrawGameArea();
    COORD bufferSize = {CONSOLE_WIDTH, CONSOLE_HEIGHT}, bufferCoord = {0, 0};
    SMALL_RECT writeRegion = {0, 0, CONSOLE_WIDTH-1, CONSOLE_HEIGHT-1};
    WriteConsoleOutput(hConsole, (CHAR_INFO*)backBuffer, bufferSize, bufferCoord, &writeRegion);
}

void Input() {
    if (_kbhit()) {
        int key = _getch();
        if (key == 0xE0) {
            key = _getch();
            switch (key) {
                case 75: if (dir != RIGHT) dir = LEFT; break;
                case 77: if (dir != LEFT) dir = RIGHT; break;
                case 72: if (dir != DOWN) dir = UP; break;
                case 80: if (dir != UP) dir = DOWN; break;
            }
        } else {
            switch (key) {
                case 'a': if (dir != RIGHT) dir = LEFT; break;
                case 'd': if (dir != LEFT) dir = RIGHT; break;
                case 'w': if (dir != DOWN) dir = UP; break;
                case 's': if (dir != UP) dir = DOWN; break;
                case 'x': gameOver = true; break;
            }
        }
    }
}

void Logic() {
    if (dir == STOP) return;
    int prevX = snakeX, prevY = snakeY;
    for (int i = 0; i < snakeLen; i++) {
        int tempX = tailX[i], tempY = tailY[i];
        tailX[i] = prevX; tailY[i] = prevY;
        prevX = tempX; prevY = tempY;
    }
    switch (dir) {
        case LEFT: snakeX--; break; case RIGHT: snakeX++; break;
        case UP: snakeY--; break; case DOWN: snakeY++; break;
         case STOP:
        break;
    }
    if (snakeX < 0 || snakeX >= GAME_WIDTH || snakeY < 0 || snakeY >= GAME_HEIGHT) gameOver = true;
    for (int i = 0; i < snakeLen; i++)
        if (snakeX == tailX[i] && snakeY == tailY[i]) gameOver = true;
    if (snakeX == foodX && snakeY == foodY) {
        score += 10; snakeLen++; foodEatCount++;
        if (gameSpeed > 50) gameSpeed -= 2;
        bool valid;
        do {
            foodX = rand() % GAME_WIDTH; foodY = rand() % GAME_HEIGHT;
            valid = true;
            if ((foodX == snakeX && foodY == snakeY) || (bonusExist && foodX == bonusX && foodY == bonusY)) valid = false;
            for (int k = 0; k < snakeLen; k++)
                if (tailX[k] == foodX && tailY[k] == foodY) { valid = false; break; }
        } while (!valid);
        if (foodEatCount % 3 == 0 && rand() % 10 < 3 && !bonusExist) {
            bool bonusValid;
            do {
                bonusX = rand() % GAME_WIDTH; bonusY = rand() % GAME_HEIGHT;
                bonusValid = true;
                if ((bonusX == snakeX && bonusY == snakeY) || (bonusX == foodX && bonusY == foodY)) bonusValid = false;
                for (int k = 0; k < snakeLen; k++)
                    if (tailX[k] == bonusX && tailY[k] == bonusY) { bonusValid = false; break; }
            } while (!bonusValid);
            bonusExist = true; bonusTimer = 67;
        }
    }
    if (bonusExist) {
        bonusTimer--;
        if (bonusTimer <= 0) bonusExist = false;
        if (snakeX == bonusX && snakeY == bonusY) { score += 30; snakeLen += 2; bonusExist = false; }
    }
}

int main() {
    SetConsoleTitle("Simplified Snake Game");
    InitConsole(); Setup();
    while (!gameOver) { Draw(); Input(); Logic(); if (gameSpeed < 0) gameSpeed = 0; Sleep((DWORD)gameSpeed); }
    system("cls");
    printf("===== Game Over =====\nFinal Score: %d\nPress any key to exit...", score);
    _getch();
    return 0;
}
