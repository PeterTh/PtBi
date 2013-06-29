#pragma once

#include "stdafx.h"

#include "stuff.h"
#include "Capture.h"
#include "Timer.h"
#include "Console.h"
#include "GLFragmentProgram.h"
#include "GLRenderTexture.h"
#include "GLMultiRenderTexture.h"

class AAManager {
public:
	enum aaMode { NONE, FXAA, PXAA, TPXAA, SMAA, NUM_MODES };

private:
	aaMode currentMode;
	GLFragmentProgram *applyFXAA, *initAAPredicate, *fillAAPredicateH, *fillAAPredicateV, 
		*fillAAPredicateExt, *applyAAPredicate, *extendAAPredicate, *applyAAPredicateTemp,
		*smaaEdgeDetect, *smaaCalcWeights, *smaaBlend;

	GLMultiRenderTexture *predicateBuffer;
	GLRenderTexture *predicateBuffer2;
	GLRenderTexture *predicateBufferTemp[2];

	GLRenderTexture *smaaEdgeBuffer, *smaaWeightBuffer;
	GLRenderTexture *smaaAreaTex, *smaaSearchTex;

	GLfloat lowerPredicate, upperPredicate;
	int curTBuffer;

	GLPresenter& presenter;
	
	void fxaa(GLRenderTexture *source, GLRenderTexture *target);
	void smaa(GLRenderTexture *source, GLRenderTexture *target);
	void pxaa(GLRenderTexture *source, GLRenderTexture *target);
	void tpxaa(GLRenderTexture *source, GLRenderTexture *target);

public:
	AAManager(GLPresenter& presenter);
	~AAManager();

	void applyAA(GLRenderTexture *source, GLRenderTexture *target);

	aaMode getAAMode();
	void incAAMode();
	void decAAMode();
	void setAAMode(aaMode mode);

	const char *getCurModeName() { return modeNames[currentMode]; }

	void adjustPredicate(GLfloat upper, GLfloat lower);

private:
	static const char *modeNames[NUM_MODES];
};
