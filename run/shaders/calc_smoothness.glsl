
uniform sampler2D source;

const float fac = 0.2;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;

float dist(vec4 a, vec4 b) {
  //vec3 d = abs(a.rgb-b.rgb);
  //return d.r+d.g+d.b;
  //return length(a.rgb-b.rgb)/1.75;
  //return smoothstep(0.3, 0.8, distance(a.rgb,b.rgb));
  return step(0.1, distance(a.a,b.a));
}

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);

  vec4 fragA = texture2D(source, tc + vec2(-xPitch,-yPitch));
  vec4 fragB = texture2D(source, tc + vec2(0,-yPitch));
  vec4 fragC = texture2D(source, tc + vec2(xPitch,-yPitch));
  
  float d1 = dist(fragA, fragB);
  float d2 = dist(fragB, fragC);
  
  vec4 fragD = texture2D(source, tc + vec2(-xPitch,0.0));
  vec4 fragE = texture2D(source, tc + vec2(0,0.0));
  vec4 fragF = texture2D(source, tc + vec2(xPitch,0.0));
  
  float d3 = dist(fragA, fragD);
  float d4 = dist(fragB, fragE);
  float d5 = dist(fragC, fragF);  
  float d6 = dist(fragD, fragE);
  float d7 = dist(fragE, fragF);
  
  vec4 fragG = texture2D(source, tc + vec2(-xPitch,yPitch));
  vec4 fragH = texture2D(source, tc + vec2(0,yPitch));
  vec4 fragI = texture2D(source, tc + vec2(xPitch,yPitch));
  
  float d8 = dist(fragD, fragG);
  float d9 = dist(fragE, fragH);
  float d10 = dist(fragF, fragI);  
  float d11 = dist(fragG, fragH);
  float d12 = dist(fragH, fragI);
  
  float dsum = d1+d2+d3+d4+d5+d6+d7+d8+d9+d10+d11+d12;
  
  //float jag1 = (d1+d4+d7+d12)*fac - dsum;  
  //float jag2 = (d2+d7+d9+d11)*fac - dsum;
  
  gl_FragColor = vec4(fragE.rgb, dsum*fac);
}
