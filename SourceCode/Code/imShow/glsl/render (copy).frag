uniform sampler2D alpha;
uniform float layer;
void main()
{
	float alpha_val = texture2D(alpha, gl_TexCoord[0].xy).r;
	gl_FragColor=vec4(layer, layer, layer, alpha_val);
}
