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

#if USE_WIND
  uniform vec4 time;
  uniform vec4 windData;
#endif

#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
  uniform vec4 shadowScale;
#endif

/*varying vec3 v_lighting;
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
  //#if USE_INSTANCING && USE_PARALLAX_MAP
    varying vec3 v_eyeVec;
  //#endif
#endif*/

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


varying vec4 worldPos;
varying vec4 v_tex;
varying vec4 v_shadow;
varying vec2 v_los;


vec4 fakeCos(vec4 x)
{
	vec4 tri = abs(fract(x + 0.5) * 2.0 - 1.0);
	return tri * tri *(3.0 - 2.0 * tri);  
}



void main()
{
	worldPos = instancingTransform * vec4(a_vertex, 1.0);
	
	//worldPos = a_vertex.xyz;
	//waterDepth = dot(a_encodedDepth.xyz, vec3(255.0, -255.0, 1.0));
	//gl_TexCoord[0].st = a_vertex.xz*repeatScale + translation;
	v_tex.xy = (a_uv0 + worldPos.xz) / 5.0;

	v_tex.zw = a_uv0;

	//gl_TexCoord[1] = reflectionMatrix * vec4(a_vertex, 1.0);		// projective texturing
	//gl_TexCoord[2] = refractionMatrix * vec4(a_vertex, 1.0);
	//gl_TexCoord[3] = losMatrix * vec4(a_vertex, 1.0);

	#if USE_SHADOW
		v_shadow = shadowTransform * worldPos;
		#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
			v_shadow.xy *= shadowScale.xy;
		#endif  
	#endif

	v_los = worldPos.xz * losTransform.x + losTransform.y;

	gl_Position = transform * worldPos;
}


/*
void main2()
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

  #if USE_WIND

    vec2 wind = windData.xy;

    // fractional part of model position, clamped to >.4
    vec4 modelPos = instancingTransform[3];
    modelPos = fract(modelPos);
    modelPos = clamp(modelPos, 0.4, 1);

    // crude measure of wind intensity
    float abswind = abs(wind.x) + abs(wind.y);

    vec4 cosVec;
    // these determine the speed of the wind's "cosine" waves.
    cosVec.x = time.x * modelPos[0] + position.x;
    cosVec.y = time.x * modelPos[2] / 3 + instancingTransform[3][0];
    cosVec.z = time.x * abswind / 4 + position.z;

    // calculate "cosines" in parallel, using a smoothed triangle wave
    cosVec = fakeCos(cosVec);

    float limit = clamp((a_vertex.x * a_vertex.z * a_vertex.y) / 3000, 0, 0.2);

    float diff = cosVec.x * limit; 
    float diff2 = cosVec.y * clamp(a_vertex.y / 60, 0, 0.25);

    // fluttering of model parts based on distance from model center (ie longer branches)
    position.xyz += cosVec.z * limit * clamp(abswind, 1.2, 1.7);

    // swaying of trunk based on distance from ground (higher parts sway more)
    position.xz += diff + diff2 * wind;
    
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
      //#if USE_INSTANCING && USE_PARALLAX_MAP
        v_eyeVec = eyeVec;
      //#endif
    #endif
  #endif

  v_lighting = max(0.0, dot(normal, -sunDir)) * sunColor;

  v_tex = (a_uv0 + position.xz) / 5.0;

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
*/