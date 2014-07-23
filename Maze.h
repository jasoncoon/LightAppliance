#ifndef Maze_H
#define Maze_H

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class Maze{
private:
    enum Directions {
        None = 0,
        Up = 1,
        Down = 2,
        Left = 4,
        Right = 8,
    };

    struct Point{
        int x;
        int y;

        static Point New(int x, int y) {
            Point point;
            point.x = x;
            point.y = y;
            return point;
        }

        Point Move(Directions direction) {
            switch (direction)
            {
                case Up:
                    return New(x, y - 1);

                case Down:
                    return New(x, y + 1);

                case Left:
                    return New(x - 1, y);

                case Right:
                    return New(x + 1, y);
            }
        }

        static Directions Opposite(Directions direction) {
            switch (direction) {
                case Up:
                    return Down;

                case Down:
                    return Up;

                case Left:
                    return Right;

                case Right:
                    return Left;
            }
        }
    };

    SmartMatrix *matrix;
    IRrecv *irReceiver;

    static const int width = 16;
    static const int height = 16;

    int imageWidth = width * 2;
    int imageHeight = height * 2;

    Directions grid[width][height];

    unsigned long lastInput = 0;

    Point point;

    Point start;
    Point end;
    Point player;

    Point cells[256];
    int cellCount = 0;
    int highestCellCount = 0;

    int algorithm = 0;
    int algorithmCount = 2;

    Directions directions[4] = { Up, Down, Left, Right };

    Point createPoint(int x, int y);
    int chooseIndex(int max);
    void removeCell(int index);
    void shuffleDirections();
    int generateMaze(bool animate, boolean(*checkForTermination)());

    unsigned long handleInput();
    void draw();

    int directionsCount(Directions directions);

public:
    void runPattern(SmartMatrix matrixRef, IRrecv irReceiverRef, boolean(*checkForTermination)());
    void runGame(SmartMatrix matrixRef, IRrecv irReceiverRef);
};

#endif
