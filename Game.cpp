#include "Game.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// 生成音效（兼容所有Raylib版本，无外部文件）
static Sound GenerateBeepSound(int frequency, int durationMs) {
    Wave wave = { 0 };
    wave.sampleRate = 44100;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.frameCount = wave.sampleRate * durationMs / 1000;
    wave.data = malloc(wave.frameCount * 2);

    short* samples = (short*)wave.data;
    for (int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / wave.sampleRate;
        float s = sinf(2 * PI * frequency * t);
        float env = 1.0f - (float)i / wave.frameCount;
        samples[i] = (short)(s * env * 16000.0f);
    }

    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

Game::Game() {
    score = 0;
    lives = 3;
    currentLevel = 1;
    paddleSpeed = 8.0f;
    ballSpeed = 6.0f;
    baseBallSpeed = ballSpeed;
    basePaddleW = 120.0f;
    currentState = MENU;

    flashTimer = 0.0f;
    borderFlashColor = WHITE;

    paddleTimer = slowTimer = multiTimer = 0;

    // 初始化音频
    InitAudioDevice();
    soundBrick  = GenerateBeepSound(900, 70);   // 撞砖块
    soundPaddle = GenerateBeepSound(600, 70);   // 撞挡板
    soundWall   = GenerateBeepSound(440, 60);   // 撞边界
}

Game::~Game() {
    UnloadSound(soundBrick);
    UnloadSound(soundPaddle);
    UnloadSound(soundWall);
    CloseAudioDevice();
}

void Game::LoadLevel(int level) {
    bricks.clear();
    if (level == 4) {
        GenerateRandomLevel4();
        return;
    }

    int rows = 1;
    if (level == 2) rows = 2;
    if (level == 3) rows = 3;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 8; j++) {
            Brick b;
            b.rect.x = 80 + j * 80;
            b.rect.y = 60 + i * 30;
            b.rect.width = 70;
            b.rect.height = 20;
            b.destroyed = false;

            if (i == 0)
            {
                b.color = RED;
                b.score = 50;
            }
            else if (i == 1)
            {
                b.color = ORANGE;
                b.score = 40;
            }
            else
            {
                b.color = YELLOW;
                b.score = 30;
            }
            bricks.push_back(b);
        }
    }
}

void Game::GenerateRandomLevel4() {
    bricks.clear();
    const int rowCount = 6;
    const int colCount = 9;
    const int brickW = 70;
    const int brickH = 22;
    const int gapX = 10;
    const int gapY = 8;
    const int startX = 35;
    const int startY = 70;

    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            if (GetRandomValue(1, 100) > 50)
                continue;

            Brick b;
            b.rect.x = startX + col * (brickW + gapX);
            b.rect.y = startY + row * (brickH + gapY);
            b.rect.width = brickW;
            b.rect.height = 22;
            b.destroyed = false;

            int c = GetRandomValue(0, 4);
            if (c == 0)
            {
                b.color = RED;
                b.score = 50;
            }
            else if (c == 1)
            {
                b.color = ORANGE;
                b.score = 40;
            }
            else if (c == 2)
            {
                b.color = YELLOW;
                b.score = 30;
            }
            else if (c == 3)
            {
                b.color = GREEN;
                b.score = 20;
            }
            else
            {
                b.color = BLUE;
                b.score = 10;
            }

            bricks.push_back(b);
        }
    }
}

void Game::ResetOneBall() {
    Ball b;
    b.x = screenWidth / 2;
    b.y = screenHeight / 2;
    b.dx = GetRandomValue(-3, 3);
    if (b.dx == 0) b.dx = 2;
    b.dy = -ballSpeed;
    b.active = true;
    balls.push_back(b);
}

void Game::ResetAllBalls() {
    balls.clear();
    ResetOneBall();
}

void Game::ResetGame() {
    ResetAllPowerUps();
    LoadLevel(currentLevel);
    ResetAllBalls();
    paddle.width = basePaddleW;
    paddle.height = 20;
    paddle.x = screenWidth / 2 - paddle.width / 2;
    paddle.y = screenHeight - 40;
}

bool Game::CheckWin() {
    for (auto& br : bricks)
        if (!br.destroyed)
            return false;
    return true;
}

const char* GetSaveFile(int lv) {
    switch (lv) {
        case 1: return "save1.json";
        case 2: return "save2.json";
        case 3: return "save3.json";
        case 4: return "save4.json";
        default: return "save.json";
    }
}

