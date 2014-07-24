#ifndef JuliaFractal_H
#define JuliaFractal_H

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class JuliaFractal{
private:
    SmartMatrix *matrix;
    IRrecv *irReceiver;
    
    unsigned w = 32;
    unsigned h = 32;

    unsigned long lastInput = 0;
    //each iteration, it calculates: new = old*old + c, where c is a constant and old starts at current pixel
    double cRe, cIm;                   //real and imaginary part of the constant c, determinate shape of the Julia Set
    double newRe, newIm, oldRe, oldIm;   //real and imaginary parts of new and old
    double zoom = 1, moveX = 0, moveY = 0; //you can change these to zoom and change position
    rgb24 color; //the RGB color value for the pixel
    int maxIterations = 128; //after how much iterations the function should stop
    //int halfMaxIterations = maxIterations / 2;
    
    static unsigned const MAXIMUM = 128;

    rgb24 colors[MAXIMUM];

    char stringBuffer[32];

    unsigned long handleInput();
    void draw();
    void generateColors();
    void reset();
    rgb24 createHSVColor(float hue, float saturation,  float value);
    void hsvToRGB(float hue, float saturation, float value, float * red, float * green, float * blue);

public:
    void runPattern(SmartMatrix matrixRef, IRrecv irReceiverRef, boolean(*checkForTermination)());
    void runGame(SmartMatrix matrixRef, IRrecv irReceiverRef);
};

#endif