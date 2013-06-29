#extension GL_ARB_shader_image_load_store : enable

uniform sampler2D sourceH;
uniform sampler2D sourceV;

uniform writeonly image2D targetBuff;

const float eps = 0.01;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;

const int dLimit = 128;
const int dMin = 3;
const int mLimit = 2;

void hor_writeback(ivec2 pos, int dist) {
	pos.x--;
	for(int i=0; i<dist+1; ++i) {
		pos.x++;
		imageStore(targetBuff, pos, vec4(1,1,1,1));
	}
}

void ver_writeback(ivec2 pos, int dist) {
	pos.y--;
	for(int i=0; i<dist+1; ++i) {
		pos.y++;
		imageStore(targetBuff, pos, vec4(1,1,1,1));
	}
}

void main(void)
{
	vec2 tc = vec2(gl_TexCoord[0]);

	vec4 predH = texture2D(sourceH, tc);
	vec4 predV = texture2D(sourceV, tc);
	
	// check for horizontal edges
	//          ___---
	//    ___x--
	// ---
	if(predH.r > eps) {
		int d = 1;
		int m = 0;
		while(d<dLimit) {
			vec4 rp = texture2D(sourceH, tc + vec2(d*xPitch,0));
			if(rp.b > eps) { // still on edge
				// nothing to do but continue
			} else {
				vec4 rpt = texture2D(sourceH, tc + vec2(d*xPitch,yPitch));
				if(rp.g > eps && d > 3) { // top edge
					hor_writeback(ivec2(tc/vec2(xPitch,yPitch)), d);
					break;
				} else if(rpt.r > eps) { // standard rising edge
					hor_writeback(ivec2(tc/vec2(xPitch,yPitch)), d);
					break;
				} else {
					m += 1;
					if(m>mLimit) {
						//imageStore(targetBuff, ivec2((tc + vec2(d*xPitch,0))/vec2(xPitch,yPitch)), rp);
						break; // regardless of what happened, we are done here
					}
				}
			}
			d += 1;
		}
	}

	// check for horizontal edges
	// ---___
	//       --x___
	//             ---
	else if(predH.g > eps) {
		int d = 1;
		int m = 0;
		vec2 tcb = tc + vec2(0,-yPitch);
		while(d<dLimit) {
			vec4 rp = texture2D(sourceH, tcb + vec2(d*xPitch,0));
			if(rp.b > eps) { // still on edge
				// nothing to do but continue
			} else {
				if(rp.g > eps) { // standard falling edge
					hor_writeback(ivec2(tcb/vec2(xPitch,yPitch)), d);
					break;
				} else {
					m += 1;
					if(m>mLimit) {
						break; // regardless of what happened, we are done here
					}
				}
			}
			d += 1;
		}
	}

	
	// check for vertical edges
	//  |
	//  |
	//   X
	//   |
	//    |
	//    |
	else if(predV.r > eps) {
		int d = 1;
		int m = 0;
		while(d<dLimit) {
			vec4 rp = texture2D(sourceV, tc + vec2(0,-d*yPitch));
			if(rp.b > eps) { // still on edge
				// nothing to do but continue
			} else {
				vec4 rpr = texture2D(sourceV, tc + vec2(xPitch,-d*yPitch));
				if(rp.g > eps && d > 3) { // side edge
					ver_writeback(ivec2(tc/vec2(xPitch,yPitch)), d);
					break;
				} else if(rpr.r > eps) { // standard right-slope edge
					ver_writeback(ivec2(tc/vec2(xPitch,yPitch)), d);
					break;
				} else {
					m += 1;
					if(m>mLimit) {
						break; // regardless of what happened, we are done here
					}
				}
			}
			d += 1;
		}
	}

	// check for vertical edges
	//    |
	//    |
	//   |
	//   X
	//  |
	//  |
	else if(predV.g > eps) {
		int d = 1;
		int m = 0;
		while(d<dLimit) {
			vec4 rp = texture2D(sourceV, tc + vec2(0,d*yPitch));
			if(rp.b > eps) { // still on edge
				// nothing to do but continue
			} else {
				vec4 rpr = texture2D(sourceV, tc + vec2(xPitch,d*yPitch));
				if(rpr.g > eps) { // standard left-slope edge
					ver_writeback(ivec2((tc + vec2(xPitch,d*yPitch))/vec2(xPitch,yPitch)), d);
					break;
				} else {
					m += 1;
					if(m>mLimit) {
						break; // regardless of what happened, we are done here
					}
				}
			}
			d += 1;
		}
	}
	
	// reduce predicate
	else {
		imageStore(targetBuff, ivec2(tc/vec2(xPitch,yPitch)), vec4(0));
	}

	//imageStore(targetBuff, ivec2(tc/vec2(xPitch,yPitch)), predV);
}
