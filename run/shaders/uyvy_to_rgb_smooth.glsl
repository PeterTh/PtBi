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

const float smoothFactor = 0.33;//0.22; // experimentally determined 

uniform float lowB = 16.0f/255.0f;
uniform float scale1 = 255.0f/(235.0f-16.0f);
uniform float scale2 = 255.0f/(240.0f-16.0f);


vec3 convert601(float y, float u, float v) {
  return vec3(
    y + 1.402*v,
//    y + (-0.344)*u + (-0.714)*v,
    y + (-0.172)*u + (-0.357)*v,
    y + 1.772*u);
}
//vec4 convertYUV(float y, float u, float v) {
//  return vec4(
//    y + 1.13983*v,
//    y + (-0.39465)*u + (-0.5806)*v,
//    y + 2.03211*u,
//    1.0 );
//}
//vec4 convert709(float y, float u, float v) {
//  return vec4(
//    y + 1.5701*v, 
//    y + (-0.1870)*u + (-0.4664)*v,
//    y + (-1.8556)*u,
//    1.0 );
//}

const float pitch = 1.0f/(FRAME_WIDTH/2.0f);
const float pitch_y = 1.0f/FRAME_HEIGHT;

void main(void)
{
  vec2 tc = vec2(gl_FragCoord.x/2.0f*pitch, 1.0f-((gl_FragCoord.y-1)*pitch_y)); 
  vec4 uyvy = texture2D(source, tc);
  vec4 uyvy_l = texture2D(source, tc + vec2(-pitch,0));
  vec4 uyvy_r = texture2D(source, tc + vec2(pitch,0));
  
  // ABRG upload
  uyvy.rgba = uyvy.abgr;
  uyvy_l.rgba = uyvy_l.abgr;
  uyvy_r.rgba = uyvy_r.abgr;
  
  // Fix scale (y from 16-235, Cb/Cr 16-240)
  uyvy -= vec4(lowB);
  uyvy_l -= vec4(lowB);
  uyvy_r -= vec4(lowB);
  uyvy.ga *= vec2(scale1);
  uyvy.rb *= vec2(scale2);
  uyvy_l.rb *= vec2(scale2);
  uyvy_r.rb *= vec2(scale2);
	
  // 1 for pixels in the right of the tuple, 0 otherwise
  float rightpixel = mod(gl_FragCoord.x, 2.0)-0.5;
  float leftpixel = 1.0-rightpixel;
  // select correct y for output pixel
  float y = leftpixel*uyvy[1] + rightpixel*uyvy[3];
  
  float invSmooth = 1.0f - smoothFactor;
  float u = (uyvy[0]-0.5f)*invSmooth + (uyvy_l[0]-0.5f)*smoothFactor*leftpixel + (uyvy_r[0]-0.5f)*smoothFactor*rightpixel;
  float v = (uyvy[2]-0.5f)*invSmooth + (uyvy_l[2]-0.5f)*smoothFactor*leftpixel + (uyvy_r[2]-0.5f)*smoothFactor*rightpixel;

  v*=255.0/233.0;
  u*=255.0/245.0;

  //vec3 rgb = convert601(y*1.04f, u*1.08f, v*1.1f); // tweaking
  vec3 rgb = convert601(y, u, v);
  gl_FragColor = vec4(rgb.r, rgb.g, rgb.b, y);
}
