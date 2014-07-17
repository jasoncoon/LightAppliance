#ifndef _TETRISGAME_h
#define _TETRISGAME_h

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class TetrisGame
{
public:
  TetrisGame();
  ~TetrisGame();
  void run(SmartMatrix matrixRef, IRrecv irReceiverRef);

private:
  SmartMatrix *matrix;
  IRrecv *irReceiver;

  long delays = 0;
  short delay_ = 500;
  long bdelay = 0;
  short buttondelay = 150;
  short btdowndelay = 30;
  short btsidedelay = 80;
  unsigned char blocktype;
  unsigned char blockrotation;

  int16_t screenWidth;
  int16_t screenHeight;

  unsigned long lastInput = 0;

  static const int FIELD_WIDTH = 10;
  static const int FIELD_HEIGHT = 20;

  const rgb24 COLOR_BLACK = { 0, 0, 0 };
  const rgb24 COLOR_RED = { 255, 0, 0 };
  const rgb24 COLOR_GREEN = { 0, 255, 0 };
  const rgb24 COLOR_LGREEN = { 0, 80, 0 };
  const rgb24 COLOR_BLUE = { 0, 0, 255 };
  const rgb24 COLOR_WHITE = { 255, 255, 255 };
  const rgb24 COLOR_GRAY = { 127, 127, 127 };
  const rgb24 COLOR_CYAN = { 0, 255, 255 };
  const rgb24 COLOR_ORANGE = { 255, 165, 0 };
  const rgb24 COLOR_YELLOW = { 255, 255, 0 };
  const rgb24 COLOR_PURPLE = { 160, 32, 240 };

  rgb24 blockColors[8] = {
    COLOR_BLACK,  // 0 Blank
    COLOR_CYAN,   // 1 I
    COLOR_BLUE,   // 2 J
    COLOR_ORANGE, // 3 L
    COLOR_YELLOW, // 4 O
    COLOR_GREEN,  // 5 S
    COLOR_PURPLE, // 6 T
    COLOR_RED,    // 7 Z
  };

  int block[FIELD_WIDTH][FIELD_HEIGHT + 2]; //2 extra for rotation
  int pile[FIELD_WIDTH][FIELD_HEIGHT];

  unsigned long startTime;
  unsigned long elapsedTime;
  int cnt = 0;

  int newBlockIndex;
  int nextBlockIndex;
  int blockBag[7] = { 0, 1, 2, 3, 4, 5, 6 };
  int nextBlockType;
  int nextBlock[6][4];
  
  int score;
  char scoreText[8];

  int linesCleared;
  char linesClearedText[8];

  bool isPaused = false;

  bool moveleft();
  bool moveright();
  void movedown();
  void rotate();
  bool check_overlap();
  bool space_left();
  bool space_left2();
  bool space_left3();
  bool space_right();
  bool space_right2();
  bool space_right3();
  bool space_below();
  void newBlock();
  void check_gameover();
  void gameover();

  void reset();
  void setup();
  unsigned long handleInput();
  void update();
  void draw();
};

#endif

