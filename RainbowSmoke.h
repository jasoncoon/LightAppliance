#ifndef RainbowSmoke_H
#define RainbowSmoke_H

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class RainbowSmoke{
private:
    SmartMatrix *matrix;
    IRrecv *irReceiver;

    struct Point {
        int x;
        int y;
    };

    bool average = false;
    static const int NUMCOLORS = 11;
    static const int COLOR_COUNT = 1024;
    static const int WIDTH = 32;
    static const int HEIGHT = 32;
    int startx = 15;
    int starty = 15;

    rgb24 colors[COLOR_COUNT];
    bool hasColor[WIDTH][HEIGHT];
    bool isAvailable[WIDTH][HEIGHT];

    void markAvailableNeighbors(Point point);

    Point getAvailablePoint(int algorithm, rgb24 color);
    Point getAvailablePointWithClosestNeighborColor(rgb24 color);
    Point getAvailablePointWithClosestAverageNeighborColor(rgb24 color);

    void sortColors();
    void sortColorsHSV();
    void sortColorsRGB();
    void sortColorsBRG();
    void sortColorsGBR();
    void shuffleColors();

#define MAX_COLOR_VALUE     255

    void hsvToRGB(float hue, float saturation, float value, float * red, float * green, float * blue);
    rgb24 createHSVColor(float hue, float saturation, float value);

    int colorDifference(rgb24 c1, rgb24 c2) {
        int r = c1.red - c2.red;
        int g = c1.green - c2.green;
        int b = c1.blue - c2.blue;
        return r * r + g * g + b * b;
    }

public:
    void runPattern(SmartMatrix matrixRef, IRrecv irReceiverRef, boolean(*checkForTermination)());
};

#endif