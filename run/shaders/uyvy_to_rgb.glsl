// Convert UYVY to RGB colorspace
// (UYVY is YcbCr with 4:2:2 encoding)

// ITU.BT-601
//    R'= Y' + 0.000*U' + 1.403*V'
//    G'= Y' - 0.344*U' - 0.714*V'
//    B'= Y' + 1.773*U' + 0.000*V'

// ITU.BT-709
//    R'= Y' + 0.0000*Cb + 1.5701*Cr
//    G'= Y' - 0.1870*Cb - 0.4664*Cr
//    B'= Y' - 1.8556*Cb + 0.0000*Cr

uniform sampler2D source;

const float lowB = 16.0f/255.0f;
const float scale1 = 255.0f/(235.0f-16.0f);
const float scale2 = 255.0f/(240.0f-16.0f);

vec4 convert601(float y, float u, float v) {
  return vec4(
    y + 1.403*v, 
    y + (-0.344)*u + (-0.714)*v,
    y + 1.773*u,
    1.0 );
}
//vec4 convert709(float y, float u, float v) {
//  return vec4(
//    y + 1.5701*v, 
//    y + (-0.1870)*u + (-0.4664)*v,
//    y + (-1.8556)*u,
//    1.0 );
//}

void main(void)
{
	vec4 uyvy = texture2D(source, gl_TexCoord[0]);
  
	// ABRG upload
	uyvy.rgba = uyvy.abgr;
  
	// Fix scale (y from 16-235, Cb/Cr 16-240)
	uyvy -= vec4(lowB);
	uyvy.ga *= vec2(scale1);
	uyvy.rb *= vec2(scale2);
	
	// 1 for pixels in the right of the tuple, 0 otherwise
	float rightpixel = mod(gl_FragCoord.x, 2.0)-0.5;
	float leftpixel = 1.0-rightpixel;
	// select correct y for output pixel
	float y = leftpixel*uyvy[1] + rightpixel*uyvy[3];
	
	gl_FragColor = convert601(y, uyvy[0]-0.5f, uyvy[2]-0.5f);
}
