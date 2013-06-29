#pragma once
#include "stdafx.h"

class GLPresenter;

class KeyBinding {
	map<int,string> binding;
	void handleBinding(string bind);
	void handleStartup(string start);

	bool isFunCall(const string& fun, string action);
	string decodeParam(string action, unsigned num = 0);
	float decodeFloatParam(const string& fun, unsigned num = 0);
	int decodeIntParam(const string& fun, unsigned num = 0);

	GLPresenter *pres;

public:
	KeyBinding(GLPresenter *pres);
	void handleEvent(int key, int action);
	void performAction(string action);
};