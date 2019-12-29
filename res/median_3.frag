/*
3x3 Median optimized for GeForce 8800

Morgan McGuire and Kyle Whitson
Williams College

Register allocation tips by Victor Huang Xiaohuang
University of Illinois at Urbana-Champaign

http://graphics.cs.williams.edu


Copyright (c) Morgan McGuire and Williams College, 2006
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Input texture
uniform sampler2D T;

// Change these 2 defines to change precision
#define vec vec3

#define s2(a, b)        temp = a; a = min(a, b); b = max(temp, b);
#define mn3(a, b, c)      s2(a, b); s2(a, c);
#define mx3(a, b, c)      s2(b, c); s2(a, c);

#define mnmx3(a, b, c)      mx3(a, b, c); s2(a, b);                                   // 3 exchanges
#define mnmx4(a, b, c, d)   s2(a, b); s2(c, d); s2(a, c); s2(b, d);                   // 4 exchanges
#define mnmx5(a, b, c, d, e)  s2(a, b); s2(c, d); mn3(a, c, e); mx3(b, d, e);           // 6 exchanges
#define mnmx6(a, b, c, d, e, f) s2(a, d); s2(b, e); s2(c, f); mn3(a, b, c); mx3(d, e, f); // 7 exchanges

void main() {

  vec2 Tinvsize = vec2(1.0/800.0, 1.0/800.0);
  vec v[6];

  v[0] = texture2D(T, gl_TexCoord[0].xy + vec2(-1.0, -1.0) * Tinvsize).xyz;
  v[1] = texture2D(T, gl_TexCoord[0].xy + vec2( 0.0, -1.0) * Tinvsize).xyz;
  v[2] = texture2D(T, gl_TexCoord[0].xy + vec2(+1.0, -1.0) * Tinvsize).xyz;
  v[3] = texture2D(T, gl_TexCoord[0].xy + vec2(-1.0,  0.0) * Tinvsize).xyz;
  v[4] = texture2D(T, gl_TexCoord[0].xy + vec2( 0.0,  0.0) * Tinvsize).xyz;
  v[5] = texture2D(T, gl_TexCoord[0].xy + vec2(+1.0,  0.0) * Tinvsize).xyz;

  // Starting with a subset of size 6, remove the min and max each time
  vec temp;
  mnmx6(v[0], v[1], v[2], v[3], v[4], v[5]);

  v[5] = texture2D(T, gl_TexCoord[0].xy + vec2(-1.0, +1.0) * Tinvsize).xyz;

  mnmx5(v[1], v[2], v[3], v[4], v[5]);

  v[5] = texture2D(T, gl_TexCoord[0].xy + vec2( 0.0, +1.0) * Tinvsize).xyz;

  mnmx4(v[2], v[3], v[4], v[5]);

  v[5] = texture2D(T, gl_TexCoord[0].xy + vec2(+1.0, +1.0) * Tinvsize).xyz;

  mnmx3(v[3], v[4], v[5]);
  gl_FragColor = vec4(v[4], 1.0);
}
