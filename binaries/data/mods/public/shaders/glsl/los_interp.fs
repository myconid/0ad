#version 110

varying vec2 v_tex;

uniform sampler2D bgl_RenderedTexture;
uniform sampler2D bgl_DepthTexture;
uniform sampler2D losTex1, losTex2;
uniform float bgl_RenderedTextureWidth;
uniform float bgl_RenderedTextureHeight;

#define PI    3.14159265

float width = bgl_RenderedTextureWidth; //texture width
float height = bgl_RenderedTextureHeight; //texture height

uniform vec3 delta;

vec2 texcoord = v_tex;




void main(void)
{	
	//vec3 colour = texture2D(bgl_RenderedTexture, texcoord);

	vec4 los2 = texture2D(losTex1, texcoord);
	vec4 los1 = texture2D(losTex2, texcoord);

	//colour *= los;
	
	//gl_FragColor.rgb = colour;

	//gl_FragColor = vec4(0,0,0,los);

	gl_FragColor = mix(los1, los2, clamp(delta.r, 0.0, 1.0));

	//gl_FragColor.a = (1.0 - clamp(delta.r, 0.0, 1.0)) * los;//delta.r;

	//gl_FragColor.rgb = (depth - 0.99) * 100;

}

