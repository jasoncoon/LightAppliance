/*
 * Game based on the excellent 'Ending' by Aaron Steed:
 * http://robotacid.com/flash/ending/
 * Reworked for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
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

#include "EndingGame.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

EndingGame::EndingGame() {
}

EndingGame::~EndingGame(){/*nothing to destruct*/
}

void EndingGame::reset(SmartMatrix &matrix) {
  currentLevelIndex = 2;

  loadLevel(currentLevelIndex);
}

void EndingGame::loadLevel(int levelIndex) {
  Serial.println("loading next level...");

  int i = 0;
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      currentLevel[i] = EMPTY;

      enemies[i].isActive = false;

      int tileType = levels[levelIndex][i];

      if (tileType == PLAYER) {
        player.x = x;
        player.y = y;
        currentLevel[i] = tileType;
      }
      else if (isEnemy(tileType)) {
        Enemy enemy;
        enemy.x = x;
        enemy.y = y;
        enemy.index = i;
        enemy.tileType = tileType;
        enemy.isActive = true;
        enemies[i] = enemy;
      }
      else {
        currentLevel[i] = tileType;
      }

      i++;
    }
  }

  Serial.println("finished loading next level");

  /*for (int i = 0; i < 81; i++) {
    currentLevel[i] = levels[levelIndex][i];
    }*/
}

