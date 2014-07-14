/*
 * IR Remote Controlled Light Appliance Application for the 32x32 RGB LED Matrix
 *
 * Makes the LED Matrix into a display appliance that can display:
 *  Open/Closed sign for business
 *  Digital clock with time and temperature display
 *  Mood lighting effects
 *  Various colorful graphic effects (including animations) for attention getting fun
 *  Animated 32x32 GIFs
 *
 * Requires the SmartMatrix Library by Pixelmatix
 * See: https://github.com/pixelmatix/SmartMatrix/releases
 *
 * Required Hardware
 *  the LED matrix. SparkFun (SF) part number: COM-12584
 *  a Teensy 3.1 microcontroller. SF part number: DEV-12646
 *  a IR receiver diode. SF part number: SEN-10266
 *  an IR remote control. SF part number: COM-11759
 *  a 5 VDC 2 amp (min) power supply. SF part number: TOL-11296 or equivalent
 *
 * Optional Hardware
 *  32.768 KHz crystal for Teensy 3.1 RTC operation. SF part number: COM-00540
 *  DS18B20 temperature sensor. SF part number: SEN-00245
 *  Breakout board for SD-MMC memory card. SF part number: BOB-11403
 *  SD memory card up to 2 GBytes in size
 *
 * Written by: Craig A. Lindley
 * Version: 1.2
 * Last Update: 07/04/2014
 *
 * Copyright (c) 2014 Craig A. Lindley
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

// Open and Closed Sign Messages
#define OPEN_MSG   "Please Come In"
#define CLOSED_MSG "Hours: M-F 9-5 Sat 10-3 Closed Sun"

// Hardware chip selects
// If your hardware isn't connected this way, this sketch won't work correctly
#define SD_CARD_CS     15
#define IR_RECV_CS     18
#define TEMP_SENSOR_CS 19

// Define the optional hardware. If missing hardware set value to 0
#define HAS_RTC         1
#define HAS_TEMP_SENSOR 1
#define HAS_SD_CARD     1

// Include all include files
#include "IRremote.h"
#include "SdFat.h"
#include "SdFatUtil.h"
#include "Time.h"
#include "OneWire.h"
#include "SmartMatrix_32x32.h"
#include "Types.h"

// Defined in FilenameFunctions.cpp
extern int numberOfFiles;
extern int enumerateGIFFiles(const char *directoryName, boolean displayFilenames);
extern void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer);
extern void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer);

// Defined in GIFParseFunctions.cpp
extern int processGIFFile(const char *pathname);

// GIF file directories
#define GENERAL_GIFS   "/gengifs/"
#define CHRISTMAS_GIFS "/xmasgifs/"
#define HALLOWEEN_GIFS "/halogifs/"
#define VALENTINE_GIFS "/valgifs/"
#define FOURTH_GIFS    "/4thgifs/"

// IR Raw Key Codes for SparkFun remote
#define IRCODE_HOME  0x10EFD827
#define IRCODE_A     0x10EFF807
#define IRCODE_B     0x10EF7887
#define IRCODE_C     0x10EF58A7
#define IRCODE_UP    0x10EFA05F
#define IRCODE_LEFT  0x10EF10EF
#define IRCODE_SEL   0x10EF20DF
#define IRCODE_RIGHT 0x10EF807F
#define IRCODE_DOWN  0x10EF00FF

// Pattern and animation display timeout values
#define PATTERN_DISPLAY_DURATION_SECONDS   30
#define ANIMATION_DISPLAY_DURATION_SECONDS 20

// Create required instances
IRrecv irReceiver(IR_RECV_CS);
SmartMatrix matrix;

#if (HAS_TEMP_SENSOR == 1)
// Create instance
OneWire tempSensor(TEMP_SENSOR_CS);

// Sensor address
byte sensorAddr[12];
byte sensorType;
boolean sensorPresent;
#endif

#if (HAS_SD_CARD == 1)
// Create instance
SdFat sd;    // SD card interface
#endif

const int DEFAULT_BRIGHTNESS = 100;
const rgb24 COLOR_BLACK  = {
    0,   0,   0};
const rgb24 COLOR_RED    = {
    255,   0,   0};
const rgb24 COLOR_GREEN  = {
    0, 255,   0};
const rgb24 COLOR_LGREEN = {
    0,  80,   0};
const rgb24 COLOR_BLUE   = {
    0,   0, 255};
const rgb24 COLOR_WHITE  = {
    255, 255, 255};

const int WIDTH = 32;
const int MINX  = 0;
const int MAXX  = WIDTH - 1;
const int MIDX  = WIDTH / 2;

const int HEIGHT = 32;
const int MINY   = 0;
const int MAXY   = HEIGHT - 1;
const int MIDY   = HEIGHT / 2;

// Array of LED Matrix modes
NAMED_FUNCTION modes [] = {
    "Turn Off",              offMode,
    "General Animations",    generalAnimationsMode,
    "Christmas Animations",  christmasAnimationsMode,
    "Halloween Animations",  halloweenAnimationsMode,
    "Valentine Animations",  valentineAnimationsMode,
    "4th Animations",        fourthAnimationsMode,
    "Patterns Mode",         randomPatternsMode,
    "Select Pattern Mode",   selectPatternMode,
    "Mood Light Mode",       moodLightMode,

#if (HAS_RTC == 1)
    "Set Time & Date Mode",  setTimeDateMode,
    "Time & Date Mode",      timeDateMode,
    "Analog Clock Mode",     analogClockMode,

#if (HAS_TEMP_SENSOR == 1)
    "Time & Temp Mode",      timeAndTempMode,
#endif
#endif
    "Open Sign Mode",        openSignMode,
    "Closed Sign Mode",      closedSignMode,
};

// Determine how many modes of operation there are
#define NUMBER_OF_MODES (sizeof(modes) / sizeof(NAMED_FUNCTION))

// Array of named pattern display functions
// To add a pattern, just create a new function and insert it and its name
// in this array.
NAMED_FUNCTION namedPatternFunctions [] = {
    "Animation",           animationPattern,
    "Recursive Circles",   recursiveCircles,
    "Concentric Circles",  concentricCirclesPattern,
    "Concentric Squares",  concentricSquaresPattern,
    "Colored Boxes",       coloredBoxesPattern,
    "T Square Fractal",    tSquareFractalPattern,
    "Rotating Rects",      rectRotatingColorsPattern,
    "Rotating Lines",      rotatingLinesPattern,
    "Matrix",              matrixPattern,
    "Rotating Rectangles", rotatingRectsPattern,
    "Welcome",             welcomePattern,
    "Sin Waves 1",         sineWaves1Pattern,
    "Sin Waves 2",         sineWaves2Pattern,
    "Radiating Lines",     radiatingLinesPattern,
    "Plasma1",             plasma1Pattern,
    "Plasma2",             plasma2Pattern,
    "Aurora1",             aurora1Pattern,
    "Aurora2",             aurora2Pattern,
    "Crawlers",            crawlerPattern,
    "Random Circles",      randomCirclesPattern,
    "Random Triangles",    randomTrianglesPattern,
    "Random Lines 1",      randomLines1Pattern,
    "Random Lines 2",      randomLines2Pattern,
    "Random Pixels",       randomPixelsPattern,
    "Horiz Palette Lines", horizontalPaletteLinesPattern,
    "Vert Palette Lines",  verticalPaletteLinesPattern,
};

// Determine the number of display patterns from the entries in the array
#define NUMBER_OF_PATTERNS (sizeof(namedPatternFunctions) / sizeof(NAMED_FUNCTION))

// Create array of flags for display pattern selection
byte flags[NUMBER_OF_PATTERNS];

// Simulate turning the light appliance off
void offMode() {
    unsigned long irCode;

    matrix.scrollText("", 1);
    matrix.fillScreen(COLOR_BLACK);
    matrix.swapBuffers();

    while (true) {
        irCode = waitForIRCode();
        if (irCode == IRCODE_HOME) {
            return;
        }
        delay(500);
    }
}

// Open Sign Mode
void openSignMode() {

    rgb24 bgColor = {
        0, 30, 30                                                                                                                        };

    matrix.fillScreen(bgColor);

    // Setup for scrolling mode
    matrix.setScrollMode(wrapForward);
    matrix.setScrollSpeed(10);
    matrix.setScrollFont(font6x10);
    matrix.setScrollColor(COLOR_GREEN);
    matrix.setScrollOffsetFromEdge(22);
    matrix.scrollText(OPEN_MSG, -1);

    matrix.setFont(font8x13);

    int textMode = 0;

    while (true) {

        // First clear the string area
        clearString(0, 3, bgColor, "OPEN");
        switch (textMode) {

        case 0:
            matrix.drawString(0, 3, COLOR_WHITE, "O");
            break;

        case 1:
            matrix.drawString(0, 3, COLOR_WHITE, "OP");
            break;

        case 2:
            matrix.drawString(0, 3, COLOR_WHITE, "OPE");
            break;

        case 3:
            matrix.drawString(0, 3, COLOR_WHITE, "OPEN");
            break;

        case 4:
            clearString(0, 3, bgColor, "OPEN");
            break;
        }
        textMode++;
        if (textMode >= 5) {
            textMode = 0;
        }

        matrix.swapBuffers();
        delay(1000);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

// Closed Sign Mode
void closedSignMode() {
    rgb24 bgColor = {
        0, 30, 30                                                                                                                        };

    matrix.fillScreen(bgColor);

    // Setup for scrolling mode
    matrix.setScrollMode(wrapForward);
    matrix.setScrollSpeed(10);
    matrix.setScrollFont(font5x7);
    matrix.setScrollColor(COLOR_GREEN);
    matrix.setScrollOffsetFromEdge(22);
    matrix.scrollText(CLOSED_MSG, -1);

    matrix.setFont(font5x7);

    int textMode = 0;

    while (true) {

        // First clear the string area
        clearString(2, 5, bgColor, "CLOSED");
        switch (textMode) {

        case 0:
            matrix.drawString(2, 5, COLOR_WHITE, "C");
            break;

        case 1:
            matrix.drawString(2, 5, COLOR_WHITE, "CL");
            break;

        case 2:
            matrix.drawString(2, 5, COLOR_WHITE, "CLO");
            break;

        case 3:
            matrix.drawString(2, 5, COLOR_WHITE, "CLOS");
            break;

        case 4:
            matrix.drawString(2, 5, COLOR_WHITE, "CLOSE");
            break;

        case 5:
            matrix.drawString(2, 5, COLOR_WHITE, "CLOSED");
            break;

        case 6:
            clearString(2, 5, bgColor, "CLOSED");
            break;
        }
        textMode++;
        if (textMode >= 7) {
            textMode = 0;
        }

        matrix.swapBuffers();
        delay(1000);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

// Select a specific pattern to run until aborted
void selectPatternMode() {
    char *patternName;
    ptr2Function patternFunction;

    int patternIndex = 0;

    while (true) {

        // Clear screen
        matrix.fillScreen(COLOR_BLACK);

        // Fonts are font3x5, font5x7, font6x10, font8x13
        matrix.setFont(font5x7);

        // Static Mode Selection Text
        matrix.drawString(2, 0, COLOR_BLUE, "Select");

        matrix.setFont(font3x5);
        matrix.drawString(3, 7, COLOR_BLUE, "Pattern");
        matrix.drawString(3, 14, COLOR_BLUE, "< use >");
        matrix.swapBuffers();

        // Setup for scrolling mode
        matrix.setScrollMode(wrapForward);
        matrix.setScrollSpeed(10);
        matrix.setScrollFont(font5x7);
        matrix.setScrollColor(COLOR_GREEN);
        matrix.setScrollOffsetFromEdge(22);
        matrix.scrollText("", 1);

        boolean patternSelected = false;

        while (! patternSelected) {
            // Get mode name and function
            patternName     = namedPatternFunctions[patternIndex].name;
            patternFunction = namedPatternFunctions[patternIndex].function;

            // Set pattern selection text
            matrix.scrollText(patternName, 32000);

            unsigned long irCode = waitForIRCode();
            switch(irCode) {
            case IRCODE_HOME:
                return;

            case IRCODE_LEFT:
                patternIndex--;
                if (patternIndex < 0) {
                    patternIndex = NUMBER_OF_PATTERNS - 1;
                }
                break;

            case IRCODE_RIGHT:
                patternIndex++;
                if (patternIndex >= NUMBER_OF_PATTERNS) {
                    patternIndex = 0;
                }
                break;

            case IRCODE_SEL:
                // Turn off any text scrolling
                matrix.scrollText("", 1);
                matrix.setScrollMode(off);

                // Clear screen
                matrix.fillScreen(COLOR_BLACK);
                matrix.swapBuffers();

                // Run pattern
                (*patternFunction)();
                patternSelected = true;
                break;
            }
        }
    }
}

#define POLL_COUNT 50

// Random pattern selection mode
void randomPatternsMode() {

    while (true) {
        // Select a random pattern and run it
        selectPatternAndRun();

        for (int i = 0; i < POLL_COUNT; i++) {
            // Has user aborted ?
            if (readIRCode() == IRCODE_HOME) {
                return;
            }
            delay(10);
        }
    }
}


// Mood Light Mode
void moodLightMode() {
    float hue, sat, val;
    rgb24 color, scrollTextColor;
    unsigned long irCode;
    int adjIndex;


    const float hueIncrement = 10.0;  // 36 possible hue values
    const float satIncrement =  0.1;  // 10 possible saturation values
    const float valIncrement =  0.05; // 20 possible value values

    // This process has two distinct states
    enum STATES {
        STATE_WAITING, STATE_SHOW_COLOR
    };

    // This process has three types of display
    enum DISPLAY_TYPE {
        TYPE_FULL, TYPE_HALF, TYPE_THIRD
    };

    // Set up text scrolling parameters
    matrix.setScrollMode(wrapForward);
    matrix.setScrollSpeed(10);
    matrix.setScrollFont(font6x10);
    matrix.setScrollOffsetFromEdge(11);

    // Set initial settings
    STATES state = STATE_SHOW_COLOR;
    DISPLAY_TYPE displayType = TYPE_FULL;
    adjIndex = 0;        // Hue adjustment selected

    matrix.setScrollColor({
        255, 255, 0                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    }
    );
    matrix.scrollText("Hue Adjust", 1);

    // Default mood color
    hue = 240;
    sat = 1.0;
    val = 1.0;

    while (true) {
        switch (state) {
        case STATE_WAITING:
            {
                // Read the remote
                irCode = waitForIRCode();

                // Process the code
                switch (irCode) {

                case IRCODE_HOME:
                    // Return to caller
                    return;

                case IRCODE_A:
                    {
                        displayType = TYPE_FULL;
                        state = STATE_SHOW_COLOR;
                    }
                    break;

                case IRCODE_B:
                    {
                        displayType = TYPE_HALF;
                        state = STATE_SHOW_COLOR;
                    }
                    break;

                case IRCODE_C:
                    {
                        displayType = TYPE_THIRD;
                        state = STATE_SHOW_COLOR;
                    }
                    break;

                case IRCODE_LEFT:
                    {
                        // Prepare a compliment color for text
                        scrollTextColor.red   = 255 - color.red;
                        scrollTextColor.green = 255 - color.green;
                        scrollTextColor.blue  = 255 - color.blue;

                        // Set the scroll text color
                        matrix.setScrollColor(scrollTextColor);

                        adjIndex--;
                        if (adjIndex < 0) {
                            adjIndex = 0;
                        }

                        switch (adjIndex) {
                        case 0:
                            matrix.scrollText("Hue Adjust", 1);
                            break;
                        case 1:
                            matrix.scrollText("Saturation Adjust", 1);
                            break;
                        case 2:
                            matrix.scrollText("Value Adjust", 1);
                            break;
                        }
                    }
                    break;

                case IRCODE_RIGHT:
                    {
                        // Prepare a compliment color for text
                        scrollTextColor.red   = 255 - color.red;
                        scrollTextColor.green = 255 - color.green;
                        scrollTextColor.blue  = 255 - color.blue;

                        // Set the scroll text color
                        matrix.setScrollColor(scrollTextColor);

                        adjIndex++;
                        if (adjIndex > 2) {
                            adjIndex = 2;
                        }

                        switch (adjIndex) {
                        case 0:
                            matrix.scrollText("Hue Adjust", 1);
                            break;
                        case 1:
                            matrix.scrollText("Saturation Adjust", 1);
                            break;
                        case 2:
                            matrix.scrollText("Value Adjust", 1);
                            break;
                        }
                    }
                    break;

                case IRCODE_UP:  // Parameter increment
                    {
                        switch (adjIndex) {
                        case 0:
                            {
                                hue += hueIncrement;
                                if (hue >= 360.0) {
                                    hue = 359.9;
                                }
                            }
                            break;

                        case 1:
                            {
                                sat += satIncrement;
                                if (sat > 1.0) {
                                    sat = 1.0;
                                }
                            }
                            break;

                        case 2:
                            {
                                val += valIncrement;
                                if (val > 1.0) {
                                    val = 1.0;
                                }
                            }
                            break;
                        }

                        state = STATE_SHOW_COLOR;
                    }
                    break;

                case IRCODE_DOWN:    // Parameter decrement
                    {
                        switch (adjIndex) {
                        case 0:
                            {
                                hue -= hueIncrement;
                                if (hue <  0.0) {
                                    hue = 0.0;
                                }
                            }
                            break;

                        case 1:
                            {
                                sat -= satIncrement;
                                if (sat < 0.0) {
                                    sat = 0.0;
                                }
                            }
                            break;

                        case 2:
                            {
                                val -= valIncrement;
                                if (val < 0.0) {
                                    val = 0.0;
                                }
                            }
                            break;
                        }

                        state = STATE_SHOW_COLOR;
                    }
                    break;
                }
                break;

            case STATE_SHOW_COLOR:
                {
                    // First clear the display
                    matrix.fillScreen(COLOR_BLACK);

                    // Convert hsv color to rgb color
                    color = createHSVColor(hue, sat, val);

                    switch (displayType) {

                    case TYPE_FULL:
                        matrix.fillScreen(color);
                        break;

                    case TYPE_HALF:
                        for (int y = 0; y < MAXY; y += 2) {
                            for (int x = 0; x < MAXX; x += 2) {
                                matrix.drawPixel(x, y, color);
                            }
                        }
                        break;


                    case TYPE_THIRD:
                        for (int y = 0; y < MAXY; y += 3) {
                            for (int x = 0; x < MAXX; x += 3) {
                                matrix.drawPixel(x, y, color);
                            }
                        }
                        break;
                    }
                    // Make changes visable
                    matrix.swapBuffers();

                    // Set next state
                    state = STATE_WAITING;
                }
                break;
            }
        }
    }
}

#if (HAS_RTC == 1)

char timeDateBuffer[32];
char *monthNameArray [] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jly", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *dayNameArray [] = {
    "", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

int cHour, cMin, cDay, cMon, cDate, cYear, cAmPm;

// Process User Interaction when setting the RTC
USER_INTERACTION_CODE doUserInteraction(ptr2SetFunction upFunction, ptr2SetFunction downFunction,
rgb24 fgColor, rgb24 bgColor) {

    while (true) {
        switch (waitForIRCode()) {

        case IRCODE_UP:
            (*upFunction)(fgColor, bgColor);
            break;

        case IRCODE_DOWN:
            (*downFunction)(fgColor, bgColor);
            break;

        case IRCODE_HOME:
            return UICODE_HOME;

        case IRCODE_LEFT:
            return UICODE_LEFT;

        case IRCODE_SEL:
            return UICODE_SELECT;

        case IRCODE_RIGHT:
            return UICODE_RIGHT;
        }
    }
}

// Time and Date Up and Down Functions for use with function above
void hourUpFunction(rgb24 fgColor, rgb24 bgColor) {
    cHour++;
    if (cHour > 12) {
        cHour = 12;
    }
    sprintf(timeDateBuffer, "%d", cHour);

    clearString(8, 20, bgColor, "    ");
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
}

void hourDownFunction(rgb24 fgColor, rgb24 bgColor) {
    cHour--;
    if (cHour < 1) {
        cHour = 1;
    }
    sprintf(timeDateBuffer, "%d", cHour);

    clearString(8, 20, bgColor, "    ");
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
}

// Set Hour UI
enum USER_INTERACTION_CODE doSetHour(rgb24 fgColor, rgb24 bgColor) {

    matrix.fillScreen(bgColor);
    matrix.setFont(font6x10);
    matrix.drawString(8, 0, fgColor, "Set");
    matrix.drawString(5, 8, fgColor, "Hour");
    sprintf(timeDateBuffer, "%d", cHour);
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
    return doUserInteraction(hourUpFunction, hourDownFunction, fgColor, bgColor);
}

void minUpFunction(rgb24 fgColor, rgb24 bgColor) {
    cMin++;
    if (cMin > 59) {
        cMin = 59;
    }
    sprintf(timeDateBuffer, "%d", cMin);

    clearString(8, 20, bgColor, "    ");
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
}

void minDownFunction(rgb24 fgColor, rgb24 bgColor) {
    cMin--;
    if (cMin < 0) {
        cMin = 0;
    }
    sprintf(timeDateBuffer, "%d", cMin);

    clearString(8, 20, bgColor, "    ");
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
}

// Set Min UI
enum USER_INTERACTION_CODE doSetMin(rgb24 fgColor, rgb24 bgColor) {

    matrix.fillScreen(bgColor);
    matrix.setFont(font6x10);
    matrix.drawString(8, 0, fgColor, "Set");
    matrix.drawString(8, 8, fgColor, "Min");
    sprintf(timeDateBuffer, "%d", cMin);
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
    return doUserInteraction(minUpFunction, minDownFunction, fgColor, bgColor);
}

void amPmUpFunction(rgb24 fgColor, rgb24 bgColor) {
    cAmPm++;
    if (cAmPm > 1) {
        cAmPm = 1;
    }
    clearString(8, 20, bgColor, "    ");
    matrix.drawString(11, 20, fgColor, (cAmPm == 0) ? "AM" : "PM");
    matrix.swapBuffers();
}

void amPmDownFunction(rgb24 fgColor, rgb24 bgColor) {
    cAmPm--;
    if (cAmPm < 0) {
        cAmPm = 0;
    }
    clearString(8, 20, bgColor, "    ");
    matrix.drawString(11, 20, fgColor, (cAmPm == 0) ? "AM" : "PM");
    matrix.swapBuffers();
}

// Set Am/Pm UI
enum USER_INTERACTION_CODE doSetAmPm(rgb24 fgColor, rgb24 bgColor) {

    matrix.fillScreen(bgColor);
    matrix.setFont(font6x10);
    matrix.drawString(8, 0, fgColor, "Set");
    matrix.drawString(2, 8, fgColor, "AM/PM");
    matrix.drawString(11, 20, fgColor, (cAmPm == 0) ? "AM" : "PM");
    matrix.swapBuffers();
    return doUserInteraction(amPmUpFunction, amPmDownFunction, fgColor, bgColor);
}

void dayUpFunction(rgb24 fgColor, rgb24 bgColor) {

    cDay++;
    if (cDay > 7) {
        cDay = 7;
    }
    clearString(8, 20, bgColor, "    ");
    matrix.drawString(8, 20, fgColor, dayNameArray[cDay]);
    matrix.swapBuffers();
}

void dayDownFunction(rgb24 fgColor, rgb24 bgColor) {

    cDay--;
    if (cDay < 1) {
        cDay = 1;
    }
    clearString(8, 20, bgColor, "    ");
    matrix.drawString(8, 20, fgColor, dayNameArray[cDay]);
    matrix.swapBuffers();
}

// Set Day UI
enum USER_INTERACTION_CODE doSetDay(rgb24 fgColor, rgb24 bgColor) {

    matrix.fillScreen(bgColor);
    matrix.setFont(font6x10);
    matrix.drawString(8, 0, fgColor, "Set");
    matrix.drawString(8, 8, fgColor, "Day");
    matrix.drawString(8, 20, fgColor, dayNameArray[cDay]);
    matrix.swapBuffers();
    return doUserInteraction(dayUpFunction, dayDownFunction, fgColor, bgColor);
}

void monUpFunction(rgb24 fgColor, rgb24 bgColor) {

    cMon++;
    if (cMon > 12) {
        cMon = 12;
    }
    clearString(8, 20, bgColor, "    ");
    matrix.drawString(8, 20, fgColor, monthNameArray[cMon]);
    matrix.swapBuffers();
}

void monDownFunction(rgb24 fgColor, rgb24 bgColor) {

    cMon--;
    if (cMon < 1) {
        cMon = 1;
    }
    clearString(8, 20, bgColor, "    ");
    matrix.drawString(8, 20, fgColor, monthNameArray[cMon]);
    matrix.swapBuffers();
}

// Set Mon UI
enum USER_INTERACTION_CODE doSetMon(rgb24 fgColor, rgb24 bgColor) {

    matrix.fillScreen(bgColor);
    matrix.setFont(font6x10);
    matrix.drawString(8, 0, fgColor, "Set");
    matrix.drawString(2, 8, fgColor, "Month");
    matrix.drawString(8, 20, fgColor, monthNameArray[cMon]);
    matrix.swapBuffers();
    return doUserInteraction(monUpFunction, monDownFunction, fgColor, bgColor);
}

void dateUpFunction(rgb24 fgColor, rgb24 bgColor) {

    cDate++;
    if (cDate > 31) {
        cDate = 31;
    }
    clearString(8, 20, bgColor, "    ");
    sprintf(timeDateBuffer, "%d", cDate);
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
}

void dateDownFunction(rgb24 fgColor, rgb24 bgColor) {

    cDate--;
    if (cDate < 1) {
        cDate = 1;
    }
    clearString(8, 20, bgColor, "    ");
    sprintf(timeDateBuffer, "%d", cDate);
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
}

// Set Date UI
enum USER_INTERACTION_CODE doSetDate(rgb24 fgColor, rgb24 bgColor) {

    matrix.fillScreen(bgColor);
    matrix.setFont(font6x10);
    matrix.drawString(8, 0, fgColor, "Set");
    matrix.drawString(5, 8, fgColor, "Date");
    sprintf(timeDateBuffer, "%d", cDate);
    matrix.drawString(12, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
    return doUserInteraction(dateUpFunction, dateDownFunction, fgColor, bgColor);
}

void yearUpFunction(rgb24 fgColor, rgb24 bgColor) {

    cYear++;

    clearString(8, 20, bgColor, "    ");
    sprintf(timeDateBuffer, "%d", cYear);
    matrix.drawString(5, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
}

void yearDownFunction(rgb24 fgColor, rgb24 bgColor) {

    cYear--;
    if (cYear < 2014) {
        cYear = 2014;
    }
    clearString(8, 20, bgColor, "    ");
    sprintf(timeDateBuffer, "%d", cYear);
    matrix.drawString(5, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
}

// Set Year UI
enum USER_INTERACTION_CODE doSetYear(rgb24 fgColor, rgb24 bgColor) {

    matrix.fillScreen(bgColor);
    matrix.setFont(font6x10);
    matrix.drawString(8, 0, fgColor, "Set");
    matrix.drawString(5, 8, fgColor, "Year");
    sprintf(timeDateBuffer, "%d", cYear);
    matrix.drawString(5, 20, fgColor, timeDateBuffer);
    matrix.swapBuffers();
    return doUserInteraction(yearUpFunction, yearDownFunction, fgColor, bgColor);
}

// Set Time and Date Mode
void setTimeDateMode() {
    Serial.println("Set Time and Date Mode");

    enum SET_STATES {
        SET_HOUR, SET_MIN, SET_AMPM, SET_DAY, SET_MON, SET_DATE, SET_YEAR, SET_DONE
    };

    // We start by setting hours
    SET_STATES state = SET_HOUR;

    boolean dirty = false;

    // Disable scrolling if any
    matrix.scrollText("", 1);

    rgb24 bgColor = COLOR_BLACK;
    rgb24 fgColor = {
        9, 255, 202                                                                                                                                                                                                                                                                                                                                                                                                                };

    time_t t = now();        // Get now time
    cHour = hourFormat12();
    cMin  = minute(t);
    cDay  = weekday(t);
    cMon  = month(t);
    cDate = day(t);
    cYear = year(t);
    cAmPm = isAM() ? 0 : 1;

    while (true) {

        // Run time and date setting state machine
        switch (state) {
        case SET_HOUR:
            {
                switch (doSetHour(fgColor, bgColor)) {
                case UICODE_HOME:
                    return;
                case UICODE_SELECT:
                    dirty = true;
                    state = SET_MIN;
                    break;
                case UICODE_LEFT:
                    break;
                case UICODE_RIGHT:
                    state = SET_MIN;
                    break;
                }
            }
            break;

        case SET_MIN:
            {
                switch (doSetMin(fgColor, bgColor)) {
                case UICODE_HOME:
                    return;
                case UICODE_SELECT:
                    dirty = true;
                    state = SET_AMPM;
                    break;
                case UICODE_LEFT:
                    state = SET_HOUR;
                    break;
                case UICODE_RIGHT:
                    state = SET_AMPM;
                    break;
                }
            }
            break;

        case SET_AMPM:
            {
                switch (doSetAmPm(fgColor, bgColor)) {
                case UICODE_HOME:
                    return;
                case UICODE_SELECT:
                    dirty = true;
                    state = SET_DAY;
                    break;
                case UICODE_LEFT:
                    state = SET_MIN;
                    break;
                case UICODE_RIGHT:
                    state = SET_DAY;
                    break;
                }
            }
            break;

        case SET_DAY:
            {
                switch (doSetDay(fgColor, bgColor)) {
                case UICODE_HOME:
                    return;
                case UICODE_SELECT:
                    dirty = true;
                    state = SET_MON;
                    break;
                case UICODE_LEFT:
                    state = SET_AMPM;
                    break;
                case UICODE_RIGHT:
                    state = SET_MON;
                    break;
                }
            }
            break;

        case SET_MON:
            {
                switch (doSetMon(fgColor, bgColor)) {
                case UICODE_HOME:
                    return;
                case UICODE_SELECT:
                    dirty = true;
                    state = SET_DATE;
                    break;
                case UICODE_LEFT:
                    state = SET_DAY;
                    break;
                case UICODE_RIGHT:
                    state = SET_DATE;
                    break;
                }
            }
            break;

        case SET_DATE:
            {
                switch (doSetDate(fgColor, bgColor)) {
                case UICODE_HOME:
                    return;
                case UICODE_SELECT:
                    dirty = true;
                    state = SET_YEAR;
                    break;
                case UICODE_LEFT:
                    state = SET_MON;
                    break;
                case UICODE_RIGHT:
                    state = SET_YEAR;
                    break;
                }
            }
            break;

        case SET_YEAR:
            {
                switch (doSetYear(fgColor, bgColor)) {
                case UICODE_HOME:
                    return;
                case UICODE_SELECT:
                    dirty = true;
                    state = SET_DONE;
                    break;
                case UICODE_LEFT:
                    state = SET_DATE;
                    break;
                case UICODE_RIGHT:
                    state = SET_DONE;
                    break;
                }
            }
            break;

        case SET_DONE:
            {
                if (dirty) {
                    // Convert 12 hour to 24 hour format for setting RTC
                    if ((cAmPm == 1) && (cHour < 12)) {
                        cHour += 12;
                    }
                    if ((cAmPm == 0) && (cHour == 12)) {
                        cHour -= 12;
                    }

                    // Set the time and date into the Time library
                    setTime(cHour, cMin, 0, cDate, cMon, cYear);

                    // Set the system time into the RTC
                    Teensy3Clock.set(now());
                }
                return;
            }
            break;
        }
    }
}

// Analog Clock Attributes
#define NUMERICS_COLOR   {120, 120, 120}

#define HOUR_HAND_RADIUS 6
#define HOUR_HAND_COLOR  {0, 0, 255}

#define MIN_HAND_RADIUS  10
#define MIN_HAND_COLOR   {0, 255, 0}

#define SEC_IND_RADIUS   15
#define SEC_IND_COLOR    {255, 165, 0}

#define TIC_RADIUS       13
#define TIC_COLOR        {255, 0, 0}

// Draw clock tics
void drawClockTics() {
    int x, y;

    // Draw tics
    for (int d = 0; d < 360; d+=30) {
        int x = round(MIDX + TIC_RADIUS * cos(d * M_PI / 180.0));
        int y = round(MIDY - TIC_RADIUS * sin(d * M_PI / 180.0));

        matrix.drawPixel(x, y, TIC_COLOR);
    }
    matrix.swapBuffers();        
}

// Draw clock numerics
void drawClockNumerics() {

    // Draw numerics
    matrix.drawString(13,  4, NUMERICS_COLOR, "12");
    matrix.drawString(26, 14, NUMERICS_COLOR, "3");
    matrix.drawString(15, 24, NUMERICS_COLOR, "6");
    matrix.drawString( 4, 14, NUMERICS_COLOR, "9");

    matrix.swapBuffers();        
}

void _drawHourHand(int hour, rgb24 color) {

    double radians = (90 - (hour * 30)) * M_PI / 180.0;

    int x = round(MIDX + HOUR_HAND_RADIUS * cos(radians));
    int y = round(MIDY - HOUR_HAND_RADIUS * sin(radians));

    matrix.drawLine(MIDX, MIDY, x, y, color);
}

void drawHourHand(int hour) {

    static int oldHour = 12;

    _drawHourHand(oldHour, COLOR_BLACK);
    _drawHourHand(hour, HOUR_HAND_COLOR);
    matrix.swapBuffers();        
    oldHour = hour;
}

void _drawMinHand(int min, rgb24 color) {

    double radians = (90 - (min * 6)) * M_PI / 180.0;

    int x = round(MIDX + MIN_HAND_RADIUS * cos(radians));
    int y = round(MIDY - MIN_HAND_RADIUS * sin(radians));

    matrix.drawLine(MIDX, MIDY, x, y, color);
}

void drawMinHand(int min) {

    static int oldMin = 0;

    _drawMinHand(oldMin, COLOR_BLACK);
    _drawMinHand(min, MIN_HAND_COLOR);
    matrix.swapBuffers();        
    oldMin = min;
}

void _drawSecIndicator(int sec, rgb24 color) {

    double radians = (90 - (sec * 6)) * M_PI / 180.0;

    int x = round(MIDX + SEC_IND_RADIUS * cos(radians));
    int y = round(MIDY - SEC_IND_RADIUS * sin(radians));

    matrix.drawPixel(x, y, color);
}

void drawSecIndicator(int sec) {

    static int oldSec = 0;

    _drawSecIndicator(oldSec, COLOR_BLACK);
    _drawSecIndicator(sec, SEC_IND_COLOR);
    matrix.swapBuffers();        
    oldSec = sec;
}

// Analog Clock Mode
void analogClockMode() {

    time_t t;
    int hr, min, sec, oldSec = -1;

    Serial.println("Analog Clock Mode");
    
    // Clear screen
    matrix.fillScreen(COLOR_BLACK);
    matrix.swapBuffers();
    
    // Turn off scrolling
    matrix.scrollText("", 1);

    // Draw clock face
    drawClockTics();
    drawClockNumerics();

    while (true) {

        t = now();              // Get now time
        sec = second(t);        // Get the seconds count

        if (oldSec != sec) {    // Has a second elasped ?
            // Yes, time to update display
            oldSec = sec;       // Update old value
            
            hr = hourFormat12();
            min = minute(t);

            drawHourHand(hr);
            drawMinHand(min);
            drawSecIndicator(sec);

            // Redraw numerics in case min hand drew over them
            drawClockNumerics();
        }    

        // See if user has aborted
        if (readIRCode() == IRCODE_HOME) {
            return;
        }
        
        delay(200);
    }
}

extern int scrollcounter;

// Show Time and Date Mode
void timeDateMode() {

    unsigned long irCode;

    Serial.println("Time and Date Mode");

    rgb24 bgColor = COLOR_BLACK;
    rgb24 fgColor = {
        255, 202, 9                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                };

    matrix.fillScreen(bgColor);
    matrix.swapBuffers();

    // Setup for scrolling
    matrix.setScrollMode(wrapForward);
    matrix.setScrollSpeed(15);
    matrix.setScrollFont(font8x13);
    matrix.setScrollColor(fgColor);
    matrix.setScrollOffsetFromEdge(10);

    while (true) {

        // Format the time and date into timeDateBuffer
        formatTimeDate();

        // Scroll the formatted time and date once
        matrix.scrollText(timeDateBuffer, 1);

        // Wait while scrolling
        while ((scrollcounter != 0) && ((irCode = readIRCode()) != IRCODE_HOME)) {
            delay(200);
        }
        // See if user has aborted
        if (irCode == IRCODE_HOME) {
            return;
        }
        // Wait a second
        delay(1000);
    }
}

#if (HAS_TEMP_SENSOR == 1)

char temperatureBuffer[10];

// Time and Temperature Display Mode
void timeAndTempMode() {
    unsigned long irCode;
    float tempF;

    Serial.println("Time, Date and Temp Mode");

    rgb24 bgColor = COLOR_BLACK;
    rgb24 fgColor = {
        255, 202, 9                                                                                                                                                                                                                                                                                                                                                                                                                                                                            };
    rgb24 tempColor = {
        9, 255, 202                                                                                                                                                                                                                                                                                                                                                                                                                                                                            };

    matrix.fillScreen(bgColor);
    matrix.swapBuffers();

    // Setup for scrolling display
    matrix.setScrollMode(wrapForward);
    matrix.setScrollSpeed(15);
    matrix.setScrollFont(font8x13);
    matrix.setScrollColor(fgColor);
    matrix.setScrollOffsetFromEdge(19);
    matrix.scrollText("", 1);

    while (true) {
        // Clear the display
        matrix.fillScreen(bgColor);

        // Pick font
        matrix.setFont(font6x10);

        // Display temp message
        matrix.drawString(4, 0, tempColor, "Temp");

        // Get the temperature
        tempF = getTemperature(true);

        // Format the temperature string
        sprintf(temperatureBuffer, "%.1fF", tempF);

        // Pick font
        matrix.setFont(font5x7);

        // Auto center temperature string
        int width = 5 * strlen(temperatureBuffer);
        int xOffset = (WIDTH - width) / 2;

        // Adjust layout for lower temperature
        if (tempF < 100.0) {
            xOffset++;
        }

        // Display temperature
        matrix.drawString(xOffset, 12, tempColor, temperatureBuffer);

        // Make temp visible
        matrix.swapBuffers();

        // Format the time and date into timeDateBuffer
        formatTimeDate();

        // Scroll the formatted time and date once
        matrix.scrollText(timeDateBuffer, 1);

        // Wait while scrolling
        while ((scrollcounter != 0) && ((irCode = readIRCode()) != IRCODE_HOME)) {
            delay(200);
        }
        // See if user has aborted
        if (irCode == IRCODE_HOME) {
            return;
        }
        // Wait a second
        delay(1000);
    }
}
#endif
#endif


/*******************************************************************/
/***                Pattern Display Infrastructure               ***/
/*******************************************************************/


