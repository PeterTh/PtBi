#pragma once

#include "stdafx.h"
#include <memory>

#include "Timer.h"

namespace font {
	typedef std::pair<float,float> position;
	position print(float x, float y, const char *text, bool measure = false);
}

class ConsoleLine {
	Timer t;
	string msg;
	float ypos;

public:
	ConsoleLine(const string& msg) : msg(msg), ypos(-1.0f) {
	}

	float draw(float y);
};

struct StaticText {
	string text;
	float x, y;

	StaticText(string text, float x, float y) 
		: text(text), x(x), y(y) {

	}
};

typedef std::shared_ptr<StaticText> StaticTextPtr;

class Console {
	static Console* self;
	
	vector<ConsoleLine> lines;
	vector<StaticTextPtr> statics;
	int start, width, height;
	float lineHeight;
	bool showStatics;

public:
	Console() : start(0), showStatics(false) {
	}

	static Console& get() { 
		if(self == NULL) self = new Console();
		return *self; 
	}

	void add(const string& msg) {
		lines.push_back(ConsoleLine(msg));
	}

	void add(StaticTextPtr text) {
		statics.push_back(text);
	}

	void draw();

	void setSize(int w, int h) {
		width = w;
		height = h;
	}

	int getW() { return width; }
	int getH() { return height; }

	void toggleStatics() { showStatics = !showStatics; }
};