void EndingGame::setup(SmartMatrix &matrix) {
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

unsigned long EndingGame::handleInput(IRrecv &irReceiver) {
  unsigned long input = 0;

  decode_results results;

  results.value = 0;

  // Attempt to read an IR code ?
  if (irReceiver.decode(&results)) {
    input = results.value;

    // Prepare to receive the next IR code
    irReceiver.resume();
  }

  // start with the player's current position
  int newX = player.x;
  int newY = player.y;

  bool moved = false;

  if (input == IRCODE_HOME) {
    return input;
  }
  // handle move buttons
  else if (input == IRCODE_LEFT) {
    newX--;
    moved = true;
  }
  else if (input == IRCODE_RIGHT) {
    newX++;
    moved = true;
  }
  else if (input == IRCODE_UP) {
    newY--;
    moved = true;
  }
  else if (input == IRCODE_DOWN) {
    Serial.println("attempting to move player down");
    newY++;
    moved = true;
  }

  // handle player movement
  if (moved) {
    bool usedTurn = move(newX, newY);

    if (usedTurn) {
      moveEnemies();
    }
  }

  return input;
}

bool EndingGame::move(int newX, int newY) {
  int playerPositionIndex = player.y * 9 + player.x;

  // get player's potential new location
  int playerNewPositionIndex = newY * 9 + newX;

  // get the tile in player's potential new location
  int tile = currentLevel[playerNewPositionIndex];

  Serial.println("");
  Serial.println("-------------------------------------");

  // determine if the player can move, based on the tile
  // in the player's potential new location
  bool canMove = false;
  bool usedTurn = false;
  switch (tile) {
    case EMPTY:
      canMove = true;
      usedTurn = true;
      break;

    case WALL:
      canMove = false;
      usedTurn = false;
      break;

    case BLOCK:
      Serial.println("breaking block at");
      Serial.println(playerNewPositionIndex);

      // break the block with the player's current turn
      currentLevel[playerNewPositionIndex] = EMPTY;

      // the player can move next turn
      canMove = false;
      usedTurn = true;
      break;

    case END:
      canMove = true;
      usedTurn = true;
      isPlayerFinished = true;
      break;
  }

  Enemy enemy = enemies[playerNewPositionIndex];
  if (enemy.isActive) {
    switch (enemy.tileType)
    {
      case STATIC_CRUSHER_LEFT:
      case STATIC_CRUSHER_RIGHT:
      case STATIC_CRUSHER_UP:
      case STATIC_CRUSHER_DOWN:
      case STATIC_CRUSHER_UP_DOWN:
      case STATIC_CRUSHER_LEFT_RIGHT:
      case MOBILE_CRUSHER_LEFT:
      case MOBILE_CRUSHER_RIGHT:
      case MOBILE_CRUSHER_UP:
      case MOBILE_CRUSHER_DOWN:
        Serial.println("breaking enemy at");
        Serial.println(playerNewPositionIndex);
        // break the block with the player's current turn
        enemies[playerNewPositionIndex] = Enemy();

        // the player can move next turn
        canMove = false;
        usedTurn = true;
        break;
    }
  }

  if (canMove) {
    Serial.println("moving player");
    player.x = newX;
    player.y = newY;

    currentLevel[playerPositionIndex] = EMPTY;
    currentLevel[playerNewPositionIndex] = PLAYER;
  }

  return usedTurn;
}

void EndingGame::moveEnemies() {
  // move enemies, check for attacks
  isPlayerDead = false;

  Serial.println("moving enemies");

  for (int enemyPosition = 0; enemyPosition < 81; enemyPosition++) {
    Enemy enemy = enemies[enemyPosition];
    enemy.alreadyMoved = false;
    enemies[enemyPosition] = enemy;
  }

  for (int enemyPosition = 0; enemyPosition < 81; enemyPosition++) {
    Enemy enemy = enemies[enemyPosition];
    if (!enemy.isActive || enemy.alreadyMoved)
      continue;

    enemy.alreadyMoved = true;

    int x = enemy.x;
    int y = enemy.y;

    Serial.print("handling enemy at ");
    Serial.print(x);
    Serial.print(",");
    Serial.println(y);

    int newX = x;
    int newY = y;

    int currentDirection = getDirection(enemy.tileType);

    // attack?
    if (isFacingDirection(enemy.tileType, LEFT) && player.x == x - 1 && player.y == y) {
      newX--;
      isPlayerDead = true;
      Serial.println("killing player from the left");
    }
    else if (isFacingDirection(enemy.tileType, RIGHT) && player.x == x + 1 && player.y == y) {
      newX++;
      isPlayerDead = true;
      Serial.println("killing player from the right");
    }
    else if (isFacingDirection(enemy.tileType, UP) && player.x == x && player.y == y - 1) {
      newY--;
      isPlayerDead = true;
      Serial.println("killing player from the top");
    }
    else if (isFacingDirection(enemy.tileType, DOWN) && player.x == x && player.y == y + 1) {
      newY++;
      isPlayerDead = true;
      Serial.println("killing player from the bottom");
    }

    if (!isPlayerDead && isMobile(enemy.tileType)) {
      // move or turn?
      double shortestDistance = 10000;
      int bestDirection = currentDirection;
      for (int i = 0; i < 4; i++) {
        DIRECTION direction = directions[i];
        int dx = x;
        int dy = y;
        switch (direction)
        {
          case UP:
            dy--;
            break;
          case DOWN:
            dy++;
            break;
          case LEFT:
            dx--;
            break;
          case RIGHT:
            dx++;
            break;
        }

        int di = dy * 9 + dx;

        // empty?
        int tileInDirection = currentLevel[di];
        if (tileInDirection != PLAYER && tileInDirection != EMPTY) {
          continue;
        }

        double distance = getDistance(dx, dy, player.x, player.y);

        if (distance < shortestDistance ||
          (distance == shortestDistance && direction == currentDirection)) {
          Serial.print("new shortest distance to player after moving ");
          Serial.print(direction);
          Serial.print(": ");
          Serial.println(distance);
          bestDirection = direction;
          shortestDistance = distance;
          newX = dx;
          newY = dy;
        }
      }

      // turn?
      if (currentDirection != bestDirection) {
        Serial.print("turning enemy from ");
        Serial.print(currentDirection);
        Serial.print(" to ");
        Serial.println(bestDirection);

        Serial.print("changing enemy tile from ");
        Serial.print(enemy.tileType);
        Serial.print(" to ");

        enemy.tileType = turnEnemy(enemy.tileType, bestDirection);

        Serial.println(enemy.tileType);
        // undo any moves
        newX = x;
        newY = y;
      }
    }

    // get enemy's new location
    int newEnemyPosition = newY * 9 + newX;

    if (newEnemyPosition > 80) {
      Serial.print("!!! invalid new enemy position: ");
      Serial.println(newEnemyPosition);
      newEnemyPosition = 80;
    }

    if (enemyPosition > 80) {
      Serial.print("!!! invalid enemy position: ");
      Serial.println(enemyPosition);
      enemyPosition = 80;
    }

    if (enemyPosition != newEnemyPosition) {
      Serial.print("moving enemy from ");
      Serial.print(enemyPosition);
      Serial.print(" to ");
      Serial.println(newEnemyPosition);
    }

    /*currentLevel[enemyPosition] = EMPTY;
    currentLevel[newEnemyPosition] = enemy.tileType;*/
    enemy.index = newEnemyPosition;
    enemy.x = newX;
    enemy.y = newY;

    enemies[enemyPosition] = Enemy();
    enemies[newEnemyPosition] = enemy;

    if (isPlayerDead) {
      return;
    }
  }

  //for (int y = 0; y < 9; y++) {
  //  for (int x = 0; x < 9; x++) {
  //    int tileType = currentLevel[enemyPosition];

  //    if (isEnemy(tileType)) {
  //      int newX = x;
  //      int newY = y;

  //      Serial.print("handling enemy at ");
  //      Serial.print(x);
  //      Serial.print(",");
  //      Serial.println(y);

  //      int currentDirection = getDirection(tileType);

  //      // attack?
  //      if (isFacingDirection(tileType, LEFT) && player.x == x - 1 && player.y == y) {
  //        newX--;
  //        isPlayerDead = true;
  //        Serial.println("killing player from the left");
  //      }
  //      else if (isFacingDirection(tileType, RIGHT) && player.x == x + 1 && player.y == y) {
  //        newX++;
  //        isPlayerDead = true;
  //        Serial.println("killing player from the right");
  //      }
  //      else if (isFacingDirection(tileType, UP) && player.x == x && player.y == y - 1) {
  //        newY--;
  //        isPlayerDead = true;
  //        Serial.println("killing player from the top");
  //      }
  //      else if (isFacingDirection(tileType, DOWN) && player.x == x && player.y == y + 1) {
  //        newY++;
  //        isPlayerDead = true;
  //        Serial.println("killing player from the bottom");
  //      }

  //      if (!isPlayerDead && isMobile(tileType)) {
  //        // move or turn?
  //        double shortestDistance = 10000;
  //        DIRECTION bestDirection;
  //        for (int i = 0; i < 4; i++) {
  //          DIRECTION direction = directions[i];
  //          int dx = x;
  //          int dy = y;
  //          switch (direction)
  //          {
  //            case UP:
  //              dy--;
  //              break;
  //            case DOWN:
  //              dy++;
  //              break;
  //            case LEFT:
  //              dx--;
  //              break;
  //            case RIGHT:
  //              dx++;
  //              break;
  //          }

  //          int di = dy * 9 + dx;

  //          // empty?
  //          if (currentLevel[di] != EMPTY) {
  //            continue;
  //          }

  //          double distance = getDistance(dx, dy, player.x, player.y);

  //          if (distance < shortestDistance ||
  //            (distance == shortestDistance && direction == currentDirection)) {
  //            bestDirection = direction;
  //            shortestDistance = distance;
  //            newX = dx;
  //            newY = dy;
  //          }
  //        }

  //        // turn?
  //        if (currentDirection != bestDirection) {
  //          Serial.print("turning enemy from ");
  //          Serial.print(currentDirection);
  //          Serial.print(" to ");
  //          Serial.println(bestDirection);

  //          Serial.print("changing enemy tile from ");
  //          Serial.print(tileType);
  //          Serial.print(" to ");

  //          tileType = turnEnemy(tileType, bestDirection);

  //          Serial.println(tileType);
  //          // undo any moves
  //          newX = x;
  //          newY = y;
  //        }
  //      }

  //      // get enemy's new location
  //      int newEnemyPosition = newY * 9 + newX;

  //      if (enemyPosition != newEnemyPosition) {
  //        Serial.print("moving enemy from ");
  //        Serial.print(enemyPosition);
  //        Serial.print(" to ");
  //        Serial.println(newEnemyPosition);
  //      }

  //      currentLevel[enemyPosition] = EMPTY;
  //      currentLevel[newEnemyPosition] = tileType;

  //      if (isPlayerDead) {
  //        return;
  //      }
  //    }

  //    enemyPosition++;
  //  }
  //}
}

bool EndingGame::isEnemy(int tileType) {
  return tileType > 4;
}

bool EndingGame::isMobile(int tileType) {
  switch (tileType) {
    case MOBILE_CRUSHER_LEFT:
    case MOBILE_CRUSHER_RIGHT:
    case MOBILE_CRUSHER_UP:
    case MOBILE_CRUSHER_DOWN:
      return true;
  }

  return false;
}

int EndingGame::turnEnemy(int tileType, int direction) {
  switch (tileType) {
    case MOBILE_CRUSHER_LEFT:
    case MOBILE_CRUSHER_RIGHT:
    case MOBILE_CRUSHER_UP:
    case MOBILE_CRUSHER_DOWN:
      return mobileCrusherDirections[direction];
  }

  return tileType;
}

int EndingGame::getDirection(int tileType) {
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

bool EndingGame::isFacingDirection(int tileType, DIRECTION direction) {
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

double EndingGame::getDistance(int x1, int y1, int x2, int y2) {
  return abs(sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2)));
}

void EndingGame::update(SmartMatrix &matrix) {
}

void EndingGame::die() {
  delay(1000);
  isPlayerDead = false;
  loadLevel(currentLevelIndex);
}

void EndingGame::draw(SmartMatrix &matrix) {
  // clear screen
  matrix.fillScreen(COLOR_BLACK);

  //bool alternateColor = true;
  int i = 0;

  for (int y = -2; y < 32; y += 4) {
    for (int x = -2; x < 32; x += 4)
    {
      //// draw background
      //rgb24 color = alternateColor ? COLOR_DDGRAY : COLOR_DGRAY;
      //matrix.fillRectangle(x, y, x + 3, y + 3, color);
      //alternateColor = !alternateColor;

      // draw tiles
      int tile = currentLevel[i];
      switch (tile) {
        case EMPTY:
          break;

        case WALL:
          drawWall(matrix, x, y);
          break;

        case PLAYER:
          drawPlayer(matrix, x, y);
          break;

        case BLOCK:
          drawBlock(matrix, x, y);
          break;

        case END:
          drawEnd(matrix, x, y);
          break;
      }

      Enemy enemy = enemies[i];
      if (enemy.isActive) {
        switch (enemy.tileType){
          case STATIC_CRUSHER_LEFT:
            drawStaticCrusherLeft(matrix, x, y);
            break;

          case STATIC_CRUSHER_RIGHT:
            drawStaticCrusherRight(matrix, x, y);
            break;

          case STATIC_CRUSHER_UP:
            drawStaticCrusherUp(matrix, x, y);
            break;

          case STATIC_CRUSHER_DOWN:
            drawStaticCrusherDown(matrix, x, y);
            break;

          case STATIC_CRUSHER_UP_DOWN:
            drawStaticCrusherUp(matrix, x, y);
            drawStaticCrusherDown(matrix, x, y);
            break;

          case STATIC_CRUSHER_LEFT_RIGHT:
            drawStaticCrusherLeft(matrix, x, y);
            drawStaticCrusherRight(matrix, x, y);
            break;

          case MOBILE_CRUSHER_LEFT:
            drawMobileCrusherLeft(matrix, x, y);
            break;

          case MOBILE_CRUSHER_RIGHT:
            drawMobileCrusherRight(matrix, x, y);
            break;

          case MOBILE_CRUSHER_UP:
            drawMobileCrusherUp(matrix, x, y);
            break;

          case MOBILE_CRUSHER_DOWN:
            drawMobileCrusherDown(matrix, x, y);
            break;
        }
      }

      i++;
    }
  }

  matrix.swapBuffers();
}

void EndingGame::drawWall(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x, y, x, y + 3, COLOR_LGRAY); // left
  matrix.drawLine(x, y, x + 3, y, COLOR_LGRAY); // top
  matrix.drawLine(x + 3, y, x + 3, y + 3, COLOR_LGRAY); // right
  matrix.drawLine(x, y + 3, x + 3, y + 3, COLOR_LGRAY); // bottom
  matrix.drawPixel(x + 1, y + 1, COLOR_BLACK); // inset shadow
  matrix.drawPixel(x + 2, y + 1, COLOR_BLACK); // inset shadow
  matrix.drawPixel(x + 1, y + 2, COLOR_BLACK); // inset shadow
}

