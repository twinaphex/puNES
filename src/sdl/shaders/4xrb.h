/*****************************************************************************************/
/* 4xRB                                                                                  */
/*****************************************************************************************/
{
	// vertex shader
	NULL,
	// fragment shader
	"uniform vec2 size_texture;\n"
	"uniform sampler2D texture_scr;\n"

	"const vec3 dtt = vec3(65536.0, 255.0, 1.0);\n"
	"float reduce(vec3 color) {\n"
	"	return dot(color, dtt);\n"
	"}"

	"void main() {\n"
	"	vec2 ps = 1.0 / size_texture;\n"
	"	vec2 dx = vec2(ps.x, 0.0);\n"
	"	vec2 dy = vec2(0.0, ps.y);\n"

	"	vec2 pixcoord = gl_TexCoord[0].xy / ps;\n;"
	"	vec2 fp = fract(pixcoord);\n"
	"	vec2 d11 = gl_TexCoord[0].xy - fp * ps;\n"

	"	// Reading the texels\n"
	"	vec3 A = texture2D(texture_scr, d11-dx-dy).xyz;\n"
	"	vec3 B = texture2D(texture_scr, d11   -dy).xyz;\n"
	"	vec3 C = texture2D(texture_scr, d11+dx-dy).xyz;\n"
	"	vec3 D = texture2D(texture_scr, d11-dx   ).xyz;\n"
	"	vec3 E = texture2D(texture_scr, d11      ).xyz;\n"
	"	vec3 F = texture2D(texture_scr, d11+dx   ).xyz;\n"
	"	vec3 G = texture2D(texture_scr, d11-dx+dy).xyz;\n"
	"	vec3 H = texture2D(texture_scr, d11+dy   ).xyz;\n"
	"	vec3 I = texture2D(texture_scr, d11+dx+dy).xyz;\n"

	"	vec3 E0 = E;\n"
	"	vec3 E1 = E;\n"
	"	vec3 E2 = E;\n"
	"	vec3 E3 = E;\n"
	"	vec3 E4 = E;\n"
	"	vec3 E5 = E;\n"
	"	vec3 E6 = E;\n"
	"	vec3 E7 = E;\n"
	"	vec3 E8 = E;\n"
	"	vec3 E9 = E;\n"
	"	vec3 E10 = E;\n"
	"	vec3 E11 = E;\n"
	"	vec3 E12 = E;\n"
	"	vec3 E13 = E;\n"
	"	vec3 E14 = E;\n"
	"	vec3 E15 = E;\n"

	"	float a = reduce(A);\n"
	"	float b = reduce(B);\n"
	"	float c = reduce(C);\n"
	"	float d = reduce(D);\n"
	"	float e = reduce(E);\n"
	"	float f = reduce(F);\n"
	"	float g = reduce(G);\n"
	"	float h = reduce(H);\n"
	"	float i = reduce(I);\n"

	"	if ((h == f)&&(h != e)) {\n"
	"		if (((e == g) && ((i == h) || (e == d))) ||"
	"			((e == c) && ((i == h) || (e == b)))) {\n"
	"			E11 = mix(E11, F, 0.5);\n"
	"			E14 = E11;\n"
	"			E15 = F;\n"
	"		}\n"
	"	}\n"

	"	if ((f == b)&&(f != e)) {\n"
	"		if (((e == i) && ((f == c) || (e == h))) ||"
	"			((e == a) && ((f == c) || (e == d)))) {\n"
	"			E2 = mix(E2, B, 0.5);\n"
	"			E7 = E2;\n"
	"			E3 = B;\n"
	"		}\n"
	"	}\n"

	"	if ((b == d)&&(b != e)) {\n"
	"		if (((e == c) && ((b == a) || (e == f))) ||"
	"			((e == g) && ((b == a) || (e == h)))) {\n"
	"			E1 = mix(E1, D, 0.5);\n"
	"			E4 = E1;\n"
	"			E0 = D;\n"
	"		}\n"
	"	}\n"

	"	if ((d == h)&&(d != e)) {\n"
	"		if (((e == a) && ((d == g) || (e == b))) ||\n"
	"			((e == i) && ((d == g) || (e == f)))) {\n"
	"			E8 = mix(E8, H, 0.5);\n"
	"			E13 = E8;\n"
	"			E12 = H;\n"
	"		}\n"
	"	}\n"

	"vec3 res;\n"

	"	if (fp.x < 0.25) {\n"
	"		if (fp.y < 0.25) res = E0;\n"
	"		else if ((fp.y > 0.25) && (fp.y < 0.50)) res = E4;\n"
	"		else if ((fp.y > 0.50) && (fp.y < 0.75)) res = E8;\n"
	"		else res = E12;\n"
	"	} else if ((fp.x > 0.25) && (fp.x < 0.50)) {\n"
	"		if (fp.y < 0.25) res = E1;\n"
	"		else if ((fp.y > 0.25) && (fp.y < 0.50)) res = E5;\n"
	"		else if ((fp.y > 0.50) && (fp.y < 0.75)) res = E9;\n"
	"		else res = E13;\n"
	"	} else if ((fp.x > 0.50) && (fp.x < 0.75)) {\n"
	"		if (fp.y < 0.25) res = E2;\n"
	"		else if ((fp.y > 0.25) && (fp.y < 0.50)) res = E6;\n"
	"		else if ((fp.y > 0.50) && (fp.y < 0.75)) res = E10;\n"
	"		else res = E14;\n"
	"	} else {\n"
	"		if (fp.y < 0.25) res = E3;\n"
	"		else if ((fp.y > 0.25) && (fp.y < 0.50)) res = E7;\n"
	"		else if ((fp.y > 0.50) && (fp.y < 0.75)) res = E11;\n"
	"		else res = E15;\n"
	"	}\n"

	"	gl_FragColor = vec4(res, 1.0);\n"
	"}"
},