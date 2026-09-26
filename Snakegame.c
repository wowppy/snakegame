// 引入标准输入输出库，提供printf等基础控制台打印功能
#include <stdio.h>
// 引入标准工具库，提供rand随机数生成、system系统命令调用等工具函数
#include <stdlib.h>
// 引入C99布尔类型库，定义true和false关键字，用于游戏状态的直观判断
#include <stdbool.h>
// 引入时间库，用于获取系统时间作为随机数种子，保证每次游戏食物位置随机
#include <time.h>
// 引入控制台输入库，提供_kbhit检测按键、_getch无回显读按键的游戏输入功能
#include <conio.h>
// 引入Windows系统API库，提供控制台窗口操作、双缓冲画面输出等底层系统接口
#include <Windows.h>
// 引入字符串处理库，提供strlen函数用于计算状态栏文本的字符长度
#include <string.h>

// 定义游戏活动区域的宽度，单位为字符单元格
#define GAME_WIDTH 50
// 定义游戏活动区域的高度，单位为字符单元格
#define GAME_HEIGHT 20
// 定义蛇身的最大长度上限，防止蛇身无限增长超出数组容量，避免内存越界
#define MAX_TAIL 200
// 定义整个控制台窗口的总宽度，预留状态栏和边框的显示空间
#define CONSOLE_WIDTH 60
// 定义整个控制台窗口的总高度，预留状态栏和边框的显示空间
#define CONSOLE_HEIGHT 25

// 自定义方向枚举类型，用语义化名称替代数字，清晰表示蛇的5种移动状态
typedef enum { STOP = 0, LEFT, RIGHT, UP, DOWN } Direction;

// 全局布尔变量，标记游戏是否结束，主循环根据该变量判断是否退出游戏
bool gameOver;
// 全局方向变量，记录蛇当前的移动方向
Direction dir;
// 蛇头的X坐标、Y坐标，普通食物的X坐标、Y坐标，当前游戏得分，均为全局整型变量
int snakeX, snakeY, foodX, foodY, score;
// 两个数组分别存储蛇身每一节的X坐标和Y坐标，snakeLen记录蛇当前的总长度
int tailX[MAX_TAIL], tailY[MAX_TAIL], snakeLen;
// 限时奖励食物的X坐标、Y坐标，以及奖励食物的剩余存在帧数计时器
int bonusX, bonusY, bonusTimer;
// 标记当前场景中是否存在限时奖励食物的布尔变量
bool bonusExist;
// 记录游戏当前的移动间隔速度（单位毫秒），以及蛇累计吃到普通食物的数量
int gameSpeed, foodEatCount;
// Windows控制台句柄，相当于控制台窗口的身份标识，所有控制台API操作都需要传入该句柄
HANDLE hConsole;
// 定义双缓冲字符缓冲区，提前绘制下一帧所有画面内容后一次性输出，彻底解决画面闪烁问题
CHAR_INFO backBuffer[CONSOLE_HEIGHT][CONSOLE_WIDTH];

// 向双缓冲指定位置写入字符和颜色属性的工具函数
void SetChar(int x, int y, char ch, WORD color) {
    // 加入边界判断，防止写入超出控制台范围的无效位置，避免内存异常
    if (x >= 0 && x < CONSOLE_WIDTH && y >= 0 && y < CONSOLE_HEIGHT) {
        // 将指定字符写入缓冲区对应位置的字符属性中
        backBuffer[y][x].Char.AsciiChar = ch;
        // 将指定颜色值写入缓冲区对应位置的颜色属性中
        backBuffer[y][x].Attributes = color;
    }
}

