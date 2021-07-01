

void main()
{
	float alpha = ((gl_TexCoord[0].x * gl_TexCoord[0].x) + (gl_TexCoord[0].y * gl_TexCoord[0].y)) <= 1.0 ? 1.0 : 0.0;
	gl_FragColor = vec4(alpha,alpha,alpha,alpha);
	gl_FragDepth = 1.0-alpha;
}
