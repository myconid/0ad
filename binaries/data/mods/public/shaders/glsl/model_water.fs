#version 120

uniform sampler2D baseTex;
uniform sampler2D losTex;
uniform sampler2D aoTex;
uniform sampler2D normTex;
uniform sampler2D specTex;

uniform sampler2D waterTex;
uniform samplerCube skyCube;

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

#if USE_OBJECTCOLOR
  uniform vec3 objectColor;
#else
#if USE_PLAYERCOLOR
  uniform vec3 playerColor;
#endif
#endif

uniform vec3 shadingColor;
uniform vec3 ambient;
uniform vec3 sunColor;
uniform vec3 sunDir;
uniform vec3 cameraPos;






float shininess = 32.0;		// Blinn-Phong specular strength
float specularStrength = 2.0;	// Scaling for specular reflection (specular color is (this,this,this))
float waviness = 0.0;			// "Wildness" of the reflections and refractions; choose based on texture
vec3 tint = vec3(0.2,0.3,0.2);				// Tint for refraction (used to simulate particles in water)
float murkiness = 0.6;		// Amount of tint to blend in with the refracted colour
float fullDepth = 5.0;		// Depth at which to use full murkiness (shallower water will be clearer)
vec3 reflectionTint = vec3(0.3,0.4,0.3);	// Tint for reflection (used for really muddy water)
float reflectionTintStrength = 0.1;	// Strength of reflection tint (how much of it to mix in)
float waterDepth = 4.0;


/*varying vec3 v_lighting;

varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
#if USE_INSTANCING && USE_AO
  varying vec2 v_tex2;
#endif*/

#if USE_SPECULAR
  uniform float specularPower;
  uniform vec3 specularColor;
#endif

#if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX_MAP || USE_AO
  uniform vec4 effectSettings;
#endif

varying vec4 worldPos;
varying vec4 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;

/*#if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX_MAP
  varying vec3 v_normal;
  #if USE_INSTANCING && (USE_NORMAL_MAP || USE_PARALLAX_MAP)
    varying vec4 v_tangent;
    varying vec3 v_bitangent;
  #endif
  #if USE_SPECULAR || USE_SPECULAR_MAP
    varying vec3 v_half;
  #endif
  //#if USE_INSTANCING && USE_PARALLAX_MAP
    varying vec3 v_eyeVec;
  //#endif
#endif*/


float get_shadow()
{
  #if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
    #if USE_SHADOW_SAMPLER
      #if USE_SHADOW_PCF
        vec2 offset = fract(v_shadow.xy - 0.5);
        vec4 size = vec4(offset + 1.0, 2.0 - offset);
        vec4 weight = (vec4(2.0 - 1.0 / size.xy, 1.0 / size.zw - 1.0) + (v_shadow.xy - offset).xyxy) * shadowScale.zwzw;
        return (1.0/9.0)*dot(size.zxzx*size.wwyy,
          vec4(shadow2D(shadowTex, vec3(weight.zw, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.xw, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.zy, v_shadow.z)).r,
               shadow2D(shadowTex, vec3(weight.xy, v_shadow.z)).r));
      #else
        return shadow2D(shadowTex, v_shadow.xyz).r;
      #endif
    #else
      if (v_shadow.z >= 1.0)
        return 1.0;
      return (v_shadow.z <= texture2D(shadowTex, v_shadow.xy).x ? 1.0 : 0.0);
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

	n = normalize(texture2D(waterTex, v_tex.xy).xzy - vec3(0.5, 0.5, 0.5));
	l = -sunDir;
	v = normalize(cameraPos - worldPos.xyz);
	h = normalize(l + v);
	
	ndotl = dot(n, l);
	ndoth = dot(n, h);
	ndotv = dot(n, v);
	
	fresnel = pow(1.0 - ndotv, 0.8);	// A rather random Fresnel approximation
	
	//refrCoords = (0.5*gl_TexCoord[2].xy - 0.8*waviness*n.xz) / gl_TexCoord[2].w + 0.5;	// Unbias texture coords
	//reflCoords = (0.5*gl_TexCoord[1].xy + waviness*n.xz) / gl_TexCoord[1].w + 0.5;	// Unbias texture coords
	
	//vec3 dir = normalize(v + vec3(waviness*n.x, 0.0, waviness*n.z));

	vec3 eye = reflect(v, n);
	
	vec3 tex = textureCube(skyCube, eye).rgb;

	reflColor = mix(tex, sunColor * reflectionTint, 
					reflectionTintStrength);

	waterDepth = 4.0 + 2.0 * dot(abs(v_tex.zw - 0.5), vec2(0.5));
	
	//refrColor = (0.5 + 0.5*ndotl) * mix(texture2D(refractionMap, refrCoords).rgb, sunColor * tint,
	refrColor = (0.5 + 0.5*ndotl) * mix(vec3(0.3), sunColor * tint,
					murkiness * clamp(waterDepth / fullDepth, 0.0, 1.0)); // Murkiness and tint at this pixel (tweaked based on lighting and depth)
	
	specular = pow(max(0.0, ndoth), shininess) * sunColor * specularStrength;

	losMod = texture2D(losTex, v_los).a;

	gl_FragColor.rgb = mix(refrColor + 0.3*specular, reflColor + specular, fresnel) * losMod;
	
	// Make alpha vary based on both depth (so it blends with the shore) and view angle (make it
	// become opaque faster at lower view angles so we can't look "underneath" the water plane)
	t = 18.0 * max(0.0, 0.7 - v.y);
	gl_FragColor.a = 0.15 * waterDepth * (1.2 + t + fresnel);
}