// 控制台初始化函数，用于设置窗口尺寸、隐藏光标、初始化缓冲区
void InitConsole() {
    // 获取标准输出设备的控制台句柄，赋值给全局变量hConsole
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    // 定义控制台窗口的矩形尺寸，左上角坐标(0,0)，右下角坐标对应窗口最后一个单元格
    SMALL_RECT windowSize = {0, 0, CONSOLE_WIDTH-1, CONSOLE_HEIGHT-1};
    // 定义控制台屏幕缓冲区的尺寸，和窗口尺寸保持一致
    COORD bufferSize = {CONSOLE_WIDTH, CONSOLE_HEIGHT};
    // 根据定义的尺寸设置控制台窗口的实际显示大小
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
    // 设置控制台后台缓冲区的尺寸，避免缓冲区和窗口尺寸不匹配导致画面显示异常
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    // 定义光标信息结构体，第一个参数设置光标尺寸为1，第二个参数设为0代表隐藏光标
    CONSOLE_CURSOR_INFO cursorInfo = {1, 0};
    // 将隐藏光标的设置应用到当前控制台窗口，避免闪烁光标干扰游戏画面
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    // 双层循环遍历整个双缓冲数组
    for (int y = 0; y < CONSOLE_HEIGHT; y++)
        for (int x = 0; x < CONSOLE_WIDTH; x++) {
            // 把所有位置的字符初始化为空白字符，清空缓冲区残留内容
            backBuffer[y][x].Char.AsciiChar = ' ';
            // 把所有位置的颜色初始化为默认白色
            backBuffer[y][x].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
}

// 游戏初始化函数，用于重置所有游戏状态、生成初始蛇身和第一个食物
void Setup() {
    // 初始化游戏状态，标记游戏未结束
    gameOver = false; dir = STOP;
    // 把蛇头初始位置设置在游戏区域的正中心
    snakeX = GAME_WIDTH / 2; snakeY = GAME_HEIGHT / 2;
    // 循环初始化初始蛇身，初始长度为3节，全部水平排列在蛇头的左侧
    for (int i = 0; i < 3; i++) { tailX[i] = snakeX - i - 1; tailY[i] = snakeY; }
    // 初始化蛇长度为3、得分0、初始移动速度150毫秒、吃食物计数器为0
    snakeLen = 3; score = 0; gameSpeed = 150; foodEatCount = 0;
    // 初始状态下没有限时奖励食物，计时器清零
    bonusExist = false; bonusTimer = 0;
    // 以当前系统时间作为种子初始化随机数生成器，保证每次游戏的食物位置都不重复
    srand((unsigned int)time(NULL));
    // 定义标记变量，用于校验生成的食物位置是否合法
    bool valid;
    // 循环生成第一个食物的随机位置，直到生成合法位置才退出循环
    do {
        // 在游戏区域范围内随机生成食物的X和Y坐标
        foodX = rand() % GAME_WIDTH; foodY = rand() % GAME_HEIGHT;
        // 先标记当前生成的位置为合法
        valid = true;
        // 如果食物刚好生成在蛇头位置，标记为不合法
        if (foodX == snakeX && foodY == snakeY) valid = false;
        // 遍历所有蛇身节，检查食物是否生成在蛇身上
        for (int k = 0; k < snakeLen; k++)
            if (tailX[k] == foodX && tailY[k] == foodY) valid = false;
    } while (!valid); // 位置不合法就重新生成，直到得到合法位置
}

// 状态栏绘制函数，在窗口顶部显示实时得分、游戏速度和退出提示
void DrawStatusBar() {
    // 定义足够大的字符数组存储状态栏文本
    char status[128];
    // 格式化生成状态栏文本，实时显示得分、速度和退出提示，限制写入长度避免缓冲区溢出
    snprintf(status, sizeof(status), "Score: %-4d | Speed: %-3dms | X:Exit", score, gameSpeed);
    // 计算状态栏文本的实际字符长度
    int len = (int)strlen(status);
    // 遍历状态栏整行的所有单元格
    for (int x = 0; x < CONSOLE_WIDTH; x++) {
        // 如果当前位置在文本长度范围内，写入对应字符，设置高亮白色
        if (x < len)
            SetChar(x, 0, status[x], FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        // 超出文本长度的位置填充空白字符，保证状态栏整行显示完整
        else
            SetChar(x, 0, ' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}

// 游戏区域绘制函数，绘制边框、蛇身、蛇头、普通食物和奖励食物
void DrawGameArea() {
    // 双层循环清空游戏区域的所有单元格，清除上一帧的残留画面
    for (int y = 1; y <= GAME_HEIGHT + 1; y++)
        for (int x = 0; x < CONSOLE_WIDTH; x++)
            SetChar(x, y, ' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    // 计算游戏边框的偏移坐标，让游戏区域在控制台窗口中水平居中显示
    int ox = (CONSOLE_WIDTH - (GAME_WIDTH + 2)) / 2, oy = 1;
    // 循环绘制游戏区域的上下两条水平边框，使用'-'字符
    for (int x = 0; x < GAME_WIDTH + 2; x++) {
        SetChar(ox + x, oy, '-', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        SetChar(ox + x, oy + GAME_HEIGHT + 1, '-', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    // 循环绘制游戏区域的左右两条垂直边框，使用'|'字符
    for (int y = 1; y <= GAME_HEIGHT; y++) {
        SetChar(ox, oy + y, '|', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        SetChar(ox + GAME_WIDTH + 1, oy + y, '|', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    // 遍历所有蛇身节，把蛇身绘制到缓冲区中，用绿色小写'o'表示
    for (int i = 0; i < snakeLen; i++)
        SetChar(ox + tailX[i] + 1, oy + tailY[i] + 1, 'o', FOREGROUND_GREEN);
    // 单独绘制蛇头，用高亮的绿色大写'O'，和蛇身形成明显区分
    SetChar(ox + snakeX + 1, oy + snakeY + 1, 'O', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    // 绘制普通食物，用高亮红色的'F'表示
    SetChar(ox + foodX + 1, oy + foodY + 1, 'F', FOREGROUND_RED | FOREGROUND_INTENSITY);
    // 如果奖励食物存在，用亮黄色的'*'绘制限时奖励食物
    if (bonusExist)
        SetChar(ox + bonusX + 1, oy + bonusY + 1, '*', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

// 总绘制函数，整合所有画面内容，通过双缓冲一次性输出到控制台
void Draw() {
    // 双层循环清空整个双缓冲，避免上一帧的内容残留
    for (int y = 0; y < CONSOLE_HEIGHT; y++)
        for (int x = 0; x < CONSOLE_WIDTH; x++) {
            backBuffer[y][x].Char.AsciiChar = ' ';
            backBuffer[y][x].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    // 调用状态栏绘制函数，将状态栏内容写入缓冲区
    DrawStatusBar(); DrawGameArea();
    // 定义缓冲区尺寸参数、起始坐标和输出区域参数
    COORD bufferSize = {CONSOLE_WIDTH, CONSOLE_HEIGHT}, bufferCoord = {0, 0};
    SMALL_RECT writeRegion = {0, 0, CONSOLE_WIDTH-1, CONSOLE_HEIGHT-1};
    // 调用Windows API把整个缓冲区的内容一次性输出到控制台窗口，实现无闪烁画面刷新
    WriteConsoleOutput(hConsole, (CHAR_INFO*)backBuffer, bufferSize, bufferCoord, &writeRegion);
}

// 输入处理函数，实时检测键盘按键，处理方向键和WASD键的转向逻辑
void Input() {
    // 检测当前是否有键盘按键按下，没有按键就直接跳过输入处理，不阻塞游戏主循环
    if (_kbhit()) {
        // 读取当前按键的ASCII值
        int key = _getch();
        // 如果返回值是0xE0，说明这是方向键等特殊功能键，需要读取第二个字节获取具体键值
        if (key == 0xE0) {
            // 再次读取键盘缓冲区，获取方向键的具体扫描码
            key = _getch();
            // 根据不同的方向键扫描码执行转向逻辑，加入基础校验防止180度反向掉头
            switch (key) {
                // 左键扫描码75：蛇当前不向右移动时，才能转向左
                case 75: if (dir != RIGHT) dir = LEFT; break;
                // 右键扫描码77：蛇当前不向左移动时，才能转向右
                case 77: if (dir != LEFT) dir = RIGHT; break;
                // 上键扫描码72：蛇当前不向下移动时，才能转向上
                case 72: if (dir != DOWN) dir = UP; break;
                // 下键扫描码80：蛇当前不向上移动时，才能转向下
                case 80: if (dir != UP) dir = DOWN; break;
            }
        } else {
            // 处理普通字符按键，实现WASD操控蛇移动
            switch (key) {
                // A键：蛇当前不向右移动时，转向左
                case 'a': if (dir != RIGHT) dir = LEFT; break;
                // D键：蛇当前不向左移动时，转向右
                case 'd': if (dir != LEFT) dir = RIGHT; break;
                // W键：蛇当前不向下移动时，转向上
                case 'w': if (dir != DOWN) dir = UP; break;
                // S键：蛇当前不向上移动时，转向下
                case 's': if (dir != UP) dir = DOWN; break;
                // X键：直接标记游戏结束
                case 'x': gameOver = true; break;
            }
        }
    }
}

// 核心游戏逻辑函数，处理蛇身移动、碰撞检测、吃食物、奖励食物等所有游戏规则
void Logic() {
    // 如果蛇当前处于静止状态，直接跳过移动逻辑
    if (dir == STOP) return;
    // 暂存蛇头当前的坐标，作为蛇身移动的起始前值
    int prevX = snakeX, prevY = snakeY;
    // 循环实现蛇身整体跟随移动：从蛇头之后的第一节开始，每一节的坐标替换成前一节的坐标
    for (int i = 0; i < snakeLen; i++) {
        int tempX = tailX[i], tempY = tailY[i];
        tailX[i] = prevX; tailY[i] = prevY;
        prevX = tempX; prevY = tempY;
    }
    // 根据当前方向更新蛇头的坐标
    switch (dir) {
        case LEFT: snakeX--; break; case RIGHT: snakeX++; break;
        case UP: snakeY--; break; case DOWN: snakeY++; break;
         case STOP:
        break;
    }
    // 边界碰撞检测：如果蛇头超出游戏区域范围，判定游戏结束
    if (snakeX < 0 || snakeX >= GAME_WIDTH || snakeY < 0 || snakeY >= GAME_HEIGHT) gameOver = true;
    // 自撞检测：遍历所有蛇身节，蛇头坐标和任意蛇身坐标重合则判定游戏结束
    for (int i = 0; i < snakeLen; i++)
        if (snakeX == tailX[i] && snakeY == tailY[i]) gameOver = true;
    // 吃到普通食物的逻辑处理
    if (snakeX == foodX && snakeY == foodY) {
        // 得分加10，蛇长度加1，吃食物计数器加1
        score += 10; snakeLen++; foodEatCount++;
        // 吃到食物后游戏速度提升，移动间隔减少2毫秒，最低速度限制为50毫秒，难度逐步上升
        if (gameSpeed > 50) gameSpeed -= 2;
        // 定义标记变量，校验新生成的食物位置是否合法
        bool valid;
        // 循环生成新的普通食物，直到位置合法
        do {
            foodX = rand() % GAME_WIDTH; foodY = rand() % GAME_HEIGHT;
            valid = true;
            // 食物不能生成在蛇头位置，也不能和奖励食物位置重合
            if ((foodX == snakeX && foodY == snakeY) || (bonusExist && foodX == bonusX && foodY == bonusY)) valid = false;
            // 食物不能生成在蛇身上
            for (int k = 0; k < snakeLen; k++)
                if (tailX[k] == foodX && tailY[k] == foodY) { valid = false; break; }
        } while (!valid);
        // 每吃3个普通食物，有30%的概率生成限时奖励食物，且当前没有奖励食物时才触发
        if (foodEatCount % 3 == 0 && rand() % 10 < 3 && !bonusExist) {
            // 定义标记变量，校验奖励食物的位置是否合法
            bool bonusValid;
            // 循环生成奖励食物的随机位置，直到合法
            do {
                bonusX = rand() % GAME_WIDTH; bonusY = rand() % GAME_HEIGHT;
                bonusValid = true;
                // 奖励食物不能生成在蛇头或普通食物的位置
                if ((bonusX == snakeX && bonusY == snakeY) || (bonusX == foodX && bonusY == foodY)) bonusValid = false;
                // 奖励食物不能生成在蛇身上
                for (int k = 0; k < snakeLen; k++)
                    if (tailX[k] == bonusX && tailY[k] == bonusY) { bonusValid = false; break; }
            } while (!bonusValid);
            // 标记奖励食物存在，设置它的存在时长为67帧
            bonusExist = true; bonusTimer = 67;
        }
    }
    // 限时奖励食物的逻辑处理
    if (bonusExist) {
        // 奖励食物的倒计时计时器每帧减1
        bonusTimer--;
        // 计时器归零时，奖励食物自动消失
        if (bonusTimer <= 0) bonusExist = false;
        // 如果蛇头吃到奖励食物，得分加30，蛇长度加2，奖励食物立即消失
        if (snakeX == bonusX && snakeY == bonusY) { score += 30; snakeLen += 2; bonusExist = false; }
    }
}

// 主函数，程序的入口点，组织整个游戏的主循环流程
int main() {
    // 设置控制台窗口的标题为“Simplified Snake Game”
    SetConsoleTitle("Simplified Snake Game");
    // 执行控制台初始化操作
    InitConsole(); Setup();
    // 游戏主循环：游戏未结束时，依次执行画面绘制、输入处理、逻辑更新，之后休眠对应毫秒数控制蛇的移动速度
    while (!gameOver) { Draw(); Input(); Logic(); if (gameSpeed < 0) gameSpeed = 0; Sleep((DWORD)gameSpeed); }
    // 游戏结束后清空整个控制台屏幕
    system("cls");
    // 打印游戏结束界面，显示最终得分，提示玩家按任意键退出
    printf("===== Game Over =====\nFinal Score: %d\nPress any key to exit...", score);
    // 等待玩家按下任意按键，不回显输入字符
    _getch();
    // 程序正常退出，返回0给操作系统标记执行成功
    return 0;
}

