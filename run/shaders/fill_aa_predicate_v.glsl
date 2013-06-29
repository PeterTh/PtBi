
uniform sampler2D source;

const float eps = 0.01;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);

  // early exit if not on edge
  //if(texture2D(source, tc + vec2(-xPitch,0)).b + texture2D(source, tc).b < eps) discard;

  {
  // check for vertical edges
  //  |
  //   |
  //    |
  int d = 0;
  bool t = false, b = false, c = true;
  vec4 fragT, fragB;
  for(float d = 1; d<64 && c && !(t&&b); ++d) {
	if(!t) {
		fragT = texture2D(source, tc + vec2(-xPitch,d*yPitch));
		if(fragT.r > eps) t = true;
	}
	if(!b) {
		fragB = texture2D(source, tc + vec2(0,-d*yPitch));
		if(fragB.r > eps) b = true;
		else c = fragT.b > eps;
	}
  }
  if(t && b) {
	gl_FragColor = vec4(1);
	return;
  }
  }

  {
  // check for vertical edges
  //    |
  //   |
  //  |
  int d = 0;
  bool t = false, b = false, c = true;
  vec4 fragT, fragB;
  for(float d = 1; d<64 && c && !(t&&b); ++d) {
	if(!t) {
		fragT = texture2D(source, tc + vec2(0,d*yPitch));
		if(fragT.g > eps) t = true;
	}
	if(!b) {
		fragB = texture2D(source, tc + vec2(-xPitch,-d*yPitch));
		if(fragB.g > eps) b = true;
		else c = fragB.b > eps;
	}
  }
  if(t && b) {
	gl_FragColor = vec4(1);
	return;
  }
  }

  discard;
}
