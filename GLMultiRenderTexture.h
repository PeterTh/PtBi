#pragma once

#include <IL/il.h>

#include "GLRenderTexture.h"

#define MULTIRT_MAX 8

class GLMultiRenderTexture
{
	GLuint fboID, texIDs[MULTIRT_MAX];
	unsigned width, height, num;
	GLint viewPort[4];
	bool current;

	GLint internalformat;
	GLenum glformat, gltype;

public:
	GLMultiRenderTexture(unsigned width, unsigned height, unsigned num, GLint internalformat, GLenum glformat, GLenum gltype) 
		: width(width), height(height), num(num), current(0), internalformat(internalformat), glformat(glformat), gltype(gltype) {

		RT_ASSERT(MULTIRT_MAX > num, "Invalid number of MTs");
		glGetIntegerv(GL_VIEWPORT, viewPort);

		glGenFramebuffersEXT(1, &fboID);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);

		for(unsigned i=0; i<num; ++i) {
			glGenTextures(1, &texIDs[i]);
			bindTexture(i);
			setFilterMode(i, GL_LINEAR);
			setWrappingMode(i, GL_CLAMP_TO_EDGE);

			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, glformat, gltype, NULL);			
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, GL_TEXTURE_2D, texIDs[i], 0);
		}
		releaseCurrent();
	}
	~GLMultiRenderTexture(void) {
		if(fboID) glDeleteFramebuffersEXT(1, &fboID);
		for(unsigned i=0; i<num; ++i) {
			if(texIDs[i] > 0) glDeleteTextures(1, &texIDs[i]);
		}
	}

	void setFilterMode(unsigned t, GLint filter) {
		bindTexture(t);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	}
	void setWrappingMode(unsigned t, GLint behaviour) {
		bindTexture(t);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, behaviour);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, behaviour);
	}

	void clear() {
		makeCurrent();
		glClear(GL_COLOR_BUFFER_BIT);
	}

	inline void makeCurrent() {
		const static GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT, 
										  GL_COLOR_ATTACHMENT4_EXT, GL_COLOR_ATTACHMENT5_EXT, GL_COLOR_ATTACHMENT6_EXT, GL_COLOR_ATTACHMENT7_EXT };
		if(!current) {
			glGetIntegerv(GL_VIEWPORT, viewPort);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);
			glDrawBuffers(num, buffers);
			RT_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "FB incomplete");
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
	
	inline void bindTexture(unsigned t, unsigned to = 0) {
		glActiveTextureARB(GL_TEXTURE0_ARB + to);
		glBindTexture(GL_TEXTURE_2D, texIDs[t]);
		if(to>0) glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	void saveToImage(const char* filename) {
		makeCurrent();
		for(unsigned i=0; i<num; ++i) {
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT + i); 
			unsigned char *pixels = new unsigned char[width*height*3];
			glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			glFinish();
			ImageHandler::saveInThread(pixels, width, height, format("ATC%u_%s", i, filename));
		}
		releaseCurrent();
	}
	void saveToImage(string filename) { saveToImage(filename.c_str()); }

	void loadFromImage(unsigned t, const char* filename) {
		bindTexture(t);
		ImageHandler::load(filename);
	}
};
