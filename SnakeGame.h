#ifndef SnakeGame_H
#define SnakeGame_H

#include <QueueArray.h>

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class SnakeGame{

private:
  enum DIRECTION {
    UP, DOWN, LEFT, RIGHT
  };

  struct Point {
    int16_t x;
    int16_t y;
  };

  DIRECTION direction = RIGHT;

  int16_t screenWidth;
  int16_t screenHeight;
  char positionBuffer[5]; // XX,YY
  bool isPaused = false;
  
  Point snakeHead;
  Point apple;
  QueueArray<Point> segments;
  int segmentCount = 1;
  int maxSegmentCount = 1024;
  int segmentIncrement = 1;
  int segmentIncrementMultiplier = 1;
  
  unsigned long lastMillis = 0;
  int moveSpeed = 150;

  void reset(SmartMatrix &matrix);
  void setup(SmartMatrix &matrix);
  unsigned long handleInput(IRrecv &irReceiver);
  void update(SmartMatrix &matrix);
  void draw(SmartMatrix &matrix);
  void newApple(SmartMatrix &matrix);
  void die(SmartMatrix &matrix);
  
public:
  SnakeGame();
  ~SnakeGame();
  void run(SmartMatrix &matrix, IRrecv &irReceiver);
};

#endif
