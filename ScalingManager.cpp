#include "ScalingManager.h"

#include "PtOpenGL.h"
#include "GLUtils.h"


const char *ScalingManager::modeNames[ScalingManager::NUM_MODES] = { "Bilinear", "Nearest Neighbour", "Fixed Lanczos", "Adaptive Lanczos" };

ScalingManager::ScalingManager(GLPresenter& presenter) : currentMode(BILINEAR), buffer(NULL), lanczosLut(NULL),
	calcSmoothness(NULL), generateLanczosLut(NULL), scaleLanczosStatic(NULL), scaleLanczosAdaptive(NULL),
	presenter(presenter)
{
	scaleLanczosStatic = new GLFragmentProgram("shaders/scale_lanczos_static.glsl", presenter.getShaderPrependString());
	scaleLanczosStatic->setParameter("source", 0);
	scaleLanczosStatic->setParameter("lanczosLut", 1);
	scaleLanczosAdaptive = new GLFragmentProgram("shaders/scale_lanczos_adaptive.glsl", presenter.getShaderPrependString());
	scaleLanczosAdaptive->setParameter("source", 0);
	scaleLanczosAdaptive->setParameter("lanczosLut", 1);
	calcSmoothness = new GLFragmentProgram("shaders/calc_smoothness.glsl", presenter.getShaderPrependString());
	buffer = new GLRenderTexture(presenter.getW(), presenter.getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);

	generateLanczosLut = new GLFragmentProgram("shaders/generate_lanczos_lut.glsl", presenter.getShaderPrependString());
	lanczosLut = new GLRenderTexture(64, 8, GL_LUMINANCE16F_ARB, GL_LUMINANCE, GL_FLOAT);
	RT_GL_ASSERT("Error during ScalingManager initialization.");
	lanczosLut->makeCurrent();
	RT_GL_ASSERT("Error during ScalingManager initialization.");
	generateLanczosLut->use();
	RT_GL_ASSERT("Error during ScalingManager initialization.");
	glutil::drawQuad();
	generateLanczosLut->stopUsing();
	RT_GL_ASSERT("Error during ScalingManager initialization.");
	lanczosLut->releaseCurrent();
	RT_GL_ASSERT("Error during ScalingManager initialization.");
}

ScalingManager::~ScalingManager()
{
	if(scaleLanczosStatic) delete scaleLanczosStatic;
	if(calcSmoothness) delete calcSmoothness;
	if(generateLanczosLut) delete generateLanczosLut;
	if(lanczosLut) delete lanczosLut;
	if(buffer) delete buffer;
}

void ScalingManager::drawScaled(GLRenderTexture *source, GLRenderTexture *target /*= NULL*/)
{
	NONRELEASE(
		glFinish();
		static SlidingAverage avg(60);
		Timer time;
	);
	switch(currentMode) {
	case BILINEAR:
		if(target) target->makeCurrent();
		source->bindTexture();
		glutil::drawQuad();
		if(target) target->releaseCurrent();
		break;
	case NEAREST:
		if(target) target->makeCurrent();
		source->bindTexture();
		source->setFilterMode(GL_NEAREST);
		glutil::drawQuad();
		source->setFilterMode(GL_LINEAR);
		if(target) target->releaseCurrent();
		break;
	case FIXED:
		if(target) target->makeCurrent();
		source->bindTexture(0);
		lanczosLut->bindTexture(1);
		scaleLanczosStatic->use();
		glutil::drawQuad();
		scaleLanczosStatic->stopUsing();
		if(target) target->releaseCurrent();
		break;
	case ADAPTIVE:
		buffer->makeCurrent();
		source->bindTexture();
		calcSmoothness->use();
		glutil::drawQuad();
		calcSmoothness->stopUsing();
		buffer->releaseCurrent();

		if(target) target->makeCurrent();
		buffer->bindTexture(0);
		lanczosLut->bindTexture(1);
		scaleLanczosAdaptive->use();
		glutil::drawQuad();
		scaleLanczosAdaptive->stopUsing();
		if(target) target->releaseCurrent();
		break;
	}
	NONRELEASE(
		glFinish();
		avg.add(time.elapsed());
		if(avg.justFilled()) printf("(3) SC step average time: %6.2lf microseconds\n", avg.get());
	);
}

void ScalingManager::incScalingMode()
{
	currentMode = (interpMode)(currentMode+1); if(currentMode==NUM_MODES) currentMode = (interpMode)0;
}
void ScalingManager::decScalingMode()
{
	if(currentMode==0) currentMode = NUM_MODES; currentMode = (interpMode)(currentMode-1);
}
void ScalingManager::setScalingMode(interpMode mode)
{
	currentMode = mode;
}

const char * ScalingManager::getCurModeName()
{
	return modeNames[currentMode];
}
