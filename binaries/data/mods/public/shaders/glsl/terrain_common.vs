#version 120

uniform mat4 transform;
uniform vec3 cameraPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec2 textureTransform;
uniform vec2 losTransform;
uniform mat4 shadowTransform;

#if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
  uniform vec4 shadowScale;
#endif

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


#if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX_MAP || USE_TRIPLANAR
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

/*#if USE_SPECULAR
  varying vec3 v_normal;
  varying vec3 v_half;
#endif*/

attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec3 a_color;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

void main()
{
  vec4 position = vec4(a_vertex, 1.0);


  #if USE_GRASS
    #if BLEND
      return;
    #endif

    //position.xyz = a_vertex + (a_normal * 0.025 * LAYER);
    position.y = a_vertex.y + (a_normal.y * 0.035 * LAYER);
    position.x = a_vertex.x + (0.005 * LAYER);
  #endif

  gl_Position = transform * position;

  #if !USE_NORMAL_MAP
    v_lighting = a_color * sunColor;
  #endif
  
  #if DECAL
    v_tex.xy = a_uv0;
  #else
    // Compute texcoords from position and terrain-texture-dependent transform
    float c = textureTransform.x;
    float s = -textureTransform.y;
    ///v_tex = vec2(a_vertex.x * c + a_vertex.z * -s, a_vertex.x * -s + a_vertex.z * -c);

    #if USE_TRIPLANAR
      //v_tex = vec3(a_vertex.x * c + a_vertex.z * -s, a_vertex.y, a_vertex.x * -s + a_vertex.z * -c);
      v_tex = a_vertex;
    #else
      v_tex = vec2(a_vertex.x * c + a_vertex.z * -s, a_vertex.x * -s + a_vertex.z * -c) / 2;
    #endif

    ///v_tex = a_vertex;

    #if GL_ES
      // XXX: Ugly hack to hide some precision issues in GLES
      v_tex = mod(v_tex, vec2(9.0, 9.0));
    #endif
  #endif

  #if BLEND
    v_blend = a_uv1;
  #endif

  #if USE_SHADOW
    v_shadow = shadowTransform * vec4(a_vertex, 1.0);
    #if USE_SHADOW_SAMPLER && USE_SHADOW_PCF
      v_shadow.xy *= shadowScale.xy;
    #endif  
  #endif

  /*#if USE_SPECULAR
    // TODO: for proper specular terrain, we need to provide vertex normals.
    // But we don't have that yet, so do something wrong instead.
    vec3 normal = vec3(0, 1, 0);

    vec3 eyeVec = normalize(cameraPos.xyz - position.xyz);
    vec3 sunVec = -sunDir;
    v_half = normalize(sunVec + eyeVec);
    v_normal = normal;
  #endif*/
  //v_normal = a_normal;

  #if USE_SPECULAR || USE_NORMAL_MAP || USE_SPECULAR_MAP || USE_PARALLAX_MAP || USE_TRIPLANAR
    ///v_normal = vec3(0,1,0);
    v_normal = a_normal;

    #if USE_NORMAL_MAP || USE_PARALLAX_MAP

	/*vec3 tangent; 
	vec3 binormal; 
	
	vec3 c1 = cross(a_normal, vec3(0.0, 0.0, 1.0)); 
	vec3 c2 = cross(a_normal, vec3(0.0, 1.0, 0.0)); 
	
	if(length(c1)>length(c2))
	{
		tangent = c1;	
	}
	else
	{
		tangent = c2;	
	}
	
	tangent = normalize(tangent);
	
	binormal = cross(a_normal, tangent); 
	binormal = normalize(binormal);

	v_tangent = vec4(tangent,1);
	v_bitangent = binormal;*/

	vec3 t = vec3(1.0, 0.0, 0.0);
	//vec3 v = vec3(0.0, 0.0, 1.0);
	t = normalize(t - v_normal * dot(v_normal, t));
	v_tangent = vec4(t, -1.0);
	v_bitangent = cross(v_normal, t);


	/*vec3 t = vec3(1.0, 0.0, 0.0);
	vec3 v = vec3(0.0, 0.0, 1.0);
	//t = normalize(t - v_normal * dot(v_normal, t));
	//v_tangent = vec4(t, -1.0);
	//v_bitangent = cross(v_normal, t);
	v_tangent = vec4(cross(v_normal, t), 1.0);
	//v_bitangent = cross(v_normal, v_tangent.xyz);
	v_bitangent = cross(v_normal, v_tangent);*/
	//v_tangent = vec4(1,0,0,1);
	//v_bitangent = vec3(0,0,1);
    #endif

    #if USE_SPECULAR || USE_SPECULAR_MAP || USE_PARALLAX_MAP
      vec3 eyeVec = cameraPos.xyz - position.xyz;
      #if USE_SPECULAR || USE_SPECULAR_MAP
        vec3 sunVec = -sunDir;
        v_half = normalize(sunVec + normalize(eyeVec));
      #endif
      #if USE_PARALLAX_MAP
        v_eyeVec = eyeVec;
      #endif
    #endif
  #endif

  v_los = a_vertex.xz * losTransform.x + losTransform.yy;
}
