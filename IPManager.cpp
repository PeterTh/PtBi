#include "IPManager.h"

#include "PtOpenGL.h"
#include "GLUtils.h"


IPManager::IPManager(GLPresenter& presenter) : activeFilters(NONE),
	localContrastGather(NULL), localContrastApply(NULL), 
	localContrastBuffers(NULL),
	contrastIncrease(0.15f), gamma(1.05f),
	localContrastRange(5),
	presenter(presenter)
{
	localContrastGather = new GLFragmentProgram("shaders/local_contrast_gather.glsl", presenter.getShaderPrependString());
	localContrastApply = new GLFragmentProgram("shaders/local_contrast_apply.glsl", presenter.getShaderPrependString());
	localContrastApply->setParameter("original", 0);
	localContrastApply->setParameter("gathered", 1);
	localContrastApply->setParameter("contrastIncrease", contrastIncrease);

	gammaCorrection = new GLFragmentProgram("shaders/gamma.glsl", presenter.getShaderPrependString());
	gammaCorrection->setParameter("gamma", gamma);

	localContrastBuffers = new GLRenderTexture*[localContrastRange];
	for(unsigned i=0; i<localContrastRange; ++i) {
		localContrastBuffers[i] = new GLRenderTexture(presenter.getW()/(2<<i), presenter.getH()/(2<<i), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	}

	RT_GL_ASSERT("Error during IPManager initialization.");
}

IPManager::~IPManager()
{
	if(localContrastGather) delete localContrastGather;
	if(localContrastApply) delete localContrastApply;
	if(gammaCorrection) delete gammaCorrection;
	for(unsigned i=0; i<localContrastRange; ++i) {
		delete localContrastBuffers[i];
	}
	delete [] localContrastBuffers;
}

void IPManager::applyIP(GLRenderTexture *source, GLRenderTexture *target)
{
	NONRELEASE(
		glFinish();
		static SlidingAverage avg(60);
		Timer time;
	);

	if(activeFilters == NONE) {
		target->makeCurrent();
		source->bindTexture();
		glutil::drawQuad();
		target->releaseCurrent();
	} 
	else {
		GLRenderTexture *c_src = source, *c_trg = target, *tmp;

		if(activeFilters & LOCAL_CONTRAST) {
			localContrastGather->use();
			for(unsigned i=0; i<localContrastRange; ++i) {
				localContrastBuffers[i]->makeCurrent();
				if(i==0) c_src->bindTexture();
				else localContrastBuffers[i-1]->bindTexture();
				glutil::drawQuad();
				localContrastBuffers[i]->releaseCurrent();
			}
			localContrastGather->stopUsing();

			c_trg->makeCurrent();
			localContrastApply->use();
			c_src->bindTexture(0);
			localContrastBuffers[localContrastRange-1]->bindTexture(1);
			glutil::drawQuad();
			localContrastApply->stopUsing();
			c_trg->releaseCurrent();
			tmp = c_src; c_src = c_trg; c_trg = c_src; 
		}
	
		if(activeFilters & GAMMA_CORRECTION) {
			c_trg->makeCurrent();
			gammaCorrection->use();
			c_src->bindTexture();
			glutil::drawQuad();
			gammaCorrection->stopUsing();
			c_trg->releaseCurrent();
			tmp = c_src; c_src = c_trg; c_trg = c_src; 
		}
	
		// if last target isn't real target, copy
		if(c_trg != target) {
			target->makeCurrent();
			c_trg->bindTexture();
			glutil::drawQuad();
			target->releaseCurrent();
		}
	}

	NONRELEASE(
		glFinish();
		avg.add(time.elapsed());
		if(avg.justFilled()) printf("(2) IP step average time: %6.2lf microseconds\n", avg.get());
	);
}

IPManager::ipFilters IPManager::getActiveFilters()
{
	return activeFilters;
}

void IPManager::toggleFilter( ipFilters filter )
{
	activeFilters = (ipFilters)(activeFilters ^ filter);
	Console::get().add(format("Active image processing filters: %s", getActiveFilterString().c_str()));
}

std::string IPManager::getActiveFilterString()
{
	string ret = "";
	if(activeFilters == NONE) ret = "None, ";
	if(activeFilters & LOCAL_CONTRAST) ret += "Local Contrast Enhancement, ";
	if(activeFilters & GAMMA_CORRECTION) ret += "Gamma Correction, ";
	ret = ret.substr(0, ret.length()-2);
	return ret;
}

void IPManager::adjustContrastIncrease( GLfloat adjustment )
{
	contrastIncrease += adjustment;
	localContrastApply->setParameter("contrastIncrease", contrastIncrease);
	Console::get().add(format("Adjusted local contrast enhancement to: %3.2f", contrastIncrease));
}

void IPManager::adjustGamma( GLfloat adjustment )
{
	gamma += adjustment;
	if(gamma < 0.1f) gamma = 0.1f;
	if(gamma > 10.0f) gamma = 10.0f;
	gammaCorrection->setParameter("gamma", gamma);
	Console::get().add(format("Adjusted gamma correction to: %4.2f", gamma));
}
