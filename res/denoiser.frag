uniform sampler2D tInput;

void main() {
	float exponent = 3.0;
	float strength = 3.0;
	vec2 resolution = vec2(1280.0 * 0.5, 720.0 * 0.5);
	vec2 pxl_coord = gl_TexCoord[0].xy;

	vec4 center = texture2D(tInput, pxl_coord);
	vec4 color = vec4(0.0);
	float total = 0.0;
	for (float x = -4.0; x <= 4.0; x += 1.0) {
		for (float y = -4.0; y <= 4.0; y += 1.0) {
			vec4 sample = texture2D(tInput, pxl_coord + vec2(x, y) / resolution);
			float weight = 1.0 - abs(dot(sample.rgb - center.rgb, vec3(0.25)));
			weight = pow(weight, exponent);
			color += sample * weight;
			total += weight;
		}
	}
	gl_FragColor = color / total;

}