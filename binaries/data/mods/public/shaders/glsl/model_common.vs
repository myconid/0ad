#if USE_GPU_SKINNING
// Skinning requires GLSL 1.30 for ivec4 vertex attributes
#version 130
#else
#version 120
#endif

uniform mat4 transform;
uniform vec3 cameraPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec2 losTransform;
uniform mat4 shadowTransform;
uniform mat4 instancingTransform;

#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
  uniform vec4 shadowScale;
#endif

#if !USE_NORMAL_MAP
   varying vec3 v_lighting;
#endif
varying vec2 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;
#if USE_INSTANCING && USE_AO
  varying vec2 v_tex2;
#endif

#if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX_MAP
  varying vec3 v_normal;
  #if USE_INSTANCING && (USE_NORMAL_MAP || USE_PARALLAX_MAP)
    varying vec4 v_tangent;
    varying vec3 v_bitangent;
  #endif
  #if USE_SPECULAR || USE_SPECULAR_MAP
    varying vec3 v_half;
  #endif
  #if USE_INSTANCING && USE_PARALLAX_MAP
    varying vec3 v_eyeVec;
  #endif
#endif

attribute vec3 a_vertex;
attribute vec3 a_normal;
#if USE_INSTANCING
  attribute vec4 a_tangent;
#endif
attribute vec2 a_uv0;
attribute vec2 a_uv1;

#if USE_GPU_SKINNING
  const int MAX_INFLUENCES = 4;
  const int MAX_BONES = 64;
  uniform mat4 skinBlendMatrices[MAX_BONES];
  attribute ivec4 a_skinJoints;
  attribute vec4 a_skinWeights;
#endif


void main()
{
  #if USE_GPU_SKINNING
    vec3 p = vec3(0.0);
    vec3 n = vec3(0.0);
    for (int i = 0; i < MAX_INFLUENCES; ++i) {
      int joint = a_skinJoints[i];
      if (joint != 0xff) {
        mat4 m = skinBlendMatrices[joint];
        p += vec3(m * vec4(a_vertex, 1.0)) * a_skinWeights[i];
        n += vec3(m * vec4(a_normal, 0.0)) * a_skinWeights[i];
      }
    }
    vec4 position = instancingTransform * vec4(p, 1.0);
    vec3 normal = mat3(instancingTransform) * normalize(n);
  #else
  #if USE_INSTANCING
    vec4 position = instancingTransform * vec4(a_vertex, 1.0);
    mat3 normalMatrix = mat3(instancingTransform[0].xyz, instancingTransform[1].xyz, instancingTransform[2].xyz);
    vec3 normal = normalMatrix * a_normal;
    #if (USE_NORMAL_MAP || USE_PARALLAX_MAP)
      vec4 tangent = vec4(normalMatrix * a_tangent.xyz, a_tangent.w);
    #endif
  #else
    vec4 position = vec4(a_vertex, 1.0);
    vec3 normal = a_normal;
  #endif
  #endif

  gl_Position = transform * position;

  #if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX_MAP
    v_normal = normal;

    #if USE_INSTANCING && (USE_NORMAL_MAP || USE_PARALLAX_MAP)
	v_tangent = tangent;
	v_bitangent = cross(v_normal, v_tangent.xyz);
    #endif

    #if USE_SPECULAR || USE_SPECULAR_MAP || USE_PARALLAX_MAP
      vec3 eyeVec = cameraPos.xyz - position.xyz;
      #if USE_SPECULAR || USE_SPECULAR_MAP     
        vec3 sunVec = -sunDir;
        v_half = normalize(sunVec + normalize(eyeVec));
      #endif
      #if USE_INSTANCING && USE_PARALLAX_MAP
        v_eyeVec = eyeVec;
      #endif
    #endif
  #endif

  #if !USE_NORMAL_MAP
    v_lighting = max(0.0, dot(normal, -sunDir)) * sunColor;
  #endif

  v_tex = a_uv0;

  #if USE_INSTANCING && USE_AO
    v_tex2 = a_uv1;
  #endif

  #if USE_SHADOW
    v_shadow = shadowTransform * position;
    #if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
      v_shadow.xy *= shadowScale.xy;
    #endif  
  #endif

  v_los = position.xz * losTransform.x + losTransform.y;
}
