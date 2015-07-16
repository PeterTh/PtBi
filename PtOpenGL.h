#pragma once

#include "stdafx.h"

#include <IL/il.h>
#include <ctime>

#include "stuff.h"
#include "Capture.h"
#include "Timer.h"
#include "GLFragmentProgram.h"
#include "GLRenderTexture.h"
#include "GLMultiRenderTexture.h"
#include "ResHandling.h"
#include "Console.h"
#include "keys.h"
#include "ScalingManager.h"
#include "AAManager.h"
#include "IPManager.h"

class GLPresenter : public FrameListener {
	static GLPresenter* self;

	static int GLFWCALL closeCallback() { self->closeEvent(); return 1; }
	static void GLFWCALL resizeCallback(int width, int height) { self->reshape(width, height); }
	static void GLFWCALL mousePosCallback(int x, int y) { self->mouseMoved(); }
	static void GLFWCALL keyCallback(int key, int action) { self->keyboard(key, action); }

	const unsigned data_size;

	DeckLinkCapture &capture;
	GLuint texId;
	GLuint displayList;

	GLFragmentProgram *initialConvert;
	GLRenderTexture *buffer, *buffer2;
	ScalingManager *scalingManager;
	AAManager *aaManager;
	IPManager *ipManager;
	KeyBinding *keyBinding;

	HGLRC hrc;
	HDC hdc;
	HWND hwnd;

	RECT windowRect;
	double aspect, oneToNScaleFactor;
	bool running, fullscreen, useVsync, rgbFull;
	clock_t cursorTimeout;
	int reqW, reqH, reqHz, wSizeW, wSizeH, captureBufferW;

	void *dataPointers[2];
	volatile unsigned long long frameIndex, drawnFrameIndex;

	GLcharARB prepend[512];
	
	StaticTextPtr graphicsReportText;
	SlidingAverage frameProcTimes;

public:
	GLPresenter(DeckLinkCapture &capture, int w, int h, int hz);

	const GLcharARB* getShaderPrependString();
	unsigned getW();
	unsigned getH();

	bool run();

	virtual void frameRecieved(void* data);
	
	int fsww, fswh;
	void switchFullscreen();

	void toggleRGBRange();

	void adjustPredicate(GLfloat upper, GLfloat lower) {
		aaManager->adjustPredicate(upper, lower);
	}

	void changeAA() {
		aaManager->incAAMode();
		Console::get().add(format("AA mode set to %s", aaManager->getCurModeName()));
	}
	void setAA(int mode) {
		if(mode<0 || mode>=AAManager::NUM_MODES) {
			Console::get().add(format("Invalid AA mode %d requested", mode));
		} else {
			aaManager->setAAMode(static_cast<AAManager::aaMode>(mode));
			Console::get().add(format("AA mode set to %s", aaManager->getCurModeName()));
		}
	}

	void testAA() {
		testAAinternal();
		Console::get().add("AA modes tested, output files generated");
	}

	void switchVSync() {
		useVsync = !useVsync;
		glfwSwapInterval(useVsync?1:0);
		Console::get().add(useVsync ? "Enabled VSYNC" : "Disabled VSYNC");
	}

	void switchAspect() {
		if(aspect < 16.0/9.0-0.01) {
			aspect = 16.0/9.0;
			Console::get().add("Set 16:9 aspect ratio");
		} else {
			aspect = 4.0/3.0;
			Console::get().add("Set 4:3 aspect ratio");
		}
		glfwSetWindowSize(wSizeW-1, wSizeH-1); // stupid
		glfwSetWindowSize(wSizeW+1, wSizeH+1);
	}

	void changeScaling() {
		scalingManager->incScalingMode();
		Console::get().add(format("Scaling mode set to %s", scalingManager->getCurModeName()));
	}
	void setScaling(int mode) {
		if(mode<0 || mode>=ScalingManager::NUM_MODES) {
			Console::get().add(format("Invalid scaling mode %d requested", mode));
		} else {
			scalingManager->setScalingMode(static_cast<ScalingManager::interpMode>(mode));
			Console::get().add(format("Scaling mode set to %s", scalingManager->getCurModeName()));
		}
	}

	void setDisplayHz() {
		setClosestHz(reqHz);
		Console::get().add(format("Set %dHz mode", reqHz));
	}

	void saveScreenshot() {
		string fn = format("captures/%s.png", timeString());
		buffer->saveToImage(fn.c_str());
		Console::get().add(format("Captured screenshot to %s", fn.c_str()));
	}

	void resizeTo(int height) {
		int windowH = height;
		int windowW = height*16/9;
		if(fullscreen) {
			switchFullscreen();
		}
		glfwSetWindowSize(windowW, windowH);
		Console::get().add(format("Resized to %dx%d", windowW, windowH));
	}

	void toggleOneToNScaling(float n) {
		if(oneToNScaleFactor > 0.0) {
			oneToNScaleFactor = -1.0;
			Console::get().add("Disabled 1:N scaling");
		} else {
			oneToNScaleFactor = n;
			Console::get().add(format("Enabled 1:%2.1lf scaling", oneToNScaleFactor));
		}
		reshape(wSizeW, wSizeH);
	}

	void closeEvent() {
		close();
		exit(0);
	}

	IPManager* getIPManager() {
		return ipManager;
	}

private:
	void initGL();

	void display();

	void reshape(int w, int h);

	void close();

	void testScaling();
	
	void testAAinternal();

	void keyboard(int key, int action) {
		keyBinding->handleEvent(key, action);
	}

	void mouseMoved() {
		cursorTimeout = clock() + 5000;
	}

	void checkExtensions() {
		RT_ASSERT(GLEW_ARB_shading_language_100!=0, "OpenGL shading language 1.0 not supported!");
		RT_ASSERT(GLEW_ARB_shader_objects!=0, "GL_ARB_shader_objects extension not found.");
		RT_ASSERT(GLEW_EXT_framebuffer_object!=0, "GL_ARB_framebuffer_object extension was not found");
		RT_ASSERT(GLEW_ARB_texture_float!=0, "GL_ARB_texture_float extension was not found");
	}
};