// How many time infrastructure will attempt to generate a random index
#define MAX_SPINS 100

boolean timeOutEnabled;
unsigned long timeOut;

// Check for pattern termination
// This must be called by every display pattern
boolean checkForTermination() {

    boolean timeOutCondition = timeOutEnabled && (millis() > timeOut);
    boolean userAbortCondition = (readIRCode() == IRCODE_HOME);

    if (! (timeOutCondition || userAbortCondition)) {
        return false;
    }
    timeOutEnabled = false;

    return true;
}

// Randomly select a pattern to run
// Return all patterns before allowing any repeats
int selectPattern() {

    int index;
    int spins = 0;

    while (true) {
        // Pick a candidate pattern index
        index = random(NUMBER_OF_PATTERNS);

        // Check to see if this pattern was previously selected
        if (flags[index] == 0) {
            // This index was not previously used, so mark it so
            flags[index] = 1;
            return index;
        }
        else  {
            if (++spins > MAX_SPINS) {
                memset(flags, 0, sizeof(flags));
                spins = 0;
            }
        }
    }
}

// Select a display pattern randomly and execute it
void selectPatternAndRun() {

    // Turn off any text scrolling
    matrix.scrollText("", 1);
    matrix.setScrollMode(off);

    matrix.setColorCorrection(cc24);

    // Clear screen
    matrix.fillScreen(COLOR_BLACK);
    matrix.swapBuffers();

    // Then a short delay
    delay(500);

    // Select a pattern to run
    int index = selectPattern();

    // Calculate future time to switch pattern
    timeOut = millis() + (1000 * PATTERN_DISPLAY_DURATION_SECONDS);

    // Enable time outs
    timeOutEnabled = true;

    // Start up the selected pattern by index
    (*namedPatternFunctions[index].function)();
}

