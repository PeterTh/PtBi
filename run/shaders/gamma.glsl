
uniform sampler2D source;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;

uniform float gamma;

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);
  
  vec4 frag = texture2D(source, tc);  
  gl_FragColor = pow(frag, vec4(gamma));
}
