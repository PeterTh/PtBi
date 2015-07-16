#include "PtOpenGL.h"

#include "GLUtils.h"


GLPresenter* GLPresenter::self = NULL;

GLPresenter::GLPresenter(DeckLinkCapture &capture, int w, int h, int hz) : 
	data_size(w*h*capture.getBytesPerPixel()),
	capture(capture), running(true), fullscreen(false), useVsync(false), rgbFull(false),
	texId(0), displayList(0), 
	initialConvert(NULL),
	buffer(NULL), buffer2(NULL),
	reqW(w), reqH(h), reqHz(hz), captureBufferW(reqW / (4 / capture.getBytesPerPixel())),
	frameIndex(0), drawnFrameIndex(0), aspect(16.0/9.0), oneToNScaleFactor(-1.0),
	frameProcTimes(300)
{
	self = this;

	sprintf(prepend, "#version 410 compatibility \n#define FRAME_WIDTH %d \n#define FRAME_HEIGHT %d \n", reqW, reqH);

	RT_ASSERT(glfwInit() == GL_TRUE, "Failed to initalize GLFW.");
	RT_ASSERT(glfwOpenWindow(w, h, 0, 0, 0, 0, 0, 0, GLFW_WINDOW) == GL_TRUE, "Failed to open GLFW window.");

	string title("PtBi ");
	title += VER_STRING;
	glfwSetWindowTitle(title.c_str());
	glfwSetWindowPos(10, 10);

	dataPointers[0] = malloc(data_size);
	dataPointers[1] = malloc(data_size);

	glewInit();
	checkExtensions();

	ilInit();

	glfwDisable(GLFW_AUTO_POLL_EVENTS);
	glfwSwapInterval(0);
	glfwSetWindowCloseCallback(closeCallback);
	glfwSetWindowSizeCallback(resizeCallback);
	glfwSetMousePosCallback(mousePosCallback);
	glfwSetKeyCallback(keyCallback);

	hdc = wglGetCurrentDC();
	hrc = wglGetCurrentContext();

	initGL();

	switch(capture.getPixelFormat()) {
	case CapturePixelFormat::YUV: 
		initialConvert = new GLFragmentProgram("shaders/uyvy_to_rgb_smooth.glsl", getShaderPrependString());
		break;
	case CapturePixelFormat::ARGB8:
		initialConvert = new GLFragmentProgram("shaders/argb_input.glsl", getShaderPrependString());
		break;
	case CapturePixelFormat::BGRA8:
		initialConvert = new GLFragmentProgram("shaders/bgra_input.glsl", getShaderPrependString());
		break;
	default:
		RT_ASSERT(false, "Unsupported pixel type.");
	}

	buffer = new GLRenderTexture(getW(), getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	buffer->setFilterMode(GL_LINEAR);
	buffer2 = new GLRenderTexture(getW(), getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	buffer2->setFilterMode(GL_LINEAR);

	scalingManager = new ScalingManager(*this);
	aaManager = new AAManager(*this);
	ipManager = new IPManager(*this);
	keyBinding = new KeyBinding(this);
	capture.registerDisplayListener(this);

	hwnd = GetForegroundWindow();

	RT_GL_ASSERT("Error during GL initialization.");

	reshape(reqW, reqH);
	
	graphicsReportText = std::make_shared<StaticText>("", 25.0f,440.0f);
	Console::get().add(graphicsReportText);
}

const GLcharARB* GLPresenter::getShaderPrependString()
{
	return prepend;
}

bool GLPresenter::run()
{
	if(!running) {
		return false;
	}
	if(frameIndex > drawnFrameIndex) {
		Timer t;
		glBindTexture(GL_TEXTURE_2D, texId);
		drawnFrameIndex = frameIndex;
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, captureBufferW, reqH, GL_ABGR_EXT, GL_UNSIGNED_BYTE, dataPointers[drawnFrameIndex % 2]);
		NONRELEASE( RT_GL_ASSERT("Error uploading frame data.") );

		// convert UYVY
		buffer->makeCurrent();
		initialConvert->use();
		glutil::drawQuad();
		initialConvert->stopUsing();
		buffer->releaseCurrent();
		// apply AA
		aaManager->applyAA(buffer, buffer2);
		// apply IP
		ipManager->applyIP(buffer2, buffer);	
		display();

		// statistics
		frameProcTimes.add(t.elapsed()/1000.0);
		graphicsReportText->text = format("Frame processing time (CPU): \n% 6.2lf ms avg, % 6.2lf ms max", frameProcTimes.get(), frameProcTimes.getMax());
	}
	glfwPollEvents();

	if(GetFocus() == hwnd) {
		if(clock()>cursorTimeout) {
			glfwDisable(GLFW_MOUSE_CURSOR);
		} else {
			glfwEnable(GLFW_MOUSE_CURSOR);
		}
	}
	return true;
}

void GLPresenter::frameRecieved(void* data)
{
	if(!running) return;
	unsigned ni = (frameIndex+1)%2;

	memcpy(dataPointers[ni], data, data_size);
	frameIndex++;
}

void GLPresenter::switchFullscreen()
{
	if(fullscreen) {
		SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		MoveWindow(hwnd, windowRect.left, windowRect.top, fsww, fswh, TRUE);
		glfwSetWindowSize(fsww, fswh);
		reshape(fsww, fswh);
	} else {
		glfwGetWindowSize(&fsww, &fswh);
		GetWindowRect(hwnd, &windowRect);
		SetWindowLongPtr(hwnd, GWL_STYLE, WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE);
		int w = GetSystemMetrics(SM_CXSCREEN), h = GetSystemMetrics(SM_CYSCREEN);
		MoveWindow(hwnd, 0, 0, w, h, TRUE);
		reshape(w, h);
	}
	fullscreen = !fullscreen;
	Console::get().add(fullscreen ? "Fullscreen Mode" : "Windowed Mode");
}

void GLPresenter::toggleRGBRange()
{
	rgbFull = !rgbFull;
	if(rgbFull) {
		if(capture.isYUV()) {
			initialConvert->setParameter("lowB", 5.0f / 255.0f);
			initialConvert->setParameter("scale1", 255.0f / (250.0f - 5.0f));
			initialConvert->setParameter("scale2", 255.0f / (255.0f - 5.0f));
		}
		else {
			initialConvert->setParameter("fullRange", 1);
		}
	}
	else {
		if(capture.isYUV()) {
			initialConvert->setParameter("lowB", 16.0f / 255.0f);
			initialConvert->setParameter("scale1", 255.0f / (235.0f - 16.0f));
			initialConvert->setParameter("scale2", 255.0f / (240.0f - 16.0f));
		}
		else {
			initialConvert->setParameter("fullRange", 0);
		}
	}
	Console::get().add(rgbFull ? "Full range RGB mode" : "Limited range RGB mode");
}

void GLPresenter::initGL()
{
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_TEXTURE_2D);

	glViewport(0, 0, getW(), getH());

	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	GLubyte* data = (GLubyte*)new GLubyte[data_size];
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, captureBufferW, reqH, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)data);
	delete data;
}

