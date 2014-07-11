#ifndef PacManGame_H
#define PacManGame_H

#include <QueueArray.h>

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class PacManGame{

public:
  PacManGame();
  ~PacManGame();
  void run(SmartMatrix matrix, IRrecv irReceiver);

private:
  SmartMatrix *matrix;
  IRrecv *irReceiver;

  enum DIRECTION {
    UP, LEFT, DOWN, RIGHT, NONE,
  };

  DIRECTION directions[4] = {
    UP, LEFT, DOWN, RIGHT,
  };

  enum MODE {
    SCATTER, CHASE, SCARED, DEAD,
  };

  MODE mode;

  struct Point {
    int16_t x;
    int16_t y;
  };

  struct PacMan {
    int16_t x;
    int16_t y;
    int lastMoveMillis;
    int moveSpeed = 150;
    int lives;
  };

  int activeGhostCount = 0;

  struct Ghost {
    int16_t x;
    int16_t y;
    rgb24 color;
    bool isActive;
    bool hasExitedHome;
    int inactiveTimer;
    DIRECTION direction;
    Point scatterTarget;
    int moveSpeed;
    int lastMoveMillis;
    MODE mode;
  };

  Point ghostHome;

  int pacmanSpeedEnergized = 135;
  int pacmanSpeedNormal = 150;
  int ghostSpeedNormal = 160;
  int ghostSpeedScared = 240;

  // 120   - 100%
  // 135   -  90% - pacman energized
  // 150   -  80% - pacman normal
  // 160   -  75% - ghost normal
  // 240   -  50% - ghost scared

  Ghost ghosts[4];

  const int BLINKY = 0;
  const int INKY = 1;
  const int PINKY = 2;
  const int CLYDE = 3;

  const rgb24 COLOR_BLINKY = { 255, 0, 0 };
  const rgb24 COLOR_INKY = { 0, 255, 255 };
  const rgb24 COLOR_PINKY = { 255, 184, 255 };
  const rgb24 COLOR_CLYDE = { 255, 184, 81 };
  const rgb24 COLOR_GHOST_SCARED = { 33, 33, 255 };
  const rgb24 COLOR_GHOST_DEAD = { 255, 255, 255 };

  rgb24 ghostColors[4] = {
    COLOR_BLINKY,
    COLOR_INKY,
    COLOR_PINKY,
    COLOR_CLYDE,
  };

  const rgb24 COLOR_PACMAN = { 255, 255, 0 };
  const rgb24 COLOR_WALL = { 33, 33, 255 };
  const rgb24 COLOR_GHOST_HOME = { 1, 0, 0 };
  const rgb24 COLOR_DOT = { 64, 64, 64 };
  const rgb24 COLOR_ENERGIZER = { 0, 255, 33 };

  DIRECTION direction = RIGHT;

  int16_t screenWidth;
  int16_t screenHeight;

  unsigned long lastInput = 0;

  bool isPaused = false;

  PacMan pacman;

  struct Dot {
    int16_t x;
    int16_t y;
    rgb24 color;
    bool isEnergizer;
    bool isActive;
  };

  int scatterTimer;
  int scatterDuration;

  int scaredTimer;
  int scaredDuration;

  static const int DOT_COUNT = 244;
  Dot dots[DOT_COUNT];
  int eatenDotCount = 0;

  int lastMillis;

  void reset();
  void setup();
  unsigned long handleInput();
  void update();
  void draw();
  void die();
  void drawGhost(Ghost ghost);
  void moveGhost(Ghost &ghost);
  void killGhost(Ghost &ghost);
  void planNextMove(Ghost &ghost, Point target);
  double getDistance(int x1, int y1, int x2, int y2);
  void energize();

  const byte LEVEL1[1024] = {
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0, 0,
    0, 0, 1, 4, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 4, 1, 0, 0,
    0, 0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0, 0,
    0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0, 0,
    0, 0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0, 0,
    0, 0, 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 1, 1, 1, 3, 3, 1, 1, 1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 3, 3, 3, 3, 3, 3, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0, 0,
    0, 0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0, 0,
    0, 0, 1, 4, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 4, 1, 0, 0,
    0, 0, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0, 0,
    0, 0, 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 0, 0,
    0, 0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 0, 0,
    0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
};
#endif
