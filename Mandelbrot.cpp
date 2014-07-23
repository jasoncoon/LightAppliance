/*
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

#include "Mandelbrot.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

void Mandelbrot::runPattern(SmartMatrix matrixRef, IRrecv irReceiverRef, boolean(*checkForTermination)()) {
    matrix = &matrixRef;
    irReceiver = &irReceiverRef;

    matrix->fillScreen(COLOR_BLACK);
    matrix->swapBuffers();

    generateColors();

    while (!checkForTermination()) {
        draw();

        // Check for termination
        if (checkForTermination()) {
            return;
        }

        // translate along the x-axis
        MinRe -= .0201; // left
        MaxRe = MinRe + width; // right

        // zoom
        MinRe *= .99;
        MaxRe *= .99;
        MinIm *= .99;
        width *= .99;
    }
}

void Mandelbrot::runGame(SmartMatrix matrixRef, IRrecv irReceiverRef) {
    matrix = &matrixRef;
    irReceiver = &irReceiverRef;
    
    matrix->setScrollMode(wrapForward);
    matrix->setScrollSpeed(64);
    matrix->setScrollFont(font3x5);
    matrix->setScrollColor(COLOR_RED);
    matrix->setScrollOffsetFromEdge(10);

    matrix->fillScreen(COLOR_BLACK);
    matrix->swapBuffers();

    generateColors();

    draw();

    while (true) {
        unsigned long input = handleInput();

        if (input == IRCODE_HOME)
            return;
    }
}

unsigned long Mandelbrot::handleInput() {
    unsigned long input = 0;

    decode_results results;

    results.value = 0;

    // Attempt to read an IR code ?
    if (irReceiver->decode(&results)) {
        input = results.value;

        //delay(50);

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

    bool update = false;

    if (input == IRCODE_HOME) {
        return input;
    }
    // handle move buttons
    else if (input == IRCODE_LEFT) {
        // pan left
        // translate along the x-axis
        MinRe -= .01; // left
        MaxRe = MinRe + width; // right
        update = true;
    }
    else if (input == IRCODE_RIGHT) {
        // pan right
        MinRe += .01; // left
        MaxRe = MinRe + width; // right
        update = true;
    }
    else if (input == IRCODE_UP) {
        // pan up
        MinIm += .01; // top
        update = true;
    }
    else if (input == IRCODE_DOWN) {
        // pan down
        MinIm -= .01; // top
        update = true;
    }
    else if (input == IRCODE_SEL) {
        // zoom in
        MinRe *= .99;
        MaxRe *= .99;
        MinIm *= .99;
        width *= .99;
        update = true;
    }
    else if (input == IRCODE_A) {
        // zoom out
        MinRe *= 1.01;
        MaxRe *= 1.01;
        MinIm *= 1.01;
        width *= 1.01;
        update = true;
    }
    else if (input == IRCODE_B) {
        // decrease max iterations
        if (MaxIterations > 1) {
            MaxIterations--;
            generateColors();
            update = true;
            sprintf(stringBuffer, "%d MaxIterations", MaxIterations);
            matrix->scrollText(stringBuffer, 1);
        }
    }
    else if (input == IRCODE_C) {
        // increase max iterations
        if (MaxIterations < MAXIMUM) {
            MaxIterations++;
            generateColors();
            update = true;
            sprintf(stringBuffer, "%d MaxIterations", MaxIterations);
            matrix->scrollText(stringBuffer, 1);
        }
    }

    if (update) {
        draw();
    }

    return input;
}

void Mandelbrot::draw() {
    MaxIm = MinIm + (MaxRe - MinRe)*imageHeight / imageWidth; // top
    Re_factor = (MaxRe - MinRe) / (imageWidth - 1);
    Im_factor = (MaxIm - MinIm) / (imageHeight - 1);

    matrix->fillScreen(COLOR_BLACK);

    for (y = 0; y < imageHeight; ++y)
    {
        c_im = MaxIm - y*Im_factor;
        for (x = 0; x < imageWidth; ++x)
        {
            c_re = MinRe + x*Re_factor;

            Z_re = c_re;
            Z_im = c_im;
            isInside = true;

            for (n = 0; n<MaxIterations; ++n)
            {
                Z_re2 = Z_re*Z_re;
                Z_im2 = Z_im*Z_im;
                if (Z_re2 + Z_im2 > 4)
                {
                    isInside = false;
                    break;
                }
                Z_im = 2 * Z_re*Z_im + c_im;
                Z_re = Z_re2 - Z_im2 + c_re;
            }
            if (!isInside) {
                matrix->drawPixel(x, y, colors[n]);
            }
        }

    }

    matrix->swapBuffers();
}

void Mandelbrot::generateColors() {
    halfMaxIterations = MaxIterations / 2;

    for (int i = 0; i < halfMaxIterations; i++) {
        colors[i] = createHSVColor(240, 1.0, i * (1.0 / halfMaxIterations));
    }
    for (int i = halfMaxIterations; i < MaxIterations; i++) {
        colors[i] = createHSVColor(240, 2.0 - (i * (1.0 / halfMaxIterations)), 1.0);
    }
}

#define NUM_OF_COLOR_VALUES 256
#define MIN_COLOR_VALUE     0
#define MAX_COLOR_VALUE     255

// Create a HSV color
rgb24 Mandelbrot::createHSVColor(float hue, float saturation, float value) {

    float r, g, b;
    rgb24 color;

    hsvToRGB(hue, saturation, value, &r, &g, &b);

    color.red = r * MAX_COLOR_VALUE;
    color.green = g * MAX_COLOR_VALUE;
    color.blue = b * MAX_COLOR_VALUE;

    return color;
}

// HSV to RGB color conversion
// Input arguments
// hue in degrees (0 - 360.0)
// saturation (0.0 - 1.0)
// value (0.0 - 1.0)
// Output arguments
// red, green blue (0.0 - 1.0)
void Mandelbrot::hsvToRGB(float hue, float saturation, float value, float * red, float * green, float * blue) {

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