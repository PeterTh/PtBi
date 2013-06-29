#pragma once

#include "stdafx.h"

#include "Timer.h"

class ConsoleLine {
	Timer t;
	string msg;
	float ypos;

public:
	ConsoleLine(const string& msg) : msg(msg), ypos(-1.0f) {
	}

	float draw(float y);
};

class Console {
	static Console* self;

	vector<ConsoleLine> lines;
	int start, width, height;
	float lineHeight;

public:
	Console() : start(0) {
	}

	static Console& get() { 
		if(self == NULL) self = new Console();
		return *self; 
	}

	void add(const string& msg) {
		lines.push_back(ConsoleLine(msg));
	}

	void draw() {
		if(lineHeight > 0.0f) {
			glEnable(GL_BLEND);
			glBegin(GL_QUADS);
			glColor4d(0.0,0.0,0.0,0.4);
			glVertex2d(-1.0, 1.0);
			glVertex2d( 1.0, 1.0);
			glVertex2d( 1.0, 1.0-lineHeight/height);
			glVertex2d(-1.0, 1.0-lineHeight/height);
			glColor4d(1.0,1.0,1.0,1.0);
			glEnd();
			glDisable(GL_BLEND);
		}
		float y = 0.0f;
		// maximum: 5 lines
		if(lines.size()-start > 5) start += lines.size()-start-5; 
		for(size_t i=start; i<lines.size(); ++i) {
			float ret = lines[i].draw(y);
			if(ret == 0.0f) start = i+1; // if text timed out increase start
			else y = ret + 2.0f;
		}
		if(y == 0.0f) {
			if(lineHeight>0.2f) lineHeight *= 0.6f;
			else lineHeight = 0.0f;
		}
		else lineHeight = y + 10.0f;
	}

	void setSize(int w, int h) {
		width = w;
		height = h;
	}

	int getW() { return width; }
	int getH() { return height; }
};
