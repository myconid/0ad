#version 120

uniform sampler2D baseTex;
uniform sampler2D blendTex;
uniform sampler2D losTex;
uniform sampler2D normTex;
uniform sampler2D specTex;

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

uniform vec3 shadingColor;
uniform vec3 ambient;
uniform vec3 sunColor;
uniform vec3 sunDir;

uniform vec2 textureTransform;

#if !USE_NORMAL_MAP
   varying vec3 v_lighting;
#endif

varying vec4 v_shadow;
varying vec2 v_los;
varying vec2 v_blend;

#if USE_TRIPLANAR
  varying vec3 v_tex;
#else
  varying vec2 v_tex;
#endif


/*#if USE_SPECULAR
  uniform float specularPower;
  uniform vec3 specularColor;
  varying vec3 v_normal;
  varying vec3 v_half;
#endif*/

#if USE_SPECULAR || USE_SPECULAR_MAP
  uniform float specularPower;
  #if USE_SPECULAR
    uniform vec3 specularColor;
  #endif
#endif

varying vec3 v_normal;

#if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX_MAP
  #if USE_NORMAL_MAP || USE_PARALLAX_MAP
    varying vec4 v_tangent;
    varying vec3 v_bitangent;
  #endif
  #if USE_SPECULAR || USE_SPECULAR_MAP
    varying vec3 v_half;
  #endif
  #if USE_PARALLAX_MAP
    varying vec3 v_eyeVec;
  #endif
#endif

