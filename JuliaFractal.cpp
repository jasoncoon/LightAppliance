/*
 * Julia fractal pattern and "game" with interactive pan and zoom
 * for IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix.
 * Used the excellent documentation by Lode Vandevenne at http://lodev.org/cgtutor/juliamandelbrot.html
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

#include "JuliaFractal.h"
#include "Types.h"
#include "Codes.h"
#include "Colors.h"

void JuliaFractal::runPattern(SmartMatrix matrixRef, IRrecv irReceiverRef, boolean(*checkForTermination)()) {
    matrix = &matrixRef;
    irReceiver = &irReceiverRef;

    matrix->fillScreen(COLOR_BLACK);
    matrix->swapBuffers();

    reset();

    while (!checkForTermination()) {
        draw();

        // Check for termination
        if (checkForTermination()) {
            return;
        }

        // translate along the x-axis
        moveX -= .0201; // left

        // zoom
        zoom *= 1.01;
    }
}

void JuliaFractal::runGame(SmartMatrix matrixRef, IRrecv irReceiverRef) {
    matrix = &matrixRef;
    irReceiver = &irReceiverRef;

    matrix->setScrollMode(wrapForward);
    matrix->setScrollSpeed(64);
    matrix->setScrollFont(font3x5);
    matrix->setScrollColor(COLOR_WHITE);
    matrix->setScrollOffsetFromEdge(10);

    matrix->fillScreen(COLOR_BLACK);
    matrix->swapBuffers();

    reset();

    draw();

    while (true) {
        unsigned long input = handleInput();

        if (input == IRCODE_HOME)
            return;
    }
}

unsigned long JuliaFractal::handleInput() {
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
        moveX -= 0.03 * zoom;
        update = true;
    }
    else if (input == IRCODE_RIGHT) {
        // pan right
        moveX += 0.03 * zoom;
        update = true;
    }
    else if (input == IRCODE_UP) {
        // pan up
        moveY += 0.03 * zoom;
        update = true;
    }
    else if (input == IRCODE_DOWN) {
        // pan down
        moveY -= 0.03 * zoom;
        update = true;
    }
    else if (input == IRCODE_SEL) {
        // zoom in
        zoom *= 1.01;
        update = true;
    }
    else if (input == IRCODE_A) {
        // zoom out
        zoom *= .99;
        update = true;
    }
    else if (input == IRCODE_B) {
        // decrease max iterations
        if (maxIterations > 1) {
            maxIterations--;
            generateColors();
            update = true;
            sprintf(stringBuffer, "%d MaxIterations", maxIterations);
            matrix->scrollText(stringBuffer, 1);
        }
    }
    else if (input == IRCODE_C) {
        // increase max iterations
        //if (maxIterations < MAXIMUM) {
        maxIterations++;
        generateColors();
        update = true;
        sprintf(stringBuffer, "%d maxIterations", maxIterations);
        matrix->scrollText(stringBuffer, 1);
        //}
    }

    if (update) {
        draw();
    }

    return input;
}

void JuliaFractal::draw() {
    matrix->fillScreen(COLOR_BLACK);

    //loop through every pixel
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++)
        {
            //calculate the initial real and imaginary part of z, based on the pixel location and zoom and position values
            newRe = 1.5 * (x) / (zoom * w) + moveX;
            newIm = (y) / (zoom * h) + moveY;
            //i will represent the number of iterations
            int i;
            //start the iteration process
            for (i = 0; i < maxIterations; i++)
            {
                //remember value of previous iteration
                oldRe = newRe;
                oldIm = newIm;
                //the actual iteration, the real and imaginary part are calculated
                newRe = oldRe * oldRe - oldIm * oldIm + cRe;
                newIm = 2 * oldRe * oldIm + cIm;
                //if the point is outside the circle with radius 2: stop
                if ((newRe * newRe + newIm * newIm) > 4) break;
            }

            if (i < maxIterations) {
                ////use color model conversion to get rainbow palette, make brightness black if maxIterations reached
                color = colors[i]; // createHSVColor(i % 360, 1.0, i < maxIterations ? 1.0 : 0.0);

                // color = colors[i];

                //draw the pixel
                matrix->drawPixel(x, y, color);
            }
        }
    }

    matrix->swapBuffers();
}

void JuliaFractal::generateColors() {
    for (int i = 0; i < maxIterations; i++) {
        colors[i] = createHSVColor(i % 360, 1.0, i < maxIterations ? 1.0 : 0.0);
    }
}

void JuliaFractal::reset() {
    // red spirals
    zoom = 0.8303507625737443;
    moveX = 0.0872668560626856;
    moveY = -0.01363821746275637;
    maxIterations = 32;
    cRe = -0.7709787210451183;
    cIm = -0.08545;

    //// green fingers
    //zoom = 1265.761100292908;
    //moveX = 0.2093925202229247;
    //moveY = 0.104598694622353;
    //maxIterations = 128;
    //cRe = -0.6548832053365524;
    //cIm = -0.4477065469412519;

    generateColors();
}

#define NUM_OF_COLOR_VALUES 256
#define MIN_COLOR_VALUE     0
#define MAX_COLOR_VALUE     255

// Create a HSV color
rgb24 JuliaFractal::createHSVColor(float hue, float saturation, float value) {

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
void JuliaFractal::hsvToRGB(float hue, float saturation, float value, float * red, float * green, float * blue) {

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