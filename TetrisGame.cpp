/*
* Basic tetris game for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
*
* Written by: Jason Coon
* Copyright (c) 2014 Jason Coon
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "TetrisGame.h"
#include "Types.h"
#include "Codes.h"

TetrisGame::TetrisGame() {
}

TetrisGame::~TetrisGame(){/*nothing to destruct*/
}

void TetrisGame::reset() {
  newBlockIndex = 0;

  // generate new random bag of blocks
  for (int i = 0; i < 7; i++) {
    int r = random(i, 6);
    int temp = blockBag[i];
    blockBag[i] = blockBag[r];
    blockBag[r] = temp;
  }

  linesCleared = 0;
  sprintf(linesClearedText, "L:%d", linesCleared);

  score = 0;
  sprintf(scoreText, "S:%d", score);

  for (int y = FIELD_HEIGHT - 1; y >= 0; y--)
  {
    for (int x = 0; x < FIELD_WIDTH; x++)
    {
      pile[x][y] = 0;
      block[x][y] = 0;
    }
  }
}

void TetrisGame::setup() {
  isPaused = false;

  screenWidth = matrix->getScreenWidth();
  screenHeight = matrix->getScreenHeight();

  randomSeed(analogRead(5));

  // Turn off any text scrolling
  matrix->scrollText("", 1);
  matrix->setScrollMode(off);

  matrix->setColorCorrection(cc24);

  // Fonts are font3x5, font5x7, font6x10, font8x13
  matrix->setFont(font3x5);

  //screenWidth = matrix->getScreenWidth();
  //screenHeight = matrix->getScreenHeight();

  reset();

  newBlock();
}

unsigned long TetrisGame::handleInput() {
  unsigned long input = 0;

  decode_results results;

  results.value = 0;

  // Attempt to read an IR code ?
  if (irReceiver->decode(&results)) {
    input = results.value;

    // Prepare to receive the next IR code
    irReceiver->resume();
  }

  if (input == IRCODE_HOME) {
    return input;
  }
  else if (input == IRCODE_UP) {
    rotate();
  }
  else if (input == IRCODE_SEL) {
    isPaused = !isPaused;
  }
  else if (input == IRCODE_LEFT) {
    moveleft();
  }
  else if (input == IRCODE_RIGHT) {
    moveright();
  }

  // handle held (repeating) buttons
  bool isHeld = false;

  if (input == IRCODE_HELD) {
    input = lastInput;
    isHeld = true;
  }

  if (input != 0) {
    lastInput = input;
  }

  if (input == IRCODE_DOWN) {
    movedown();
  }

  return input;
}

void TetrisGame::check_gameover()
{
  int i;
  int j;
  int cnt = 0;
  int lineCount = 0;

  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    cnt = 0;
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (pile[j][i] > 0)
      {
        cnt++;
      }
    }
    if (cnt == FIELD_WIDTH)
    {
      lineCount++;
      // we have a solid line all the way across
      for (j = 0; j<FIELD_WIDTH; j++)
      {
        pile[j][i] = 0;
      }
      delay(50);

      int k;
      for (k = i; k>0; k--)
      {
        for (j = 0; j < FIELD_WIDTH; j++)
        {
          pile[j][k] = pile[j][k - 1];
        }
      }
      for (j = 0; j < FIELD_WIDTH; j++)
      {
        pile[j][0] = 0;
      }
      delay(50);
      i++;
    }
  }

  for (i = 0; i < FIELD_WIDTH; i++)
  {
    if (pile[i][0] > 0) {
      gameover();
      return;
    }
  }

  if (lineCount > 0) {
    linesCleared += lineCount;
    sprintf(linesClearedText, "L:%d", linesCleared);

    score += lineCount * lineCount;
    sprintf(scoreText, "S:%d", score);
  }
}

void TetrisGame::gameover()
{
  startTime = millis();

  delay(3000);

  reset();
}

bool TetrisGame::space_below()
{
  int i;
  int j;
  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (block[j][i] > 0)
      {
        if (i == FIELD_HEIGHT - 1)
          return false;
        if (pile[j][i + 1] > 0)
        {
          return false;
        }
      }
    }
  }
  return true;
}

bool TetrisGame::space_left()
{
  int i;
  int j;
  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (block[j][i] > 0)
      {
        if (j == 0)
          return false;
        if (pile[j - 1][i] > 0)
        {
          return false;
        }
      }
    }
  }
  return true;
}