void EndingGame::drawPlayer(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x, y, x, y + 3, COLOR_WHITE); // left
  matrix.drawLine(x, y, x + 3, y, COLOR_WHITE); // top
  matrix.drawLine(x + 2, y + 1, x + 3, y + 1, COLOR_WHITE); // right
  matrix.drawLine(x, y + 3, x + 3, y + 3, COLOR_WHITE); // bottom
}

void EndingGame::drawBlock(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x, y, x, y + 3, COLOR_WHITE); // left
  matrix.drawLine(x, y, x + 3, y, COLOR_WHITE); // top
  matrix.drawLine(x + 3, y, x + 3, y + 3, COLOR_WHITE); // right
  matrix.drawLine(x, y + 3, x + 3, y + 3, COLOR_WHITE); // bottom
  matrix.drawPixel(x + 1, y + 1, COLOR_BLACK); // inset shadow
  matrix.drawPixel(x + 2, y + 1, COLOR_BLACK); // inset shadow
  matrix.drawPixel(x + 1, y + 2, COLOR_BLACK); // inset shadow
}

void EndingGame::drawEnd(SmartMatrix &matrix, int x, int y) {
  bool alternateColor = true;

  for (int cy = y; cy < y + 4; cy++) {
    for (int cx = x; cx < x + 4; cx++) {
      rgb24 color = alternateColor ? COLOR_WHITE : COLOR_BLACK;
      matrix.drawPixel(cx, cy, color);
      alternateColor = !alternateColor;
    }
    alternateColor = !alternateColor;
  }
}

