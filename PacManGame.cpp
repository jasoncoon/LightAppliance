/*
* Basic pac-man game for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
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

#include "PacManGame.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

PacManGame::PacManGame() {
}

PacManGame::~PacManGame(){/*nothing to destruct*/
}

void PacManGame::reset() {
  isPaused = true;

  resetGhosts();
  resetDots();
  resetPacman();
}

void PacManGame::resetPacman() {
  direction = NONE;
  pacman.x = 15;
  pacman.y = 23;
  pacman.lastMoveMillis = 0;
  pacman.moveSpeed = 150;
  pacman.lives = 2;

  lastMillis = 0;
}

void PacManGame::resetGhosts() {
  scatterDuration = 7000;
  scatterTimer = 0;
  timesScattered = 0;

  chaseDuration = 20000;
  chaseTimer = 0;

  mode = SCATTER;

  activeGhostCount = 0;

  for (int i = 0; i < 4; i++) {
    Ghost ghost = ghosts[i];
    ghost.isActive = false;
    ghost.hasExitedHome = false;
    ghost.color = ghostColors[i];
    ghost.moveSpeed = ghostSpeedNormal;
    ghost.mode = SCATTER;
    ghost.direction = NONE;

    if (i == BLINKY) {
      ghost.isActive = true;
      ghost.hasExitedHome = true;
      ghost.x = 15;
      ghost.y = 11;
      ghost.scatterTarget.x = 27;
      ghost.scatterTarget.y = 0;
      ghost.direction = RIGHT;
    }
    else if (i == INKY) {
      ghost.x = 14;
      ghost.y = 14;
      ghost.scatterTarget.x = 31;
      ghost.scatterTarget.y = 31;
    }
    else if (i == PINKY) {
      ghost.isActive = true;
      ghost.x = 15;
      ghost.y = 14;
      ghost.scatterTarget.x = 4;
      ghost.scatterTarget.y = 0;
    }
    else if (i == CLYDE) {
      ghost.x = 16;
      ghost.y = 14;
      ghost.scatterTarget.x = 0;
      ghost.scatterTarget.y = 31;
    }

    ghosts[i] = ghost;
  }
}

void PacManGame::resetDots() {
  int x = 0;
  int y = 0;

  int dotIndex = 0;

  // reset dots
  for (int i = 0; i < 1024; i++) {
    byte b = LEVEL1[i];

    bool isDot = b == (byte) 2;
    bool isEnergizer = b == (byte) 4;

    if (isDot || isEnergizer) {
      Dot dot;
      dot.color = isEnergizer ? COLOR_ENERGIZER : COLOR_DOT;
      dot.isActive = true;
      dot.isEnergizer = isEnergizer;
      dot.x = x;
      dot.y = y;
      dots[dotIndex] = dot;
      dotIndex++;
    }

    x++;
    if (x > 31) {
      x = 0;
      y++;
    }
  }

  eatenDotCount = 0;
  globalDotCounter = 0;
  globalDotCounterEnabled = false;
}

void PacManGame::setup() {
  isPaused = false;

  randomSeed(0);

  // Turn off any text scrolling
  matrix->scrollText("", 1);
  matrix->setScrollMode(off);

  matrix->setColorCorrection(cc24);

  // Fonts are font3x5, font5x7, font6x10, font8x13
  matrix->setFont(font3x5);

  screenWidth = matrix->getScreenWidth();
  screenHeight = matrix->getScreenHeight();

  ghostHome.x = 15;
  ghostHome.y = 15;

  score = 0;

  reset();
}