/*void main2()
{
  vec2 coord = v_tex / 1.5;

  //vec3 eye = reflect(v_eyeVec, v_normal);

  //vec4 tex = vec4(0.3, 0.4, 0.45, 0.93);
  

  #if USE_SPECULAR || USE_SPECULAR_MAP || USE_NORMAL_MAP
    vec3 normal = v_normal;
  #endif

  #if USE_INSTANCING && USE_NORMAL_MAP
    float sign = v_tangent.w;
    mat3 tbn = (mat3(v_tangent.xyz, v_bitangent * -sign, v_normal));
    vec3 ntex = texture2D(waterTex, coord).rgb * 2.0 - 1.0;
    normal = normalize(tbn * ntex);
    vec3 bumplight = max(dot(-sunDir, normal), 0.0) * sunColor;
    vec3 sundiffuse = (bumplight - v_lighting) * effectSettings.x + v_lighting;
  #else
    vec3 sundiffuse = v_lighting;
  #endif

  vec4 specular = vec4(0.0);
  #if USE_SPECULAR || USE_SPECULAR_MAP
    vec3 specCol;
    float specPow;
    #if USE_INSTANCING && USE_SPECULAR_MAP
      vec4 s = texture2D(specTex, coord);
      specCol = s.rgb;
      specular.a = s.a;
      specPow = effectSettings.y;
    #else
      specCol = vec3(1.0);
      specPow = 16.0;
    #endif
    specular.rgb = sunColor * specCol * pow(max(0.0, dot(normalize(normal), v_half)), specPow);
  #endif

  vec3 eye = reflect(v_eyeVec, normal);

  vec4 tex = textureCube(skyCube, eye);

  float ndotv = dot(normal, eye);

  float fresnel = pow(1.0 - ndotv, 0.8);	// A rather random Fresnel approximation
  float t = 18.0 * max(0.0, 0.7 - eye.y);
  tex.a = 0.15 * 2.5 * (1.2 + t + fresnel);
  tex.a = 0.92;

  

  // Alpha-test as early as possible
  #ifdef REQUIRE_ALPHA_GEQUAL
    if (tex.a < REQUIRE_ALPHA_GEQUAL)
      discard;
  #endif

  #if USE_TRANSPARENT
    gl_FragColor.a = tex.a;
  #else
    gl_FragColor.a = 1.0;
  #endif
  
  vec3 texdiffuse = tex.rgb;

  // Apply-coloring based on texture alpha
  #if USE_OBJECTCOLOR
    texdiffuse *= mix(objectColor, vec3(1.0, 1.0, 1.0), tex.a);
  #else
  #if USE_PLAYERCOLOR
    texdiffuse *= mix(playerColor, vec3(1.0, 1.0, 1.0), tex.a);
  #endif
  #endif


  vec3 color = (texdiffuse * sundiffuse + specular.rgb) * get_shadow();
  vec3 ambColor = texdiffuse * ambient;

  color += ambColor;

  vec3 refrColor = vec3(0.4);
  vec3 reflColor = tex;

  //vec3 color = mix(refrColor + 0.3*specular, reflColor + specular, fresnel);

  #if !IGNORE_LOS
    float los = texture2D(losTex, v_los).a;
    color *= los;
  #endif

  color *= shadingColor;

  gl_FragColor.rgb = color;

gl_FragColor.rgb = fresnel;
  //gl_FragColor.rgb = tex;
}
*/