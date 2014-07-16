#ifndef EndingGame_H
#define EndingGame_H

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class EndingGame{

private:
  enum DIRECTION {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3,
  };

  DIRECTION directions[4] = {
    UP, DOWN, LEFT, RIGHT,
  };

  struct Player {
    int16_t x;
    int16_t y;
  };

  struct Enemy {
    int16_t x;
    int16_t newX;

    int16_t y;
    int16_t newY;

    int index;
    int newIndex;

    bool isActive;

    int tileType;
    int newTileType;

    bool alreadyMoved;

    static const int STATIC_CRUSHER_LEFT = 5;
    static const int STATIC_CRUSHER_RIGHT = 6;
    static const int STATIC_CRUSHER_UP = 7;
    static const int STATIC_CRUSHER_DOWN = 8;
    static const int STATIC_CRUSHER_LEFT_RIGHT = 9;
    static const int STATIC_CRUSHER_UP_DOWN = 10;
    static const int MOBILE_CRUSHER_LEFT = 11;
    static const int MOBILE_CRUSHER_RIGHT = 12;
    static const int MOBILE_CRUSHER_UP = 13;
    static const int MOBILE_CRUSHER_DOWN = 14;
    static const int SPAWNER_MOBILE_CRUSHER_LEFT = 15;
    static const int SPAWNER_MOBILE_CRUSHER_RIGHT = 16;
    static const int SPAWNER_MOBILE_CRUSHER_UP = 17;
    static const int SPAWNER_MOBILE_CRUSHER_DOWN = 18;

    int mobileCrusherDirections[4] = {
      MOBILE_CRUSHER_UP,
      MOBILE_CRUSHER_DOWN,
      MOBILE_CRUSHER_LEFT,
      MOBILE_CRUSHER_RIGHT,
    };

    int spawnerMobileCrusherDirections[4] = {
      SPAWNER_MOBILE_CRUSHER_UP,
      SPAWNER_MOBILE_CRUSHER_DOWN,
      SPAWNER_MOBILE_CRUSHER_LEFT,
      SPAWNER_MOBILE_CRUSHER_RIGHT,
    };

    static bool isEnemy(int tileType) {
      return tileType > 4;
    }

    bool isEnemy() {
      return Enemy::isEnemy(tileType);
    }

    bool isMobile() {
      switch (tileType) {
      case MOBILE_CRUSHER_LEFT:
      case MOBILE_CRUSHER_RIGHT:
      case MOBILE_CRUSHER_UP:
      case MOBILE_CRUSHER_DOWN:
        return true;
      }

      return false;
    }

    int turn(int direction) {
      switch (tileType) {
        case MOBILE_CRUSHER_LEFT:
        case MOBILE_CRUSHER_RIGHT:
        case MOBILE_CRUSHER_UP:
        case MOBILE_CRUSHER_DOWN:
          return mobileCrusherDirections[direction];

        case SPAWNER_MOBILE_CRUSHER_LEFT:
        case SPAWNER_MOBILE_CRUSHER_RIGHT:
        case SPAWNER_MOBILE_CRUSHER_UP:
        case SPAWNER_MOBILE_CRUSHER_DOWN:
          return spawnerMobileCrusherDirections[direction];
      }

      return tileType;
    }

    int getDirection() {
      switch (tileType) {
      case STATIC_CRUSHER_LEFT:
      case MOBILE_CRUSHER_LEFT:
        return LEFT;

      case STATIC_CRUSHER_RIGHT:
      case MOBILE_CRUSHER_RIGHT:
        return RIGHT;

      case STATIC_CRUSHER_UP:
      case MOBILE_CRUSHER_UP:
        return UP;

      case STATIC_CRUSHER_DOWN:
      case MOBILE_CRUSHER_DOWN:
        return DOWN;
      }

      return DOWN;
    }

    bool isFacingDirection(DIRECTION direction) {
      switch (tileType) {
      case STATIC_CRUSHER_LEFT:
      case MOBILE_CRUSHER_LEFT:
        return direction == LEFT;

      case STATIC_CRUSHER_RIGHT:
      case MOBILE_CRUSHER_RIGHT:
        return direction == RIGHT;

      case STATIC_CRUSHER_LEFT_RIGHT:
        return direction == LEFT || direction == RIGHT;
        break;

      case STATIC_CRUSHER_UP:
      case MOBILE_CRUSHER_UP:
        return direction == UP;

      case STATIC_CRUSHER_DOWN:
      case MOBILE_CRUSHER_DOWN:
        return direction == DOWN;

      case STATIC_CRUSHER_UP_DOWN:
        return direction == UP || direction == DOWN;
      }

      return false;
    }
  };

  Enemy enemies[81];

  Player player;
  bool isPlayerDead;
  bool isPlayerFinished;

  int16_t screenWidth;
  int16_t screenHeight;

  double getDistance(int x1, int y1, int x2, int y2);

  void reset(SmartMatrix &matrix);
  void setup(SmartMatrix &matrix);
  unsigned long handleInput(IRrecv &irReceiver);
  void update(SmartMatrix &matrix);
  void die();
  void loadLevel(int levelIndex);

  void draw(SmartMatrix &matrix);
  void drawWall(SmartMatrix &matrix, int x, int y);
  void drawPlayer(SmartMatrix &matrix, int x, int y);
  void drawBlock(SmartMatrix &matrix, int x, int y);

  void drawEnd(SmartMatrix &matrix, int x, int y);

  void drawStaticCrusherLeft(SmartMatrix &matrix, int x, int y);
  void drawStaticCrusherRight(SmartMatrix &matrix, int x, int y);
  void drawStaticCrusherUp(SmartMatrix &matrix, int x, int y);
  void drawStaticCrusherDown(SmartMatrix &matrix, int x, int y);

  void drawMobileCrusherLeft(SmartMatrix &matrix, int x, int y);
  void drawMobileCrusherRight(SmartMatrix &matrix, int x, int y);
  void drawMobileCrusherUp(SmartMatrix &matrix, int x, int y);
  void drawMobileCrusherDown(SmartMatrix &matrix, int x, int y);

  void drawSpawnerMobileCrusherLeft(SmartMatrix &matrix, int x, int y);
  void drawSpawnerMobileCrusherRight(SmartMatrix &matrix, int x, int y);
  void drawSpawnerMobileCrusherUp(SmartMatrix &matrix, int x, int y);
  void drawSpawnerMobileCrusherDown(SmartMatrix &matrix, int x, int y);

  bool isEnemy(int tileType);
  bool isMobile(int tileType);
  int turnEnemy(int tileType, int direction);
  int getDirection(int tileType);
  bool isFacingDirection(int tileType, DIRECTION direction);

  bool move(int newX, int newY);
  void moveEnemies();

  static const int EMPTY = 0;
  static const int WALL = 1;
  static const int PLAYER = 2;
  static const int BLOCK = 3;
  static const int END = 4;

  int currentLevelIndex = 0;

  int currentLevel[81];

  static const int LEVEL_COUNT = 6;

  int levels[LEVEL_COUNT][81] = {
      { // level 0
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 3, 0, 0, 0, 1,
        1, 2, 0, 0, 3, 0, 1, 0, 4,
        1, 0, 0, 0, 3, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
      },
      { // level 1
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 1, 0, 0, 8, 0, 0, 0, 1,
        0, 1, 0, 2, 0, 0, 10, 0, 4,
        0, 1, 0, 0, 7, 0, 0, 0, 1,
        0, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
      },
      { // level 2
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 0, 0,
        0, 1, 11, 11, 0, 0, 1, 0, 0,
        0, 1, 14, 1, 0, 0, 1, 0, 0,
        0, 1, 2, 1, 0, 0, 4, 0, 0,
        0, 1, 14, 1, 0, 0, 1, 0, 0,
        0, 1, 12, 12, 0, 0, 1, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
      },
      { // level 3
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 0, 0, 0, 0, 11, 1, 0,
        1, 1, 0, 1, 0, 0, 0, 1, 1,
        1, 2, 0, 0, 0, 0, 0, 11, 4,
        1, 1, 0, 1, 0, 0, 0, 1, 1,
        0, 1, 0, 0, 0, 0, 11, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
      },
      { // level 4
         0,  0,  1,  1,  1,  1, 1, 0, 0,
         0,  0,  1, 14,  8, 14, 0, 0, 0,
         0,  0,  1,  0,  0,  0, 1, 0, 0,
         0,  0,  1,  0,  2,  0, 1, 0, 0,
         0,  0,  1,  0,  0,  0, 1, 0, 0,
         0,  0,  1, 13,  7, 13, 1, 0, 0,
         0,  0,  1,  0,  0,  0, 1, 0, 0,
         0,  0,  1,  0,  0,  0, 1, 0, 0,
         0,  0,  1,  1,  4,  1, 1, 0, 0,
      },
      { // level 5
         1,  1,  1,  1,  1,  0,  0,  0,  0,
         1,  0,  0,  0,  1,  0,  0,  0,  0,
         1,  0,  2,  0,  1,  0,  0,  0,  0,
         1,  0,  0,  0,  1,  0,  0,  0,  0,
         1,  0,  0,  0,  1,  1,  1,  1,  1,
         1,  0,  0,  0, 11,  0,  0,  0,  1,
         1,  0,  0,  0,  0,  0, 15,  0,  4,
         1,  0, 13,  0,  0,  0,  0,  0,  1,
         1,  1,  1,  1,  1,  1,  1,  1,  1,
      },
  };

public:
  EndingGame();
  ~EndingGame();
  void run(SmartMatrix &matrix, IRrecv &irReceiver);
};

#endif