
uniform sampler2D source;

const float eps = 0.01;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);

  // early exit if not on edge
  if(texture2D(source, tc + vec2(0,-yPitch)).b < eps) discard;

  {
  // check for horizontal edges
  //          ___---
  //    ___---
  // ---
  int d = 0;
  bool l = false, r = false, c = true;
  vec4 fragL, fragR;
  for(float d = 1; d<64 && c && !(r&&l); ++d) {
	if(!l) {
		fragL = texture2D(source, tc + vec2(-d*xPitch,-yPitch));
		if(fragL.r > eps) l = true;
		else c = fragL.b > eps;
	}
	if(!r) {
		fragR = texture2D(source, tc + vec2(d*xPitch,0));
		if(fragR.r > eps) r = true;
	}
  }
  if(l && r) {
	gl_FragColor = vec4(1);
	return;
  }
  }

  {
  // check for horizontal edges
  // ---___
  //       ---___
  //             ---
  int d = 0;
  bool l = false, r = false, c = true;
  vec4 fragL, fragR;
  for(float d = 1; d<64 && c; ++d) {
	if(!l) {
		fragL = texture2D(source, tc + vec2(-d*xPitch,0));
		if(fragL.g > eps) l = true;
	}
	if(!r) {
		fragR = texture2D(source, tc + vec2(d*xPitch,-yPitch));
		if(fragR.g > eps) r = true;
		else c = fragR.b > eps;
	}
  }
  if(l && r) {
	gl_FragColor = vec4(1);
	return;
  }
  }

  discard;
}
