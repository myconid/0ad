#version 120

uniform sampler2D renderedTex;
uniform sampler2D depthTex;
uniform sampler2D blurTex2;
uniform sampler2D blurTex4;
uniform sampler2D blurTex8;

uniform float width;
uniform float height;

uniform float zNear;
uniform float zFar;

uniform float brightness;
uniform float hdr;
uniform float saturation;
uniform float bloom;

varying vec2 v_tex;


float linearizeDepth(float depth)
{
	return -zFar * zNear / (depth * (zFar - zNear) - zFar);
}


void main(void)
{
	vec3 colour = texture2D(renderedTex, v_tex).rgb;

	vec3 blur2 = texture2D(blurTex2, v_tex).rgb;
	vec3 blur4 = texture2D(blurTex4, v_tex).rgb;
	vec3 blur8 = texture2D(blurTex8, v_tex).rgb;

	float depth = texture2D(depthTex, v_tex).r;

	float midDepth = texture2D(depthTex, vec2(0.5)).r;

	float lDepth = linearizeDepth(depth);
	float lMidDepth = linearizeDepth(midDepth);
	float amount = abs(lDepth - lMidDepth);

	amount = clamp(amount / (lMidDepth * 2), 0.0, 1.0);

	colour = (amount >= 0.0 && amount < 0.25) ? mix(colour, blur2, (amount - 0.0) / (0.25)) : colour;
	colour = (amount >= 0.25 && amount < 0.50) ? mix(blur2, blur4, (amount - 0.25) / (0.25)) : colour;
	colour = (amount >= 0.50 && amount < 0.75) ? mix(blur4, blur8, (amount - 0.50) / (0.25)) : colour;
	colour = (amount >= 0.75 && amount <= 1.00) ? blur8 : colour;

	gl_FragColor.rgb = colour;
	gl_FragColor.a = 1.0;
}


