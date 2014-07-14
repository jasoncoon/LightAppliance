/*
 * Basic snake game for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
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

#include "SnakeGame.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

SnakeGame::SnakeGame() {
}

SnakeGame::~SnakeGame(){/*nothing to destruct*/
}

void SnakeGame::reset(SmartMatrix &matrix) {
  // Clear screen
  matrix.fillScreen(COLOR_BLACK);

  newApple(matrix);

  segmentCount = 4;
  segmentIncrement = 4;
  segmentIncrementMultiplier = 1;

  while (segments.count() > 0) {
    segments.dequeue();
  }

  snakeHead.x = 16;
  snakeHead.y = 16;

  direction = RIGHT;

  segments.enqueue(snakeHead);

  matrix.drawPixel(snakeHead.x, snakeHead.y, COLOR_GREEN);
}

void SnakeGame::newApple(SmartMatrix &matrix) {
  while (true) {
    apple.x = random(32);
    apple.y = random(32);

    rgb24 color = matrix.readPixel(apple.x, apple.y);
    if (RGB24_ISEQUAL(color, COLOR_BLACK))
      break;
  }

  matrix.drawPixel(apple.x, apple.y, COLOR_RED);
}

void SnakeGame::setup(SmartMatrix &matrix) {
  isPaused = false;

  randomSeed(analogRead(5));

  // Turn off any text scrolling
  matrix.scrollText("", 1);
  matrix.setScrollMode(off);

  matrix.setColorCorrection(cc24);

  // Fonts are font3x5, font5x7, font6x10, font8x13
  matrix.setFont(font3x5);

  screenWidth = matrix.getScreenWidth();
  screenHeight = matrix.getScreenHeight();

  reset(matrix);
}

unsigned long SnakeGame::handleInput(IRrecv &irReceiver) {
  unsigned long input = 0;

  decode_results results;

  results.value = 0;

  // Attempt to read an IR code ?
  if (irReceiver.decode(&results)) {
    input = results.value;

    // Prepare to receive the next IR code
    irReceiver.resume();
  }

  if (input == IRCODE_HOME) {
    return input;
  }
  else if (input == IRCODE_SEL) {
    isPaused = !isPaused;
  }
  else if (input == IRCODE_LEFT && direction != RIGHT) {
    direction = LEFT;
  }
  else if (input == IRCODE_RIGHT && direction != LEFT) {
    direction = RIGHT;
  }
  else if (input == IRCODE_UP && direction != DOWN) {
    direction = UP;
  }
  else if (input == IRCODE_DOWN && direction != UP) {
    direction = DOWN;
  }

  return input;
}

void SnakeGame::update(SmartMatrix &matrix) {
  if (millis() - lastMillis >= moveSpeed)
  {
    Point newSnakeHead;
    newSnakeHead.x = snakeHead.x;
    newSnakeHead.y = snakeHead.y;

    // move the snake
    switch (direction)
    {
    case UP:
      newSnakeHead.y--;
      break;
    case DOWN:
      newSnakeHead.y++;
      break;
    case LEFT:
      newSnakeHead.x--;
      break;
    case RIGHT:
      newSnakeHead.x++;
      break;
    }

    // wrap the snake if it hits the edge of the screen (for now)
    if (newSnakeHead.x >= screenWidth) {
      newSnakeHead.x = 0;
    }
    else if (newSnakeHead.x < 0) {
      newSnakeHead.x = screenWidth - 1;
    }

    if (newSnakeHead.y >= screenHeight) {
      newSnakeHead.y = 0;
    }
    else if (newSnakeHead.y < 0) {
      newSnakeHead.y = screenHeight - 1;
    }

    rgb24 color = matrix.readPixel(newSnakeHead.x, newSnakeHead.y);
    if (RGB24_ISEQUAL(color, COLOR_GREEN)) {
      // snake ate itself
      die(matrix);
    }

    segments.enqueue(newSnakeHead);

    // draw the new location for the snake head
    matrix.drawPixel(newSnakeHead.x, newSnakeHead.y, COLOR_GREEN);

    if (newSnakeHead.x == apple.x && newSnakeHead.y == apple.y) {
      segmentCount += segmentIncrement * segmentIncrementMultiplier;

      if (segmentCount > maxSegmentCount) {
        segmentCount = maxSegmentCount;
      }
      newApple(matrix);
    }

    // trim the end of the snake if it gets too long
    while (segments.count() > segmentCount) {
      Point oldSnakeSegment = segments.dequeue();
      matrix.drawPixel(oldSnakeSegment.x, oldSnakeSegment.y, COLOR_BLACK);
    }

    snakeHead = newSnakeHead;

    lastMillis = millis();
  }
}

void SnakeGame::die(SmartMatrix &matrix) {
  delay(1000);
  reset(matrix);
}

void SnakeGame::draw(SmartMatrix &matrix) {
  matrix.swapBuffers();
}

void SnakeGame::run(SmartMatrix &matrix, IRrecv &irReceiver) {

  setup(matrix);

  while (true) {
    unsigned long input = handleInput(irReceiver);

    if (input == IRCODE_HOME)
      return;

    if (!isPaused) {
      update(matrix);
    }

    draw(matrix);
  }
}

