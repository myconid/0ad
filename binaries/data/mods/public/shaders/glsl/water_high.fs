#version 110

//uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 cameraPos;
uniform sampler2D normalMap;
uniform sampler2D normalMap2;
uniform sampler2D reflectionMap;
uniform sampler2D refractionMap;
uniform sampler2D depthTex;
uniform sampler2D losMap;
uniform float shininess;		// Blinn-Phong specular strength
uniform float specularStrength;	// Scaling for specular reflection (specular color is (this,this,this))
uniform float waviness;			// "Wildness" of the reflections and refractions; choose based on texture
uniform vec3 tint;				// Tint for refraction (used to simulate particles in water)
uniform float murkiness;		// Amount of tint to blend in with the refracted colour
uniform float fullDepth;		// Depth at which to use full murkiness (shallower water will be clearer)
uniform vec3 reflectionTint;	// Tint for reflection (used for really muddy water)
uniform float reflectionTintStrength;	// Strength of reflection tint (how much of it to mix in)

uniform float time;

#if USE_SHADOW
  #if USE_SHADOW_SAMPLER
    uniform sampler2DShadow shadowTex;
    #if USE_SHADOW_PCF
      uniform vec4 shadowScale;
    #endif
  #else
    uniform sampler2D shadowTex;
  #endif
#endif

varying vec3 worldPos;
varying float waterDepth;
varying vec4 v_shadow;


float get_shadow(vec4 coords)
{
  #if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
    #if USE_SHADOW_SAMPLER
      #if USE_SHADOW_PCF
        vec2 offset = fract(coords.xy - 0.5);
        vec4 size = vec4(offset + 1.0, 2.0 - offset);
        vec4 weight = (vec4(2.0 - 1.0 / size.xy, 1.0 / size.zw - 1.0) + (coords.xy - offset).xyxy) * shadowScale.zwzw;
        return (1.0/9.0)*dot(size.zxzx*size.wwyy,
          vec4(shadow2D(shadowTex, vec3(weight.zw, coords.z)).r,
               shadow2D(shadowTex, vec3(weight.xw, coords.z)).r,
               shadow2D(shadowTex, vec3(weight.zy, coords.z)).r,
               shadow2D(shadowTex, vec3(weight.xy, coords.z)).r));
      #else
        return shadow2D(shadowTex, coords.xyz).r;
      #endif
    #else
      if (coords.z >= 1.0)
        return 1.0;
      return (coords.z <= texture2D(shadowTex, coords.xy).x ? 1.0 : 0.0);
    #endif
  #else
    return 1.0;
  #endif
}


void main()
{
	vec3 n, l, h, v;		// Normal, light vector, half-vector and view vector (vector to eye)
	float ndotl, ndoth, ndotv;
	float fresnel;
	float t;				// Temporary variable
	vec2 reflCoords, refrCoords;
	vec3 reflColor, refrColor, specular;
	float losMod;

	//vec3 ww = mix(texture2D(normalMap, gl_TexCoord[0].st).xzy, texture2D(normalMap, gl_TexCoord[0].st * 1.3).xzy, 0.5);

	vec3 ww = texture2D(normalMap, gl_TexCoord[0].st).xzy;
	vec3 ww2 = texture2D(normalMap2, gl_TexCoord[0].st).xzy;
	ww = mix(ww, ww2, mod(time * 60, 8.0) / 8.0);
	//ww = time.xxx;
	n = normalize(ww - vec3(0.5, 0.5, 0.5));

	l = -sunDir;
	v = normalize(cameraPos - worldPos);
	h = normalize(l + v);
	
	ndotl = dot(n, l);
	ndoth = dot(n, h);
	ndotv = dot(n, v);

	// Don't change these two. They should match the values in the config (TODO: dec uniforms).
	float zNear = 2.0;
	float zFar = 4096.0;

	float z_b = texture2D(depthTex, gl_FragCoord.xy).x;
	float z_n = 2.0 * z_b - 1.0;
	float waterDepth2 = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));

	waterDepth2 = gl_FragCoord.z / waterDepth2;
	
	fresnel = pow(1.0 - ndotv, 0.8);	// A rather random Fresnel approximation
	
	refrCoords = (0.5*gl_TexCoord[2].xy - 0.8*waviness*n.xz) / gl_TexCoord[2].w + 0.5;	// Unbias texture coords
	reflCoords = (0.5*gl_TexCoord[1].xy + waviness*n.xz) / gl_TexCoord[1].w + 0.5;	// Unbias texture coords
	
	reflColor = mix(texture2D(reflectionMap, reflCoords).rgb, sunColor * reflectionTint, 
					reflectionTintStrength);
	
	refrColor = (0.5 + 0.5*ndotl) * mix(texture2D(refractionMap, refrCoords).rgb, sunColor * tint,
					murkiness * clamp(waterDepth2 / fullDepth, 0.0, 1.0)); // Murkiness and tint at this pixel (tweaked based on lighting and depth)
	
	specular = pow(max(0.0, ndoth), shininess) * sunColor * specularStrength;

	losMod = texture2D(losMap, gl_TexCoord[3].st).a;

#if USE_SHADOW
	float shadow = get_shadow(vec4(v_shadow.xy - 8*waviness*n.xz, v_shadow.zw));
	float fresShadow = mix(fresnel, fresnel*shadow, dot(sunColor, vec3(0.16666)));
#else
	float fresShadow = fresnel;
#endif
	
	vec3 colour = mix(refrColor + 0.3*specular, reflColor + specular, fresShadow);

	gl_FragColor.rgb = colour * losMod;
	
	// Make alpha vary based on both depth (so it blends with the shore) and view angle (make it
	// become opaque faster at lower view angles so we can't look "underneath" the water plane)
	t = 18.0 * max(0.0, 0.7 - v.y);
	gl_FragColor.a = 0.15 * waterDepth * (1.2 + t + fresnel);
}