bool TetrisGame::space_left2()
{
  int i;
  int j;
  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (block[j][i] > 0)
      {
        if (j == 0 || j == 1)
          return false;
        if (pile[j - 1][i] > 0 | pile[j - 2][i] > 0)
        {
          return false;
        }
      }
    }
  }
  return true;
}

bool TetrisGame::space_left3()
{
  int i;
  int j;
  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (block[j][i] > 0)
      {
        if (j == 0 || j == 1 || j == 2)
          return false;
        if (pile[j - 1][i] > 0 | pile[j - 2][i] > 0 | pile[j - 3][i] > 0)
        {
          return false;
        }
      }
    }
  }
  return true;
}

bool TetrisGame::space_right()
{
  int i;
  int j;
  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (block[j][i] > 0)
      {
        if (j == FIELD_WIDTH - 1)
          return false;
        if (pile[j + 1][i] > 0)
        {
          return false;
        }
      }
    }
  }
  return true;
}

bool TetrisGame::space_right2()
{
  int i;
  int j;
  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (block[j][i] > 0)
      {
        if (j == FIELD_WIDTH - 1 || j == FIELD_WIDTH - 2)
          return false;
        if (pile[j + 1][i] > 0 | pile[j + 2][i] > 0)
        {
          return false;
        }
      }
    }
  }
  return true;
}

bool TetrisGame::space_right3()
{
  int i;
  int j;
  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (block[j][i] > 0)
      {
        if (j == FIELD_WIDTH - 1 || j == FIELD_WIDTH - 2 || j == FIELD_WIDTH - 3)
          return false;
        if (pile[j + 1][i] > 0 | pile[j + 2][i] > 0 | pile[j + 3][i] > 0)
        {
          return false;
        }
      }
    }
  }
  return true;
}

bool TetrisGame::moveleft()
{
  if (space_left())
  {
    int i;
    int j;
    for (i = 0; i < FIELD_WIDTH - 1; i++)
    {
      for (j = 0; j < FIELD_HEIGHT; j++)
      {
        block[i][j] = block[i + 1][j];
      }
    }

    for (j = 0; j < FIELD_HEIGHT; j++)
    {
      block[FIELD_WIDTH - 1][j] = 0;
    }

    return 1;
  }

  return 0;
}

bool TetrisGame::moveright()
{
  if (space_right())
  {
    int i;
    int j;
    for (i = FIELD_WIDTH - 1; i > 0; i--)
    {
      for (j = 0; j < FIELD_HEIGHT; j++)
      {
        block[i][j] = block[i - 1][j];
      }
    }

    for (j = 0; j < FIELD_HEIGHT; j++)
    {
      block[0][j] = 0;
    }

    return 1;

  }
  return 0;
}

void TetrisGame::movedown()
{
  if (space_below())
  {
    //move down
    int i;
    for (i = FIELD_HEIGHT - 1; i >= 0; i--)
    {
      int j;
      for (j = 0; j < FIELD_WIDTH; j++)
      {
        block[j][i] = block[j][i - 1];
      }
    }
    for (i = 0; i < FIELD_WIDTH - 1; i++)
    {
      block[i][0] = 0;
    }
  }
  else
  {
    //merge and new block
    int i;
    int j;
    for (i = 0; i < FIELD_WIDTH; i++)
    {
      for (j = 0; j < FIELD_HEIGHT; j++)
      {
        if (block[i][j] > 0)
        {
          pile[i][j] = block[i][j];
          block[i][j] = 0;
        }
      }
    }
    newBlock();
  }
}

