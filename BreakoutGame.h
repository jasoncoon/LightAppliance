#ifndef BreakoutGame_H
#define BreakoutGame_H

#include "SmartMatrix_32x32.h"
#include "IRremote.h"

class BreakoutGame{

private:

struct Rect {
	float left;
	float right;
	float top;
	float bottom;

	float width;
	float height;

	float speedX;
	float speedY;

	bool inActive = false;

	rgb24 color;

	void setLeft(float newLeft) {
		left = newLeft;
		right = left + (width - 1);
	}

	void setRight(float newRight) {
		right = newRight;
		left = right - (width - 1);
	}

	void setTop(float y) {
		top = y;
		bottom = top + (height - 1);
	}

	void setBottom(float newBottom) {
		bottom = newBottom;
		top = bottom - (height - 1);
	}

	bool intersectsWith(Rect rect) {
		return (rect.left <= right + 1 && rect.right >= left - 1 && rect.top <= bottom) && rect.bottom >= top - 1;
	}
};

	Rect ball;
	Rect paddle;
	Rect blocks[32];
	int16_t screenWidth;
	int16_t screenHeight;
	unsigned long lastInput = 0;
	char positionBuffer[5]; // XX,YY
	bool showPosition = false;
	bool ballFellOutBottom = false;
  bool isPaused;
  int lives;
  int score;
  char scoreText[8];

	void resetBall();
	void setup(SmartMatrix &matrix);
	unsigned long handleInput(IRrecv &irReceiver);
	void update();
	void draw(SmartMatrix &matrix);
  void reset();
  void generateBlocks();

public:
	BreakoutGame();
	~BreakoutGame();
	void run(SmartMatrix &matrix, IRrecv &irReceiver);
};

#endif
