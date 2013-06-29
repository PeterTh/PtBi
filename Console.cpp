#include "Console.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

Console* Console::self = NULL;

namespace font {
	const unsigned BMPSIZE = 512;
	unsigned char ttf_buffer[1<<20];
	unsigned char temp_bitmap[BMPSIZE*BMPSIZE];
	bool inited;

	stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
	GLuint ftex;

	void init(void) {
		FILE* ff = fopen("font.ttf", "rb");
		fread(ttf_buffer, 1, 1<<20, ff);
		fclose(ff);
		stbtt_BakeFontBitmap(ttf_buffer,0, 44.0, temp_bitmap,BMPSIZE,BMPSIZE, 32,96, cdata); // no guarantee this fits!
		// can free ttf_buffer at this point
		glGenTextures(1, &ftex);
		glBindTexture(GL_TEXTURE_2D, ftex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, BMPSIZE,BMPSIZE, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
		// can free temp_bitmap at this point
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		inited = true;
	}

	void print(float x, float y, const char *text) {
		if(!inited) init();
		glEnable(GL_BLEND);
		// assume orthographic projection with units = screen pixels, origin at top left
		float wF = 1.0f/Console::get().getW(), hF = 1.0f/Console::get().getH();
		glBindTexture(GL_TEXTURE_2D, ftex);
		glBegin(GL_QUADS);
		while (*text) {
		  if (*text >= 32 && *text < 128) {
			 stbtt_aligned_quad q;
			 stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl,0=old d3d
			 glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0*wF-1.0f,-q.y1*hF+1.0f);
			 glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1*wF-1.0f,-q.y1*hF+1.0f);
			 glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1*wF-1.0f,-q.y0*hF+1.0f);
			 glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0*wF-1.0f,-q.y0*hF+1.0f);
		  }
		  ++text;
		}
		glEnd();
		glDisable(GL_BLEND);
	}
}

float ConsoleLine::draw(float y) {
	if(ypos<0.0f) ypos = y;
	else ypos += (y-ypos)*0.2f; 
	//cout << "Printed: " << msg << " at " << (10.0f + ypos) << "\n";
	font::print(25.0f, (46.0f + ypos), msg.c_str());
	return t.elapsed() > 10000000.0 ? 0.0f : 45.0f + ypos;
}