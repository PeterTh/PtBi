
const float pi = 3.14159265;

//const float N_start = 1.5;
//const float N_stop = 3.5;
const float N_start = 5.0/3.0;
const float N_stop = 8.0/3.0;

const float dist_max = 10.0;

float sinc(float x) {
  return sin(x*pi)/(x*pi);
}
float lanczos(float dist, float N) {
  return sinc(dist) * sinc(dist/N);
}

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);
  float result;
  result = lanczos(tc[0]*dist_max, tc[1]*(N_stop-N_start)+N_start);
  if(tc[0]<0.01) result = 1.0;
  gl_FragColor = vec4(result);
}