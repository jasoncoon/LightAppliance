/*
 * RGB color image generator for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix.
 * Based on the fantastic Rainbow Smoke by József Fejes at http://joco.name/2014/03/02/all-rgb-colors-in-one-image
 * and http://rainbowsmoke.hu/.
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

#include "RainbowSmoke.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

void RainbowSmoke::runPattern(SmartMatrix matrixRef, IRrecv irReceiverRef, boolean(*checkForTermination)()) {
    matrix = &matrixRef;
    irReceiver = &irReceiverRef;

    randomSeed(analogRead(5));

    while (true) {
        // clear all flags
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                hasColor[x][y] = false;
                isAvailable[x][y] = false;
            }
        }

        matrix->fillScreen(COLOR_BLACK);

        sortColors();

        bool first = true;

        int algorithm = random(3);

        int update = 0;

        for (int i = 0; i < COLOR_COUNT; i++) {
            Point point;
            rgb24 color = colors[i];

            if (first) {
                // use the starting point
                point.x = random(32);
                point.y = random(32);
                first = false;
            }
            else {
                point = getAvailablePoint(algorithm, color);
            }

            isAvailable[point.x][point.y] = false;
            hasColor[point.x][point.y] = true;

            matrix->drawPixel(point.x, point.y, color);

            if (update == 4)
            {
                matrix->swapBuffers();
                update = 0;
            }

            update++;

            markAvailableNeighbors(point);

            // Check for termination
            if (checkForTermination()) {
                return;
            }
        }

        matrix->swapBuffers();

        // wait a bit, while checking for termination
        for (int i = 0; i < 20; i++) {
            if (checkForTermination())
                return;

            delay(100);
        }
    }
}

void RainbowSmoke::markAvailableNeighbors(Point point) {
    for (int dy = -1; dy <= 1; dy++) {
        int ny = point.y + dy;

        if (ny == -1 || ny == HEIGHT)
            continue;

        for (int dx = -1; dx <= 1; dx++) {
            int nx = point.x + dx;

            if (nx == -1 || nx == WIDTH)
                continue;

            if (!hasColor[nx][ny]) {
                isAvailable[nx][ny] = true;
            }
        }
    }
}

RainbowSmoke::Point RainbowSmoke::getAvailablePoint(int algorithm, rgb24 color) {
    switch (algorithm) {
        case 0:
            return getAvailablePointWithClosestNeighborColor(color);
        case 1:
            return getAvailablePointWithClosestAverageNeighborColor(color);
        case 2:
            // random mix of both
            if (random(2) == 0) {
                return getAvailablePointWithClosestNeighborColor(color);
            }
            else {
                return getAvailablePointWithClosestAverageNeighborColor(color);
            }
    }
}

RainbowSmoke::Point RainbowSmoke::getAvailablePointWithClosestNeighborColor(rgb24 color) {
    Point best;

    // find the pixel with the smallest difference between the current color and all of it's neighbors' colors
    int smallestDifference = 999999;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // skip any that arent' available
            if (!isAvailable[x][y])
                continue;

            // loop through its neighbors
            int smallestDifferenceAmongNeighbors = 999999;
            for (int dy = -1; dy <= 1; dy++) {
                if (y + dy == -1 || y + dy == HEIGHT)
                    continue;

                for (int dx = -1; dx <= 1; dx++) {
                    if (x + dx == -1 || x + dx == WIDTH)
                        continue;

                    int nx = x + dx;
                    int ny = y + dy;

                    // skip any neighbors that don't already have a color
                    if (!hasColor[nx][ny])
                        continue;

                    rgb24 neighborColor = matrix->readPixel(nx, ny);

                    int difference = colorDifference(neighborColor, color);
                    if (difference < smallestDifferenceAmongNeighbors) {
                        smallestDifferenceAmongNeighbors = difference;
                    }
                }
            }

            if (smallestDifferenceAmongNeighbors < smallestDifference) {
                smallestDifference = smallestDifferenceAmongNeighbors;
                best.x = x;
                best.y = y;
            }
        }
    }

    return best;
}

RainbowSmoke::Point RainbowSmoke::getAvailablePointWithClosestAverageNeighborColor(rgb24 color) {
    Point best;

    int smallestAverageDifference = 999999;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // skip any that arent' available
            if (!isAvailable[x][y])
                continue;

            int neighborCount = 0;
            int neighborColorDifferenceTotal = 0;

            // loop through its neighbors
            for (int dy = -1; dy <= 1; dy++) {
                if (y + dy == -1 || y + dy == HEIGHT)
                    continue;

                for (int dx = -1; dx <= 1; dx++) {
                    if (x + dx == -1 || x + dx == WIDTH)
                        continue;

                    int nx = x + dx;
                    int ny = y + dy;

                    // skip any neighbors that don't already have a color
                    if (!hasColor[nx][ny])
                        continue;

                    neighborCount++;

                    rgb24 neighborColor = matrix->readPixel(nx, ny);

                    int difference = colorDifference(neighborColor, color);
                    neighborColorDifferenceTotal += difference;
                }
            }

            int averageDifferenceAmongNeighbors = neighborColorDifferenceTotal / neighborCount;

            if (averageDifferenceAmongNeighbors < smallestAverageDifference) {
                smallestAverageDifference = averageDifferenceAmongNeighbors;
                best.x = x;
                best.y = y;
            }
        }
    }

    return best;
}

void RainbowSmoke::sortColors() {
    int colorSort = random(5);

    switch (colorSort) {
        case 0:
            sortColorsRGB();
            break;
        case 1:
            sortColorsGBR();
            break;
        case 2:
            sortColorsBRG();
            break;
        case 3:
            sortColorsRGB();
            shuffleColors();
            break;
        case 4:
            sortColorsHSV();
            break;
    }
}

void RainbowSmoke::sortColorsRGB() {
    int i = 0;

    int d = 255 / (NUMCOLORS - 1);

    for (int b = 0; b < NUMCOLORS; b++) {
        for (int g = 0; g < NUMCOLORS; g++) {
            for (int r = 0; r < NUMCOLORS; r++) {
                rgb24 color;
                color.red = r * d;
                color.green = g * d;
                color.blue = b * d;
                colors[i] = color;
                i++;
                if (i == COLOR_COUNT)
                    return;
            }
        }
    }
}

void RainbowSmoke::sortColorsGBR() {
    int i = 0;

    int d = 255 / (NUMCOLORS - 1);

    for (int r = 0; r < NUMCOLORS; r++) {
        for (int b = 0; b < NUMCOLORS; b++) {
            for (int g = 0; g < NUMCOLORS; g++) {
                rgb24 color;
                color.red = r * d;
                color.green = g * d;
                color.blue = b * d;
                colors[i] = color;
                i++;
                if (i == COLOR_COUNT)
                    return;
            }
        }
    }
}

void RainbowSmoke::sortColorsBRG() {
    int i = 0;

    int d = 255 / (NUMCOLORS - 1);

    for (int g = 0; g < NUMCOLORS; g++) {
        for (int r = 0; r < NUMCOLORS; r++) {
            for (int b = 0; b < NUMCOLORS; b++) {
                rgb24 color;
                color.red = r * d;
                color.green = g * d;
                color.blue = b * d;
                colors[i] = color;
                i++;
                if (i == COLOR_COUNT)
                    return;
            }
        }
    }
}

void RainbowSmoke::shuffleColors() {
    for (int a = 0; a < COLOR_COUNT; a++)
    {
        int r = random(a, COLOR_COUNT);
        rgb24 temp = colors[a];
        colors[a] = colors[r];
        colors[r] = temp;
    }
}

void RainbowSmoke::sortColorsHSV() {
    int i = 0;

    float dh = 360.0 / NUMCOLORS;
    float sh = 1.0 / NUMCOLORS;

    for (float h = 0.0; h < 360; h += dh) {
        for (float s = 1.0; s > 0.0; s -= sh) {
            for (float v = 1.0; v > 0.0; v -= sh) {
                rgb24 color = createHSVColor(h, s, v);
                colors[i] = color;
                i++;
                if (i == COLOR_COUNT)
                    return;
            }
        }
    }
}

// HSV to RGB color conversion
// Input arguments
// hue in degrees (0 - 360.0)
// saturation (0.0 - 1.0)
// value (0.0 - 1.0)
// Output arguments
// red, green blue (0.0 - 1.0)
void RainbowSmoke::hsvToRGB(float hue, float saturation, float value, float * red, float * green, float * blue) {

    int i;
    float f, p, q, t;

    if (saturation == 0) {
        // achromatic (grey)
        *red = *green = *blue = value;
        return;
    }
    hue /= 60;                  // sector 0 to 5
    i = floor(hue);
    f = hue - i;                // factorial part of h
    p = value * (1 - saturation);
    q = value * (1 - saturation * f);
    t = value * (1 - saturation * (1 - f));
    switch (i) {
        case 0:
            *red = value;
            *green = t;
            *blue = p;
            break;
        case 1:
            *red = q;
            *green = value;
            *blue = p;
            break;
        case 2:
            *red = p;
            *green = value;
            *blue = t;
            break;
        case 3:
            *red = p;
            *green = q;
            *blue = value;
            break;
        case 4:
            *red = t;
            *green = p;
            *blue = value;
            break;
        default:
            *red = value;
            *green = p;
            *blue = q;
            break;
    }
}

// Create a HSV color
rgb24 RainbowSmoke::createHSVColor(float hue, float saturation, float value) {

    float r, g, b;
    rgb24 color;

    hsvToRGB(hue, saturation, value, &r, &g, &b);

    color.red = r * MAX_COLOR_VALUE;
    color.green = g * MAX_COLOR_VALUE;
    color.blue = b * MAX_COLOR_VALUE;

    return color;
}
