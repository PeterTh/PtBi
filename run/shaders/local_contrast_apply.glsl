
uniform sampler2D original;
uniform sampler2D gathered;

uniform float contrastIncrease = 0.1;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);
  
  vec4 fragOrig = texture2D(original, tc);
  vec4 fragCon = texture2D(gathered, tc);

  gl_FragColor = fragOrig - (fragCon-fragOrig)*contrastIncrease;
}
