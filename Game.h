#ifndef GAME_H
#define GAME_H

#include <raylib.h>
#include <vector>

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float lifetime;
};

struct PowerUp {
    Vector2 pos;
    float speed;
    bool active;
    int type;
};

struct Ball {
    float x, y;
    float dx, dy;
    bool active;
};

struct Brick {
    Rectangle rect;
    bool destroyed;
    int score;
    Color color;
};

class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    void LoadLevel(int level);
    void ResetOneBall();
    void ResetAllBalls();
    void ResetGame();
    bool CheckWin();
    void SaveProgress();
    bool SaveExists(int lv);
    void LoadSave(int lv);

    void SpawnBorderParticles(float x, float y);
    void SpawnBrickParticles(float x, float y, Color c);
    void UpdateParticles();
    void DrawParticles();

    void SpawnPowerUp(float x, float y);
    void UpdatePowerUps();
    void DrawPowerUps();
    void ActivatePowerUp(int type);
    void UpdatePowerUpTimers();
    void DrawPowerUpHUD();
    void ResetAllPowerUps();

    void SpawnThreeBalls();
    void UpdateBalls();
    void DrawBalls();
    int  GetActiveBallCount();

    void DrawBackButton();
    bool CheckBackButtonClick();

    void GenerateRandomLevel4();

    // 动态生成音效
    Sound soundBrick;
    Sound soundPaddle;
    Sound soundWall;

    int currentLevel;
    int score;
    int lives;
    Rectangle paddle;
    std::vector<Ball> balls;
    std::vector<Brick> bricks;

    float paddleSpeed;
    float ballSpeed;
    float basePaddleW;
    float baseBallSpeed;

    float paddleTimer;
    float slowTimer;
    float multiTimer;

    enum GameState { MENU, PLAYING, GAME_OVER };
    GameState currentState;

    const int screenWidth = 800;
    const int screenHeight = 600;

    std::vector<Particle> particles;
    Color borderFlashColor;
    float flashTimer;
    std::vector<PowerUp> powerUps;
};

#endif