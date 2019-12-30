// Input texture
uniform sampler2D T;


void main() 
{
  vec4 color = texture2D(T, gl_TexCoord[0].xy);

  if (length(color.xyz) > 0.35) {
    gl_FragColor = color;
  } else {
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
}