void TetrisGame::newBlock()
{
  check_gameover();

  blocktype = blockBag[newBlockIndex];

  newBlockIndex++;

  // need to generate a new bag of blocks?
  if (newBlockIndex == 7) {
    newBlockIndex = 0;

    for (int i = 0; i < 7; i++) {
      int r = random(i, 6);
      int temp = blockBag[i];
      blockBag[i] = blockBag[r];
      blockBag[r] = temp;
    }
  }

  nextBlockIndex = newBlockIndex;
  nextBlockType = blockBag[newBlockIndex];

  blockrotation = 0;

  if (blocktype == 0) // I
    // 0000
  {
    block[3][0] = blocktype + 1;
    block[4][0] = blocktype + 1;
    block[5][0] = blocktype + 1;
    block[6][0] = blocktype + 1;

    blockrotation = 1;
  }
  else if (blocktype == 1) // J
    // 0
    // 0 0 0
  {
    block[3][0] = blocktype + 1;
    block[3][1] = blocktype + 1;
    block[4][1] = blocktype + 1;
    block[5][1] = blocktype + 1;
  }
  else if (blocktype == 2) // L
    //     0
    // 0 0 0
  {
    block[5][0] = blocktype + 1;
    block[3][1] = blocktype + 1;
    block[4][1] = blocktype + 1;
    block[5][1] = blocktype + 1;
  }
  else if (blocktype == 3) // O
    // 0 0
    // 0 0
  {
    block[4][0] = blocktype + 1;
    block[4][1] = blocktype + 1;
    block[5][0] = blocktype + 1;
    block[5][1] = blocktype + 1;
  }
  else if (blocktype == 4) // S
    //   0 0
    // 0 0
  {
    block[4][0] = blocktype + 1;
    block[5][0] = blocktype + 1;
    block[3][1] = blocktype + 1;
    block[4][1] = blocktype + 1;
  }
  else if (blocktype == 5) // T
    //   0
    // 0 0 0
  {
    block[4][0] = blocktype + 1;
    block[3][1] = blocktype + 1;
    block[4][1] = blocktype + 1;
    block[5][1] = blocktype + 1;
  }
  else if (blocktype == 6) // Z
    // 0 0
    //   0 0
  {
    block[3][0] = blocktype + 1;
    block[4][0] = blocktype + 1;
    block[4][1] = blocktype + 1;
    block[5][1] = blocktype + 1;
  }

  // next block
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 6; x++) {
      nextBlock[x][y] = 0;
    }
  }

  if (nextBlockType == 0) // I
    // 0000
  {
    nextBlock[1][2] = nextBlockType + 1;
    nextBlock[2][2] = nextBlockType + 1;
    nextBlock[3][2] = nextBlockType + 1;
    nextBlock[4][2] = nextBlockType + 1;
  }
  else if (nextBlockType == 1) // J
    // 0
    // 0 0 0
  {
    nextBlock[1][1] = nextBlockType + 1;
    nextBlock[1][2] = nextBlockType + 1;
    nextBlock[2][2] = nextBlockType + 1;
    nextBlock[3][2] = nextBlockType + 1;
  }
  else if (nextBlockType == 2) // L
    //     0
    // 0 0 0
  {
    nextBlock[1][2] = nextBlockType + 1;
    nextBlock[2][2] = nextBlockType + 1;
    nextBlock[3][2] = nextBlockType + 1;
    nextBlock[3][1] = nextBlockType + 1;
  }
  else if (nextBlockType == 3) // O
    // 0 0
    // 0 0
  {
    nextBlock[1][1] = nextBlockType + 1;
    nextBlock[2][1] = nextBlockType + 1;
    nextBlock[1][2] = nextBlockType + 1;
    nextBlock[2][2] = nextBlockType + 1;
  }
  else if (nextBlockType == 4) // S
    //   0 0
    // 0 0
  {
    nextBlock[2][1] = nextBlockType + 1;
    nextBlock[3][1] = nextBlockType + 1;
    nextBlock[1][2] = nextBlockType + 1;
    nextBlock[2][2] = nextBlockType + 1;
  }
  else if (nextBlockType == 5) // T
    //   0
    // 0 0 0
  {
    nextBlock[1][2] = nextBlockType + 1;
    nextBlock[2][1] = nextBlockType + 1;
    nextBlock[2][2] = nextBlockType + 1;
    nextBlock[3][2] = nextBlockType + 1;
  }
  else if (nextBlockType == 6) // Z
    // 0 0
    //   0 0
  {
    nextBlock[1][1] = nextBlockType + 1;
    nextBlock[2][1] = nextBlockType + 1;
    nextBlock[2][2] = nextBlockType + 1;
    nextBlock[3][2] = nextBlockType + 1;
  }
}

bool TetrisGame::check_overlap()
{
  int i;
  int j;
  for (i = 0; i < FIELD_HEIGHT; i++)
  {
    for (j = 0; j < FIELD_WIDTH - 1; j++)
    {
      if (block[j][i] > 0)
      {
        if (pile[j][i])
          return false;
      }
    }
  }
  for (i = FIELD_HEIGHT; i < FIELD_HEIGHT + 2; i++)
  {
    for (j = 0; j < FIELD_WIDTH - 1; j++)
    {
      if (block[j][i] > 0)
      {
        return false;
      }
    }
  }
  return true;
}

