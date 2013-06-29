#include "AAManager.h"

#include "PtOpenGL.h"
#include "GLUtils.h"

#define AREATEX_WIDTH 160
#define AREATEX_HEIGHT 560
#define SEARCHTEX_WIDTH 66
#define SEARCHTEX_HEIGHT 33

const char *AAManager::modeNames[AAManager::NUM_MODES] = { "None", "FXAA", "PXAA", "TPXAA", "SMAA" };

AAManager::AAManager(GLPresenter& presenter) : currentMode(NONE),
	applyFXAA(NULL), initAAPredicate(NULL), fillAAPredicateH(NULL), 
	fillAAPredicateV(NULL), fillAAPredicateExt(NULL), applyAAPredicate(NULL),
	smaaEdgeDetect(NULL), smaaCalcWeights(NULL), smaaBlend(NULL),
	predicateBuffer(NULL), predicateBuffer2(NULL), 
	smaaEdgeBuffer(NULL), smaaWeightBuffer(NULL),
	smaaAreaTex(NULL), smaaSearchTex(NULL),
	lowerPredicate(0.07f), upperPredicate(0.15f),
	curTBuffer(0),
	presenter(presenter)
{
	applyFXAA = new GLFragmentProgram("shaders/fxaa_pass.glsl", presenter.getShaderPrependString());

	// load P*AA shaders
	initAAPredicate = new GLFragmentProgram("shaders/init_aa_predicate.glsl", presenter.getShaderPrependString());
	fillAAPredicateH = new GLFragmentProgram("shaders/fill_aa_predicate_h.glsl", presenter.getShaderPrependString());
	fillAAPredicateV = new GLFragmentProgram("shaders/fill_aa_predicate_v.glsl", presenter.getShaderPrependString());
	fillAAPredicateExt = new GLFragmentProgram("shaders/fill_aa_predicate_ext.glsl", presenter.getShaderPrependString());
	fillAAPredicateExt->setParameter("sourceH", 0);
	fillAAPredicateExt->setParameter("sourceV", 1);
	applyAAPredicate = new GLFragmentProgram("shaders/apply_aa_predicate.glsl", presenter.getShaderPrependString());
	applyAAPredicate->setParameter("predicate", 0);
	applyAAPredicate->setParameter("image", 1);
	extendAAPredicate = new GLFragmentProgram("shaders/extend_aa_predicate.glsl", presenter.getShaderPrependString());
	extendAAPredicate->setParameter("source", 0);
	extendAAPredicate->setParameter("prev", 1);
	applyAAPredicateTemp = new GLFragmentProgram("shaders/apply_aa_predicate_temp.glsl", presenter.getShaderPrependString());
	applyAAPredicateTemp->setParameter("predicate0", 0);
	applyAAPredicateTemp->setParameter("image", 1);

	// load SMAA shaders
	smaaEdgeDetect = new GLFragmentProgram("shaders/smaa_edge_pass.glsl", presenter.getShaderPrependString());
	smaaCalcWeights = new GLFragmentProgram("shaders/smaa_weight_pass.glsl", presenter.getShaderPrependString());
	smaaCalcWeights->setParameter("edgesTex", 0);
	smaaCalcWeights->setParameter("areaTex", 1);
	smaaCalcWeights->setParameter("searchTex", 2);
	smaaBlend = new GLFragmentProgram("shaders/smaa_blend_pass.glsl", presenter.getShaderPrependString());
	smaaBlend->setParameter("colorTex", 0);
	smaaBlend->setParameter("blendTex", 1);

	// generate P*AA buffers
	predicateBuffer = new GLMultiRenderTexture(presenter.getW(), presenter.getH(), 2, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	predicateBuffer2 = new GLRenderTexture(presenter.getW(), presenter.getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	predicateBufferTemp[0] = new GLRenderTexture(presenter.getW(), presenter.getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	predicateBufferTemp[1] = new GLRenderTexture(presenter.getW(), presenter.getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	
	// generate SMAA buffers
	smaaEdgeBuffer = new GLRenderTexture(presenter.getW(), presenter.getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	smaaWeightBuffer = new GLRenderTexture(presenter.getW(), presenter.getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	smaaEdgeBuffer->setFilterMode(GL_LINEAR);
	smaaWeightBuffer->setFilterMode(GL_LINEAR);

	// Load SMAA textures
	unsigned char* buffer = new unsigned char[1024 * 1024];
	FILE* f = NULL;
	f = fopen("textures/smaa_area.raw", "rb");
	RT_ASSERT(f != NULL, "textures/smaa_area.raw not found.");
	fread(buffer, AREATEX_WIDTH * AREATEX_HEIGHT * 2, 1, f);
	fclose(f);
	smaaAreaTex = new GLRenderTexture(AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG8, GL_RG, GL_UNSIGNED_BYTE);
	smaaAreaTex->setWrappingMode(GL_CLAMP_TO_EDGE);
	smaaAreaTex->setFilterMode(GL_LINEAR);
	smaaAreaTex->bindTexture();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, (GLsizei)AREATEX_WIDTH, (GLsizei)AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, buffer);
	f = fopen("textures/smaa_search.raw", "rb");
	RT_ASSERT(f != NULL, "textures/smaa_search.raw not found.");
	fread(buffer, SEARCHTEX_WIDTH * SEARCHTEX_HEIGHT, 1, f);
	fclose(f);
	smaaSearchTex = new GLRenderTexture(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
	smaaSearchTex->setWrappingMode(GL_CLAMP_TO_EDGE);
	smaaSearchTex->setFilterMode(GL_NEAREST);
	smaaSearchTex->bindTexture();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, (GLsizei)SEARCHTEX_WIDTH, (GLsizei)SEARCHTEX_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, buffer );
	delete [] buffer;

	RT_GL_ASSERT("Error during AAManager initialization.");
}

AAManager::~AAManager()
{
	if(applyFXAA) delete applyFXAA;

	if(initAAPredicate) delete initAAPredicate;
	if(fillAAPredicateH) delete fillAAPredicateH;
	if(fillAAPredicateV) delete fillAAPredicateV;
	if(fillAAPredicateExt) delete fillAAPredicateExt;
	if(applyAAPredicate) delete applyAAPredicate;
	
	if(smaaEdgeDetect) delete smaaEdgeDetect;
	if(smaaCalcWeights) delete smaaCalcWeights;
	if(smaaBlend) delete smaaBlend;
	
	if(predicateBuffer) delete predicateBuffer;
	if(predicateBuffer2) delete predicateBuffer2;
	if(predicateBufferTemp[0]) delete predicateBufferTemp[0];
	if(predicateBufferTemp[1]) delete predicateBufferTemp[1];
	
	if(smaaAreaTex) delete smaaAreaTex;
	if(smaaSearchTex) delete smaaSearchTex;
}

void AAManager::applyAA(GLRenderTexture *source, GLRenderTexture *target)
{
	NONRELEASE(
		glFinish();
		static SlidingAverage avg(60);
		Timer time;
	);
	switch(currentMode) {
	case NONE:
		target->makeCurrent();
		source->bindTexture();
		glutil::drawQuad();
		target->releaseCurrent();
		break;
	case FXAA:
		fxaa(source, target);
		break;
	case PXAA:
		pxaa(source, target);
		break;
	case TPXAA:
		tpxaa(source, target);
		break;
	case SMAA:
		smaa(source, target);
		break;
	}
	NONRELEASE(
		glFinish();
		avg.add(time.elapsed());
		if(avg.justFilled()) printf("(1) AA step average time: %6.2lf microseconds\n", avg.get());
	);
}


void AAManager::fxaa(GLRenderTexture *source, GLRenderTexture *target) {
	target->makeCurrent();
	source->bindTexture();
	applyFXAA->use();
	glutil::drawQuad();
	applyFXAA->stopUsing();
	target->releaseCurrent();
}

void AAManager::pxaa(GLRenderTexture *source, GLRenderTexture *target) {
	// init predicate
	predicateBuffer->makeCurrent();
	source->bindTexture();
	source->setFilterMode(GL_NEAREST);
	initAAPredicate->use();
	glutil::drawQuad();
	initAAPredicate->stopUsing();
	predicateBuffer->releaseCurrent();
	source->setFilterMode(GL_LINEAR);
	// fill predicate
	//predicateBuffer2->makeCurrent();
	//// fill H
	//predicateBuffer->bindTexture(0);
	//fillAAPredicateH->use();
	//glutil::drawQuad();
	//fillAAPredicateH->stopUsing();
	//// fill V
	//predicateBuffer->bindTexture(1);
	//fillAAPredicateV->use();
	//glutil::drawQuad();
	//fillAAPredicateV->stopUsing();
	//predicateBuffer2->releaseCurrent();
	fillAAPredicateExt->use();
	predicateBuffer->bindTexture(0,0);
	predicateBuffer->bindTexture(1,1);
	glBindImageTexture(/*unit*/ 0, /*texture*/ predicateBuffer2->getTexID(), /*level*/ 0, /*layered*/ false, /*layer*/ 0, 
		/*access*/ GL_WRITE_ONLY, /*format*/ GL_RGBA8);
	RT_GL_ASSERT("BindImageTexture bugged out.");
	glutil::drawQuad();
	fillAAPredicateExt->stopUsing();
	//predicateBuffer->saveToImage("init_pred.png");
	//predicateBuffer2->saveToImage("filled_pred.png");
	//buffer2->saveToImage("buffer2.png");
	// apply AA predicate
	applyAAPredicate->use();
	predicateBuffer2->bindTexture(0);
	source->bindTexture(1);
	target->makeCurrent();
	glutil::drawQuad();
	target->releaseCurrent();
	applyAAPredicate->stopUsing();
}

void AAManager::tpxaa(GLRenderTexture *source, GLRenderTexture *target) {
	// init predicate
	predicateBuffer->makeCurrent();
	source->bindTexture();
	source->setFilterMode(GL_NEAREST);
	initAAPredicate->use();
	glutil::drawQuad();
	initAAPredicate->stopUsing();
	predicateBuffer->releaseCurrent();
	source->setFilterMode(GL_LINEAR);
	// fill predicate
	fillAAPredicateExt->use();
	predicateBuffer->bindTexture(0,0);
	predicateBuffer->bindTexture(1,1);
	glBindImageTexture(/*unit*/ 0, /*texture*/ predicateBuffer2->getTexID(), /*level*/ 0, /*layered*/ false, /*layer*/ 0, 
		/*access*/ GL_WRITE_ONLY, /*format*/ GL_RGBA8);
	RT_GL_ASSERT("BindImageTexture bugged out.");
	glutil::drawQuad();
	fillAAPredicateExt->stopUsing();
	// extend predicate to T buffer
	predicateBufferTemp[curTBuffer]->makeCurrent();
	predicateBuffer2->bindTexture(0);
	predicateBufferTemp[1-curTBuffer]->bindTexture(1);
	extendAAPredicate->use();
	glutil::drawQuad();
	extendAAPredicate->stopUsing();
	predicateBufferTemp[curTBuffer]->releaseCurrent();
	// apply predicate buffer array
	applyAAPredicateTemp->use();
	predicateBufferTemp[curTBuffer]->bindTexture(0);
	source->bindTexture(1);
	target->makeCurrent();
	glutil::drawQuad();
	target->releaseCurrent();
	applyAAPredicateTemp->stopUsing();
	curTBuffer = 1-curTBuffer;
}

void AAManager::smaa(GLRenderTexture *source, GLRenderTexture *target) {
	source->setFilterMode(GL_LINEAR);
	glDisable(GL_BLEND);
	// init edge texture
	smaaEdgeBuffer->clear();
	smaaEdgeBuffer->makeCurrent();
	source->bindTexture();
	smaaEdgeDetect->use();
	glutil::drawQuad();
	smaaEdgeDetect->stopUsing();
	smaaEdgeBuffer->releaseCurrent();
	// calculate weights
	smaaWeightBuffer->clear();
	smaaWeightBuffer->makeCurrent();
	smaaEdgeBuffer->bindTexture(0);
	smaaAreaTex->bindTexture(1);
	smaaSearchTex->bindTexture(2);
	smaaCalcWeights->use();
	glutil::drawQuad();
	smaaCalcWeights->stopUsing();
	smaaWeightBuffer->releaseCurrent();
	// blend image
	target->makeCurrent();
	source->bindTexture(0);
	smaaWeightBuffer->bindTexture(1);
	smaaBlend->use();
	glEnable(GL_FRAMEBUFFER_SRGB);
	glutil::drawQuad();
	glDisable(GL_FRAMEBUFFER_SRGB);
	smaaBlend->stopUsing();
	target->releaseCurrent();
}

AAManager::aaMode AAManager::getAAMode()
{
	return currentMode;
}

void AAManager::incAAMode()
{
	currentMode = (aaMode)(currentMode+1); if(currentMode==NUM_MODES) currentMode = (aaMode)0;
}

void AAManager::decAAMode()
{
	if(currentMode==0) currentMode = NUM_MODES; currentMode = (aaMode)(currentMode-1);
}

void AAManager::setAAMode(aaMode mode)
{
	currentMode = mode;
}

void AAManager::adjustPredicate(GLfloat upper, GLfloat lower)
{
	lowerPredicate += lower;
	upperPredicate += upper;
	initAAPredicate->setParameter("fac", lowerPredicate);
	initAAPredicate->setParameter("facN", upperPredicate);
	Console::get().add(format("Adjusted predicate range to: %3.2f - %3.2f", lowerPredicate, upperPredicate));
}