#if (HAS_RTC == 1)
// Get the time from the RTC
time_t getTeensy3Time() {
    return Teensy3Clock.get();
}

// Format the time and date into buffer in the form:
// hh:min AM/PM Mon Jan 31 2014
void formatTimeDate() {

    boolean isAm = isAM();

    time_t t = now();        // Get now time
    int hr = hourFormat12();
    int mi = minute(t);
    int dy = day(t);
    int weekDay = weekday(t);
    int mo = month(t);
    int yr = year(t);

    sprintf(timeDateBuffer, "%d:%02d %s %s %s %d %d",
    hr, mi, isAm ? "AM" : "PM", dayNameArray[weekDay],  monthNameArray[mo], dy, yr);

    Serial.println(timeDateBuffer);
}
#endif

#if (HAS_TEMP_SENSOR == 1)
// Retrive the current temperature from DS18B20 sensor
float getTemperature(boolean inFahrenheit) {

    byte data[12];

    // Check if sensor is present
    if (! sensorPresent) {
        Serial.println("sensorPresent == false");
        return 0.0;
    }

    // Start temperature conversion with parasite power on at the end
    tempSensor.reset();
    tempSensor.select(sensorAddr);
    tempSensor.write(0x44, 1);

    delay(1000);                // 750 ms should be enough

    // Read scratchpad
    tempSensor.reset();
    tempSensor.select(sensorAddr);
    tempSensor.write(0xBE);

    // Read 9 bytes of temperature data
    for (int i = 0; i < 9; i++) {
        data[i] = tempSensor.read();
    }
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];

    if (sensorType) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
        }
    }
    else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
    }
    float celsius = (float) raw / 16.0;
    float fahrenheit = celsius * 1.8 + 32.0;

    Serial.println(fahrenheit);

    if (inFahrenheit) {
        return fahrenheit;
    }
    else    {
        return celsius;
    }
}
#endif

/*******************************************************************/
/***                        IR Functions                         ***/
/*******************************************************************/

// Low level IR code reading function
// Function will return 0 if no IR code available
unsigned long _readIRCode() {

    decode_results results;

    results.value = 0;

    // Attempt to read an IR code ?
    if (irReceiver.decode(&results)) {

        delay(20);

        // Prepare to receive the next IR code
        irReceiver.resume();
    }
    return results.value;
}

// Read an IR code
// Function will return 0 if no IR code available
unsigned long readIRCode() {

    // Is there an IR code to read ?
    unsigned long code = _readIRCode();
    if (code == 0) {
        // No code so return 0
        return 0;
    }
    // Keep reading until code changes
    while (_readIRCode() == code) {
        ;
    }
    return code;
}

unsigned long waitForIRCode() {

    unsigned long irCode = readIRCode();
    while ((irCode == 0) || (irCode == 0xFFFFFFFF)) {
        delay(200);
        irCode = readIRCode();
    }
    return irCode;
}

/*******************************************************************/
/***                         Misc Functions                      ***/
/*******************************************************************/

extern bitmap_font *font;

// Clear a portion of the matrix for overwriting
void clearString(int16_t x, int16_t y, rgb24 color, const char text[]) {
    int xcnt, ycnt, i = 0, offset = 0;
    char character;

    // limit text to 10 chars, why?
    for (i = 0; i < 10; i++) {
        character = text[offset++];
        if (character == '\0')
            return;

        for (ycnt = 0; ycnt < font->Height; ycnt++) {
            for (xcnt = 0; xcnt < font->Width; xcnt++) {
                matrix.drawPixel(x + xcnt, y + ycnt, color);
            }
        }
        x += font->Width;
    }
}

/*******************************************************************/
/***                     Main Program Setup                     ***/
/*******************************************************************/

void setup() {

#if (HAS_RTC == 1)
    setSyncProvider(getTeensy3Time);
#endif

    // Setup serial interface
    Serial.begin(115200);

    // Wait for serial interface to settle
    delay(2000);

    // Seed the random number generator
    randomSeed(analogRead(A14));

#if (HAS_SD_CARD == 1)
    // Initialize SD card interface
    Serial.print("Initializing SD card...");
    if (! sd.begin(SD_CARD_CS, SPI_HALF_SPEED)) {
        sd.initErrorHalt();
    }
    Serial.println("SD card initialized");
#endif

#if (HAS_RTC == 1)
    // Sync system time with RTC time
    if (timeStatus() != timeSet) {
        Serial.println("Unable to sync with the RTC");
    }
    else {
        Serial.println("RTC has set the system time");
    }
#endif

#if (HAS_TEMP_SENSOR == 1)
    // Search for a OneWire connected sensor
    sensorPresent = tempSensor.search(sensorAddr);

    if (sensorPresent) {

        if (OneWire::crc8(sensorAddr, 7) != sensorAddr[7]) {
            Serial.println("CRC is not valid!");
            sensorPresent = false;
        }
    }
    if (sensorPresent) {
        Serial.print("Temperature sensor present.");

        // First ROM byte indicates which chip
        switch (sensorAddr[0]) {
        case 0x10:
            Serial.println(" Chip = DS18S20");  // or old DS1820
            sensorType = 1;
            break;
        case 0x28:
            Serial.println(" Chip = DS18B20");
            sensorType = 0;
            break;
        case 0x22:
            Serial.println(" Chip = DS1822");
            sensorType = 0;
            break;
        default:
            Serial.println(" But device is not a DS18x20 family device.");
            sensorPresent = false;
        }
    }
#endif

    // Initialize IR receiver
    irReceiver.enableIRIn();

    // Initialize 32x32 LED Matrix
    matrix.begin();
    matrix.setBrightness(DEFAULT_BRIGHTNESS);
    matrix.setColorCorrection(cc24);

    // Clear screen
    matrix.fillScreen(COLOR_BLACK);
    matrix.swapBuffers();

    // Clear flags array
    memset(flags, 0, sizeof(flags));
}

