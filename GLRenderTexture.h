#pragma once

#include <IL/il.h>
#if _MSC_VER > 1600
#include <future>
#include <mutex>
#endif

#include "stdafx.h"

namespace ImageHandler {
	#if _MSC_VER > 1600
	static mutex ilLib;
	#endif

	static void save(unsigned char *pixels, unsigned width, unsigned height, string fn) {
		#if _MSC_VER > 1600
		ilLib.lock();
		#endif
		ILuint handle;
		ilGenImages(1, &handle);
		ilBindImage(handle);
		ilTexImage(width, height, 1, /*Bpp*/3, IL_RGB, IL_UNSIGNED_BYTE, pixels);
		ilSaveImage(fn.c_str());
		ilDeleteImages(1, &handle);
		#if _MSC_VER > 1600
		ilLib.unlock();
		#endif
		delete pixels;
	}

	static void saveInThread(unsigned char *pixels, unsigned width, unsigned height, string fn) {
		#if _MSC_VER > 1600
		async(bind(save, pixels, width, height, fn));
		#else 
		save(pixels, width, height, fn);
		#endif
	}

	static void load(const char* fn) {
		#if _MSC_VER > 1600
		ilLib.lock();
		#endif
		RT_ASSERT(ilLoadImage(const_cast<ILstring>(fn)) == IL_TRUE, 
			"ImageHandler: could not load image \"" << fn << "\".");
		int width = ilGetInteger(IL_IMAGE_WIDTH);
		int height = ilGetInteger(IL_IMAGE_HEIGHT);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
		const ILuint ci = ilGetInteger(IL_CUR_IMAGE);
		ilDeleteImages(1, &ci);
		#if _MSC_VER > 1600
		ilLib.unlock();
		#endif
	}
};

class GLRenderTexture
{
	GLuint fboID, texID;
	unsigned width, height;
	GLint viewPort[4];
	bool current;

	GLint internalformat;
	GLenum glformat, gltype;

public:
	GLuint getTexID() { return texID; }

	GLRenderTexture(unsigned width, unsigned height, GLint internalformat, GLenum glformat, GLenum gltype) 
		: width(width), height(height), current(0), internalformat(internalformat), glformat(glformat), gltype(gltype) {

		glGenTextures(1, &texID);
		bindTexture();
		setFilterMode(GL_LINEAR);
		setWrappingMode(GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, glformat, gltype, NULL);

		// Generate FBO
		glGenFramebuffersEXT(1, &fboID);
		makeCurrent();
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texID, 0);
		releaseCurrent();
	}
	~GLRenderTexture(void) {
		if(fboID) glDeleteFramebuffersEXT(1, &fboID);
		if(texID > 0) glDeleteTextures(1, &texID);
	}

	void setFilterMode(GLint filter) {
		bindTexture();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	}
	void setWrappingMode(GLint behaviour) {
		bindTexture();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, behaviour);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, behaviour);
	}

	void clear() {
		makeCurrent();
		glClearColor( 0, 0, 0, 0 );
		glClear(GL_COLOR_BUFFER_BIT);
	}

	inline void makeCurrent() {
		if(!current) {
			glGetIntegerv(GL_VIEWPORT, viewPort);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);
			glViewport(0, 0, width, height);
			current = true;
		}
	}
	inline void releaseCurrent() {
		if(current) {
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, NULL);
			glViewport(viewPort[0], viewPort[1], viewPort[2], viewPort[3]);
			current = false;
		}
	}

	inline void bindTexture(GLuint to=0) {
		glActiveTextureARB(GL_TEXTURE0_ARB + to);
		glBindTexture(GL_TEXTURE_2D, texID);
		if(to>0) glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	void saveToImage(const char* filename) {
		makeCurrent();
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT); 
		unsigned char *pixels = new unsigned char[width*height*3];
		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		ImageHandler::saveInThread(pixels, width, height, filename);
		releaseCurrent();
	}
	void saveToImage(string filename) { saveToImage(filename.c_str()); }

	void loadFromImage(const char* filename) {
		bindTexture();
		ImageHandler::load(filename);
	}
};
