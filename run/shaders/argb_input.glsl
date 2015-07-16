// Convert RGBA input for further processing

uniform sampler2D source;

const float pitch = 1.0f/FRAME_WIDTH;
const float pitch_y = 1.0f/FRAME_HEIGHT;

uniform bool fullRange = false;

void main(void)
{
  vec2 tc = vec2(gl_FragCoord.x*pitch, 1.0f-((gl_FragCoord.y-1)*pitch_y)); 
  vec4 rgba = texture2D(source, tc).bgra;
	
  if(!fullRange) {
    rgba -= 16.0f / 255.0f;
    rgba *= 255.0f / (235.0f - 16.0f);
  }
  
  float lum = 0.299*rgba.r + 0.587*rgba.g + 0.114*rgba.b;
  gl_FragColor = vec4(rgba.r, rgba.g, rgba.b, lum);
}
