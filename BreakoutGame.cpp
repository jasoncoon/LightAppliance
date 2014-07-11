/*
* Basic breakout game for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
*
* Written by: Jason Coon
* Version: 1.2
* Last Update: 07/11/2014
*
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

#include "BreakoutGame.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

BreakoutGame::BreakoutGame() {
}

BreakoutGame::~BreakoutGame(){/*nothing to destruct*/ }

void BreakoutGame::resetBall() {
  ball.width = 1.0;
  ball.height = 1.0;
  ball.setLeft(11.0);
  ball.setTop(18.0);
  ball.speedX = 0.0625;
  ball.speedY = 0.125;
  ball.color = COLOR_WHITE;
}

void BreakoutGame::setup(SmartMatrix &matrix) {
  isPaused = true;

  randomSeed(analogRead(5));

  // Turn off any text scrolling
  matrix.scrollText("", 1);
  matrix.setScrollMode(off);

  matrix.setColorCorrection(cc24);

  // Clear screen
  matrix.fillScreen(COLOR_BLACK);

  // Fonts are font3x5, font5x7, font6x10, font8x13
  matrix.setFont(font3x5);

  resetBall();

  paddle.width = 8.0;
  paddle.height = 2.0;
  paddle.setLeft(16.0);
  paddle.setTop(29.0);
  paddle.color = COLOR_WHITE;

  screenWidth = matrix.getScreenWidth();
  screenHeight = matrix.getScreenHeight();

  int index = 0;
  for (float y = 0.0; y < 4.0; y++) {
    for (float x = 0.0; x < 8.0; x++) {
      Rect block;
      block.width = 4.0;
      block.height = 2.0;
      block.setLeft(x * 4.0);
      block.setTop(y * 2.0);
      block.inActive = false;
      switch (random(3))
      {
      case 0:
        block.color = COLOR_RED;
        break;
      case 1:
        block.color = COLOR_GREEN;
        break;
      case 2:
        block.color = COLOR_BLUE;
        break;
      }
      blocks[index] = block;
      char buffer[20];
      sprintf(buffer, "%d,%d,%d", index, x, y);
      Serial.println(buffer);
      index++;
    }
  }
}

unsigned long BreakoutGame::handleInput(IRrecv &irReceiver) {
  unsigned long input = 0;

  decode_results results;

  results.value = 0;

  // Attempt to read an IR code ?
  if (irReceiver.decode(&results)) {
    input = results.value;

    // Prepare to receive the next IR code
    irReceiver.resume();
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

  int16_t paddleNewLeft = paddle.left;


  if (input == IRCODE_HOME) {
    return input;
  }
  else if (!isHeld && input == IRCODE_SEL) {
    isPaused = !isPaused;
  }
  else if (!isHeld && input == IRCODE_A) {
    showPosition = !showPosition;
  }
  else if (input == IRCODE_LEFT) {
    paddleNewLeft -= 2.0;
  }
  else if (input == IRCODE_RIGHT) {
    paddleNewLeft += 2.0;
  }
  else if (input == IRCODE_UP) {
    ball.speedX *= 1.1;
    ball.speedY *= 1.1;
  }
  else if (input == IRCODE_DOWN) {
    ball.speedX *= .9;
    ball.speedY *= .9;
  }

  if (paddleNewLeft < 0)
    paddleNewLeft = 0;
  else if (paddle.right >= screenWidth)
    paddleNewLeft = screenWidth - paddle.width;

  paddle.setLeft(paddleNewLeft);

  return input;
}

void BreakoutGame::update() {
  bool collisionOnX = false;
  bool collisionOnY = false;

  // move the ball on the x axis
  ball.setLeft(ball.left + ball.speedX);

  // check for collisions on the x axis in the new position
  collisionOnX = ball.left <= -1 || ball.right >= screenWidth || ball.intersectsWith(paddle);

  if (!collisionOnX) {
    for (int index = 0; index < 32; index++) {
      Rect &block = blocks[index];
      if (block.inActive == true)
        continue;

      if (ball.intersectsWith(block)) {
        collisionOnX = true;
        block.inActive = true;
      }
    }
  }

  // we're testing for collisions on each axis independently, so
  // move the ball back to the old x position
  ball.setLeft(ball.left - ball.speedX);

  // move the ball on the y axis
  ball.setTop(ball.top + ball.speedY);

  ballFellOutBottom = ball.bottom >= 31.0;

  // check for collisions on the y axis in the new position
  collisionOnY = ball.top <= 0 || ball.bottom >= screenHeight - 1 || ball.intersectsWith(paddle);

  if (!collisionOnY) {
    for (int index = 0; index < 32; index++) {
      Rect &block = blocks[index];
      if (block.inActive == true)
        continue;

      if (ball.intersectsWith(block)) {
        collisionOnY = true;
        block.inActive = true;
      }
    }
  }

  // move the ball back on the y axis
  ball.setTop(ball.top - ball.speedY);

  if (ballFellOutBottom) {
    return;
  }

  // handle any collisions
  if (collisionOnX) {
    // reflect the ball on the x axis
    ball.speedX *= -1.0;
  }
  else {
  }

  if (collisionOnY) {
    // reflect the ball on the y axis
    ball.speedY *= -1.0;
  }
  else {
  }

  // move the ball to the new x position
  ball.setLeft(ball.left + ball.speedX);

  // move the ball to the new y position
  ball.setTop(ball.top + ball.speedY);
}

void BreakoutGame::draw(SmartMatrix &matrix) {
  matrix.fillScreen(COLOR_BLACK);

  if (showPosition) {
    // Format the position string
    sprintf(positionBuffer, "%d,%d", (int) ball.left, (int) ball.top);
    matrix.drawString(13, 0, COLOR_GREEN, positionBuffer);

    sprintf(positionBuffer, "%d,%d", (int) paddle.left, (int) paddle.top);
    matrix.drawString(13, 5, COLOR_GREEN, positionBuffer);
  }

  for (int index = 0; index < 32; index++) {
    Rect block = blocks[index];
    if (block.inActive == true)
      continue;
    matrix.drawRectangle(block.left, block.top, block.right, block.bottom, block.color);
  }

  matrix.drawRectangle(ball.left, ball.top, ball.right, ball.bottom, ball.color);
  matrix.drawRectangle(paddle.left, paddle.top, paddle.right, paddle.bottom, paddle.color);

  matrix.swapBuffers();
}

void BreakoutGame::run(SmartMatrix &matrix, IRrecv &irReceiver) {

  setup(matrix);

  while (true) {
    unsigned long input = handleInput(irReceiver);

    if (input == IRCODE_HOME)
      return;

    if (!isPaused) {
      update();

      if (ballFellOutBottom) {
        ballFellOutBottom = false;
        isPaused = true;
        resetBall();
      }

    }

    draw(matrix);
  }
}
