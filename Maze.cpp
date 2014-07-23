/*
 * Maze pattern for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
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

#include "Maze.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

void Maze::runPattern(SmartMatrix matrixRef, IRrecv irReceiverRef, boolean(*checkForTermination)()) {
    matrix = &matrixRef;
    irReceiver = &irReceiverRef;

    randomSeed(analogRead(5));

    while (!checkForTermination()) {
        int x = random(width);
        int y = random(height);

        start = createPoint(x, y);

        if (generateMaze(true, checkForTermination) != 0)
            return;

        // algorithm++;
        // if (algorithm > 2)
        //     algorithm = 0;
    }
}

void Maze::runGame(SmartMatrix matrixRef, IRrecv irReceiverRef) {
    matrix = &matrixRef;
    irReceiver = &irReceiverRef;

    randomSeed(analogRead(5));

    int x = random(width);
    int y = random(height);

    start = createPoint(x, y);

    generateMaze(false, NULL);

    player = start;

    matrix->drawPixel(start.x * 2, start.y * 2, COLOR_GREEN);
    matrix->drawPixel(end.x * 2, end.y * 2, COLOR_RED);
    matrix->drawPixel(player.x * 2, player.y * 2, COLOR_BLUE);
    matrix->swapBuffers();

    while (true) {
        unsigned long input = handleInput();

        if (input == IRCODE_HOME)
            return;
    }
}

int Maze::generateMaze(bool animate, boolean(*checkForTermination)()) {
    matrix->fillScreen(COLOR_BLACK);

    for (int i = 0; i < 256; i++) {
        cells[i].isActive = false;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            grid[y][x] = None;
        }
    }

    cells[0] = start;
    cellCount = 1;

    highestCellCount = 0;

    while (cellCount > 0) {
        int index = chooseIndex(cellCount);

        if (index < 0)
            break;

        point = cells[index];

        Point imagePoint = createPoint(point.x * 2, point.y * 2);
        if (animate) {
            matrix->drawPixel(imagePoint.x, imagePoint.y, COLOR_BLUE);
        }

        shuffleDirections();

        for (int i = 0; i < 4; i++) {
            Directions direction = directions[i];

            Point newPoint = point.Move(direction);
            if (newPoint.x >= 0 && newPoint.y >= 0 && newPoint.x < width && newPoint.y < height && grid[newPoint.y][newPoint.x] == None) {
                grid[point.y][point.x] = (Directions) ((int) grid[point.y][point.x] | (int) direction);
                grid[newPoint.y][newPoint.x] = (Directions) ((int) grid[newPoint.y][newPoint.x] | (int) point.Opposite(direction));

                Point newImagePoint = imagePoint.Move(direction);
                matrix->drawPixel(newImagePoint.x, newImagePoint.y, COLOR_WHITE);
                if (animate) {
                    matrix->swapBuffers();
                }

                cellCount++;
                cells[cellCount - 1] = newPoint;

                if (cellCount > highestCellCount) {
                    end = newPoint;
                    highestCellCount = cellCount;
                }

                index = -1;
                break;
            }
        }

        if (index > -1) {
            Point finishedPoint = cells[index];
            imagePoint = createPoint(finishedPoint.x * 2, finishedPoint.y * 2);
            matrix->drawPixel(imagePoint.x, imagePoint.y, COLOR_WHITE);
            if (animate) {
                matrix->swapBuffers();
            }
            cellCount--;
            cells[index].isActive = false;
        }

        if (cellCount < 1) {
            matrix->swapBuffers();
        }

        if (checkForTermination != NULL && checkForTermination())
            return 1;
    }

    //delay(500);
    return 0;
}

void Maze::shuffleDirections() {
    for (int a = 0; a < 4; a++)
    {
        int r = random(a, 4);
        Directions temp = directions[a];
        directions[a] = directions[r];
        directions[r] = temp;
    }
}

Maze::Point Maze::createPoint(int x, int y) {
    Point point;
    point.x = x;
    point.y = y;
    point.isActive = true;
    return point;
}

int Maze::chooseIndex(int max) {
    switch (algorithm) {
        case 0:
        default:
            // choose newest (recursive backtracker)
            return max - 1;

        case 1:
            // choose oldest
            return 0;

        case 2:
            // choose random(Prim's)
            return random(max);
    }
}

unsigned long Maze::handleInput() {
    unsigned long input = 0;

    decode_results results;

    results.value = 0;

    // Attempt to read an IR code ?
    if (irReceiver->decode(&results)) {
        input = results.value;

        delay(50);

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

    // start with the player's current position
    int newX = player.x;
    int newY = player.y;

    bool moved = false;

    Directions direction;

    if (input == IRCODE_HOME) {
        return input;
    }
    // handle move buttons
    else if (input == IRCODE_LEFT) {
        direction = Left;
        moved = true;
    }
    else if (input == IRCODE_RIGHT) {
        direction = Right;
        moved = true;
    }
    else if (input == IRCODE_UP) {
        direction = Up;
        moved = true;
    }
    else if (input == IRCODE_DOWN) {
        direction = Down;
        moved = true;
    }

    // handle player movement
    if (moved) {
        // test player movement in direction
        Point newPoint = player.Move(direction);
        // get the allowed directions of movement from current position
        Directions allowed = grid[player.y][player.x];
        int allowedDirectionsCount = directionsCount(allowed);

        // move the player in the selected direction until they hit a dead end, or an intersection with more than two allowed directions (a 'T')
        // then they have to choose which direction to go
        while (moved || allowedDirectionsCount < 3) {
            moved = false;
            // is the proposed new direction valid?
            if (newPoint.x >= 0 && newPoint.y >= 0 && newPoint.x < width && newPoint.y < height && (allowed & direction) != 0)
            {
                Serial.print("move allowed in direction: ");
                Serial.println(direction);

                // clear the player's old position
                matrix->drawPixel(player.x * 2, player.y * 2, COLOR_WHITE);

                // move to the new position
                player = newPoint;

                // draw the maze start and end points
                matrix->drawPixel(start.x * 2, start.y * 2, COLOR_GREEN);
                matrix->drawPixel(end.x * 2, end.y * 2, COLOR_RED);

                // draw the new position
                matrix->drawPixel(player.x * 2, player.y * 2, COLOR_BLUE);

                // apply the display changes
                matrix->swapBuffers();

                // player hit the end of the maze?
                if (player.x == end.x && player.y == end.y) {
                    // pause to let the player bask in the glory of victory!
                    delay(1000);

                    start = end;

                    // generate a new maze, starting from the current end (like we're working our way down or up)
                    generateMaze(false, NULL);

                    // move them to the start
                    player = start;

                    // refresh the display
                    matrix->drawPixel(start.x * 2, start.y * 2, COLOR_GREEN);
                    matrix->drawPixel(end.x * 2, end.y * 2, COLOR_RED);
                    matrix->drawPixel(player.x * 2, player.y * 2, COLOR_BLUE);
                    matrix->swapBuffers();

                    // bail
                    return input;
                }

                // try to keep moving them in the selected direction
                newPoint = player.Move(direction);
                allowed = grid[player.y][player.x];
                allowedDirectionsCount = directionsCount(allowed);
            }
            else {
                Serial.print("move not allowed in direction: ");
                Serial.println(direction);

                // can't move in direction any more
                allowedDirectionsCount = directionsCount(allowed);

                Serial.print("allowed directions: ");
                Serial.print(allowed);
                Serial.print(", count: ");
                Serial.println(allowedDirectionsCount);

                // if there are only two allowed directions, move in the newly available direction
                if (allowedDirectionsCount == 2)
                {
                    // remove the opposite direction from the list of allowed directions
                    Directions opposite = player.Opposite(direction);
                    Directions otherDirection = (Directions) ((int) allowed & (int) (allowed - opposite));
                    direction = otherDirection;

                    Serial.print("changing to direction: ");
                    Serial.println(direction);

                    newPoint = player.Move(direction);
                }
                else {
                    break;
                }
            }

            // pause so the player can follow the movement
            delay(30);
        }
    }

    return input;
}

int Maze::directionsCount(Directions directions) {
    int iCount = 0;

    //Loop the value while there are still bits
    while (directions != 0)
    {
        //Remove the end bit
        directions = (Directions) ((int) directions & (int) (directions - 1));

        //Increment the count
        iCount++;
    }

    //Return the count
    return iCount;
}