float get_shadow()
{
  #if USE_SHADOW
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

/*vec4 triplanar1(sampler2D samp, vec3 wpos, float scale)
{
  //float tighten = 0.4679f;
  float tighten = 0.379f;


vec3 v = 0;  
v += texture2D(samp, wpos.xz * 3.97) * 1.00;  
v += texture2D(samp, wpos.zy * 8.06) * 0.50;  
v += texture2D(samp, wpos.yx * 15.96) * 0.25;  
//N = normalize(N + v);  

  //vec3 nabs = v_normal * v_normal - vec3(tighten);

  vec3 nabs = abs(normalize(v_normal)) - tighten;
  nabs = max(nabs, 0);

  nabs /= vec3(nabs.x + nabs.y + nabs.z);
  //nabs = (nabs - 0.2) * 7; 


  float c = textureTransform.x;
  float s = -textureTransform.y;

  vec2 tr = vec2(wpos.x * c + wpos.y * -s, wpos.x * -s + wpos.y * -c);
  vec4 cXY = texture2D(samp, tr);

  tr = vec2(wpos.x * c + wpos.z * -s, wpos.x * -s + wpos.z * -c);
  vec4 cXZ = texture2D(samp, tr);

  tr = vec2(wpos.y * c + wpos.z * -s, wpos.y * -s + wpos.z * -c);
  vec4 cYZ = texture2D(samp, tr); 


  //return cXY * nabs.z + cXZ * nabs.y + cYZ * nabs.x;
  return cXY * nabs.z + cXZ * nabs.y + cYZ * nabs.x;  

}*/

#if USE_TRIPLANAR
vec2 getcoord(vec2 wpos)
{
	float c = textureTransform.x;
	float s = -textureTransform.y;
	return vec2(wpos.x * c + wpos.y * -s, wpos.x * -s + wpos.y * -c);
}

vec4 triplanar(sampler2D sampler, vec3 wpos)
{
	//vec3 blending = (abs( v_normal ) - 0.2) * 7.0;

	//vec3 blending = (abs( v_normal ) - 0.4679f);
	//blending = normalize(max(blending, 0.0));

	float tighten = 0.4679f;

	vec3 blending = abs(normalize(v_normal)) - tighten;
	blending = max(blending, 0.0);

	blending /= vec3(blending.x + blending.y + blending.z);

	vec3 signedBlending = sign(v_normal) * blending;

	vec3 coords = wpos;// * 1.0 / 32.0;
	//coords.xz = getcoord(coords.xz);
	coords.xyz /= 32.0;

	/**vec4 col1 = texture2D(sampler, coords.zy);
	vec4 col2 = texture2D(sampler, coords.zx);
	vec4 col3 = texture2D(sampler, coords.yx);*/

	/**vec4 col1 = texture2D(sampler, coords.zy);
	vec4 col2 = texture2D(sampler, coords.zx);
	vec4 col3 = texture2D(sampler, coords.yx);*/

	vec4 col1 = texture2D(sampler, coords.yz);
	vec4 col2 = texture2D(sampler, coords.zx);
	vec4 col3 = texture2D(sampler, coords.yx);

	// Blend the results of the 3 planar projections.
	///vec4 col1 = texture2D( sampler, coords.yz / 32.0 );
	///////vec4 col1 = texture2D( sampler, coords.zy / 32.0 );
	//vec4 col2 = texture2D( sampler, getcoord(coords.zx) );
	///////vec4 col2 = texture2D( sampler, coords.zx / 32.0 );
	//vec4 col3 = texture2D( sampler, coords.xy / 32.0 );
	///////vec4 col3 = texture2D( sampler, coords.yx / 32.0 );
	vec4 colBlended = col1 * blending.x + col2 * blending.y + col3 * blending.z;
	//vec4 colBlended = color;

	return colBlended;
}

vec4 triplanarNormals(sampler2D sampler, vec3 wpos)
{
	//vec3 blending = (abs( v_normal ) - 0.2) * 7.0;

	//vec3 blending = (abs( v_normal ) - 0.4679f);
	//blending = normalize(max(blending, 0.0));

	float tighten = 0.4679f;

	vec3 blending = abs(normalize(v_normal)) - tighten;
	blending = max(blending, 0.0);

	blending /= vec3(blending.x + blending.y + blending.z);

	vec3 signedBlending = sign(v_normal) * blending;

	vec3 coords = wpos;// * 1.0 / 32.0;
	//coords.xz = getcoord(coords.xz);
	coords.xyz /= 32.0;

	/**vec4 col1 = texture2D(sampler, coords.zy);
	vec4 col2 = texture2D(sampler, coords.zx);
	vec4 col3 = texture2D(sampler, coords.yx);*/

	/**vec4 col1 = texture2D(sampler, coords.zy);
	vec4 col2 = texture2D(sampler, coords.zx);
	vec4 col3 = texture2D(sampler, coords.yx);*/

	vec4 col1 = texture2D(sampler, coords.yz).xyzw;
	col1.y = 1.0 - col1.y;
	vec4 col2 = texture2D(sampler, coords.zx).yxzw;
	col2.y = 1.0 - col2.y;
	vec4 col3 = texture2D(sampler, coords.yx).yxzw;
	col3.y = 1.0 - col3.y;

	// Blend the results of the 3 planar projections.
	///vec4 col1 = texture2D( sampler, coords.yz / 32.0 );
	///////vec4 col1 = texture2D( sampler, coords.zy / 32.0 );
	//vec4 col2 = texture2D( sampler, getcoord(coords.zx) );
	///////vec4 col2 = texture2D( sampler, coords.zx / 32.0 );
	//vec4 col3 = texture2D( sampler, coords.xy / 32.0 );
	///////vec4 col3 = texture2D( sampler, coords.yx / 32.0 );
	vec4 colBlended = col1 * blending.x + col2 * blending.y + col3 * blending.z;
	//vec4 colBlended = color;

	return colBlended;
}
#endif




/*vec4 triplanar2(sampler2D samp, vec3 wpos, float scale)
{
	vec3 tpweights = abs(normalize(v_normal)) - 0.4679f;
        //tpweights = (tpweights - 0.2) * 7.0;
        tpweights = max(tpweights, vec3(0.0));
        tpweights /= tpweights.x + tpweights.y + tpweights.z;

	vec3 signs = sign(v_normal);
        
        vec2 tpcoord1 = vec2(vec2(-signs.x * v_tex.y, v_tex.z) * scale);
        vec2 tpcoord2 = vec2(vec2(signs.y * v_tex.x, -v_tex.z) * scale);
        vec2 tpcoord3 = vec2(vec2(signs.z * v_tex.x, v_tex.y) * scale);

	vec4 cXY = texture2D(samp, tpcoord1);
	vec4 cXZ = texture2D(samp, tpcoord2);
	vec4 cYZ = texture2D(samp, tpcoord3);

	vec4 ddc = vec4(0.0);
        ddc += tpweights.x * cXY;
        ddc += tpweights.y * cXZ;
        ddc += tpweights.z * cYZ;
	
	return ddc;

}*/

void main()
{
  #if BLEND
    #if USE_GRASS
      discard;
    #endif

    // Use alpha from blend texture
    gl_FragColor.a = 1.0 - texture2D(blendTex, v_blend).a;
  #endif

  //vec2 coord = v_tex.xy;

  /*float scale = 1.0 / 32.0;


  vec3 nabs = v_normal * v_normal;

  float scale = 1.0 / 32.0;

  vec4 cXY = texture2D(baseTex, v_tex.xy * scale);
  vec4 cXZ = texture2D(baseTex, v_tex.xz * scale);
  vec4 cYZ = texture2D(baseTex, v_tex.yz * scale);

  vec4 tex = cXY * nabs.z + cXZ * nabs.y + cYZ * nabs.x; */

  //float uvScale = 1.0 / 32.0;

  #if USE_TRIPLANAR
    vec4 tex = triplanar(baseTex, v_tex);
  #else
    vec4 tex = texture2D(baseTex, v_tex.xy);
  #endif


  //vec4 tex = texture2D(baseTex, coord);

  #if DECAL
    // Use alpha from main texture
    gl_FragColor.a = tex.a;
  #endif

  vec3 texdiffuse = tex.rgb;
  //vec3 sundiffuse = v_lighting;

  #if USE_SPECULAR || USE_SPECULAR_MAP || USE_NORMAL_MAP
    vec3 normal = v_normal;
  #endif

  #if USE_NORMAL_MAP
    float sign = v_tangent.w;
    mat3 tbn = transpose(mat3(v_tangent.xyz, v_bitangent * -sign, v_normal));

    #if USE_TRIPLANAR
      vec3 ntex = triplanarNormals(normTex, v_tex).rgb * 2.0 - 1.0;
    #else
      vec3 ntex = texture2D(normTex, v_tex).rgb * 2.0 - 1.0;
    #endif

    normal = normalize(ntex * tbn);
    vec3 sundiffuse = max(dot(-sunDir, normal), 0.0) * sunColor;
  #else
    vec3 sundiffuse = v_lighting;
  #endif

  vec4 specular = vec4(0.0);
  #if USE_SPECULAR || USE_SPECULAR_MAP
    vec3 specCol;
    #if USE_SPECULAR
      specCol = specularColor;
    #else
      #if USE_TRIPLANAR
        vec4 s = triplanar(specTex, v_tex);
      #else
        vec4 s = texture2D(specTex, v_tex);
      #endif
      specCol = s.rgb;
      specular.a = s.a;
    #endif
    specular.rgb = sunColor * specCol * pow(max(0.0, dot(normalize(normal), v_half)), specularPower);
  #endif

  /*#if USE_SPECULAR
    // Interpolated v_normal needs to be re-normalized since it varies
    // significantly between adjacenent vertexes;
    // v_half changes very gradually so don't bother normalizing that
    vec3 specular = specularColor * pow(max(0.0, dot(normalize(v_normal), v_half)), specularPower);
  #else
    vec3 specular = vec3(0.0);
  #endif*/

  vec3 color = (texdiffuse * sundiffuse + specular.rgb) * get_shadow() + texdiffuse * ambient;

  #if USE_SPECULAR_MAP && USE_SELF_LIGHT
    color = mix(texdiffuse, color, specular.a);
  #endif

  float los = texture2D(losTex, v_los).a;
  color *= los;

  #if DECAL
    color *= shadingColor;
  #endif

  gl_FragColor.rgb = color;

  #if USE_GRASS
    gl_FragColor.a = tex.a;
  #endif

  #if USE_SPECULAR_MAP
	//gl_FragColor.rgb =  v_half;
   //gl_FragColor.rgb = specular.rgb;
	//gl_FragColor.r = 1.0;
    //gl_FragColor.rgb = abs(v_normal);
    //gl_FragColor.rgb = abs(v_tangent.xyz);
    //gl_FragColor.rgb = abs(v_bitangent.xyz);
    //gl_FragColor.rgb = abs(v_eyeVec) / 200;
  #endif
}
