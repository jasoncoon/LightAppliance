#ifndef Mandelbrot_H
#define Mandelbrot_H

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class Mandelbrot{
private:
    SmartMatrix *matrix;
    IRrecv *irReceiver;
    
    unsigned long lastInput = 0;
    
    static unsigned const MAXIMUM = 32;

    unsigned MaxIterations = 8;
    int NUMBER_OF_COLORS = MaxIterations;
    unsigned halfMaxIterations = MaxIterations / 2;
    
    rgb24 colors[MAXIMUM];
    
    double imageHeight = 32;
    double imageWidth = 32;
    double MinRe = -2.0; // left
    double MaxRe = 1.0; // right
    double MinIm = -1.5; // bottom
    double zoomFactor = .99;
    double width = 3.0;
    double MaxIm;
    double Re_factor;
    double Im_factor;
    unsigned y;
    double c_im;
    unsigned x;
    double c_re;
    double Z_re;
    double Z_im;
    bool isInside;
    unsigned n;
    double Z_re2;
    double Z_im2;
    
    char stringBuffer[32];

    unsigned long handleInput();
    void draw();
    void generateColors();
    rgb24 createHSVColor(float hue, float saturation,  float value);
    void hsvToRGB(float hue, float saturation, float value, float * red, float * green, float * blue);

public:
    void runPattern(SmartMatrix matrixRef, IRrecv irReceiverRef, boolean(*checkForTermination)());
    void runGame(SmartMatrix matrixRef, IRrecv irReceiverRef);
};

#endif