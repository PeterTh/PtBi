#pragma once

#include "stdafx.h"

#include "stuff.h"
#include "Capture.h"
#include "Timer.h"
#include "Console.h"
#include "GLFragmentProgram.h"
#include "GLRenderTexture.h"
#include "GLMultiRenderTexture.h"

class IPManager {
public:
	enum ipFilters { NONE = 0, 
		LOCAL_CONTRAST = 1<<1,
		GAMMA_CORRECTION = 1<<2 };

private:
	ipFilters activeFilters;
	GLFragmentProgram *localContrastGather, *localContrastApply;
	GLRenderTexture **localContrastBuffers;
	GLFragmentProgram *gammaCorrection;

	GLfloat contrastIncrease, gamma;
	const unsigned localContrastRange;

	GLPresenter& presenter;

public:
	IPManager(GLPresenter& presenter);
	~IPManager();

	void applyIP(GLRenderTexture *source, GLRenderTexture *target);

	ipFilters getActiveFilters();

	void toggleFilter(ipFilters filter);
	
	string getActiveFilterString();
	
	void adjustContrastIncrease(GLfloat adjustment);

	void adjustGamma(GLfloat adjustment);
};