void GLPresenter::display()
{
	if(running) {
		//glClear(GL_COLOR_BUFFER_BIT);
		scalingManager->drawScaled(buffer);
		buffer->bindTexture(0);
		Console::get().draw();
		glfwSwapBuffers();
	}
}

void GLPresenter::reshape(int w, int h)
{
	wSizeW = w; wSizeH = h;
	Console::get().setSize(w, h);
	NONRELEASE( RT_GL_ASSERT("Error before resizing viewport.") );
	if(oneToNScaleFactor > 0.0) {
		int x = 0, y = 0, targetW = (int)(reqW*oneToNScaleFactor), targetH = (int)(reqH*oneToNScaleFactor);
		Console::get().setSize(targetW, targetH);
		int dw = targetW-w;
		x = -dw/2; w += dw;
		int dh = targetH-h;
		y = -dh/2; h += dh;
		glViewport(x, y, w, h);
	}
	else {
		const static double epsilon = 0.00001;
		if(fabs((w/(double)h) - aspect) < epsilon) {
			glViewport(0, 0, w, h);
		} else if(h*aspect>w) { // need letterboxing
			GLsizei newh = (GLsizei)(w/aspect);
			GLint box = (h-newh)/2;
			glViewport(0, box, w, newh);
			h = newh;
		} else { // need pillarboxing
			GLsizei neww = (GLsizei)(h*aspect);
			GLint box = (w-neww)/2;
			glViewport(box, 0, neww, h);
			w = neww;
		}
	}
	NONRELEASE( RT_GL_ASSERT("Error resizing viewport.") );
}

void GLPresenter::close()
{
	running = false;
	wcout << "Starting capture threads shutdown...\n";
	capture.removeDisplayListener();
	capture.stop();
	wcout << "Capture shutdown finished, OpenGL shutdown...\n";
	if(scalingManager) delete scalingManager;
	if(buffer) delete buffer;
	if(buffer2) delete buffer2;
	if(initialConvert) delete initialConvert;
	if(texId) glDeleteTextures(1, &texId);
	wcout << "OpenGL shutdown finished...\n";
}

void GLPresenter::testScaling()
{
	const char* ts = timeString();
	GLRenderTexture target(1920, 1080, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);

	scalingManager->setScalingMode(ScalingManager::BILINEAR);
	scalingManager->drawScaled(buffer, &target);
	target.saveToImage(format("%s__bilinear.png", ts));
	scalingManager->setScalingMode(ScalingManager::FIXED);
	scalingManager->drawScaled(buffer, &target);
	target.saveToImage(format("%s__fixed_lanc.png", ts));
	scalingManager->setScalingMode(ScalingManager::ADAPTIVE);
	scalingManager->drawScaled(buffer, &target);
	target.saveToImage(format("%s__adaptive_lanc.png", ts));
	glFinish();
}

void GLPresenter::testAAinternal()
{
	const char* ts = timeString();
	GLRenderTexture target(getW(), getH(), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);

	AAManager::aaMode prevMode = aaManager->getAAMode();

	aaManager->setAAMode(AAManager::NONE);
	aaManager->applyAA(buffer, &target);
	target.saveToImage(format("%s__noaa.png", ts));
	aaManager->setAAMode(AAManager::FXAA);
	aaManager->applyAA(buffer, &target);
	target.saveToImage(format("%s__fxaa.png", ts));
	aaManager->setAAMode(AAManager::PXAA);
	aaManager->applyAA(buffer, &target);
	target.saveToImage(format("%s__pxaa.png", ts));
	aaManager->setAAMode(AAManager::TPXAA);
	aaManager->applyAA(buffer, &target);
	target.saveToImage(format("%s__tpxaa.png", ts));
	aaManager->setAAMode(AAManager::SMAA);
	aaManager->applyAA(buffer, &target);
	target.saveToImage(format("%s__smaa.png", ts));


	aaManager->setAAMode(prevMode);
}

unsigned GLPresenter::getW()
{
	return reqW;
}

unsigned GLPresenter::getH()
{
	return reqH;
}