void EndingGame::drawStaticCrusherLeft(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x, y, x, y + 2, COLOR_WHITE); // left
  matrix.drawLine(x + 1, y + 1, x + 2, y, COLOR_WHITE);
  matrix.drawLine(x + 2, y + 2, x + 3, y + 1, COLOR_WHITE);
}

void EndingGame::drawStaticCrusherRight(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x + 3, y + 1, x + 3, y + 3, COLOR_WHITE); // right
  matrix.drawLine(x, y + 2, x + 1, y + 1, COLOR_WHITE);
  matrix.drawLine(x + 1, y + 3, x + 2, y + 2, COLOR_WHITE);
}

void EndingGame::drawStaticCrusherUp(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x + 1, y, x + 3, y, COLOR_WHITE); // top
  matrix.drawLine(x + 1, y + 2, x + 2, y + 1, COLOR_WHITE);
  matrix.drawLine(x + 2, y + 3, x + 3, y + 2, COLOR_WHITE);
}

void EndingGame::drawStaticCrusherDown(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x, y + 3, x + 2, y + 3, COLOR_WHITE); // bottom
  matrix.drawLine(x, y + 1, x + 1, y, COLOR_WHITE);
  matrix.drawLine(x + 1, y + 2, x + 2, y + 1, COLOR_WHITE);
}

