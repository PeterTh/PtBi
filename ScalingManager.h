#pragma once

#include "stdafx.h"

#include "stuff.h"
#include "GLFragmentProgram.h"
#include "GLRenderTexture.h"

class ScalingManager {
public:
	enum interpMode { BILINEAR, NEAREST, FIXED, ADAPTIVE, NUM_MODES };
private:
	GLFragmentProgram *calcSmoothness, *generateLanczosLut, *scaleLanczosStatic, *scaleLanczosAdaptive;
	GLRenderTexture *buffer, *lanczosLut;
	interpMode currentMode;

	GLPresenter& presenter;

public:
	ScalingManager(GLPresenter& presenter);
	~ScalingManager();

	void drawScaled(GLRenderTexture *source, GLRenderTexture *target = NULL);

	void incScalingMode();
	void decScalingMode();
	void setScalingMode(interpMode mode);

	const char *getCurModeName();

private:
	static const char *modeNames[NUM_MODES];
};
