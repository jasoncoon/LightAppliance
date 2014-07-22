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

    while (!checkForTermination()) {
        randomSeed(analogRead(5));

        matrix->fillScreen(COLOR_BLACK);

        for (int i = 0; i < 256; i++) {
            cells[i].isActive = false;
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x] = None;
            }
        }

        int x = random(width);
        int y = random(height);

        cells[0] = createPoint(x, y);
        cellCount = 1;

        while (cellCount > 0) {
            int index = chooseIndex(cellCount);

            if (index < 0)
                break;

            point = cells[index];
            if (!point.isActive)
                continue;

            Point imagePoint = createPoint(point.x * 2, point.y * 2);
            matrix->drawPixel(imagePoint.x, imagePoint.y, COLOR_BLUE);
            // matrix->swapBuffers();

            shuffleDirections();

            for (int i = 0; i < 4; i++) {
                Directions direction = directions[i];

                Point newPoint = point.Move(direction);
                if (newPoint.x >= 0 && newPoint.y >= 0 && newPoint.x < width && newPoint.y < height && grid[newPoint.y][newPoint.x] == None) {
                    grid[point.y][point.x] = (Directions) ((int) grid[point.y][point.x] | (int) direction);
                    grid[newPoint.y][newPoint.x] = (Directions) ((int) grid[newPoint.y][newPoint.x] | (int) point.Opposite(direction));

                    Point newImagePoint = imagePoint.Move(direction);
                    matrix->drawPixel(newImagePoint.x, newImagePoint.y, COLOR_WHITE);
                    matrix->swapBuffers();

                    cellCount++;
                    cells[cellCount - 1] = newPoint;
                    index = -1;
                    break;
                }
            }

            if (index > -1) {
                Point finishedPoint = cells[index];
                imagePoint = createPoint(finishedPoint.x * 2, finishedPoint.y * 2);
                matrix->drawPixel(imagePoint.x, imagePoint.y, COLOR_WHITE);
                matrix->swapBuffers();
                cellCount--;
                cells[index].isActive = false;
            }

            if (cellCount < 1) {
                matrix->swapBuffers();
            }

            if (checkForTermination())
                return;
        }

        //delay(500);
    }
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