void Game::SaveProgress() {
    const char* f = GetSaveFile(currentLevel);
    FILE* file = fopen(f, "w");
    if (!file) return;
    fprintf(file, "{%d,%d,%d,", currentLevel, score, lives);
    for (int i = 0; i < bricks.size(); i++) {
        fprintf(file, "%d,", bricks[i].destroyed ? 1 : 0);
    }
    fprintf(file, "}");
    fclose(file);
}

bool Game::SaveExists(int lv) {
    FILE* f = fopen(GetSaveFile(lv), "r");
    if (!f) return false;
    fclose(f);
    return true;
}

void Game::LoadSave(int lv) {
    currentLevel = lv;
    FILE* f = fopen(GetSaveFile(lv), "r");
    if (!f) return;

    int l, s, li;
    fscanf(f, "{%d,%d,%d,", &l, &s, &li);
    score = s;
    lives = li;

    LoadLevel(lv);

    for (int i = 0; i < bricks.size(); i++) {
        int v;
        fscanf(f, "%d,", &v);
        bricks[i].destroyed = (v == 1);
    }
    fclose(f);

    ResetAllPowerUps();
    paddle.width = basePaddleW;
    paddle.height = 20;
    paddle.x = screenWidth / 2 - paddle.width / 2;
    paddle.y = screenHeight - 40;

    ResetAllBalls();
}

void Game::SpawnBorderParticles(float x, float y) {
    int count = GetRandomValue(12, 18);
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = {x, y};
        p.velocity.x = GetRandomValue(-40,40)/10.0f;
        p.velocity.y = GetRandomValue(-40,40)/10.0f;
        p.color = SKYBLUE;
        p.lifetime = GetRandomValue(40,80)/100.0f;
        particles.push_back(p);
    }
    flashTimer = 0.1f;
    borderFlashColor = WHITE;
}

void Game::SpawnBrickParticles(float x, float y, Color c) {
    int count = GetRandomValue(10,15);
    for (int i=0; i<count; i++) {
        Particle p;
        p.position = {x,y};
        p.velocity.x = GetRandomValue(-50,50)/10.0f;
        p.velocity.y = GetRandomValue(-50,50)/10.0f;
        p.color = c;
        p.lifetime = GetRandomValue(50,90)/100.0f;
        particles.push_back(p);
    }
}

void Game::UpdateParticles() {
    if (flashTimer>0) flashTimer -= GetFrameTime();
    for (int i=particles.size()-1; i>=0; i--) {
        auto& p = particles[i];
        p.position.x += p.velocity.x*GetFrameTime()*60;
        p.position.y += p.velocity.y*GetFrameTime()*60;
        p.lifetime -= GetFrameTime();
        p.color.a = (unsigned char)(p.lifetime*255);
        if (p.lifetime <=0) particles.erase(particles.begin()+i);
    }
}

void Game::DrawParticles() {
    if (flashTimer>0) {
        DrawRectangleLines(10,10,screenWidth-20,screenHeight-20,borderFlashColor);
    }
    for (auto& p : particles) {
        DrawCircleV(p.position, 2, p.color);
    }
}

void Game::SpawnPowerUp(float x, float y) {
    PowerUp pu;
    pu.pos = {x, y};
    pu.speed = 2.2f;
    pu.active = true;
    pu.type = GetRandomValue(0, 2);
    powerUps.push_back(pu);
}

void Game::UpdatePowerUps() {
    for (int i = powerUps.size()-1; i >= 0; i--) {
        auto& pu = powerUps[i];
        pu.pos.y += pu.speed;
        if (pu.pos.y > screenHeight) {
            powerUps.erase(powerUps.begin()+i);
            continue;
        }
        if (CheckCollisionCircleRec(pu.pos, 6, paddle)) {
            ActivatePowerUp(pu.type);
            powerUps.erase(powerUps.begin()+i);
        }
    }
}

void Game::DrawPowerUps() {
    for (auto& pu : powerUps) DrawCircleV(pu.pos, 6, PURPLE);
}

void Game::ActivatePowerUp(int type) {
    if (type == 0) {
        paddleTimer += 10.0f;
        paddle.width = basePaddleW * 1.6f;
    }
    if (type == 1) {
        multiTimer += 10.0f;
        SpawnThreeBalls();
    }
    if (type == 2) {
        slowTimer += 10.0f;
        ballSpeed = baseBallSpeed * 0.55f;
    }
}