void TetrisGame::rotate()
{
  //skip for square block(3)
  if (blocktype == 3) return;

  int xi;
  int yi;
  int i;
  int j;
  //detect left
  for (i = FIELD_WIDTH - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_HEIGHT; j++)
    {
      if (block[i][j] > 0)
      {
        xi = i;
      }
    }
  }

  //detect up
  for (i = FIELD_HEIGHT - 1; i >= 0; i--)
  {
    for (j = 0; j < FIELD_WIDTH; j++)
    {
      if (block[j][i] > 0)
      {
        yi = i;
      }
    }
  }

  if (blocktype == 0)
  {
    if (blockrotation == 0)
    {
      if (!space_left())
      {
        if (space_right3())
        {
          if (!moveright())
            return;
          xi++;
        }
        else return;
      }
      else if (!space_right())
      {
        if (space_left3())
        {
          if (!moveleft())
            return;
          if (!moveleft())
            return;
          xi--;
          xi--;
        }
        else
          return;
      }
      else if (!space_right2())
      {
        if (space_left2())
        {
          if (!moveleft())
            return;
          xi--;
        }
        else
          return;
      }

      block[xi][yi] = 0;
      block[xi][yi + 2] = 0;
      block[xi][yi + 3] = 0;

      block[xi - 1][yi + 1] = blocktype + 1;
      block[xi + 1][yi + 1] = blocktype + 1;
      block[xi + 2][yi + 1] = blocktype + 1;

      blockrotation = 1;
    }
    else
    {
      block[xi][yi] = 0;
      block[xi + 2][yi] = 0;
      block[xi + 3][yi] = 0;

      block[xi + 1][yi - 1] = blocktype + 1;
      block[xi + 1][yi + 1] = blocktype + 1;
      block[xi + 1][yi + 2] = blocktype + 1;

      blockrotation = 0;
    }
  }

  //offset to mid
  xi++;
  yi++;

  if (blocktype == 1)
  {
    if (blockrotation == 0)
    {
      block[xi - 1][yi - 1] = 0;
      block[xi - 1][yi] = 0;
      block[xi + 1][yi] = 0;

      block[xi][yi - 1] = blocktype + 1;
      block[xi + 1][yi - 1] = blocktype + 1;
      block[xi][yi + 1] = blocktype + 1;

      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi][yi - 1] = 0;
      block[xi + 1][yi - 1] = 0;
      block[xi][yi + 1] = 0;

      block[xi - 1][yi] = blocktype + 1;
      block[xi + 1][yi] = blocktype + 1;
      block[xi + 1][yi + 1] = blocktype + 1;

      blockrotation = 2;
    }
    else if (blockrotation == 2)
    {
      yi--;

      block[xi - 1][yi] = 0;
      block[xi + 1][yi] = 0;
      block[xi + 1][yi + 1] = 0;

      block[xi][yi - 1] = blocktype + 1;
      block[xi][yi + 1] = blocktype + 1;
      block[xi - 1][yi + 1] = blocktype + 1;

      blockrotation = 3;
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }
      block[xi][yi - 1] = 0;
      block[xi][yi + 1] = 0;
      block[xi - 1][yi + 1] = 0;

      block[xi - 1][yi - 1] = blocktype + 1;
      block[xi - 1][yi] = blocktype + 1;
      block[xi + 1][yi] = blocktype + 1;

      blockrotation = 0;
    }
  }

  if (blocktype == 2)
  {
    if (blockrotation == 0)
    {
      block[xi + 1][yi - 1] = 0;
      block[xi - 1][yi] = 0;
      block[xi + 1][yi] = 0;

      block[xi][yi - 1] = blocktype + 1;
      block[xi + 1][yi + 1] = blocktype + 1;
      block[xi][yi + 1] = blocktype + 1;

      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi][yi - 1] = 0;
      block[xi + 1][yi + 1] = 0;
      block[xi][yi + 1] = 0;

      block[xi - 1][yi] = blocktype + 1;
      block[xi + 1][yi] = blocktype + 1;
      block[xi - 1][yi + 1] = blocktype + 1;

      blockrotation = 2;
    }
    else if (blockrotation == 2)
    {
      yi--;

      block[xi - 1][yi] = 0;
      block[xi + 1][yi] = 0;
      block[xi - 1][yi + 1] = 0;

      block[xi][yi - 1] = blocktype + 1;
      block[xi][yi + 1] = blocktype + 1;
      block[xi - 1][yi - 1] = blocktype + 1;

      blockrotation = 3;
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }
      block[xi][yi - 1] = 0;
      block[xi][yi + 1] = 0;
      block[xi - 1][yi - 1] = 0;

      block[xi + 1][yi - 1] = blocktype + 1;
      block[xi - 1][yi] = blocktype + 1;
      block[xi + 1][yi] = blocktype + 1;

      blockrotation = 0;
    }
  }

  if (blocktype == 4)
  {
    if (blockrotation == 0)
    {
      block[xi + 1][yi - 1] = 0;
      block[xi - 1][yi] = 0;

      block[xi + 1][yi] = blocktype + 1;
      block[xi + 1][yi + 1] = blocktype + 1;

      blockrotation = 1;
    }
    else
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi + 1][yi] = 0;
      block[xi + 1][yi + 1] = 0;

      block[xi - 1][yi] = blocktype + 1;
      block[xi + 1][yi - 1] = blocktype + 1;

      blockrotation = 0;
    }
  }


  if (blocktype == 5)
  {
    if (blockrotation == 0)
    {
      block[xi][yi - 1] = 0;
      block[xi - 1][yi] = 0;
      block[xi + 1][yi] = 0;

      block[xi][yi - 1] = blocktype + 1;
      block[xi + 1][yi] = blocktype + 1;
      block[xi][yi + 1] = blocktype + 1;

      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi][yi - 1] = 0;
      block[xi + 1][yi] = 0;
      block[xi][yi + 1] = 0;

      block[xi - 1][yi] = blocktype + 1;
      block[xi + 1][yi] = blocktype + 1;
      block[xi][yi + 1] = blocktype + 1;

      blockrotation = 2;
    }
    else if (blockrotation == 2)
    {
      yi--;

      block[xi - 1][yi] = 0;
      block[xi + 1][yi] = 0;
      block[xi][yi + 1] = 0;

      block[xi][yi - 1] = blocktype + 1;
      block[xi - 1][yi] = blocktype + 1;
      block[xi][yi + 1] = blocktype + 1;

      blockrotation = 3;
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }
      block[xi][yi - 1] = 0;
      block[xi - 1][yi] = 0;
      block[xi][yi + 1] = 0;

      block[xi][yi - 1] = blocktype + 1;
      block[xi - 1][yi] = blocktype + 1;
      block[xi + 1][yi] = blocktype + 1;

      blockrotation = 0;
    }
  }

  if (blocktype == 6)
  {
    if (blockrotation == 0)
    {
      block[xi - 1][yi - 1] = 0;
      block[xi][yi - 1] = 0;

      block[xi + 1][yi - 1] = blocktype + 1;
      block[xi][yi + 1] = blocktype + 1;

      blockrotation = 1;
    }
    else
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi + 1][yi - 1] = 0;
      block[xi][yi + 1] = 0;

      block[xi - 1][yi - 1] = blocktype + 1;
      block[xi][yi - 1] = blocktype + 1;

      blockrotation = 0;
    }
  }

  //if rotating made block and pile overlap, push rows up
  while (!check_overlap())
  {
    for (i = 0; i < FIELD_HEIGHT + 2; i++)
    {
      for (j = 0; j < FIELD_WIDTH; j++)
      {
        block[j][i] = block[j][i + 1];
      }
    }
    delays = millis() + delay_;
  }
}

