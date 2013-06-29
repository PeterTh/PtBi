
uniform sampler2D source;
uniform sampler2D prev;

//const float fac = 0.12;
const float fac = 0.07;
const float facN = 0.15;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);

  vec4 prevC = texture2D(prev, tc);
  vec4 fragA = texture2D(source, tc + vec2(-xPitch,yPitch));
  vec4 fragB = texture2D(source, tc + vec2(0,yPitch));
  vec4 fragC = texture2D(source, tc + vec2(xPitch,yPitch));
  vec4 fragD = texture2D(source, tc + vec2(-xPitch,0.0));
  vec4 fragE = texture2D(source, tc + vec2(0,0.0));
  vec4 fragF = texture2D(source, tc + vec2(xPitch,0.0));
  vec4 fragG = texture2D(source, tc + vec2(-xPitch,-yPitch));
  vec4 fragH = texture2D(source, tc + vec2(0,-yPitch));
  vec4 fragI = texture2D(source, tc + vec2(xPitch,-yPitch));
  
  gl_FragColor = 0.9*prevC + fragA+fragB+fragC+fragD+fragE+fragF+fragG+fragH+fragI;
}
