
uniform sampler2D source;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;
const float xP2 = 0.5*xPitch;
const float yP2 = 0.5*yPitch;

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);
  
  vec4 fragA = texture2D(source, tc + vec2(-1*xP2,+3*yP2));
  vec4 fragB = texture2D(source, tc + vec2(+1*xP2,+3*yP2));

  vec4 fragC = texture2D(source, tc + vec2(-3*xP2,+1*yP2));
  vec4 fragD = texture2D(source, tc + vec2(-1*xP2,+1*yP2));
  vec4 fragE = texture2D(source, tc + vec2(+1*xP2,+1*yP2));
  vec4 fragF = texture2D(source, tc + vec2(+3*xP2,+1*yP2));
  
  vec4 fragG = texture2D(source, tc + vec2(-3*xP2,-1*yP2));
  vec4 fragH = texture2D(source, tc + vec2(-1*xP2,-1*yP2));
  vec4 fragI = texture2D(source, tc + vec2(+1*xP2,-1*yP2));
  vec4 fragJ = texture2D(source, tc + vec2(+3*xP2,-1*yP2));
  
  vec4 fragK = texture2D(source, tc + vec2(-1*xP2,-3*yP2));
  vec4 fragL = texture2D(source, tc + vec2(+1*xP2,-3*yP2));
  
  gl_FragColor = (fragA + fragB + fragC + fragD + fragE + fragF + fragG + fragH + fragI + fragJ + fragK + fragL) / 12.0;
}