void EndingGame::drawMobileCrusherLeft(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x, y, x, y + 2, COLOR_WHITE); // left
  matrix.drawLine(x, y + 1, x + 2, y + 1, COLOR_WHITE);
}

void EndingGame::drawMobileCrusherRight(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x + 3, y + 1, x + 3, y + 3, COLOR_WHITE); // right
  matrix.drawLine(x + 1, y + 2, x + 2, y + 2, COLOR_WHITE);
}

void EndingGame::drawMobileCrusherUp(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x + 1, y, x + 3, y, COLOR_WHITE); // top
  matrix.drawLine(x + 2, y + 1, x + 2, y + 2, COLOR_WHITE);
}

void EndingGame::drawMobileCrusherDown(SmartMatrix &matrix, int x, int y) {
  matrix.drawLine(x, y + 3, x + 2, y + 3, COLOR_WHITE); // bottom
  matrix.drawLine(x + 1, y + 1, x + 1, y + 2, COLOR_WHITE);
}

void EndingGame::run(SmartMatrix &matrix, IRrecv &irReceiver) {

  setup(matrix);

  while (true) {
    unsigned long input = handleInput(irReceiver);

    if (input == IRCODE_HOME)
      return;

    update(matrix);

    draw(matrix);

    if (isPlayerDead) {
      die();
    }
    else if (isPlayerFinished) {
      Serial.println("finished, delaying...");
      delay(1000);

      isPlayerFinished = false;

      currentLevelIndex++;
      if (currentLevelIndex >= LEVEL_COUNT) {
        currentLevelIndex = 0;
      }

      Serial.println("finished, loading next level...");
      loadLevel(currentLevelIndex);
    }
  }
}