/*******************************************************************/
/***                      Main Program Loop                      ***/
/*******************************************************************/

void loop() {

    char *modeStr;
    ptr2Function modeFunct;

    int modeIndex = 0;

    while (true) {

        matrix.setColorCorrection(cc24);

        // Clear screen
        matrix.fillScreen(COLOR_BLACK);

        // Fonts are font3x5, font5x7, font6x10, font8x13
        matrix.setFont(font5x7);

        // Static Mode Selection Text
        matrix.drawString(2, 0, COLOR_GREEN, "Select");
        matrix.drawString(6, 7, COLOR_GREEN, "Mode");

        matrix.setFont(font3x5);
        matrix.drawString(3, 14, COLOR_GREEN, "< use >");
        matrix.swapBuffers();

        // Setup for scrolling mode
        matrix.setScrollMode(wrapForward);
        matrix.setScrollSpeed(18);
        matrix.setScrollFont(font5x7);
        matrix.setScrollColor(COLOR_BLUE);
        matrix.setScrollOffsetFromEdge(22);
        matrix.scrollText("", 1);

        boolean modeSelected = false;

        while (! modeSelected) {
            // Get mode name and function
            modeStr   = modes[modeIndex].name;
            modeFunct = modes[modeIndex].function;

            // Set mode selection text
            matrix.scrollText(modeStr, 32000);

            unsigned long irCode = waitForIRCode();
            switch(irCode) {
            case IRCODE_LEFT:
                modeIndex--;
                if (modeIndex < 0) {
                    modeIndex = NUMBER_OF_MODES - 1;
                }
                break;
            case IRCODE_RIGHT:
                modeIndex++;
                if (modeIndex >= NUMBER_OF_MODES) {
                    modeIndex = 0;
                }
                break;
            case IRCODE_SEL:
                (*modeFunct)();
                modeSelected = true;
                break;
            }
        }
    }
}

/*******************************************************************/
/***           Color Constants, Variables and Functions          ***/
/*******************************************************************/

#define NUM_OF_COLOR_VALUES 256
#define MIN_COLOR_VALUE     0
#define MAX_COLOR_VALUE     255

// HSV to RGB color conversion
// Input arguments
// hue in degrees (0 - 360.0)
// saturation (0.0 - 1.0)
// value (0.0 - 1.0)
// Output arguments
// red, green blue (0.0 - 1.0)
void hsvToRGB(float hue, float saturation, float value, float * red, float * green, float * blue) {

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
rgb24 createHSVColor(float hue, float saturation,  float value) {

    float r, g, b;
    rgb24 color;

    hsvToRGB(hue, saturation, value, &r, &g, &b);

    color.red   = r * MAX_COLOR_VALUE;
    color.green = g * MAX_COLOR_VALUE;
    color.blue =  b * MAX_COLOR_VALUE;

    return color;
}

// Create an HSV color
rgb24 createHSVColor(int divisions, int index, float saturation, float value) {

    float hueAngle = (360.0 * index) / divisions;
    return createHSVColor(hueAngle, saturation, value);
}

// Create an HSV color
rgb24 createAHSVColor(int divisions, int index, float saturation, float value) {

    index %= divisions;
    return createHSVColor(divisions, index, saturation, value);
}

rgb24 createHSVColorWithDivisions(int divisions, int index) {

    return createAHSVColor(divisions, index, 1.0, 1.0);
}

/*******************************************************************/
/***             Palette definitions and Functions               ***/
/*******************************************************************/

#define PALETTE_SIZE        256
#define NUM_OF_PALETTES      12
#define GRAYSCALE_PALETTE     0
#define SPECTRUM_PALETTE      1
#define SIN1_PALETTE          2
#define SIN2_PALETTE          3
#define SIN3_PALETTE          4
#define SIN4_PALETTE          5
#define SIN5_PALETTE          6
#define RANDOM_PALETTE        7
#define FIRE_PALETTE          8
#define SEA_PALETTE           9
#define BLUERED_PALETTE       10
#define COLORWHEEL_PALETTE    11

// Create a palette structure for holding color information
rgb24 palette[PALETTE_SIZE];

// Generate a palette based upon parameter
void generatePaletteNumber(int paletteNumber) {

    int i;
    float r, g, b;
    rgb24 color;

    // Create some factors for randomizing the generated palettes
    // This helps keep the display colors interesting
    float f1 = random(16, 128);
    float f2 = random(16, 128);
    float f3 = random(16, 128);

    switch (paletteNumber) {
    case GRAYSCALE_PALETTE:
        {
            // Grayscale palette
            // Sometimes the light colors at low index; other times at high index
            boolean direction = (random(2) == 0);
            if (direction) {
                for (i = 0; i < PALETTE_SIZE; i++) {
                    palette[i].red   = i;
                    palette[i].green = i;
                    palette[i].blue  = i;
                }
            }
            else {
                for (i = 0; i < PALETTE_SIZE; i++) {
                    int j = 255 - i;
                    palette[i].red   = j;
                    palette[i].green = j;
                    palette[i].blue  = j;
                }
            }
        }
        break;

    case SPECTRUM_PALETTE:
        {
            // Full spectrum palette at full saturation and value
            for (i = 0; i < PALETTE_SIZE; i++) {
                palette[i] = createHSVColorWithDivisions(PALETTE_SIZE, i);
            }
        }
        break;

    case SIN1_PALETTE:
        {
            // Use sin function to generate palette
            for (i = 0; i < PALETTE_SIZE; i++) {
                r = MAX_COLOR_VALUE * ((sin(M_PI * i / f1) + 1.0) / 2.0);
                g = MAX_COLOR_VALUE * ((sin(M_PI * i / f2) + 1.0) / 2.0);
                b = MAX_COLOR_VALUE * ((sin(M_PI * i / f3) + 1.0) / 2.0);

                palette[i].red   = r;
                palette[i].green = g;
                palette[i].blue  = b;
            }
        }
        break;

    case SIN2_PALETTE:
        {
            // Use sin function to generate palette - no blue
            for (i = 0; i < PALETTE_SIZE; i++) {
                r = MAX_COLOR_VALUE * ((sin(M_PI * i / f1) + 1.0) / 2.0);
                g = MAX_COLOR_VALUE * ((sin(M_PI * i / f2) + 1.0) / 2.0);
                b = 0;

                palette[i].red   = r;
                palette[i].green = g;
                palette[i].blue  = b;
            }
        }
        break;

    case SIN3_PALETTE:
        {
            // Use sin function to generate palette - no green
            for (i = 0; i < PALETTE_SIZE; i++) {
                r = MAX_COLOR_VALUE * ((sin(M_PI * i / f1) + 1.0) / 2.0);
                g = 0;
                b = MAX_COLOR_VALUE * ((sin(M_PI * i / f2) + 1.0) / 2.0);

                palette[i].red   = r;
                palette[i].green = g;
                palette[i].blue  = b;
            }
        }
        break;

    case SIN4_PALETTE:
        {
            // Use sin function to generate palette - no red
            for (i = 0; i < PALETTE_SIZE; i++) {
                r = 0;
                g = MAX_COLOR_VALUE * ((sin(M_PI * i / f1) + 1.0) / 2.0);
                b = MAX_COLOR_VALUE * ((sin(M_PI * i / f2) + 1.0) / 2.0);

                palette[i].red   = r;
                palette[i].green = g;
                palette[i].blue  = b;
            }
        }
        break;

    case SIN5_PALETTE:
        {
            // Use sin function to generate palette
            for (i = 0; i < PALETTE_SIZE; i++) {
                float hue = 360.0 * ((sin(M_PI * i / f1) + 1.0) / 2.0);
                float sat =         ((cos(M_PI * i / f2) + 1.0) / 2.0);
                float val =         ((sin(M_PI * i / f3) + 1.0) / 2.0);
                float aph = 255.0 * ((cos(M_PI * i)      + 1.0) / 2.0);
                palette[i] = createHSVColor(hue, sat, val);
            }
        }
        break;

    case RANDOM_PALETTE:
        {
            // Choose random color components
            for (i = 0; i < PALETTE_SIZE; i++) {
                palette[i].red   = random(NUM_OF_COLOR_VALUES);
                palette[i].green = random(NUM_OF_COLOR_VALUES);
                palette[i].blue  = random(NUM_OF_COLOR_VALUES);
            }
        }
        break;

    case FIRE_PALETTE:
        {
            // Hue goes from red to yellow
            // Saturation is always the maximum of 1.0
            // Value is 0 .. 1.0 for i = 0..128 and 1.0 for i = 128..255

            // Choose color components
            for (i = 0; i < PALETTE_SIZE; i++) {
                float value = min(255, i * 2) / 255.0;
                hsvToRGB(i / 3, 1.0, value, &r, &g, &b);
                palette[i].red   = r * MAX_COLOR_VALUE;
                palette[i].green = g * MAX_COLOR_VALUE;
                palette[i].blue =  b * MAX_COLOR_VALUE;
            }
        }
        break;

    case SEA_PALETTE:
        {
            // Hue goes from green to blue
            // Saturation is always the maximum of 1.0
            // Value is 0 .. 1.0 for i = 0..128 and 1.0 for i = 128..255

            // Choose color components
            for (i = 0; i < PALETTE_SIZE; i++) {
                float value = min(255, i * 2) / 255.0;
                hsvToRGB(120 + i / 3, 1.0, value, &r, &g, &b);
                palette[i].red   = r * MAX_COLOR_VALUE;
                palette[i].green = g * MAX_COLOR_VALUE;
                palette[i].blue =  b * MAX_COLOR_VALUE;
            }
        }
        break;

    case BLUERED_PALETTE:
        {
            // Hue goes from blue to red
            // Saturation is always the maximum of 1.0
            // Value is 0 .. 1.0 for i = 0..128 and 1.0 for i = 128..255

            // Choose color components
            for (i = 0; i < PALETTE_SIZE; i++) {
                float value = min(255, i * 2) / 255.0;
                hsvToRGB(240 + i / 3, 1.0, value, &r, &g, &b);
                palette[i].red   = r * MAX_COLOR_VALUE;
                palette[i].green = g * MAX_COLOR_VALUE;
                palette[i].blue =  b * MAX_COLOR_VALUE;
            }
        }
        break;

    case COLORWHEEL_PALETTE:
        {
            uint8_t r, g, b;
            int range = 256 / 3;
            int range2 = range * 2;

            // Calculate color components
            for (i = 0; i < PALETTE_SIZE; i++) {
                if ((i >= 0) && (i < range)) {

                    r = 255 - (3 * i);		// Red goes down
                    g = 3 * i;			// Green goes up
                    b = 0;				// No Blue

                }
                else if ((i >= range) && (i < range * 2)) {

                    r = 0;				// No Red
                    g = 255 - (3 * (i - range));	// Green goes down
                    b = 3 * (i - range);		// Blue goes up

                }
                else	{

                    r = 3 * (i - range2);		// Red goes up
                    g = 0;				// No Green
                    b = 255 - (3 * (i - range2));	// Blue goes down

                }
                palette[i].red   = r;
                palette[i].green = g;
                palette[i].blue  = b;
            }
        }
        break;
    }
}


/*******************************************************************/
/***                   Display Pattern Functions                 ***/
/*******************************************************************/

void radiatingLinesPattern() {

    // Pick the attributes of the burst
    int smallRadius = random(2, 5);
    int mediumRadius = random(smallRadius + 4, 13);
    int largeRadius = 15;

    int degreeInc;

    // Pick the burst spoke pitch
    switch(random(4)) {
    case 0:
        degreeInc = 12;
        break;
    case 1:
        degreeInc = 18;
        break;
    case 2:
        degreeInc = 30;
        break;
    case 3:
        degreeInc = 36;
        break;
    }

    int xi, xo, yi, yo;
    double rads;
    rgb24 color;

    int numberOfSpokes = 360 / degreeInc;
    int colorIndex = 0;
    boolean inner = false;

    // Pick the delay for this time through
    int delayTime = random(10, 80);

    while (true) {

        for (int degree = 0; degree < 360; degree += degreeInc) {

            // Convert degrees to radians
            rads = ((double) degree)  * PI / 180.0;

            xi = smallRadius * cos(rads) + MIDX;
            yi = smallRadius * sin(rads) + MIDY;

            if (inner) {
                // Drawing shorter spokes
                xo = mediumRadius * cos(rads) + MIDX;
                yo = mediumRadius * sin(rads) + MIDY;
            }
            else  {
                // Drawing longer spokes
                xo = largeRadius * cos(rads) + MIDX;
                yo = largeRadius * sin(rads) + MIDY;
            }
            // Toggle inner to outer and vise versa
            inner = ! inner;

            // Create longer spoke color
            color = createAHSVColor(numberOfSpokes, colorIndex++, 1.0, 1.0);

            // Draw the spoke
            matrix.drawLine(xi, yi, xo, yo, color);
            matrix.swapBuffers();

            delay(delayTime);
        }
        colorIndex += 2;

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

/*******************************************************************/
/***                       Plasma Functions                      ***/
/*******************************************************************/

#define NUM_OF_PLASMAS 4
#define PLASMA_TYPE_0  0
#define PLASMA_TYPE_1  1
#define PLASMA_TYPE_2  2
#define PLASMA_TYPE_3  3

// Dynamic plasma pattern
void plasma1Pattern() {

    float value;
    float hue;
    rgb24 color;

    // Generate some float factors to alter plasma
    float f1 = (float) random(1, 64) / (float) random(1, 8);
    float f2 = (float) random(1, 64) / (float) random(1, 8);
    float f3 = (float) random(1, 64) / (float) random(1, 8);

    // Select a random plasma type
    int plasmaType = random(NUM_OF_PLASMAS);
    float tic = 0.0;

    while (true) {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {

                // Determine plasma type
                switch (plasmaType) {
                case PLASMA_TYPE_0:
                    {
                        value = sin(sqrt((x - MIDX) * (x - MIDX) + (y - MIDY) * (y - MIDY)) / f1 + tic);
                    }
                    break;

                case PLASMA_TYPE_1:
                    {
                        value = (sin(x / f1 + tic) + sin(y / f1 + tic)) / 2.0;
                    }
                    break;

                case PLASMA_TYPE_2:
                    {
                        value = (sin(x / f1 + tic) + sin(y / f2 + tic) + sin((x + y) / f3 + tic)) / 3.0;
                    }
                    break;

                case PLASMA_TYPE_3:
                    {
                        value  = sin(x / f1 + tic);
                        value += sin(y / f2 + tic);
                        value += sin(sqrt(((x - MIDX) * (x - MIDX)) + ((y - MIDY) * (y - MIDY))) / f3 + tic);
                        value /= 3.0;
                    }
                    break;
                }
                // Convert value of -1 .. +1 to 0 .. 359
                value *= 180.0;
                value += 180.0;

                color = createHSVColor(value, 1.0,  max(abs(cos(tic)), 0.6));
                matrix.drawPixel(x, y, color);
            }
        }
        tic += 0.1;
        matrix.swapBuffers();

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

void drawPlasma2OfType(int plasmaType, int paletteNumber) {

    int x, y;
    int colorIndex;
    float value = 0;

    // Generate specified palette
    generatePaletteNumber(paletteNumber);

    // Generate some float factors to alter plasma
    float f1 = (float) random(1, 64) / (float) random(1, 8);
    float f2 = (float) random(1, 64) / (float) random(1, 8);
    float f3 = (float) random(1, 64) / (float) random(1, 8);

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {

            // Determine plasma type
            switch (plasmaType) {

            case PLASMA_TYPE_0:
                {
                    value = sin(sqrt((x - MIDX) * (x - MIDX) + (y - MIDY) * (y - MIDY)) / f1);
                }
                break;

            case PLASMA_TYPE_1:
                {
                    value = (sin(x / f1) + sin(y / f1)) / 2.0;
                }
                break;

            case PLASMA_TYPE_2:
                {
                    value = (sin(x / f1) + sin(y / f2) + sin((x + y) / f3)) / 3.0;
                }
                break;

            case PLASMA_TYPE_3:
                {
                    value  = sin(x / f1);
                    value += sin(y / f2);
                    value += sin(sqrt(((x - MIDX) * (x - MIDX)) + ((y - MIDY) * (y - MIDY))) / f3);
                    value /= 3.0;
                }
                break;
            }
            // Scale -1 ... +1 values to 0 ... 255
            value = (value * 128.0) + 128.0;
            colorIndex = ((int)value) % 256;
            rgb24 color = palette[colorIndex];

            matrix.drawPixel(x, y, color);
        }
        matrix.swapBuffers();
    }
}

void plasma2Pattern() {

    while (true) {

        int plasmaType = random(NUM_OF_PLASMAS);
        int paletteNumber = random(NUM_OF_PALETTES);

        boolean colorCorrection = random(2) == 1;
        if (colorCorrection) {
            matrix.setColorCorrection(cc24);
        }
        else    {
            matrix.setColorCorrection(ccNone);
        }

        // Draw the specified plasma with the specified palette
        drawPlasma2OfType(plasmaType, paletteNumber);

        delay(3000);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

struct PIXEL {
    byte x;
    byte y;
};

struct PIXELPLUS {
    byte x;
    byte y;
    byte lineNumber;
};

// Define pixels on display
#define NPCIRCLE 28
struct PIXELPLUS circlePixels[NPCIRCLE] = {
    10, 15, 13,    // 0
    10, 14,  0,    // 1
    10, 13, 14,    // 2
    11, 12, 15,    // 3
    12, 11,  0,    // 4
    13, 10, 16,    // 5
    14, 10,  0,    // 6
    15, 10,  1,    // 7
    16, 10,  0,    // 8
    17, 10,  2,    // 9
    18, 11,  3,    // 10
    19, 12,  0,    // 11
    20, 13,  4,    // 12
    20, 14,  0,    // 13
    20, 15,  5,    // 14
    20, 16,  0,    // 15
    20, 17,  6,    // 16
    19, 18,  7,    // 17
    18, 19,  0,    // 18
    17, 20,  8,    // 19
    16, 20,  0,    // 20
    15, 20,  9,    // 21
    14, 20,  0,    // 22
    13, 20, 10,    // 23
    12, 19, 11,    // 24
    11, 18,  0,    // 25
    10, 17, 12,    // 26
    10, 16,  0,    // 27
};

#define NPLINE1 10
struct PIXEL line1Pixels[NPLINE1] = {
    15, 9,
    15, 8,
    15, 7,
    15, 6,
    15, 5,
    15, 4,
    15, 3,
    15, 2,
    15, 1,
    15, 0,
};

rgb24 line1Colors[NPLINE1];

void shiftLine1Colors() {

    for (int i = NPLINE1 - 2; i >= 0; i--) {
        line1Colors[i + 1] = line1Colors[i];
    }
}

void drawLine1() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE1; i++) {
        p = line1Pixels[i];
        matrix.drawPixel(p.x, p.y, line1Colors[i]);
    }
}

#define NPLINE2 10
struct PIXEL line2Pixels[NPLINE2] = {
    18, 9,
    18, 8,
    19, 7,
    19, 6,
    20, 5,
    20, 4,
    21, 3,
    21, 2,
    22, 1,
    22, 0
};

rgb24 line2Colors[NPLINE2];

void shiftLine2Colors() {

    for (int i = NPLINE2 - 2; i >= 0; i--) {
        line2Colors[i + 1] = line2Colors[i];
    }
}

void drawLine2() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE2; i++) {
        p = line2Pixels[i];
        matrix.drawPixel(p.x, p.y, line2Colors[i]);
    }
}


#define NPLINE3 12
struct PIXEL line3Pixels[NPLINE3] = {
    19, 11,
    20, 10,
    21,  9,
    22,  8,
    23,  7,
    24,  6,
    25,  5,
    26,  4,
    27,  3,
    28,  2,
    29,  1,
    30,  0
};

rgb24 line3Colors[NPLINE3];

void shiftLine3Colors() {

    for (int i = NPLINE3 - 2; i >= 0; i--) {
        line3Colors[i + 1] = line3Colors[i];
    }
}

void drawLine3() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE3; i++) {
        p = line3Pixels[i];
        matrix.drawPixel(p.x, p.y, line3Colors[i]);
    }
}

