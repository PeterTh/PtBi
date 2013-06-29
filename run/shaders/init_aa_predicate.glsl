
uniform sampler2D source;

//const float fac = 0.12;
uniform float fac = 0.07;
uniform float facN = 0.15;

const float xPitch = 1.0/FRAME_WIDTH;
const float yPitch = 1.0/FRAME_HEIGHT;

float dist(vec4 a, vec4 b) {
  return step(fac, distance(a.a,b.a));
}
float distN(vec4 a, vec4 b) {
  return step(facN, distance(a.a,b.a));
}

void main(void)
{
  vec2 tc = vec2(gl_TexCoord[0]);

  vec4 fragA = texture2D(source, tc + vec2(-xPitch,yPitch));
  vec4 fragB = texture2D(source, tc + vec2(0,yPitch));
  vec4 fragC = texture2D(source, tc + vec2(xPitch,yPitch));
  
  float d1 = dist(fragA, fragB);
  float d2 = dist(fragB, fragC);
  float nd1 = distN(fragA, fragB);
  float nd2 = distN(fragB, fragC);
  
  vec4 fragD = texture2D(source, tc + vec2(-xPitch,0.0));
  vec4 fragE = texture2D(source, tc + vec2(0,0.0));
  vec4 fragF = texture2D(source, tc + vec2(xPitch,0.0));
  
  float d3 = dist(fragA, fragD);
  float d4 = dist(fragB, fragE);
  float d5 = dist(fragC, fragF);  
  float d6 = dist(fragD, fragE);
  float d7 = dist(fragE, fragF);
  float nd3 = distN(fragA, fragD);
  float nd4 = distN(fragB, fragE);
  float nd5 = distN(fragC, fragF);  
  float nd6 = distN(fragD, fragE);
  float nd7 = distN(fragE, fragF);
  
  vec4 fragG = texture2D(source, tc + vec2(-xPitch,-yPitch));
  vec4 fragH = texture2D(source, tc + vec2(0,-yPitch));
  vec4 fragI = texture2D(source, tc + vec2(xPitch,-yPitch));
  
  float d8 = dist(fragD, fragG);
  float d9 = dist(fragE, fragH);
  float d10 = dist(fragF, fragI);  
  float d11 = dist(fragG, fragH);
  float d12 = dist(fragH, fragI);
  float nd8 = distN(fragD, fragG);
  float nd9 = distN(fragE, fragH);
  float nd10 = distN(fragF, fragI);  
  float nd11 = distN(fragG, fragH);
  float nd12 = distN(fragH, fragI);
  
  // OOO
  // OXX
  // XXX

  float h1 = d4*d5*d6*d8 * (1-nd1)*(1-nd2)*(1-nd3)*(1-nd7)*(1-nd9)*(1-nd10)*(1-nd11)*(1-nd12);

  // OOO
  // XXO
  // XXX

  float h2 = d3*d4*d7*d10 * (1-nd1)*(1-nd2)*(1-nd5)*(1-nd6)*(1-nd8)*(1-nd9)*(1-nd11)*(1-nd12);
  
  // OOO
  // XXX
  // XXX

  float hc = d3*d4*d5  * (1-nd1)*(1-nd2)*(1-nd6)*(1-nd7)*(1-nd8)*(1-nd9)*(1-nd10)*(1-nd11)*(1-nd12);
  
  // XOO
  // XXO
  // XXO

  float v1 = d1*d4*d7*d12 * (1-nd2)*(1-nd3)*(1-nd5)*(1-nd6)*(1-nd8)*(1-nd9)*(1-nd10)*(1-nd11);

  // XXO
  // XXO
  // XOO

  float v2 = d2*d7*d9*d11 * (1-nd1)*(1-nd3)*(1-nd4)*(1-nd5)*(1-nd6)*(1-nd8)*(1-nd10)*(1-nd12);
  
  // XXO
  // XXO
  // XXO

  float vc = d2*d7*d12  * (1-nd1)*(1-nd3)*(1-nd4)*(1-nd5)*(1-nd6)*(1-nd8)*(1-nd9)*(1-nd10)*(1-nd11);
   
  gl_FragData[0] = vec4(h1,h2,hc,1);
  gl_FragData[1] = vec4(v1,v2,vc,1);
}
