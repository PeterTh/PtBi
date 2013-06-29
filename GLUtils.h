#pragma once

#include "stdafx.h"

namespace glutil {
	inline void drawQuad() {
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
		glEnd();
	}
}
