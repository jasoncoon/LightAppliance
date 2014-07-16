#ifndef EndingGame_H
#define EndingGame_H

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class EndingGame{

private:
  enum DIRECTION {
    UP    = 0, 
    DOWN  = 1,
    LEFT  = 2,
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
    int16_t y;
    int index;
    bool isActive;
    int tileType;
    bool alreadyMoved;
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

  int mobileCrusherDirections[4] = {
    MOBILE_CRUSHER_UP,
    MOBILE_CRUSHER_DOWN,
    MOBILE_CRUSHER_LEFT,
    MOBILE_CRUSHER_RIGHT,
  };

  int currentLevelIndex = 0;

  int currentLevel[81];

  static const int LEVEL_COUNT = 3;

  int levels[LEVEL_COUNT][81] = {
    { // level 0
       0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,
       1,  1,  1,  1,  1,  1,  1,  1,  1,
       1,  0,  0,  0,  3,  0,  0,  0,  1,
       1,  2,  0,  0,  3,  0,  1,  0,  4,
       1,  0,  0,  0,  3,  0,  0,  0,  1,
       1,  1,  1,  1,  1,  1,  1,  1,  1,
       0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,
    },
    { // level 1
       0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  1,  1,  1,  1,  1,  1,  1,  1,
       0,  1,  0,  0,  8,  0,  0,  0,  1,
       0,  1,  0,  2,  0,  0, 10,  0,  4,
       0,  1,  0,  0,  7,  0,  0,  0,  1,
       0,  1,  1,  1,  1,  1,  1,  1,  1,
       0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,
    },
    { // level 2
       0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  1,  1,  1,  1,  1,  1,  0,  0,
       0,  1, 11, 11,  0,  0,  1,  0,  0,
       0,  1, 14,  1,  0,  0,  1,  0,  0,
       0,  1,  2,  1,  0,  0,  4,  0,  0,
       0,  1, 14,  1,  0,  0,  1,  0,  0,
       0,  1, 12, 12,  0,  0,  1,  0,  0,
       0,  1,  1,  1,  1,  1,  1,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,
    },
  };

public:
  EndingGame();
  ~EndingGame();
  void run(SmartMatrix &matrix, IRrecv &irReceiver);
};

#endif
