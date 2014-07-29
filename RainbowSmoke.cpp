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

        createPalette();

        bool first = true;

        int algorithm = random(2);

        int update = 0;

        for (int i = 0; i < COLOR_COUNT; i++) {
            Point point;
            rgb24 color = colors[i];

            if (first) {
                // use a random starting point
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
            if (dx == 0 && dy == 0)
                continue;

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
                    if (x == 0 && y == 0)
                        continue;

                    if (x + dx == -1 || x + dx == WIDTH)
                        continue;

                    int nx = x + dx;
                    int ny = y + dy;

                    // skip any neighbors that don't already have a color
                    if (!hasColor[nx][ny])
                        continue;

                    rgb24 neighborColor = matrix->readPixel(nx, ny);

                    int difference = colorDifference(neighborColor, color);
                    if (difference < smallestDifferenceAmongNeighbors || (difference == smallestDifferenceAmongNeighbors && random(2) == 1)) {
                        smallestDifferenceAmongNeighbors = difference;
                    }
                }
            }

            if (smallestDifferenceAmongNeighbors < smallestDifference || (smallestDifferenceAmongNeighbors == smallestDifference && random(2) == 1)) {
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

            if (averageDifferenceAmongNeighbors < smallestAverageDifference || (averageDifferenceAmongNeighbors == smallestAverageDifference && random(2) == 1)) {
                smallestAverageDifference = averageDifferenceAmongNeighbors;
                best.x = x;
                best.y = y;
            }
        }
    }

    return best;
}

void RainbowSmoke::createPalette() {
    int colorSort = random(4);

    switch (colorSort) {
        case 0:
            createPaletteRGB();
            shuffleColors();
            break;
        case 1:
            createPaletteGBR();
            shuffleColors();
            break;
        case 2:
            createPaletteBRG();
            shuffleColors();
            break;
        case 3:
            createPaletteHSV();
            break;
    }
}

void RainbowSmoke::createPaletteRGB() {
    int i = 0;

    for (int b = 0; b < NUMCOLORS; b++) {
        for (int g = 0; g < NUMCOLORS; g++) {
            for (int r = 0; r < NUMCOLORS; r++) {
                rgb24 color;
                color.red = r * 255 / (NUMCOLORS - 1);
                color.green = g * 255 / (NUMCOLORS - 1);
                color.blue = b * 255 / (NUMCOLORS - 1);
                colors[i] = color;

                i++;
                if (i == COLOR_COUNT)
                    return;
            }
        }
    }
}

void RainbowSmoke::createPaletteGBR() {
    int i = 0;

    for (int r = 0; r < NUMCOLORS; r++) {
        for (int b = 0; b < NUMCOLORS; b++) {
            for (int g = 0; g < NUMCOLORS; g++) {
                rgb24 color;
                color.red = r * 255 / (NUMCOLORS - 1);
                color.green = g * 255 / (NUMCOLORS - 1);
                color.blue = b * 255 / (NUMCOLORS - 1);
                colors[i] = color;

                i++;
                if (i == COLOR_COUNT)
                    return;
            }
        }
    }
}

void RainbowSmoke::createPaletteBRG() {
    int i = 0;

    for (int r = 0; r < NUMCOLORS; r++) {
        for (int g = 0; g < NUMCOLORS; g++) {
            for (int b = 0; b < NUMCOLORS; b++) {
                rgb24 color;
                color.red = r * 255 / (NUMCOLORS - 1);
                color.green = g * 255 / (NUMCOLORS - 1);
                color.blue = b * 255 / (NUMCOLORS - 1);
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

void RainbowSmoke::createPaletteHSV() {
    int i = 0;

    for (int h = 0; h < 32; h++) {
        for (int s = 0; s < 16; s++) {
            float H = (float) h * 360.0F / (float) (32 - 1.0F);
            float S = (float) s * 1.0F / (float) (16 - 1.0F);
            float V = 1.0F;

            colors[i] = createHSVColor(H, S, V);

            i++;
            if (i == COLOR_COUNT)
                return;
        }

        for (int v = 16; v > 0; v--) {
            float H = (float) h * 360.0F / (float) (NUMCOLORS - 1.0F);
            float S = 1.0F;
            float V = (float) v * 1.0F / (float) (16 - 1.0F);

            colors[i] = createHSVColor(H, S, V);

            i++;
            if (i == COLOR_COUNT)
                return;
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