unsigned long PacManGame::handleInput() {
  unsigned long input = 0;

  decode_results results;

  results.value = 0;

  // Attempt to read an IR code ?
  if (irReceiver->decode(&results)) {
    input = results.value;

    // Prepare to receive the next IR code
    irReceiver->resume();
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

  DIRECTION desiredDirection = direction;
  int x = pacman.x;
  int y = pacman.y;

  if (input == IRCODE_HOME) {
    return input;
  }
  else if (!isHeld && input == IRCODE_SEL) {
    isPaused = !isPaused;
  }
  else if (input == IRCODE_LEFT) {
    desiredDirection = LEFT;
    x--;
    isPaused = false;
  }
  else if (input == IRCODE_RIGHT) {
    desiredDirection = RIGHT;
    x++;
    isPaused = false;
  }
  else if (input == IRCODE_UP) {
    desiredDirection = UP;
    y--;
    isPaused = false;
  }
  else if (input == IRCODE_DOWN) {
    desiredDirection = DOWN;
    y++;
    isPaused = false;
  }

  // check for collisions in desired direction
  rgb24 color = matrix->readPixel(x, y);
  if (!RGB24_ISEQUAL(color, COLOR_WALL) && !RGB24_ISEQUAL(color, COLOR_GHOST_HOME)) {
    direction = desiredDirection;
  }

  return input;
}

void PacManGame::update() {
  if (lastMillis == 0)
    lastMillis = millis();

  int elapsed = millis() - lastMillis;

  int elapsedSinceLastDotEaten = millis() - lastEatenDotMillis;

  if (mode == SCATTER) {
    scatterTimer += elapsed;

    if (scatterTimer > scatterDuration) {
      scatterTimer = 0;
      timesScattered++;
      mode = CHASE;
      for (int i = 0; i < 4; i++) {
        Ghost ghost = ghosts[i];
        ghost.mode = CHASE;
        ghosts[i] = ghost;
      }
    }
  }
  else if (mode == SCARED) {
    scaredTimer += elapsed;

    if (scaredTimer > scaredDuration) {
      scaredTimer = 0;
      mode = CHASE;
      pacman.moveSpeed = pacmanSpeedNormal;
      for (int i = 0; i < 4; i++) {
        Ghost ghost = ghosts[i];
        if (ghost.mode != DEAD) {
          ghost.mode = CHASE;
          ghost.color = mode == SCARED ? COLOR_GHOST_SCARED : ghostColors[i];
          ghost.moveSpeed = ghostSpeedNormal;
          ghosts[i] = ghost;
        }
      }
    }
  }
  else if (mode == CHASE) {
    chaseTimer += elapsed;
    if (chaseTimer > chaseDuration) {
      chaseTimer = 0;
      mode = SCATTER;
      if (timesScattered >= 2)
        scatterDuration = 5000;
      for (int i = 0; i < 4; i++) {
        Ghost ghost = ghosts[i];
        ghost.mode = SCATTER;
        ghosts[i] = ghost;
      }
    }
  }

  // move ghosts
  for (int i = 0; i < 4; i++){
    Ghost ghost = ghosts[i];

    if (!ghost.isActive) {
      if (i == PINKY && ((globalDotCounterEnabled && (globalDotCounter >= 17 && elapsedSinceLastDotEaten > 4000)) || !globalDotCounterEnabled)) {
        ghost.isActive = true;

        if (elapsedSinceLastDotEaten > 4000)
          lastEatenDotMillis = millis();
      }
      else if (i == INKY && ((globalDotCounterEnabled && (globalDotCounter >= 17 && elapsedSinceLastDotEaten > 4000)) || (!globalDotCounterEnabled && eatenDotCount >= 30))) {
        ghost.isActive = true;

        if (elapsedSinceLastDotEaten > 4000)
          lastEatenDotMillis = millis();
      }
      else if (i == CLYDE && ((globalDotCounterEnabled && (globalDotCounter >= 32 && elapsedSinceLastDotEaten > 4000)) || (!globalDotCounterEnabled && eatenDotCount >= 90))) {
        ghost.isActive = true;

        if (elapsedSinceLastDotEaten > 4000)
          lastEatenDotMillis = millis();
      }
      else {
        continue;
      }
    }

    int elapsed = millis() - ghost.lastMoveMillis;

    if (elapsed >= ghost.moveSpeed)
    {
      // move ghost in its predetermined direction
      moveGhost(ghost);

      if (ghost.mode == CHASE || ghost.mode == SCATTER) {
        ghost.color = ghostColors[i];
      }

      // plan next move
      Point target = ghostHome;

      if (!ghost.hasExitedHome) {
        target.x = 15;
        target.y = 11;
      }
      else if (ghost.mode == SCATTER) {
        target = ghost.scatterTarget;
      }
      else if (ghost.mode == SCARED) {
        Serial.println("setting target for scared ghost");
        target = ghostHome;
      }
      else if (ghost.mode == DEAD) {
        target = ghostHome;
      }
      else if (i == BLINKY) {
        // target pacman directly
        target.x = pacman.x;
        target.y = pacman.y;
      }
      else if (i == PINKY) {
        // target 4 places ahead of pacman
        if (direction == UP) {
          // original arcade bug, when pacman is going up, target up 4 and left 4
          target.x = pacman.x - 4;
          target.y = pacman.y - 4;
        }
        else if (direction == DOWN) {
          target.x = pacman.x;
          target.y = pacman.y + 4;
        }
        else if (direction == LEFT) {
          target.x = pacman.x - 4;
          target.y = pacman.y;
        }
        else if (direction == RIGHT) {
          target.x = pacman.x + 4;
          target.y = pacman.y;
        }
      }
      else if (i == INKY) {
        // get the point 2 places ahead of pacman
        if (direction == UP) {
          // original arcade bug, when pacman is going up, target up 2 and left 2
          target.x = pacman.x - 2;
          target.y = pacman.y - 2;
        }
        else if (direction == DOWN) {
          target.x = pacman.x;
          target.y = pacman.y + 2;
        }
        else if (direction == LEFT) {
          target.x = pacman.x - 2;
          target.y = pacman.y;
        }
        else if (direction == RIGHT) {
          target.x = pacman.x + 2;
          target.y = pacman.y;
        }

        // now get Blinky's location
        Ghost blinky = ghosts[BLINKY];

        // now get the vector from blinky to the target
        int vx = target.x - blinky.x;
        int vy = target.y - blinky.y;

        // and add it to the target point
        target.x += vx;
        target.y += vy;
      }
      else if (i == CLYDE) {
        double distanceToPacman = getDistance(ghost.x, ghost.y, pacman.x, pacman.y);
        if (distanceToPacman >= 8) {
          // target pacman directly
          target.x = pacman.x;
          target.y = pacman.y;
        }
        else {
          // target home
        }
      }

      planNextMove(ghost, target);

      ghost.lastMoveMillis = millis();

      ghosts[i] = ghost;
    }
  }

  elapsed = millis() - pacman.lastMoveMillis;

  if (elapsed >= pacman.moveSpeed)
  {
    int dx = 0;
    int dy = 0;

    // move pacman
    switch (direction)
    {
      case UP:
        dy = -1;
        break;
      case DOWN:
        dy = 1;
        break;
      case LEFT:
        dx = -1;
        break;
      case RIGHT:
        dx = 1;
        break;
    }
    pacman.x += dx;
    pacman.y += dy;

    // wrap pacman if it hits the edge of the level
    if (pacman.x > 29) {
      pacman.x = 2;
    }
    else if (pacman.x < 2) {
      pacman.x = 29;
    }

    // check for collisions with ghosts
    for (int i = 0; i < 4; i++) {
      Ghost ghost = ghosts[i];
      if (ghost.x == pacman.x && ghost.y == pacman.y) {
        if (ghost.mode == SCARED) {
          ghost.mode = DEAD;
          ghost.color = COLOR_GHOST_DEAD;
          ghost.moveSpeed = pacmanSpeedEnergized;
          ghosts[i] = ghost;
        }
        else if (ghost.mode == DEAD) {
          // pass right through
        }
        else {
          die();
          return;
        }
      }
    }

    // check for collisions with walls
    rgb24 color = matrix->readPixel(pacman.x, pacman.y);
    if (RGB24_ISEQUAL(color, COLOR_WALL) || RGB24_ISEQUAL(color, COLOR_GHOST_HOME)) {
      // stop & move back
      pacman.x -= dx;
      pacman.y -= dy;

      direction = NONE;
    }

    // hit a dot?
    for (int i = 0; i < DOT_COUNT; i++) {
      Dot dot = dots[i];
      if (!dot.isActive)
        continue;

      if (dot.x == pacman.x && dot.y == pacman.y) {
        dot.isActive = false;
        dots[i] = dot;
        eatenDotCount++;
        if (eatenDotCount == DOT_COUNT) {
          delay(1000);
          reset();
          score++;
          return;
        }

        if (dot.isEnergizer) {
          energize(); // !!!!!
        }

        if (globalDotCounterEnabled) {
          globalDotCounter++;
        }

        lastEatenDotMillis = millis();
      }
    }

    pacman.lastMoveMillis = millis();
  }

  lastMillis = millis();
}

void PacManGame::killGhost(Ghost &ghost) {
}

void PacManGame::energize() {
  mode = SCARED;
  pacman.moveSpeed = pacmanSpeedEnergized;
  scaredDuration = 6000;
  for (int i = 0; i < 4; i++) {
    Ghost ghost = ghosts[i];
    ghost.mode = SCARED;
    ghost.color = COLOR_GHOST_SCARED;
    ghost.moveSpeed = ghostSpeedScared;
    /*switch (ghost.direction)
    {
    case UP:
    ghost.direction = DOWN;
    break;
    case DOWN:
    ghost.direction = UP;
    break;
    case LEFT:
    ghost.direction = LEFT;
    break;
    case RIGHT:
    ghost.direction = RIGHT;
    break;
    }*/
    ghosts[i] = ghost;
  }
}

void PacManGame::planNextMove(Ghost &ghost, Point target) {
  double shortestDistance = 10000;
  DIRECTION bestDirection;

  if (ghost.mode == SCARED) {
    Serial.print("<!-- planning move for scared ghost moving in direction: ");
    Serial.println(ghost.direction);
    int r = random(4);

    while (true) {
      DIRECTION direction = directions[r];

      r++;
      if (r == 4)
      {
        r == 0;
        direction = directions[r];
      }

      Serial.print("checking direction for scared ghost: ");
      Serial.println(direction);

      // can't reverse direction
      if ((direction == UP && ghost.direction == DOWN) ||
        (direction == DOWN && ghost.direction == UP) ||
        (direction == LEFT && ghost.direction == RIGHT) ||
        (direction == RIGHT && ghost.direction == LEFT)) {
        Serial.println("can't reverse direction");
        continue;
      }

      int x = ghost.x;
      int y = ghost.y;

      switch (direction)
      {
        case UP:
          y--;
          break;
        case DOWN:
          y++;
          break;
        case LEFT:
          x--;
          break;
        case RIGHT:
          x++;
          break;
      }

      // wall?
      rgb24 color = matrix->readPixel(x, y);
      // can't target walls, or the ghost home unless the ghost is leaving home or dead (returning home)
      if (RGB24_ISEQUAL(color, COLOR_WALL) || (ghost.hasExitedHome && RGB24_ISEQUAL(color, COLOR_GHOST_HOME))) {
        Serial.print("direction is blocked by a wall: ");
        Serial.println(direction);
        continue;
      }
      else {
        // we've found our next move
        bestDirection = direction;
        break;
      }
    }

    Serial.print("chose direction for scared ghost: ");
    Serial.println(bestDirection);
    Serial.println("-->");
  }
  else {
    for (int i = 0; i < 4; i++) {
      DIRECTION direction = directions[i];

      // can't reverse direction
      if (direction == UP && ghost.direction == DOWN)
        continue;
      else if (direction == DOWN && ghost.direction == UP)
        continue;
      else if (direction == LEFT && ghost.direction == RIGHT)
        continue;
      else if (direction == RIGHT && ghost.direction == LEFT)
        continue;

      int x = ghost.x;
      int y = ghost.y;

      switch (direction)
      {
        case UP:
          y--;
          break;
        case DOWN:
          y++;
          break;
        case LEFT:
          x--;
          break;
        case RIGHT:
          x++;
          break;
      }

      // wall?
      rgb24 color = matrix->readPixel(x, y);
      if (RGB24_ISEQUAL(color, COLOR_WALL) || (ghost.mode != DEAD && ghost.hasExitedHome && RGB24_ISEQUAL(color, COLOR_GHOST_HOME))) {
        continue;
      } // special zone?
      else if (direction == UP && (y == 10 || y == 22) && (x == 14 || x == 17)) {
        continue;
      }

      double distance = getDistance(x, y, target.x, target.y);

      if (distance < shortestDistance) {
        bestDirection = direction;
        shortestDistance = distance;
      }
    }
  }

  ghost.direction = bestDirection;
}

double PacManGame::getDistance(int x1, int y1, int x2, int y2) {
  return sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
}

void PacManGame::moveGhost(Ghost &ghost) {
  int dx = 0;
  int dy = 0;

  switch (ghost.direction)
  {
    case UP:
      dy = -1;
      break;
    case DOWN:
      dy = 1;
      break;
    case LEFT:
      dx = -1;
      break;
    case RIGHT:
      dx = 1;
      break;
  }
  ghost.x += dx;
  ghost.y += dy;

  // wrap ghost if it hits the edge of the level
  if (ghost.x > 29) {
    ghost.x = 2;
  }
  else if (ghost.x < 2) {
    ghost.x = 29;
  }

  if (!ghost.hasExitedHome && ghost.x == 15 && ghost.y == 11) {
    ghost.hasExitedHome = true;
  }

  if (ghost.mode == DEAD && ghost.x == ghostHome.x && ghost.y == ghostHome.y) {
    ghost.mode = CHASE;
    ghost.moveSpeed = ghostSpeedNormal;
    ghost.hasExitedHome = false;
  }
}

void killGhost() {

}

void PacManGame::die() {
  delay(1000);

  isPaused = true;

  randomSeed(0);

  pacman.lives--;

  if (pacman.lives < 0) {
    reset();
    score = 0;
    return;
  }

  globalDotCounterEnabled = true;
  globalDotCounter = 0;

  scatterTimer = 0;
  chaseTimer = 0;
  mode = SCATTER;

  for (int i = 0; i < 4; i++) {
    Ghost ghost = ghosts[i];
    ghost.isActive = false;
    ghost.hasExitedHome = false;
    ghost.color = ghostColors[i];
    ghost.moveSpeed = ghostSpeedNormal;
    ghost.mode = CHASE;
    ghost.direction = NONE;

    if (i == BLINKY) {
      ghost.isActive = true;
      ghost.hasExitedHome = true;
      ghost.x = 15;
      ghost.y = 11;
      ghost.scatterTarget.x = 27;
      ghost.scatterTarget.y = 0;
      ghost.direction = RIGHT;
    }
    else if (i == INKY) {
      ghost.x = 14;
      ghost.y = 14;
      ghost.scatterTarget.x = 31;
      ghost.scatterTarget.y = 31;
    }
    else if (i == PINKY) {
      ghost.x = 15;
      ghost.y = 14;
      ghost.scatterTarget.x = 4;
      ghost.scatterTarget.y = 0;
    }
    else if (i == CLYDE) {
      ghost.x = 16;
      ghost.y = 14;
      ghost.scatterTarget.x = 0;
      ghost.scatterTarget.y = 31;
    }

    ghosts[i] = ghost;
  }

  // reset pacman
  direction = NONE;
  pacman.x = 15;
  pacman.y = 23;
  pacman.lastMoveMillis = 0;
  pacman.moveSpeed = 150;

  lastMillis = 0;
}

void PacManGame::draw() {
  // Clear screen
  matrix->fillScreen(COLOR_BLACK);

  int x = 0;
  int y = 0;

  // draw walls
  for (int i = 0; i < 1024; i++) {
    byte b = LEVEL1[i];
    if (b == (byte) 1) {
      matrix->drawPixel(x, y, COLOR_WALL);
    }
    else if (b == (byte) 3){
      matrix->drawPixel(x, y, COLOR_GHOST_HOME);
    }

    x++;
    if (x > 31) {
      x = 0;
      y++;
    }
  }

  // draw dots
  for (int i = 0; i < DOT_COUNT; i++) {
    Dot dot = dots[i];
    if (!dot.isActive)
      continue;
    matrix->drawPixel(dot.x, dot.y, dot.color);
  }

  // draw ghosts
  for (int i = 0; i < 4; i++) {
    drawGhost(ghosts[i]);
  }

  // draw pacman
  matrix->drawPixel(pacman.x, pacman.y, COLOR_PACMAN);

  // draw lives indicator
  for (int i = 0; i < pacman.lives; i++) {
    matrix->drawPixel(3 + (i * 2), 31, COLOR_PACMAN);
  }

  // draw score
  for (int i = 0; i < score; i++) {
    matrix->drawPixel(31 - i, 31, COLOR_WHITE);
  }

  matrix->swapBuffers();
}

void PacManGame::drawGhost(Ghost ghost) {
  matrix->drawPixel(ghost.x, ghost.y, ghost.color);
}

void PacManGame::run(SmartMatrix matrixRef, IRrecv irReceiverRef) {
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