void Game::UpdatePowerUpTimers() {
    if (paddleTimer > 0) paddleTimer -= GetFrameTime();
    else paddle.width = basePaddleW;

    if (slowTimer > 0) slowTimer -= GetFrameTime();
    else ballSpeed = baseBallSpeed;

    if (multiTimer > 0) multiTimer -= GetFrameTime();
}

void Game::DrawPowerUpHUD() {
    const char* t1 = "LONG";
    const char* t2 = "THREE";
    const char* t3 = "SLOW";

    Color c1 = (paddleTimer > 0) ? GREEN : DARKGRAY;
    Color c2 = (multiTimer > 0) ? GREEN : DARKGRAY;
    Color c3 = (slowTimer > 0) ? GREEN : DARKGRAY;

    DrawText(t1, 250, 550, 20, c1);
    DrawText(t2, 350, 550, 20, c2);
    DrawText(t3, 450, 550, 20, c3);
}

void Game::ResetAllPowerUps() {
    powerUps.clear();
    particles.clear();
    paddleTimer = slowTimer = multiTimer = 0;
    paddle.width = basePaddleW;
    ballSpeed = baseBallSpeed;
    balls.clear();
}

void Game::SpawnThreeBalls() {
    int cnt = GetActiveBallCount();
    if (cnt >= 3) return;
    int need = 3 - cnt;
    for (int i = 0; i < need; i++) {
        Ball b;
        b.x = screenWidth / 2;
        b.y = screenHeight / 2;
        if (i == 0) b.dx = -2.2f;
        else b.dx = 2.2f;
        b.dy = -ballSpeed * (0.95f + i*0.1f);
        b.active = true;
        balls.push_back(b);
    }
}

void Game::UpdateBalls() {
    for (int i = balls.size()-1; i >= 0; i--) {
        Ball& b = balls[i];
        if (!b.active) continue;

        b.x += b.dx;
        b.y += b.dy;

        // 撞墙 + 音效
        if (b.x < 10 || b.x > screenWidth - 10) {
            b.dx *= -1;
            PlaySound(soundWall);
            SpawnBorderParticles(b.x, b.y);
        }
        if (b.y < 10) {
            b.dy *= -1;
            PlaySound(soundWall);
            SpawnBorderParticles(b.x, b.y);
        }

        // 撞挡板 + 音效
        if (CheckCollisionCircleRec({b.x, b.y}, 10, paddle) && b.dy > 0) {
            b.dy = -b.dy;
            PlaySound(soundPaddle);
        }

        // 撞砖块 + 音效
        for (auto& br : bricks) {
            if (!br.destroyed && CheckCollisionCircleRec({b.x, b.y}, 10, br.rect)) {
                br.destroyed = true;
                score += br.score;
                b.dy *= -1;
                PlaySound(soundBrick);
                SpawnBrickParticles(br.rect.x+br.rect.width/2, br.rect.y+br.rect.height/2, br.color);
                
                int randVal = GetRandomValue(1, 100);
                if (randVal <= 30) {
                    SpawnPowerUp(br.rect.x+br.rect.width/2, br.rect.y+br.rect.height/2);
                }
                break;
            }
        }

        if (b.y > screenHeight - 10) {
            balls.erase(balls.begin() + i);
        }
    }

    if (GetActiveBallCount() == 0) {
        lives--;
        if (lives <= 0) {
            currentState = GAME_OVER;
        } else {
            ResetOneBall();
        }
    }
}

void Game::DrawBalls() {
    for (auto& b : balls) {
        DrawCircle((int)b.x, (int)b.y, 10, WHITE);
    }
}

int Game::GetActiveBallCount() {
    return balls.size();
}

void Game::DrawBackButton() {
    Rectangle btn = {screenWidth - 90, 10, 80, 35};
    DrawRectangleRec(btn, DARKBLUE);
    DrawRectangleLinesEx(btn, 2, WHITE);
    DrawText("BACK", screenWidth - 70, 18, 20, WHITE);
}