void TetrisGame::draw() {
  // Clear screen
  matrix->fillScreen(COLOR_BLACK);

  // draw border
  matrix->drawRectangle(10, 5, 21, 26, COLOR_GRAY);

  // draw score
  matrix->drawString(0, 0, COLOR_GRAY, scoreText);

  // draw lines cleared
  matrix->drawString(0, 27, COLOR_GRAY, linesClearedText);

  // draw next block
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 6; x++) {
      int b = nextBlock[x][y];

      if (b == 0)
        matrix->drawPixel(x + 23, y + 6, COLOR_GRAY);
      else
        matrix->drawPixel(x + 23, y + 6, blockColors[b]);
    }
  }

  int left = (screenWidth - FIELD_WIDTH) / 2;
  int top = (screenHeight - FIELD_HEIGHT) / 2;
  int right = screenWidth - left;
  int bottom = screenHeight - top;

  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      if (pile[x][y] > 0) {
        matrix->drawPixel(x + left, y + top, blockColors[pile[x][y]]);
      }
      else if (block[x][y] > 0) {
        matrix->drawPixel(x + left, y + top, blockColors[block[x][y]]);
      }
      else {
        matrix->drawPixel(x + left, y + top, COLOR_BLACK);
      }
    }
  }

  matrix->swapBuffers();
}

void TetrisGame::update() {
  delay(30);

  if (delays < millis())
  {
    delays = millis() + delay_;
    movedown();
  }
}

void TetrisGame::run(SmartMatrix matrixRef, IRrecv irReceiverRef) {
  matrix = &matrixRef;
  irReceiver = &irReceiverRef;

  setup();

  while (true) {
    unsigned long input = handleInput();

    if (input == IRCODE_HOME)
      return;

    if (!isPaused) {
      update();
    }

    draw();
  }
}