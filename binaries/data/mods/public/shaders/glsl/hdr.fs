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


uniform float brightness;
uniform float hdr;
uniform float saturation;
uniform float bloom;



vec2 scrsz = vec2(width, height);
vec3 samp(vec2 offs)
{
	//vec3 colour = texture2D(bgl_RenderedTexture, (gl_FragCoord.xy + offs) / scrsz).rgb;
	vec2 o = offs / 300.0;
	
	vec3 t = texture2D(bgl_RenderedTexture, texcoord + o).rgb - 0.4;

	vec3 colour = clamp(t, 0.0, 1.0);
	//return colour = dot(colour, vec3(0.333)) > 0.8 ? colour : vec3(0.0);
	return colour;
}


void main(void)
{

	/**vec3 colour;
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

	colour /= 16.0;*/
	//colour /= 4.0;

	vec3 colour = texture2D(bgl_RenderedTexture, texcoord).rgb;
	vec3 bloomv = texture2D(bloomTex, texcoord).rgb;

	///bloomv = max(bloomv - 0.05, vec3(0.0));
	bloomv = max(bloomv - bloom, vec3(0.0));

	//bloom = vec3(dot(bloom, vec3(0.299, 0.587, 0.114)));

	//bloom = (bloom - 0.5) * 0.4 + 0.5;
	//bloom *= 0.8;
	//bloom -= 0.4;
	
	///colour += bloom;

        //colour = bloom;
	
	//colour = 1.0 - (1.0 - colour) * (1.0 - tex);
 
	//colour = mix(colour, tex, 0.5);

	//colour = 1.0 - (1.0 - colour) * (1.0 - bloom);

	colour = max(bloomv, colour);
	
	//vec3 brightness = vec3(0.5);
	//float saturation = 0.9;
	//float hdr = 1.2;
	
	colour += vec3(brightness);

	colour -= vec3(0.5);
	colour *= vec3(hdr);
	colour += vec3(0.5);

	colour = mix(vec3(dot(colour, vec3(0.299, 0.587, 0.114))), colour, saturation);

	
	gl_FragColor.rgb = colour;
	gl_FragColor.a = 1.0;
}


