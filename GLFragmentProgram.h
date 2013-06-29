#pragma once

#include "GLRenderTexture.h"
#include "stuff.h"


class GLFragmentProgram {
	GLhandleARB fragmentProgram, programObject;
	GLuint *texLocations;
	const char *shaderFN;

public:
	GLFragmentProgram(const char* shaderFileName, const GLcharARB *prepend = "") 
			: shaderFN(shaderFileName) {
		programObject = glCreateProgramObjectARB();

		GLcharARB* shaderText = (GLcharARB*)new GLcharARB[fileSize(shaderFileName)+1];
		readFile(shaderFileName, shaderText);

		fragmentProgram = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		RT_GL_ASSERT("Error creating shader object for " << shaderFN);
		const GLcharARB *sources[2] = {prepend, shaderText};
		glShaderSourceARB(fragmentProgram, 2, sources, NULL);
		RT_GL_ASSERT("Error setting shader source for " << shaderFN);
		delete shaderText;
		
		GLint result;
		glCompileShaderARB(fragmentProgram);
		glGetObjectParameterivARB(fragmentProgram, GL_OBJECT_COMPILE_STATUS_ARB, &result);
		if(!result) {
			char buff[4096];
			glGetInfoLogARB(fragmentProgram, sizeof(buff), NULL, buff);
			RT_ASSERT(false, "Shader compilation error - shader file: " << shaderFN << endl << buff);
		}

		glAttachObjectARB(programObject, fragmentProgram);
		RT_GL_ASSERT("Error attaching fragment shader.");

		glLinkProgramARB(programObject);
		glGetObjectParameterivARB(programObject, GL_OBJECT_LINK_STATUS_ARB, &result);
		if(!result) {
			char buff[4096]; 
			glGetInfoLogARB(programObject, sizeof(buff), NULL, buff);
			RT_ASSERT(false, "Program object linking error: - shader file: " << shaderFN << endl << buff);
		}
	}
	~GLFragmentProgram(void) {
		if(programObject) glDeleteObjectARB(programObject);
		if(fragmentProgram) glDeleteObjectARB(fragmentProgram);
	}

	inline void use() {
		glUseProgramObjectARB(programObject);
	}
	inline static void stopUsing() {
		glUseProgramObjectARB(NULL);
	}

	void setParameter(const GLcharARB* name, GLfloat param) {
		use();
		int ul = glGetUniformLocationARB(programObject, name);
		if(ul >= 0) glUniform1fARB(ul, param);
		else WARN("Pixelshader " << shaderFN << ": Could not find float Uniform \"" << name << "\", parameter NOT set.");
	}

	void setParameter(const GLcharARB* name, GLint param) {
		use();
		int ul = glGetUniformLocationARB(programObject, name);
		if(ul >= 0) glUniform1iARB(ul, param);
		else WARN("Pixelshader " << shaderFN << ": Could not find int Uniform \"" << name << "\", parameter NOT set.");
	}
};
