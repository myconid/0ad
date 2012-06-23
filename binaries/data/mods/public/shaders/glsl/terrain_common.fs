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

varying vec3 v_lighting;
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
varying vec2 v_blend;

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

#if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX_MAP
  varying vec3 v_normal;
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

void main()
{
  #if BLEND
    // Use alpha from blend texture
    gl_FragColor.a = 1.0 - texture2D(blendTex, v_blend).a;
  #endif

  vec2 coord = v_tex;

  #if USE_PARALLAX_MAP
  {
    float h = texture2D(normTex, coord).a;
    float sign = v_tangent.w;
    //mat3 tbn = transpose(mat3(v_tangent.xyz, v_bitangent * sign, v_normal));

    mat3 tbn = transpose(mat3(v_tangent.xyz, v_bitangent * sign, v_normal));

    vec3 eyeDir = normalize(tbn * v_eyeVec);
    float dist = length(v_eyeVec);

    if (dist < 100 && h < 0.99)
    {
      //float scale = 0.0075;
      float scale = 0.0175;

      if (dist > 65)
      {
        scale = scale * (100 - dist) / 35.0;
      }

      float height = 1.0;
      float iter;

      float s;
      vec2 move;
      iter = (75 - (dist - 25)) / 3 + 5;

      //if (iter > 15) iter = 15;

      vec3 tangEye = -eyeDir;

      //iter = mix(iter * 2, iter, tangEye.z);

      s = 1.0 / iter;
      move = vec2(-eyeDir.x, eyeDir.y) * scale / (eyeDir.z * iter);

      while (h < height) 
      {
        height -= s;
        coord += move;
        h = texture2D(normTex, coord).a;
      }

    }
  }
  #endif

  vec4 tex = texture2D(baseTex, coord);

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
    vec3 ntex = texture2D(normTex, coord).rgb * 2.0 - 1.0;
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
      vec4 s = texture2D(specTex, coord);
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
