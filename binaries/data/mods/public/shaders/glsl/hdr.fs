#version 120

varying vec2 v_tex;

uniform sampler2D bgl_RenderedTexture;
uniform sampler2D bgl_DepthTexture;
uniform sampler2D bloomTex;

vec2 texcoord = v_tex;

uniform float bgl_RenderedTextureWidth;
uniform float bgl_RenderedTextureHeight;
float width = bgl_RenderedTextureWidth; //texture width
float height = bgl_RenderedTextureHeight; //texture height



vec2 scrsz = vec2(width, height);
vec3 samp(vec2 offs)
{
	//vec3 colour = texture2D(bgl_RenderedTexture, (gl_FragCoord.xy + offs) / scrsz).rgb;
	vec2 o = offs / 400.0;
	vec3 colour = texture2D(bgl_RenderedTexture, texcoord + o).rgb;
	//return colour = dot(colour, vec3(0.333)) > 0.8 ? colour : vec3(0.0);
	return colour;
}


void main(void)
{

	vec3 colour;
	colour =  samp(vec2(-0.5, -0.5));
	colour += samp(vec2( 0.5, -0.5));
	colour += samp(vec2(-0.5,  0.5));
	colour += samp(vec2( 0.5,  0.5));

	colour += samp(vec2(-1.5, -1.5));
	colour += samp(vec2(-0.5, -1.5));
	colour += samp(vec2( 0.5, -1.5));
	colour += samp(vec2( 1.5, -1.5));

	colour += samp(vec2(-1.5, -0.5));
	colour += samp(vec2( 1.5, -0.5));

	colour += samp(vec2(-1.5,  0.5));
	colour += samp(vec2( 1.5,  0.5));

	colour += samp(vec2(-1.5,  1.5));
	colour += samp(vec2(-0.5,  1.5));
	colour += samp(vec2( 0.5,  1.5));
	colour += samp(vec2( 1.5,  1.5));

	colour /= 16.0;

	vec3 tex = texture2D(bgl_RenderedTexture, texcoord).rgb;

	colour *= 0.3;
	colour = 1.0 - (1.0 - colour) * (1.0 - tex);

	colour -= 0.7;
	colour *= 1.4;
	colour += 0.7;



	vec3 depth = texture2D(bgl_DepthTexture, texcoord).xyz;

	vec3 fogColour = vec3(0.6, 0.6, 0.65);
	float dist = 0.991;
	float maxFog = 0.6;

	float rec = 1.0 / (1.0 - dist);

	float factor = (depth.r - dist) * rec;

	factor = min(factor, maxFog);
	
	gl_FragColor.rgb = depth.r >= dist && depth.r < 1.0 ? mix(colour, fogColour, factor) : colour;
	gl_FragColor.a = 1.0;
}