bool Game::CheckBackButtonClick() {
    Vector2 m = GetMousePosition();
    Rectangle btn = {screenWidth - 90, 10, 80, 35};
    return CheckCollisionPointRec(m, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void Game::Run() {
    InitWindow(screenWidth, screenHeight, "Breakout");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        if (currentState == PLAYING) {
            if (CheckBackButtonClick()) {
                currentState = MENU;
                ResetAllPowerUps();
                continue;
            }

            if (currentLevel == 4 && IsKeyPressed(KEY_R)) {
                GenerateRandomLevel4();
                ResetAllBalls();
            }

            UpdatePowerUpTimers();
            UpdatePowerUps();
            UpdateBalls();

            if (IsKeyPressed(KEY_SPACE)) {
                SaveProgress();
                currentState = MENU;
                ResetAllPowerUps();
            }
            if (IsKeyDown(KEY_LEFT)) paddle.x -= paddleSpeed;
            if (IsKeyDown(KEY_RIGHT)) paddle.x += paddleSpeed;
            if (paddle.x < 0) paddle.x = 0;
            if (paddle.x + paddle.width > screenWidth) paddle.x = screenWidth - paddle.width;

            if (CheckWin()) {
                if (currentLevel < 4) {
                    currentLevel++;
                    SaveProgress();
                    ResetGame();
                } else {
                    currentState = GAME_OVER;
                }
            }
            UpdateParticles();
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (currentState == PLAYING) {
            DrawBackButton();
        }

        if (currentState == MENU) {
            DrawText("BREAKOUT", 280, 70, 50, YELLOW);
            Rectangle bt1 = {100,200,230,50};
            Rectangle ct1 = {400,200,230,50};
            DrawRectangleRec(bt1,DARKGRAY); DrawText("LEVEL 1",150,215,25,WHITE);
            if (SaveExists(1)) { DrawRectangleRec(ct1,DARKBLUE); DrawText("CONTINUE 1",450,215,25,WHITE); }
            Rectangle bt2 = {100,280,230,50};
            Rectangle ct2 = {400,280,230,50};
            DrawRectangleRec(bt2,DARKGRAY); DrawText("LEVEL 2",150,295,25,WHITE);
            if (SaveExists(2)) { DrawRectangleRec(ct2,DARKBLUE); DrawText("CONTINUE 2",450,295,25,WHITE); }
            Rectangle bt3 = {100,360,230,50};
            Rectangle ct3 = {400,360,230,50};
            DrawRectangleRec(bt3,DARKGRAY); DrawText("LEVEL 3",150,375,25,WHITE);
            if (SaveExists(3)) { DrawRectangleRec(ct3,DARKBLUE); DrawText("CONTINUE 3",450,375,25,WHITE); }
            Rectangle bt4 = {100,440,230,50};
            Rectangle ct4 = {400,440,230,50};
            DrawRectangleRec(bt4,DARKGRAY); DrawText("LEVEL 4",150,455,25,WHITE);
            if (SaveExists(4)) { DrawRectangleRec(ct4,DARKBLUE); DrawText("CONTINUE 4",450,455,25,WHITE); }

            if (click && CheckCollisionPointRec(mouse,bt1)) {currentLevel=1;score=0;lives=3;ResetGame();currentState=PLAYING;}
            if (click && CheckCollisionPointRec(mouse,bt2)) {currentLevel=2;score=0;lives=3;ResetGame();currentState=PLAYING;}
            if (click && CheckCollisionPointRec(mouse,bt3)) {currentLevel=3;score=0;lives=3;ResetGame();currentState=PLAYING;}
            if (click && CheckCollisionPointRec(mouse,bt4)) {currentLevel=4;score=0;lives=3;ResetGame();currentState=PLAYING;}
            if (click && CheckCollisionPointRec(mouse,ct1)) {LoadSave(1);currentState=PLAYING;}
            if (click && CheckCollisionPointRec(mouse,ct2)) {LoadSave(2);currentState=PLAYING;}
            if (click && CheckCollisionPointRec(mouse,ct3)) {LoadSave(3);currentState=PLAYING;}
            if (click && CheckCollisionPointRec(mouse,ct4)) {LoadSave(4);currentState=PLAYING;}
        } else if (currentState == GAME_OVER) {
            DrawText("GAME OVER",250,150,50,RED);
            DrawText("PRESS ENTER",280,300,30,WHITE);
            if (IsKeyPressed(KEY_ENTER)) {
                currentState = MENU;
                ResetAllPowerUps();
            }
        } else {
            for (auto& br : bricks) if (!br.destroyed) DrawRectangleRec(br.rect, br.color);
            DrawRectangleRec(paddle, SKYBLUE);
            DrawBalls();
            DrawText(TextFormat("SCORE: %d",score),20,20,25,YELLOW);
            DrawText(TextFormat("LIVES: %d",lives),220,20,25,RED);
            DrawText(TextFormat("LEVEL: %d",currentLevel),420,20,25,GREEN);
            DrawPowerUpHUD();
            DrawParticles();
            DrawPowerUps();
        }
        EndDrawing();
    }
    CloseWindow();
}