#define NPLINE4 11
struct PIXEL line4Pixels[NPLINE4] = {
    21, 13,
    22, 12,
    23, 12,
    24, 11,
    25, 11,
    26, 10,
    27, 10,
    28, 9,
    29, 9,
    30, 8,
    31, 8
};

rgb24 line4Colors[NPLINE4];

void shiftLine4Colors() {

    for (int i = NPLINE4 - 2; i >= 0; i--) {
        line4Colors[i + 1] = line4Colors[i];
    }
}

void drawLine4() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE4; i++) {
        p = line4Pixels[i];
        matrix.drawPixel(p.x, p.y, line4Colors[i]);
    }
}

#define NPLINE5 11
struct PIXEL line5Pixels[NPLINE5] = {
    21, 15,
    22, 15,
    23, 15,
    24, 15,
    25, 15,
    26, 15,
    27, 15,
    28, 15,
    29, 15,
    30, 15,
    31, 15
};

rgb24 line5Colors[NPLINE5];

void shiftLine5Colors() {

    for (int i = NPLINE5 - 2; i >= 0; i--) {
        line5Colors[i + 1] = line5Colors[i];
    }
}

void drawLine5() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE5; i++) {
        p = line5Pixels[i];
        matrix.drawPixel(p.x, p.y, line5Colors[i]);
    }
}

#define NPLINE6 11
struct PIXEL line6Pixels[NPLINE6] = {
    21, 17,
    22, 18,
    23, 18,
    24, 19,
    25, 19,
    26, 20,
    27, 20,
    28, 21,
    29, 21,
    30, 22,
    31, 22
};

rgb24 line6Colors[NPLINE6];

void shiftLine6Colors() {

    for (int i = NPLINE6 - 2; i >= 0; i--) {
        line6Colors[i + 1] = line6Colors[i];
    }
}

void drawLine6() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE6; i++) {
        p = line6Pixels[i];
        matrix.drawPixel(p.x, p.y, line6Colors[i]);
    }
}

#define NPLINE7 13
struct PIXEL line7Pixels[NPLINE7] = {
    19, 19,
    20, 20,
    21, 21,
    22, 22,
    23, 23,
    24, 24,
    25, 25,
    26, 26,
    27, 27,
    28, 28,
    29, 29,
    30, 30,
    31, 31
};

rgb24 line7Colors[NPLINE7];

void shiftLine7Colors() {

    for (int i = NPLINE7 - 2; i >= 0; i--) {
        line7Colors[i + 1] = line7Colors[i];
    }
}

void drawLine7() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE7; i++) {
        p = line7Pixels[i];
        matrix.drawPixel(p.x, p.y, line7Colors[i]);
    }
}

#define NPLINE8 11
struct PIXEL line8Pixels[NPLINE8] = {
    17, 21,
    18, 22,
    18, 23,
    19, 24,
    19, 25,
    20, 26,
    20, 27,
    21, 28,
    21, 29,
    22, 30,
    22, 31
};

rgb24 line8Colors[NPLINE8];

void shiftLine8Colors() {

    for (int i = NPLINE8 - 2; i >= 0; i--) {
        line8Colors[i + 1] = line8Colors[i];
    }
}

void drawLine8() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE8; i++) {
        p = line8Pixels[i];
        matrix.drawPixel(p.x, p.y, line8Colors[i]);
    }
}

#define NPLINE9 11
struct PIXEL line9Pixels[NPLINE9] = {
    15, 21,
    15, 22,
    15, 23,
    15, 24,
    15, 25,
    15, 26,
    15, 27,
    15, 28,
    15, 29,
    15, 30,
    15, 31
};

rgb24 line9Colors[NPLINE9];

void shiftLine9Colors() {

    for (int i = NPLINE9 - 2; i >= 0; i--) {
        line9Colors[i + 1] = line9Colors[i];
    }
}

void drawLine9() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE9; i++) {
        p = line9Pixels[i];
        matrix.drawPixel(p.x, p.y, line9Colors[i]);
    }
}

#define NPLINE10 11
struct PIXEL line10Pixels[NPLINE10] = {
    13, 21,
    12, 22,
    12, 23,
    11, 24,
    11, 25,
    10, 26,
    10, 27,
    9, 28,
    9, 29,
    8, 30,
    8, 31
};

rgb24 line10Colors[NPLINE10];

void shiftLine10Colors() {

    for (int i = NPLINE10 - 2; i >= 0; i--) {
        line10Colors[i + 1] = line10Colors[i];
    }
}

void drawLine10() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE10; i++) {
        p = line10Pixels[i];
        matrix.drawPixel(p.x, p.y, line10Colors[i]);
    }
}

#define NPLINE11 12
struct PIXEL line11Pixels[NPLINE11] = {
    11, 19,
    10, 20,
    9, 21,
    8, 22,
    7, 23,
    6, 24,
    5, 25,
    4, 26,
    3, 27,
    2, 28,
    1, 29,
    0, 30,
};

rgb24 line11Colors[NPLINE11];

void shiftLine11Colors() {

    for (int i = NPLINE11 - 2; i >= 0; i--) {
        line11Colors[i + 1] = line11Colors[i];
    }
}

void drawLine11() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE11; i++) {
        p = line11Pixels[i];
        matrix.drawPixel(p.x, p.y, line11Colors[i]);
    }
}

#define NPLINE12 10
struct PIXEL line12Pixels[NPLINE12] = {
    9, 17,
    8, 18,
    7, 18,
    6, 19,
    5, 19,
    4, 20,
    3, 20,
    2, 21,
    1, 21,
    0, 22
};

rgb24 line12Colors[NPLINE12];

void shiftLine12Colors() {

    for (int i = NPLINE12 - 2; i >= 0; i--) {
        line12Colors[i + 1] = line12Colors[i];
    }
}

void drawLine12() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE12; i++) {
        p = line12Pixels[i];
        matrix.drawPixel(p.x, p.y, line12Colors[i]);
    }
}

#define NPLINE13 10
struct PIXEL line13Pixels[NPLINE13] = {
    9, 15,
    8, 15,
    7, 15,
    6, 15,
    5, 15,
    4, 15,
    3, 15,
    2, 15,
    1, 15,
    0, 15,
};

rgb24 line13Colors[NPLINE13];

void shiftLine13Colors() {

    for (int i = NPLINE13 - 2; i >= 0; i--) {
        line13Colors[i + 1] = line13Colors[i];
    }
}

void drawLine13() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE13; i++) {
        p = line13Pixels[i];
        matrix.drawPixel(p.x, p.y, line13Colors[i]);
    }
}

#define NPLINE14 10
struct PIXEL line14Pixels[NPLINE14] = {
    9, 13,
    8, 12,
    7, 12,
    6, 11,
    5, 11,
    4, 10,
    3, 10,
    2,  9,
    1,  9,
    0,  8
};

rgb24 line14Colors[NPLINE14];

void shiftLine14Colors() {

    for (int i = NPLINE14 - 2; i >= 0; i--) {
        line14Colors[i + 1] = line14Colors[i];
    }
}

void drawLine14() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE14; i++) {
        p = line14Pixels[i];
        matrix.drawPixel(p.x, p.y, line14Colors[i]);
    }
}

#define NPLINE15 12
struct PIXEL line15Pixels[NPLINE15] = {
    11, 11,
    10, 10,
    9,  9,
    8,  8,
    7,  7,
    6,  6,
    5,  5,
    4,  4,
    3,  3,
    2,  2,
    1,  1,
    0,  0,
};

rgb24 line15Colors[NPLINE15];

void shiftLine15Colors() {

    for (int i = NPLINE15 - 2; i >= 0; i--) {
        line15Colors[i + 1] = line15Colors[i];
    }
}

void drawLine15() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE15; i++) {
        p = line15Pixels[i];
        matrix.drawPixel(p.x, p.y, line15Colors[i]);
    }
}

#define NPLINE16 10
struct PIXEL line16Pixels[NPLINE16] = {
    12, 9,
    12, 8,
    11, 7,
    11, 6,
    10, 5,
    10, 4,
    9, 3,
    9, 2,
    8, 1,
    8, 0
};

rgb24 line16Colors[NPLINE16];

void shiftLine16Colors() {

    for (int i = NPLINE16 - 2; i >= 0; i--) {
        line16Colors[i + 1] = line16Colors[i];
    }
}

void drawLine16() {

    struct PIXEL p;
    for (int i = 0; i < NPLINE16; i++) {
        p = line16Pixels[i];
        matrix.drawPixel(p.x, p.y, line16Colors[i]);
    }
}

