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

	position print(float x, float y, const char *text, bool measure) {
		if(!inited) init();
		position ret = std::make_pair(-100.0f,100.0f);
		float xstart = x;
		// assume orthographic projection with units = screen pixels, origin at top left
		float wF = 1.0f/Console::get().getW(), hF = 1.0f/Console::get().getH();
		if(!measure) {
			glBindTexture(GL_TEXTURE_2D, ftex);
			glBegin(GL_QUADS);
		}
		while (*text) {
			if (*text >= 32 && *text < 128) {
				stbtt_aligned_quad q;
				stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1); // 1=opengl, 0=old d3d
				if(!measure) {
					glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0*wF-1.0f,-q.y1*hF+1.0f);
					glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1*wF-1.0f,-q.y1*hF+1.0f);
					glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1*wF-1.0f,-q.y0*hF+1.0f);
					glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0*wF-1.0f,-q.y0*hF+1.0f);
				}
				ret.first = max(ret.first, q.x0*wF-1.0f);
				ret.second = min(ret.second, -q.y0*hF+1.0f);
			}
			if (*text == '\n') {
				y += 38.0f; 
				x = xstart;
			}
			++text;
		}
		if(!measure) {
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		return ret;
	}
}

float ConsoleLine::draw(float y) {
	if(ypos<0.0f) ypos = y;
	else ypos += (y-ypos)*0.2f; 
	//cout << "Printed: " << msg << " at " << (10.0f + ypos) << "\n";
	font::print(25.0f, (46.0f + ypos), msg.c_str());
	return t.elapsed() > 10000000.0 ? 0.0f : 45.0f + ypos;
}

void drawBGQuad(float x0, float y0, float x1, float y1) {
	glBegin(GL_QUADS);
	glColor4d(0.0,0.0,0.0,0.5);
	glVertex2d(x0, y0);
	glVertex2d(x1, y0);
	glVertex2d(x1, y1);
	glVertex2d(x0, y1);
	glColor4d(1.0,1.0,1.0,1.0);
	glEnd();
};

void Console::draw()
{
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);
	// draw console
	if(lineHeight > 0.0f) {
		drawBGQuad(-1.0f, 1.0f, 1.0f, 1.0f-lineHeight/height);
	}
	float y = 0.0f;
	// maximum: 5 lines
	if(lines.size()-start > 5) start += lines.size()-start-5; 
	for(size_t i=start; i<lines.size(); ++i) {
		float ret = lines[i].draw(y);
		if(ret == 0.0f) start = i+1; // if text timed out increase start
		else y = ret + 2.0f;
	}
	if(y == 0.0f) {
		if(lineHeight>0.2f) lineHeight *= 0.6f;
		else lineHeight = 0.0f;
	}
	else lineHeight = y + 14.0f;

	// draw static text
	if(showStatics) {
		glPushMatrix();
		glScalef(0.8f,0.8f,1.0f);
		glTranslatef(-0.25f,0.0f,0.0f);
		for(auto t: statics) {
			auto p = font::print(t->x, t->y, t->text.c_str(), true);
			drawBGQuad(t->x/width-1.0f-0.01f, -(t->y-32.0f)/height+1.0f+0.01f, p.first+0.01f, p.second-0.02f);
			font::print(t->x, t->y, t->text.c_str());
		}
		glPopMatrix();
	}
	glDisable(GL_BLEND);
}
