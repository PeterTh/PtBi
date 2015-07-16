// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _DEBUG
#define RELEASE_VERSION
#endif

#include <stdio.h>
#include <tchar.h>

#include <conio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <vector>
#include <map>
using namespace std;

#include <Objbase.h>

#include <GL/glew.h>
#include <GL/glfw.h>

#include "version.h"

struct RTAssertException : public std::runtime_error {
	RTAssertException() : std::runtime_error("RT Assertion failed.") { }
};

static inline void showErrorString(std::string s) {
	ofstream log("error.log");
	log << s;
	log.close();
	std::cerr << s;
	wchar_t wtext[256];
	mbstowcs(wtext, s.c_str(), strlen(s.c_str()) + 1);
	MessageBox(NULL, wtext, L"PtBi Error", MB_ICONERROR | MB_OK);
}

#define RT_ASSERT(checkexp, message) { \
	bool cval = (checkexp); \
	if(!cval) { \
	std::stringstream ss; \
	ss << "Runtime assertion failed in " << __FILE__ << " on line " << __LINE__ \
	   << "\n# Check: " << #checkexp << "\n# Message: " << message << "\n"; \
	showErrorString(ss.str()); \
	throw new RTAssertException(); } }

#define RT_GL_ASSERT(message) { \
	GLenum err = glGetError();\
	if(err != GL_NO_ERROR) { \
	std::stringstream ss; \
	ss << "Runtime GL assertion failed in " << __FILE__ << " on line " << __LINE__ \
	   << "\n# OpenGL error string: " << gluErrorString(err) << "\n# Message: " << message << "\n"; \
	showErrorString(ss.str()); \
	throw new RTAssertException(); } }


#define WARN(message) { cout << "Warning: " << message << "\n"; }
#define WARN_IF(checkexp, message) { if(checkexp) WARN(message) }

#ifdef RELEASE_VERSION
#define NONRELEASE(code)
#define DEBUG(message)
#else
#define NONRELEASE(code) code
#define DEBUG(message) { \
	cout << "Debug " << __FILE__ << "/" << __LINE__ << ": " << message << endl << flush; }
#endif

#include "stuff.h"