void aurora1Pattern() {

    const int NUMBER_OF_COLORS = 512;

    rgb24 colors[NUMBER_OF_COLORS];

    struct PIXEL p;
    struct PIXELPLUS pp;
    rgb24 color;
    int colorIndex = 0;

    // Precalculate colors
    for (int i = 0; i < NUMBER_OF_COLORS; i++) {
        // Calculate color for the pixel
        colors[i] = createHSVColor(NUMBER_OF_COLORS,  i, 1.0, 1.0);
    }

    while (true) {

        // Draw center circle pixel by pixel
        for (int i = 0; i < NPCIRCLE; i++) {

            // Get color for pixel
            color = colors[colorIndex];
            colorIndex++;
            colorIndex %= NUMBER_OF_COLORS;

            // Get pixel's info
            pp = circlePixels[i];

            // Set the pixel's color
            matrix.drawPixel(pp.x, pp.y, color);

            // Does this pixel have a ray ?
            switch(pp.lineNumber) {
            case 0:
                break;

            case 1:
                shiftLine1Colors();
                line1Colors[0] = color;
                drawLine1();
                break;

            case 2:
                shiftLine2Colors();
                line2Colors[0] = color;
                drawLine2();
                break;

            case 3:
                shiftLine3Colors();
                line3Colors[0] = color;
                drawLine3();
                break;

            case 4:
                shiftLine4Colors();
                line4Colors[0] = color;
                drawLine4();
                break;

            case 5:
                shiftLine5Colors();
                line5Colors[0] = color;
                drawLine5();
                break;

            case 6:
                shiftLine6Colors();
                line6Colors[0] = color;
                drawLine6();
                break;

            case 7:
                shiftLine7Colors();
                line7Colors[0] = color;
                drawLine7();
                break;

            case 8:
                shiftLine8Colors();
                line8Colors[0] = color;
                drawLine8();
                break;

            case 9:
                shiftLine9Colors();
                line9Colors[0] = color;
                drawLine9();
                break;

            case 10:
                shiftLine10Colors();
                line10Colors[0] = color;
                drawLine10();
                break;

            case 11:
                shiftLine11Colors();
                line11Colors[0] = color;
                drawLine11();
                break;

            case 12:
                shiftLine12Colors();
                line12Colors[0] = color;
                drawLine12();
                break;

            case 13:
                shiftLine13Colors();
                line13Colors[0] = color;
                drawLine13();
                break;

            case 14:
                shiftLine14Colors();
                line14Colors[0] = color;
                drawLine14();
                break;

            case 15:
                shiftLine15Colors();
                line15Colors[0] = color;
                drawLine15();
                break;

            case 16:
                shiftLine16Colors();
                line16Colors[0] = color;
                drawLine16();
                break;
            }
        }
        // Make iteration visible
        matrix.swapBuffers();

        delay(250);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

void aurora2Pattern() {

    const int NUMBER_OF_COLORS = 24;

    rgb24 colors[NUMBER_OF_COLORS];

    struct PIXEL p;
    struct PIXELPLUS pp;
    rgb24 color;
    int colorIndex = 0;

    // Precalculate colors
    for (int i = 0; i < NUMBER_OF_COLORS; i++) {
        // Calculate color for the pixel
        colors[i] = createHSVColor(NUMBER_OF_COLORS,  i, 1.0, 1.0);
    }

    while (true) {

        // Draw center circle pixel by pixel
        for (int i = 0; i < NPCIRCLE; i++) {

            // Get color for pixel
            color = colors[colorIndex];
            colorIndex++;
            colorIndex %= NUMBER_OF_COLORS;

            // Get pixel's info
            pp = circlePixels[i];

            // Set the pixel's color
            matrix.drawPixel(pp.x, pp.y, color);

            // Does this pixel have a ray ?
            switch(pp.lineNumber) {
            case 0:
                break;

            case 1:
                shiftLine1Colors();
                line1Colors[0] = color;
                drawLine1();
                break;

            case 2:
                shiftLine2Colors();
                line2Colors[0] = color;
                drawLine2();
                break;

            case 3:
                shiftLine3Colors();
                line3Colors[0] = color;
                drawLine3();
                break;

            case 4:
                shiftLine4Colors();
                line4Colors[0] = color;
                drawLine4();
                break;

            case 5:
                shiftLine5Colors();
                line5Colors[0] = color;
                drawLine5();
                break;

            case 6:
                shiftLine6Colors();
                line6Colors[0] = color;
                drawLine6();
                break;

            case 7:
                shiftLine7Colors();
                line7Colors[0] = color;
                drawLine7();
                break;

            case 8:
                shiftLine8Colors();
                line8Colors[0] = color;
                drawLine8();
                break;

            case 9:
                shiftLine9Colors();
                line9Colors[0] = color;
                drawLine9();
                break;

            case 10:
                shiftLine10Colors();
                line10Colors[0] = color;
                drawLine10();
                break;

            case 11:
                shiftLine11Colors();
                line11Colors[0] = color;
                drawLine11();
                break;

            case 12:
                shiftLine12Colors();
                line12Colors[0] = color;
                drawLine12();
                break;

            case 13:
                shiftLine13Colors();
                line13Colors[0] = color;
                drawLine13();
                break;

            case 14:
                shiftLine14Colors();
                line14Colors[0] = color;
                drawLine14();
                break;

            case 15:
                shiftLine15Colors();
                line15Colors[0] = color;
                drawLine15();
                break;

            case 16:
                shiftLine16Colors();
                line16Colors[0] = color;
                drawLine16();
                break;
            }
        }
        // Make iteration visible
        matrix.swapBuffers();

        delay(250);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

#define MAX_CRAWLERS 120

struct CRAWLER {
    int x;
    int y;
    float hue;
    int speed;
};

struct CRAWLER crawlers[MAX_CRAWLERS];

// Initialize crawlers array
void initializeCrawlers() {

    for (int i = 0; i < MAX_CRAWLERS; i++) {
        crawlers[i].x = -1;
    }
}

// Find first free crawler, if any.
int findFreeCrawler() {

    for (int i = 0; i < MAX_CRAWLERS; i++) {
        if (crawlers[i].x == -1) {
            return i;
        }
    }
    return -1;    // None available
}

// Free Crawler
void freeCrawler(int index) {
    crawlers[index].x = -1;
}

// Spawn a crawler, if possible
void spawnCrawler() {

    int index = findFreeCrawler();
    if (index == -1) {
        return;
    }
    crawlers[index].x = random(32);   // Random x location
    crawlers[index].y = 0;            // Initial y location
    crawlers[index].hue = random(300);// Random hue
    crawlers[index].speed = random(1, 4);    // Random speed
}

// Process all crawlers
void processCrawlers() {

    struct CRAWLER c;
    rgb24 color;

    // Look at each crawler in array
    for (int i = 0; i < MAX_CRAWLERS; i++) {
        // Is this a valid crawler ?
        if (crawlers[i].x != -1) {
            // Yes we have a crawler, copy it
            c = crawlers[i];

            // Erase previous location
            matrix.drawPixel(c.x, c.y, COLOR_BLACK);

            // Pick a random direction to move in
            if (random(2) == 0) {
                // Move in the x direction
                if (random(2) == 0) {
                    // Move in the negative direction
                    c.x -= c.speed;
                    if (c.x < MINX) {
                        c.x = MINX;
                    }
                }
                else    {
                    // Move in the positive direction
                    c.x += c.speed;
                    if (c.x > MAXX) {
                        c.x = MAXX;
                    }
                }
            }
            else    {
                // Move in the y direction
                c.y += c.speed;
                if (c.y > MAXY) {
                    freeCrawler(i);
                    continue;
                }
            }
            // Copy crawler data back
            crawlers[i] = c;

            // Calculate crawler color
            // Saturation dependant upon y position
            color = createHSVColor(c.hue, (((float) c.y) / 31.0),  1.0);

            // Draw crawler at new location with new color
            matrix.drawPixel(c.x, c.y, color);

            // Update display
            matrix.swapBuffers();
        }
    }
}

void crawlerPattern() {

    // Initialize crawlers to none
    initializeCrawlers();

    while (true) {

        int numberToSpawn = random(1, 13);
        for (int i = 0; i < numberToSpawn; i++) {
            spawnCrawler();
        }
        // Process all crawlers
        processCrawlers();

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

void randomCirclesPattern() {

    float hue, val;
    rgb24 color;
    int xc, yc, radius, count;

    while (true) {
        // Clear the screen
        matrix.fillScreen(COLOR_BLACK);

        // Pick a random clear count
        count = random(10, 50);

        while (count-- >= 0) {

            hue = random(360);
            val = ((float) random(30, 100)) / 100.0;
            color = createHSVColor(hue, 1.0,  val);

            xc = random(32);
            yc = random(32);
            radius = random(16);

            matrix.drawCircle(xc, yc, radius, color);

            matrix.swapBuffers();

            // Check for termination
            if (checkForTermination()) {
                return;
            }
            delay(250);
        }
    }
}

void randomTrianglesPattern() {

    float hue, val;
    rgb24 color;
    int x1, x2, x3, y1, y2, y3, count;
    boolean filled;

    while (true) {
        // Clear the screen
        matrix.fillScreen(COLOR_BLACK);

        // Pick a random clear count
        count = random(10, 50);

        while (count-- >= 0) {

            hue = random(360);
            val = ((float) random(35, 100)) / 100.0;
            color = createHSVColor(hue, 1.0,  val);

            filled = random(2) == 0;

            x1 = random(32);
            y1 = random(32);
            x2 = random(32);
            y2 = random(32);
            x3 = random(32);
            y3 = random(32);

            if (filled) {
                matrix.fillTriangle(x1, y1, x2, y2, x3, y3, color, color);
            }
            else    {
                matrix.drawTriangle(x1, y1, x2, y2, x3, y3, color);
            }

            matrix.swapBuffers();

            // Check for termination
            if (checkForTermination()) {
                return;
            }
            delay(300);
        }
    }
}

void randomLines1Pattern() {

    float hue, val;
    rgb24 color;
    int x1, x2, y1, y2, count;

    while (true) {
        // Clear the screen
        matrix.fillScreen(COLOR_BLACK);

        // Pick a random clear count
        count = random(15, 60);

        while (count-- >= 0) {

            hue = random(360);
            val = ((float) random(20, 100)) / 100.0;
            color = createHSVColor(hue, 1.0,  val);

            x1 = random(32);
            y1 = random(32);
            x2 = random(32);
            y2 = random(32);

            matrix.drawLine(x1, y1, x2, y2, color);

            matrix.swapBuffers();

            // Check for termination
            if (checkForTermination()) {
                return;
            }
            delay(300);
        }
    }
}

void randomLines2Pattern() {

    float hue, val;
    rgb24 color;
    int x, y, count;
    boolean vertical;

    while (true) {
        // Clear the screen
        matrix.fillScreen(COLOR_BLACK);

        // Pick a random clear count
        count = random(15, 64);

        while (count-- >= 0) {

            hue = random(360);
            val = ((float) random(10, 100)) / 100.0;
            color = createHSVColor(hue, 1.0,  val);

            // Pick direction of line
            vertical = (random(2) == 1);

            if (vertical) {
                x = random(32);
                matrix.drawLine(x, MINY, x, MAXY, color);
            }
            else    {
                y = random(32);
                matrix.drawLine(MINX, y, MAXX, y, color);
            }

            matrix.swapBuffers();

            // Check for termination
            if (checkForTermination()) {
                return;
            }
            delay(300);
        }
    }
}

void randomPixelsPattern() {

    rgb24 color;
    int r, g, b;
    int x, y;

    int selector = random(4);

    while (true) {

        // Pick pixel
        x = random(32);
        y = random(32);

        switch(selector) {
        case 0:
            {
                // Red green combinations
                color.red   = random(256);
                color.green = random(256);
                color.blue  = 0;
            }
            break;

        case 1:
            {
                // Red blue combinations
                color.red   = random(256);
                color.green = 0;
                color.blue  = random(256);
            }
            break;

        case 2:
            {
                // Green blue combinations
                color.red   = 0;
                color.green = random(256);
                color.blue  = random(256);
            }
            break;

        case 3:
            {
                color.red   = random(256);
                color.green = random(256);
                color.blue  = random(256);
            }
            break;
        }

        matrix.drawPixel(x, y, color);
        matrix.swapBuffers();
        delay(50);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

void drawSineWave(int amplitude, int startDegrees, int endDegrees, rgb24 fillColor, rgb24 pointColor, boolean single) {

    int x, y;
    float rads, point;

    amplitude = (abs(amplitude) > 14) ? 14 : abs(amplitude);

    // Fill the screen
    matrix.fillScreen(fillColor);

    // Determine X axis scale
    int degreeSpan = (endDegrees - startDegrees) / WIDTH;

    for (int degrees = startDegrees, x = 0; degrees < endDegrees;  degrees += degreeSpan, x++) {

        rads = (M_PI * degrees) / 180.0;
        point = amplitude * sin(rads);

        // Draw the point on the matrix
        matrix.drawPixel(x, MIDY + round(point), pointColor);

        if (! single) {
            matrix.drawPixel(x, MIDY - round(point), pointColor);
        }
    }
    matrix.swapBuffers();
}

struct sineWaveParameters {
    int amplitude;
    int startDegrees;
    int endDegrees;
    rgb24 fillColor;
    rgb24 pointColor;
};

struct sineWaveParameters SINEWAVE1PATTERNS [] = {

    // Amplitude patterns
    0, 0, 720, COLOR_BLACK, COLOR_GREEN,
    4, 0, 720, COLOR_BLACK, COLOR_GREEN,
    8, 0, 720, COLOR_BLACK, COLOR_GREEN,
    14, 0, 720, COLOR_BLACK, COLOR_GREEN,
    8, 0, 720, COLOR_BLACK, COLOR_GREEN,
    4, 0, 720, COLOR_BLACK, COLOR_GREEN,
    0, 0, 720, COLOR_BLACK, COLOR_GREEN,
    4, 0, 720, COLOR_BLACK, COLOR_GREEN,
    8, 0, 720, COLOR_BLACK, COLOR_GREEN,
    14, 0, 720, COLOR_BLACK, COLOR_GREEN,

    // Frequency patterns
    14, 0,  720, COLOR_BLACK, COLOR_GREEN,
    14, 0,  810, COLOR_BLACK, COLOR_GREEN,
    14, 0,  900, COLOR_BLACK, COLOR_GREEN,
    14, 0,  990, COLOR_BLACK, COLOR_GREEN,
    14, 0, 1080, COLOR_BLACK, COLOR_GREEN,
    14, 0,  990, COLOR_BLACK, COLOR_GREEN,
    14, 0,  900, COLOR_BLACK, COLOR_GREEN,
    14, 0,  810, COLOR_BLACK, COLOR_GREEN,
    14, 0,  720, COLOR_BLACK, COLOR_GREEN,
    14, 0,  630, COLOR_BLACK, COLOR_GREEN,
    14, 0,  540, COLOR_BLACK, COLOR_GREEN,
    14, 0,  450, COLOR_BLACK, COLOR_GREEN,
    14, 0,  360, COLOR_BLACK, COLOR_GREEN,
    14, 0,  450, COLOR_BLACK, COLOR_GREEN,
    14, 0,  630, COLOR_BLACK, COLOR_GREEN,
    14, 0,  720, COLOR_BLACK, COLOR_GREEN,

    // Phase patterns
    14,   0,  900, COLOR_BLACK, COLOR_GREEN,
    14,  30,  930, COLOR_BLACK, COLOR_GREEN,
    14,  60,  960, COLOR_BLACK, COLOR_GREEN,
    14,  90,  990, COLOR_BLACK, COLOR_GREEN,
    14, 120, 1020, COLOR_BLACK, COLOR_GREEN,
    14,  90,  990, COLOR_BLACK, COLOR_GREEN,
    14,  60,  960, COLOR_BLACK, COLOR_GREEN,
    14,  30,  930, COLOR_BLACK, COLOR_GREEN,
    14,   0,  900, COLOR_BLACK, COLOR_GREEN,
    14,  30,  930, COLOR_BLACK, COLOR_GREEN,
    14,  60,  960, COLOR_BLACK, COLOR_GREEN,
    14,  90,  990, COLOR_BLACK, COLOR_GREEN,
    14, 120, 1020, COLOR_BLACK, COLOR_GREEN,

    // Amplitude patterns
    0, 0, 720, COLOR_LGREEN, COLOR_RED,
    4, 0, 720, COLOR_LGREEN, COLOR_RED,
    8, 0, 720, COLOR_LGREEN, COLOR_RED,
    14, 0, 720, COLOR_LGREEN, COLOR_RED,
    8, 0, 720, COLOR_LGREEN, COLOR_RED,
    4, 0, 720, COLOR_LGREEN, COLOR_RED,
    0, 0, 720, COLOR_LGREEN, COLOR_RED,
    4, 0, 720, COLOR_LGREEN, COLOR_RED,
    8, 0, 720, COLOR_LGREEN, COLOR_RED,
    14, 0, 720, COLOR_LGREEN, COLOR_RED,

    // Frequency patterns
    14, 0,  720, COLOR_LGREEN, COLOR_RED,
    14, 0,  810, COLOR_LGREEN, COLOR_RED,
    14, 0,  900, COLOR_LGREEN, COLOR_RED,
    14, 0,  990, COLOR_LGREEN, COLOR_RED,
    14, 0, 1080, COLOR_LGREEN, COLOR_RED,
    14, 0,  990, COLOR_LGREEN, COLOR_RED,
    14, 0,  900, COLOR_LGREEN, COLOR_RED,
    14, 0,  810, COLOR_LGREEN, COLOR_RED,
    14, 0,  720, COLOR_LGREEN, COLOR_RED,
    14, 0,  630, COLOR_LGREEN, COLOR_RED,
    14, 0,  540, COLOR_LGREEN, COLOR_RED,
    14, 0,  450, COLOR_LGREEN, COLOR_RED,
    14, 0,  360, COLOR_LGREEN, COLOR_RED,
    14, 0,  450, COLOR_LGREEN, COLOR_RED,
    14, 0,  630, COLOR_LGREEN, COLOR_RED,
    14, 0,  720, COLOR_LGREEN, COLOR_RED,

    // Phase patterns
    14,   0,  900, COLOR_LGREEN, COLOR_RED,
    14,  30,  930, COLOR_LGREEN, COLOR_RED,
    14,  60,  960, COLOR_LGREEN, COLOR_RED,
    14,  90,  990, COLOR_LGREEN, COLOR_RED,
    14, 120, 1020, COLOR_LGREEN, COLOR_RED,
    14,  90,  990, COLOR_LGREEN, COLOR_RED,
    14,  60,  960, COLOR_LGREEN, COLOR_RED,
    14,  30,  930, COLOR_LGREEN, COLOR_RED,
    14,   0,  900, COLOR_LGREEN, COLOR_RED,
    14,  30,  930, COLOR_LGREEN, COLOR_RED,
    14,  60,  960, COLOR_LGREEN, COLOR_RED,
    14,  90,  990, COLOR_LGREEN, COLOR_RED,
    14, 120, 1020, COLOR_LGREEN, COLOR_RED,
};

void sineWaves1Pattern() {

    struct sineWaveParameters swp;

    int numberOfPatterns = sizeof(SINEWAVE1PATTERNS) / sizeof(sineWaveParameters);

    while (true) {
        for (int i = 0; i < numberOfPatterns; i++) {
            swp = SINEWAVE1PATTERNS[i];

            drawSineWave(swp.amplitude, swp.startDegrees, swp.endDegrees, swp.fillColor, swp.pointColor, true);
            delay(250);

            // Check for termination
            if (checkForTermination()) {
                return;
            }
        }
    }
}

void sineWaves2Pattern() {

    rgb24 color;
    int colorIndex, colorIncrement;

    while (true) {

        generatePaletteNumber(random(NUM_OF_PALETTES));

        colorIndex = random(PALETTE_SIZE);
        colorIncrement = random(2, 5);

        for (int degs = 45; degs < 90 * 16; degs++) {

            // Get a color for the sine wave
            color = palette[colorIndex];
            colorIndex += colorIncrement;
            colorIndex %= PALETTE_SIZE;

            drawSineWave(14, 0, degs, COLOR_BLACK, color, false);

            // Check for termination
            if (checkForTermination()) {
                return;
            }
        }
        delay(600);
    }
}

void welcomePattern() {

    rgb24 color;
    float hueAngle = 0.0;

    // Write welcome message
    matrix.setScrollColor(COLOR_WHITE);
    matrix.setScrollMode(wrapForward);
    matrix.setScrollSpeed(10);
    matrix.setScrollFont(font6x10);
    matrix.setScrollOffsetFromEdge(11);
    matrix.scrollText("Welcome", 32760);

    while (true) {

        // Get a color for the line
        color = createHSVColor(hueAngle, 1.0, 1.0);
        hueAngle += 15.0;
        if (hueAngle >= 360.0) {
            hueAngle = 0.0;
        }

        // Draw diagonal lines on left
        for (int i = 0; i < 32; i++) {
            matrix.drawLine(0, i, i, 31, color);
            matrix.swapBuffers();
            delay(50);
        }

        // Get a color for the line
        color = createHSVColor(hueAngle, 1.0, 1.0);
        hueAngle += 15.0;
        if (hueAngle >= 360.0) {
            hueAngle = 0.0;
        }
        // Draw diagonal lines on bottom
        for (int i = 0; i < 32; i++) {
            matrix.drawLine(i, 31, 31, 31 - i, color);
            matrix.swapBuffers();
            delay(50);
        }

        // Get a color for the line
        color = createHSVColor(hueAngle, 1.0, 1.0);
        hueAngle += 15.0;
        if (hueAngle >= 360.0) {
            hueAngle = 0.0;
        }

        // Draw diagonal lines on right
        for (int i = 0; i < 32; i++) {
            matrix.drawLine(31, 31 - i, 31 - i, 0, color);
            matrix.swapBuffers();
            delay(50);
        }

        // Get a color for the line
        color = createHSVColor(hueAngle, 1.0, 1.0);
        hueAngle += 15.0;
        if (hueAngle >= 360.0) {
            hueAngle = 0.0;
        }
        // Draw diagonal lines on top
        for (int i = 0; i < 32; i++) {
            matrix.drawLine(31 - i, 0, 0, i, color);
            matrix.swapBuffers();
            delay(50);
        }

        // Test for termination
        if (checkForTermination()) {
            return;
        }
    }
}

void rotatingRectsPattern() {

    rgb24 color;
    int colorIndex = 0;
    int delayMillis;

    while (true) {
        // Select a random amount of delay for this loop
        delayMillis = random(80);

        // Draw rotating rects
        for (int i = 0; i < 32; i++) {
            color = createHSVColorWithDivisions(16, colorIndex);
            colorIndex++;
            colorIndex %= 16;

            matrix.drawLine(0, i, i, 31, color);
            matrix.drawLine(i, 31, 31, 31 - i, color);
            matrix.drawLine(31, 31 - i, 31 - i, 0, color);
            matrix.drawLine(31 - i, 0, 0, i, color);

            matrix.swapBuffers();
            delay(delayMillis);
        }
        colorIndex += 2;

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

const float MATRIX_HUE = 120.0;
const float MATRIX_VAL =   0.5;

const int MATRIX_CRAWLERS = 32;

struct MATRIX_CRAWLER {
    int y;
    int speed;
};

struct MATRIX_CRAWLER xCrawlers[MATRIX_CRAWLERS];

// Initialize crawlers array
void initializeXCrawlers() {

    for (int x = 0; x < MATRIX_CRAWLERS; x++) {
        xCrawlers[x].y = 0;                // Set initial position
        xCrawlers[x].speed = random(1, 5); // Set a random speed
    }
}

// See if xCrawler is available
boolean isXCrawlerAvailable(int x) {

    return (xCrawlers[x].y == -1) ? true : false;
}

// Free Crawler
void freeXCrawler(int x) {
    xCrawlers[x].y = -1;
}

// Spawn a crawler, if possible
boolean spawnXCrawler(int x) {

    boolean available = isXCrawlerAvailable(x);
    if (! available) {
        return false;
    }
    xCrawlers[x].y = 0;                // Set initial position
    xCrawlers[x].speed = random(1, 5); // Set a random speed

    return true;
}

// Process all crawlers
void processXCrawlers() {

    struct MATRIX_CRAWLER c;
    rgb24 color;

    // Look at each crawler in array
    for (int x = 0; x < MATRIX_CRAWLERS; x++) {
        // Is this a valid crawler ?
        if (xCrawlers[x].y != -1) {
            // Yes we have a crawler, copy it
            c = xCrawlers[x];

            // Delete all pixels in row
            for (int y = 0; y < HEIGHT; y++) {
                matrix.drawPixel(x, y, COLOR_BLACK);
            }

            // Move in the positive y direction
            c.y += c.speed;
            if (c.y >= MAXY + 4) {
                freeXCrawler(x);
                continue;
            }

            // Copy crawler data back
            xCrawlers[x] = c;

            // Check lead pixel
            if (c.y <= MAXY) {
                // Calculate crawler color. Saturation dependant upon y position
                color = createHSVColor(MATRIX_HUE, 1.0 - (((float) c.y) / 31.0),  MATRIX_VAL);

                // Draw crawler at new location with new color
                matrix.drawPixel(x, c.y, color);
            }

            // Check lead pixel - 1
            if ((c.y - 1 >= MINY) && (c.y - 1 <= MAXY)) {
                // Calculate crawler color. Saturation dependant upon y position
                color = createHSVColor(MATRIX_HUE, 1.0 - (((float) (c.y - 1)) / 31.0),  MATRIX_VAL / 2.0);

                // Draw crawler at new location with new color
                matrix.drawPixel(x, c.y - 1, color);
            }

            // Check lead pixel - 2
            if ((c.y - 2 >= MINY) && (c.y - 2 <= MAXY)) {
                // Calculate crawler color. Saturation dependant upon y position
                color = createHSVColor(MATRIX_HUE, 1.0 - (((float) (c.y - 2)) / 31.0),  MATRIX_VAL / 3.0);

                // Draw crawler at new location with new color
                matrix.drawPixel(x, c.y - 2, color);
            }
            // Update display
            matrix.swapBuffers();
        }
    }
}

void matrixPattern() {

    int x;

    // Initialize crawlers
    initializeXCrawlers();

    while (true) {

        // Attempt to spawn a random number of crawlers
        int numberToSpawn = random(1, 31);

        for (int i = 0; i < numberToSpawn; i++) {
            x = random(WIDTH);
            spawnXCrawler(x);
        }
        // Process all crawlers
        processXCrawlers();

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

void rotatingLinesPattern() {

    rgb24 color;
    float hueAngle = 0.0;

    while (true) {

        // Draw diagonal lines from top to bottom, left to right
        for (int x = 0; x < 32; x++) {

            // Get a color for the line
            color = createHSVColor(hueAngle, 1.0, 1.0);
            hueAngle += 5.0;
            if (hueAngle >= 360.0) {
                hueAngle = 0.0;
            }

            matrix.drawLine(x, 0, 31 - x, 31, color);
            matrix.swapBuffers();
            delay(50);
        }

        // Draw diagonal lines from right to left, top to bottom
        for (int y = 0; y < 32; y++) {

            // Get a color for the line
            color = createHSVColor(hueAngle, 1.0, 1.0);
            hueAngle += 5.0;
            if (hueAngle >= 360.0) {
                hueAngle = 0;
            }

            matrix.drawLine(31, y, 0, 31 - y, color);
            matrix.swapBuffers();
            delay(50);
        }
        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

// Define pixels on display
struct PIXEL pixels[] = {

    // Smallest rect
    14, 14,
    15, 14,
    16, 14,
    17, 14,
    17, 15,
    17, 16,
    17, 17,
    16, 17,
    15, 17,
    14, 17,
    14, 16,
    14, 15,

    // Next rect
    12, 12,
    13, 12,
    14, 12,
    15, 12,
    16, 12,
    17, 12,
    18, 12,
    19, 12,
    19, 13,
    19, 14,
    19, 15,
    19, 16,
    19, 17,
    19, 18,
    19, 19,
    18, 19,
    17, 19,
    16, 19,
    15, 19,
    14, 19,
    13, 19,
    12, 19,
    12, 18,
    12, 17,
    12, 16,
    12, 15,
    12, 14,
    12, 13,

    // Next rect
    10, 10,
    11, 10,
    12, 10,
    13, 10,
    14, 10,
    15, 10,
    16, 10,
    17, 10,
    18, 10,
    19, 10,
    20, 10,
    21, 10,
    21, 11,
    21, 12,
    21, 13,
    21, 14,
    21, 15,
    21, 16,
    21, 17,
    21, 18,
    21, 19,
    21, 20,
    21, 21,
    20, 21,
    19, 21,
    18, 21,
    17, 21,
    16, 21,
    15, 21,
    14, 21,
    13, 21,
    12, 21,
    11, 21,
    10, 21,
    10, 20,
    10, 19,
    10, 18,
    10, 17,
    10, 16,
    10, 15,
    10, 14,
    10, 13,
    10, 12,
    10, 11,

    // Next rect
    8, 8,
    9, 8,
    10, 8,
    11, 8,
    12, 8,
    13, 8,
    14, 8,
    15, 8,
    16, 8,
    17, 8,
    18, 8,
    19, 8,
    20, 8,
    21, 8,
    22, 8,
    23, 8,
    23, 9,
    23, 10,
    23, 11,
    23, 12,
    23, 13,
    23, 14,
    23, 15,
    23, 16,
    23, 17,
    23, 18,
    23, 19,
    23, 20,
    23, 21,
    23, 22,
    23, 23,
    22, 23,
    21, 23,
    20, 23,
    19, 23,
    18, 23,
    17, 23,
    16, 23,
    15, 23,
    14, 23,
    13, 23,
    12, 23,
    11, 23,
    10, 23,
    9, 23,
    8, 23,
    8, 22,
    8, 21,
    8, 20,
    8, 19,
    8, 18,
    8, 17,
    8, 16,
    8, 15,
    8, 14,
    8, 13,
    8, 12,
    8, 11,
    8, 10,
    8, 9,

    // Next rect
    6, 6,
    7, 6,
    8, 6,
    9, 6,
    10, 6,
    11, 6,
    12, 6,
    13, 6,
    14, 6,
    15, 6,
    16, 6,
    17, 6,
    18, 6,
    19, 6,
    20, 6,
    21, 6,
    22, 6,
    23, 6,
    24, 6,
    25, 6,
    25, 7,
    25, 8,
    25, 9,
    25, 10,
    25, 11,
    25, 12,
    25, 13,
    25, 14,
    25, 15,
    25, 16,
    25, 17,
    25, 18,
    25, 19,
    25, 20,
    25, 21,
    25, 22,
    25, 23,
    25, 24,
    25, 25,
    24, 25,
    23, 25,
    22, 25,
    21, 25,
    20, 25,
    19, 25,
    18, 25,
    17, 25,
    16, 25,
    15, 25,
    14, 25,
    13, 25,
    12, 25,
    11, 25,
    10, 25,
    9, 25,
    8, 25,
    7, 25,
    6, 25,
    6, 24,
    6, 23,
    6, 22,
    6, 21,
    6, 20,
    6, 19,
    6, 18,
    6, 17,
    6, 16,
    6, 15,
    6, 14,
    6, 13,
    6, 12,
    6, 11,
    6, 10,
    6,  9,
    6,  8,
    6,  7,

    // Next rect
    4, 4,
    5, 4,
    6, 4,
    7, 4,
    8, 4,
    9, 4,
    10, 4,
    11, 4,
    12, 4,
    13, 4,
    14, 4,
    15, 4,
    16, 4,
    17, 4,
    18, 4,
    19, 4,
    20, 4,
    21, 4,
    22, 4,
    23, 4,
    24, 4,
    25, 4,
    26, 4,
    27, 4,
    27, 5,
    27, 6,
    27, 7,
    27, 8,
    27, 9,
    27, 10,
    27, 11,
    27, 12,
    27, 13,
    27, 14,
    27, 15,
    27, 16,
    27, 17,
    27, 18,
    27, 19,
    27, 20,
    27, 21,
    27, 22,
    27, 23,
    27, 24,
    27, 25,
    27, 26,
    27, 27,
    26, 27,
    25, 27,
    24, 27,
    23, 27,
    22, 27,
    21, 27,
    20, 27,
    19, 27,
    18, 27,
    17, 27,
    16, 27,
    15, 27,
    14, 27,
    13, 27,
    12, 27,
    11, 27,
    10, 27,
    9, 27,
    8, 27,
    7, 27,
    6, 27,
    5, 27,
    4, 27,
    4, 26,
    4, 25,
    4, 24,
    4, 23,
    4, 22,
    4, 21,
    4, 20,
    4, 19,
    4, 18,
    4, 17,
    4, 16,
    4, 15,
    4, 14,
    4, 13,
    4, 12,
    4, 11,
    4, 10,
    4,  9,
    4,  8,
    4,  7,
    4,  6,
    4,  5,

    // Next rect

    2, 2,
    3, 2,
    4, 2,
    5, 2,
    6, 2,
    7, 2,
    8, 2,
    9, 2,
    10, 2,
    11, 2,
    12, 2,
    13, 2,
    14, 2,
    15, 2,
    16, 2,
    17, 2,
    18, 2,
    19, 2,
    20, 2,
    21, 2,
    22, 2,
    23, 2,
    24, 2,
    25, 2,
    26, 2,
    27, 2,
    28, 2,
    29, 2,
    29, 3,
    29, 4,
    29, 5,
    29, 6,
    29, 7,
    29, 8,
    29, 9,
    29, 10,
    29, 11,
    29, 12,
    29, 13,
    29, 14,
    29, 15,
    29, 16,
    29, 17,
    29, 18,
    29, 19,
    29, 20,
    29, 21,
    29, 22,
    29, 23,
    29, 24,
    29, 25,
    29, 26,
    29, 27,
    29, 28,
    29, 29,
    28, 29,
    27, 29,
    26, 29,
    25, 29,
    24, 29,
    23, 29,
    22, 29,
    21, 29,
    20, 29,
    19, 29,
    18, 29,
    17, 29,
    16, 29,
    15, 29,
    14, 29,
    13, 29,
    12, 29,
    11, 29,
    10, 29,
    9, 29,
    8, 29,
    7, 29,
    6, 29,
    5, 29,
    4, 29,
    3, 29,
    2, 29,
    2, 28,
    2, 27,
    2, 26,
    2, 25,
    2, 24,
    2, 23,
    2, 22,
    2, 21,
    2, 20,
    2, 19,
    2, 18,
    2, 17,
    2, 16,
    2, 15,
    2, 14,
    2, 13,
    2, 12,
    2, 11,
    2, 10,
    2,  9,
    2,  8,
    2,  7,
    2,  6,
    2,  5,
    2,  4,
    2,  3,

    // Next rect
    0, 0,
    1, 0,
    2, 0,
    3, 0,
    4, 0,
    5, 0,
    6, 0,
    7, 0,
    8, 0,
    9, 0,
    10, 0,
    11, 0,
    12, 0,
    13, 0,
    14, 0,
    15, 0,
    16, 0,
    17, 0,
    18, 0,
    19, 0,
    20, 0,
    21, 0,
    22, 0,
    23, 0,
    24, 0,
    25, 0,
    26, 0,
    27, 0,
    28, 0,
    29, 0,
    30, 0,
    31, 0,
    31, 1,
    31, 2,
    31, 3,
    31, 4,
    31, 5,
    31, 6,
    31, 7,
    31, 8,
    31, 9,
    31, 10,
    31, 11,
    31, 12,
    31, 13,
    31, 14,
    31, 15,
    31, 16,
    31, 17,
    31, 18,
    31, 19,
    31, 20,
    31, 21,
    31, 22,
    31, 23,
    31, 24,
    31, 25,
    31, 26,
    31, 27,
    31, 28,
    31, 29,
    31, 30,
    31, 31,
    30, 31,
    29, 31,
    28, 31,
    27, 31,
    26, 31,
    25, 31,
    24, 31,
    23, 31,
    22, 31,
    21, 31,
    20, 31,
    19, 31,
    18, 31,
    17, 31,
    16, 31,
    15, 31,
    14, 31,
    13, 31,
    12, 31,
    11, 31,
    10, 31,
    9, 31,
    8, 31,
    7, 31,
    6, 31,
    5, 31,
    4, 31,
    3, 31,
    2, 31,
    1, 31,
    0, 31,
    0, 30,
    0, 29,
    0, 28,
    0, 27,
    0, 26,
    0, 25,
    0, 24,
    0, 23,
    0, 22,
    0, 21,
    0, 20,
    0, 19,
    0, 18,
    0, 17,
    0, 16,
    0, 15,
    0, 14,
    0, 13,
    0, 12,
    0, 11,
    0, 10,
    0,  9,
    0,  8,
    0,  7,
    0,  6,
    0,  5,
    0,  4,
    0,  3,
    0,  2,
    0,  1
};

void rectRotatingColorsPattern() {

    rgb24 colors[136];

    struct PIXEL p;
    rgb24 color;
    int colorIndex = 0;

    int numberOfPixels = sizeof(pixels) / sizeof(struct PIXEL);

    // Precalculate colors
    for (int i = 0; i < 136; i++) {
        // Calculate color for the pixel
        colors[i] = createHSVColorWithDivisions(136, i);
    }

    while (true) {

        for (int i = 0; i < numberOfPixels; i++) {

            // Find the location of the pixel
            p = pixels[i];
            matrix.drawPixel(p.x, p.y, colors[colorIndex]);

            colorIndex++;
            colorIndex %= 136;
        }
        matrix.swapBuffers();
        colorIndex++;

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

void verticalPaletteLinesPattern() {

    rgb24 color;

    int colorIncrement = PALETTE_SIZE / WIDTH;

    while (true) {

        // Pick a random palette
        int paletteNumber = random(NUM_OF_PALETTES);
        generatePaletteNumber(paletteNumber);

        // Pick a random place to start within the palette
        int colorIndex = random(PALETTE_SIZE);
        for (int x = 0; x < WIDTH; x++) {

            color = palette[colorIndex];
            colorIndex += colorIncrement;
            colorIndex %= PALETTE_SIZE;

            matrix.drawLine(x, MINY, x, MAXY, color);
            matrix.swapBuffers();

            if (checkForTermination()) {
                return;
            }
            delay(50);
        }
        delay(3000);

        matrix.fillScreen(COLOR_BLACK);
    }
}

void horizontalPaletteLinesPattern() {

    rgb24 color;

    int colorIncrement = PALETTE_SIZE / HEIGHT;

    while (true) {
        // Pick a random palette
        int paletteNumber = random(NUM_OF_PALETTES);
        generatePaletteNumber(paletteNumber);

        // Pick a random place to start within the palette
        int colorIndex = random(PALETTE_SIZE);
        for (int y = 0; y < HEIGHT; y++) {

            color = palette[colorIndex];
            colorIndex += colorIncrement;
            colorIndex %= PALETTE_SIZE;

            matrix.drawLine(MINX, y, MAXX, y, color);
            matrix.swapBuffers();

            if (checkForTermination()) {
                return;
            }
            delay(50);
        }
        delay(3000);

        matrix.fillScreen(COLOR_BLACK);
    }
}

// Generate recursive T Square Fractal
void generateTSquare(int depth, float x, float y, float w, float h, rgb24 color) {

    // Draw a filled rectangle
    matrix.fillRectangle(x, y, x + w - 1, y + h - 1, color);
    matrix.swapBuffers();

    if (depth > 1)  {
        float newWidth  = w / 2.0;
        float newHeight = h / 2.0;

        generateTSquare(depth - 1, x -     (newWidth / 2.0), y -     (newHeight / 2.0), newWidth, newHeight, color);
        generateTSquare(depth - 1, x + w - (newWidth / 2.0), y -     (newHeight / 2.0), newWidth, newHeight, color);
        generateTSquare(depth - 1, x -     (newWidth / 2.0), y + h - (newHeight / 2.0), newWidth, newHeight, color);
        generateTSquare(depth - 1, x + w - (newWidth / 2.0), y + h - (newHeight / 2.0), newWidth, newHeight, color);

    }
}

void tSquareFractalPattern() {

    float x, y, w, h;
    rgb24 color, fgColor, bgColor;


    while (true) {
        // Fill the screen with a random color
        double hue = random(360);
        color = createHSVColor(hue, 1.0, 1.0);

        if (random(2) == 0) {
            fgColor = COLOR_BLACK;
            bgColor = color;
        }
        else    {
            fgColor = color;
            bgColor = COLOR_BLACK;
        }
        matrix.fillScreen(bgColor);
        matrix.swapBuffers();
        delay(500);

        x = WIDTH  / 4.0;
        y = HEIGHT / 4.0;
        w = WIDTH  / 2.0;
        h = HEIGHT / 2.0;

        int depth = random(2, 6);

        // Generate recursive fractal
        generateTSquare(depth, x, y, w, h, fgColor);

        delay(2000);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

// Colored boxes
void coloredBoxesPattern() {

    rgb24 color;
    const int bWidth  = 6;
    const int bHeight = 6;
    const int bSpace  = 2;
    const int bBorder = 1;

    while (true) {
        int numberOfColors = random(50);
        int colorIndex = random(numberOfColors);

        for (int countVert = 0; countVert < 4; countVert++) {
            for (int countHoriz = 0; countHoriz < 4; countHoriz++) {
                // Calculate color
                color = createHSVColorWithDivisions(numberOfColors, colorIndex++);

                int xPos = bBorder + countHoriz * (bWidth  + bSpace);
                int yPos = bBorder + countVert  * (bHeight + bSpace);

                // Create filled rect
                matrix.fillRectangle(xPos, yPos, xPos + bWidth - 1, yPos + bHeight - 1, color);
                matrix.swapBuffers();
                delay(80);
            }
        }

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

// Concentric squares patterm
void concentricSquaresPattern() {
    int x0, y0, x1, y1;
    rgb24 color;

    int paletteIncrement = PALETTE_SIZE / 16;

    while (true) {

        int paletteNumber = random(NUM_OF_PALETTES);
        generatePaletteNumber(paletteNumber);
        int colorIndex = random(PALETTE_SIZE);

        x0 = 15;
        y0 = 15,
        x1 = 16;
        y1 = 16;

        for (int i = 0 ; i < 16; i++) {

            color = palette[colorIndex];
            colorIndex += paletteIncrement;
            colorIndex %= PALETTE_SIZE;

            matrix.drawRectangle(x0, y0, x1, y1, color);
            matrix.swapBuffers();
            delay(80);

            x0--;
            y0--;
            x1++;
            y1++;

            // Check for termination
            if (checkForTermination()) {
                return;
            }
        }
    }
}

// Concentric circles patterm
void concentricCirclesPattern() {
    int radius;
    rgb24 color;

    int paletteIncrement = PALETTE_SIZE / 16;

    while (true) {

        int paletteNumber = random(NUM_OF_PALETTES);
        generatePaletteNumber(paletteNumber);
        int colorIndex = random(PALETTE_SIZE);

        radius = 1;

        for (int i = 0 ; i < 15; i++) {

            color = palette[colorIndex];
            colorIndex += paletteIncrement;
            colorIndex %= PALETTE_SIZE;

            matrix.drawCircle(MIDX, MIDY, radius++, color);
            matrix.swapBuffers();
            delay(80);

            // Check for termination
            if (checkForTermination()) {
                return;
            }
        }
    }
}

int rcPaletteIncrement;
int rcPaletteIndex;

void generateCircle(int depth, int xc, int yc, int radius) {

    rgb24 color = palette[rcPaletteIndex];
    rcPaletteIndex += rcPaletteIncrement;
    rcPaletteIndex %= PALETTE_SIZE;

    // Draw a circle
    matrix.drawCircle(xc, yc, radius, color);
    matrix.swapBuffers();
    delay(80);

    if (depth > 0) {
        // Call this function recursively
        generateCircle(depth - 1, xc + radius, yc, round(radius / 2.0));
        generateCircle(depth - 1, xc, yc - radius, round(radius / 2.0));
        generateCircle(depth - 1, xc - radius, yc, round(radius / 2.0));
        generateCircle(depth - 1, xc, yc + radius, round(radius / 2.0));
    }
}

void recursiveCircles() {

    float xc, yc, radius;

    while (true) {
        int paletteNumber = random(NUM_OF_PALETTES);
        generatePaletteNumber(paletteNumber);
        rcPaletteIndex = random(PALETTE_SIZE);

        matrix.fillScreen(COLOR_BLACK);
        matrix.swapBuffers();
        delay(500);

        xc = MIDX;
        yc = MIDY;
        radius = 12;

        int depth = random(1, 4);

        // Select palette increment so full palette is always used
        rcPaletteIncrement = PALETTE_SIZE / pow(4, depth);

        // Generate recursive fractal
        generateCircle(depth, xc, yc, radius);

        delay(2000);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

void animationPattern() {

    char pathname[50];

    enumerateGIFFiles(GENERAL_GIFS, false);

    // Choose a random animated GIF for display
    chooseRandomGIFFilename(GENERAL_GIFS, pathname);

    while (true) {
        // Run single cycle of animation
        processGIFFile(pathname);

        // Check for termination
        if (checkForTermination()) {
            return;
        }
    }
}

/*
// Display random animations from specified directory
 void runRandomAnimations(const char *directoryName) {
 
 char pathname[50];
 unsigned long timeOut;
 
 // Turn off any text scrolling
 matrix.scrollText("", 1);
 matrix.setScrollMode(off);
 
 // Enumerate the animated GIF files in specified directory
 enumerateGIFFiles(directoryName, false);
 
 // Do forever
 while (true) {
 
 // Clear screen for new animation
 matrix.fillScreen(COLOR_BLACK);
 matrix.swapBuffers();
 
 delay(1000);
 
 // Select an animation file by index
 chooseRandomGIFFilename(directoryName, pathname);
 
 // Calculate time in the future to terminate animation
 timeOut = millis() + (ANIMATION_DISPLAY_DURATION_SECONDS * 1000);
 
 while (timeOut > millis()) {
 processGIFFile(pathname);
 }
 
 // Check for user termination
 for (int i = 0; i < 50; i++) {
 // Has user aborted ?
 if (readIRCode() == IRCODE_HOME) {
 return;
 }
 delay(20);
 }
 }
 }
 */

// Display random animations from specified directory
void runAnimations(const char *directoryName) {

    char pathname[50];
    unsigned long timeOut;

    // Turn off any text scrolling
    matrix.scrollText("", 1);
    matrix.setScrollMode(off);

    // Enumerate the animated GIF files in specified directory
    enumerateGIFFiles(directoryName, false);

    int startIndex, index;
    startIndex = index = random(numberOfFiles);

    // Do forever
    while (true) {

        // Clear screen for new animation
        matrix.fillScreen(COLOR_BLACK);
        matrix.swapBuffers();

        delay(1000);

        // Select an animation file by index
        getGIFFilenameByIndex(directoryName, index++, pathname);

        index %= numberOfFiles;

        if (index == startIndex) {
            startIndex = index = random(numberOfFiles);
        }

        // Calculate time in the future to terminate animation
        timeOut = millis() + (ANIMATION_DISPLAY_DURATION_SECONDS * 1000);

        while (timeOut > millis()) {
            processGIFFile(pathname);
        }

        // Check for user termination
        for (int i = 0; i < 50; i++) {
            // Has user aborted ?
            if (readIRCode() == IRCODE_HOME) {
                return;
            }
            delay(20);
        }
    }
}

// Run general animations
void generalAnimationsMode() {
    runAnimations(GENERAL_GIFS);
}

// Run Christmas animations
void christmasAnimationsMode() {
    runAnimations(CHRISTMAS_GIFS);
}

// Run Halloween animations
void halloweenAnimationsMode() {
    runAnimations(HALLOWEEN_GIFS);
}

// Run Valentine animations
void valentineAnimationsMode() {
    runAnimations(VALENTINE_GIFS);
}

// Run 4th of July animations
void fourthAnimationsMode() {
    runAnimations(FOURTH_GIFS);
}






