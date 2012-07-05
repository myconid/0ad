!!ARBfp1.0
#if USE_FP_SHADOW
  OPTION ARB_fragment_program_shadow;
#endif

ATTRIB v_tex = fragment.texcoord[0];
ATTRIB v_shadow = fragment.texcoord[1];
ATTRIB v_los = fragment.texcoord[2];

#if USE_OBJECTCOLOR
  PARAM objectColor = program.local[0];
#else
#if USE_PLAYERCOLOR
  PARAM playerColor = program.local[0];
#endif
#endif

PARAM shadingColor = program.local[1];
PARAM ambient = program.local[2];

#if USE_FP_SHADOW && USE_SHADOW_PCF
  PARAM shadowScale = program.local[3];
  TEMP offset, size, weight;
#endif

#if USE_SPECULAR || USE_SPECULAR_MAP || USE_NORMAL_MAP || USE_PARALLAX_MAP
  ATTRIB v_normal = fragment.texcoord[3];
  ATTRIB v_half = fragment.texcoord[4];
    #if USE_INSTANCING && (USE_NORMAL_MAP || USE_PARALLAX_MAP)
    ATTRIB v_tangent = fragment.texcoord[5];
    ATTRIB v_bitangent = fragment.texcoord[6];
    ATTRIB v_sunDir = fragment.texcoord[7];
  #endif
    //#if USE_INSTANCING && USE_PARALLAX_MAP
    //ATTRIB v_eyeVec = fragment.texcoord[8];
	//#endif

  PARAM specularPower = program.local[4];
  PARAM specularColor = program.local[5];
  PARAM sunColor = program.local[6];
#endif

#if USE_INSTANCING && USE_AO
  ATTRIB v_texAO = fragment.texcoord[9];
#endif

TEMP tex;
TEMP texdiffuse;
TEMP sundiffuse;
TEMP temp;
TEMP color;
TEMP shadow;

TEX tex, v_tex, texture[0], 2D;

// Alpha-test as early as possible
#ifdef REQUIRE_ALPHA_GEQUAL
  SUB temp.x, tex.a, REQUIRE_ALPHA_GEQUAL;
  KIL temp.x; // discard if < 0.0
#endif

#if USE_TRANSPARENT
  MOV result.color.a, tex;
#endif

// Apply coloring based on texture alpha
#if USE_OBJECTCOLOR
  LRP temp.rgb, objectColor, 1.0, tex.a;
  MUL texdiffuse.rgb, tex, temp;
#else
#if USE_PLAYERCOLOR
  LRP temp.rgb, playerColor, 1.0, tex.a;
  MUL texdiffuse.rgb, tex, temp;
#else
  MOV texdiffuse.rgb, tex;
#endif
#endif


  MUL sundiffuse.rgb, fragment.color, 2.0;


#if USE_NORMAL_MAP || USE_SPECULAR || USE_SPECULAR_MAP
  // specular = sunColor * specularColor * pow(max(0.0, dot(normalize(v_normal), v_half)), specularPower);
  TEMP specular;
  TEMP normal;
  MOV normal, v_normal;
  #if USE_NORMAL_MAP
	TEMP tangent;
	MOV tangent, v_tangent;

	TEMP sign;
	MOV sign, tangent.w;

  //  mat3 tbn = transpose(mat3(v_tangent.xyz, v_bitangent * -sign, v_normal));
	TEMP prod;
	MUL prod, v_bitangent, -sign;

	TEMP tbn0;
	TEMP tbn1;
	TEMP tbn2;

	MOV tbn0.x, tangent.x;
	MOV tbn0.y, prod.x;
	MOV tbn0.z, normal.x;

	MOV tbn1.x, tangent.y;
	MOV tbn1.y, prod.y;
	MOV tbn1.z, normal.y;

	MOV tbn2.x, tangent.z;
	MOV tbn2.y, prod.z;
	MOV tbn2.z, normal.z;
	
  //  vec3 ntex = texture2D(normTex, coord).rgb * 2.0 - 1.0;
  	TEMP ntex;
	TEX ntex, v_tex, texture[4], 2D;
	MAD ntex, ntex, 2.0, -1.0;
  //  normal = normalize(ntex * tbn);
	DP3 normal.x, ntex, tbn0;
	DP3 normal.y, ntex, tbn1;
	DP3 normal.z, ntex, tbn2;
	
	// Normalization.
	TEMP length;
	DP3 length.w, normal, normal;
	RSQ length.w, length.w;
	MUL normal, normal, length.w;
	
  //  vec3 sundiffuse = max(dot(-sunDir, normal), 0.0) * sunColor;
	TEMP DotDirNorm;
	DP3 DotDirNorm, -v_sunDir, normal;
	MAX DotDirNorm, DotDirNorm, 0.0;
	MUL sundiffuse, DotDirNorm, sunColor;
  #else
  	TEMP length;
	DP3 length.w, normal, normal;
	RSQ length.w, length.w;
	MUL normal, normal, length.w;
  #endif

#endif

#if USE_SPECULAR || USE_SPECULAR_MAP  
#if USE_SPECULAR
  MUL specular.rgb, specularColor, sunColor;
#else
	TEMP texSpecColor;
	TEX texSpecColor, v_tex, texture[3], 2D;
	MUL specular.rgb, texSpecColor, sunColor;
#endif
  DP3_SAT temp.y, normal, v_half;
  // temp^p = (2^lg2(temp))^p = 2^(lg2(temp)*p)
  LG2 temp.y, temp.y;
  MUL temp.y, temp.y, specularPower.x;
  EX2 temp.y, temp.y;
  // TODO: why not just use POW here? (should test performance first)
  MUL specular.rgb, specular, temp.y;
#endif

#if USE_INSTANCING && USE_AO
  TEMP ao;
  TEX ao, v_texAO, texture[5], 2D;
  ADD ao, ao, ao;
#endif

// color = (texdiffuse * sundiffuse + specular) * get_shadow() + texdiffuse * ambient;
// (sundiffuse is 2*fragment.color due to clamp-avoidance in the vertex program)
#if USE_SHADOW && !DISABLE_RECEIVE_SHADOWS
  #if USE_FP_SHADOW
    #if USE_SHADOW_PCF
      SUB offset.xy, v_shadow, 0.5;
      FRC offset.xy, offset;
      ADD size.xy, offset, 1.0;
      SUB size.zw, 2.0, offset.xyxy;
      RCP weight.x, size.x;
      RCP weight.y, size.y;
      RCP weight.z, size.z;
      RCP weight.w, size.w;
      SUB weight.xy, 2.0, weight;
      SUB weight.zw, weight, 1.0;

      SUB offset.xy, v_shadow, offset;
      MOV offset.z, v_shadow;
      ADD weight, weight, offset.xyxy;
      MUL weight, weight, shadowScale.zwzw;

      MOV offset.xy, weight.zwww;
      TEX temp.x, offset, texture[1], SHADOW2D;
      MOV offset.x, weight.x;
      TEX temp.y, offset, texture[1], SHADOW2D;
      MOV offset.xy, weight.zyyy;
      TEX temp.z, offset, texture[1], SHADOW2D;
      MOV offset.x, weight.x;
      TEX temp.w, offset, texture[1], SHADOW2D;

      MUL size, size.zxzx, size.wwyy;
      DP4 shadow.x, temp, size;
      MUL shadow.x, shadow.x, 0.111111;
    #else
      TEX shadow.x, v_shadow, texture[1], SHADOW2D;
    #endif
  #else
    TEX tex, v_shadow, texture[1], 2D;
    MOV_SAT temp.z, v_shadow.z;
    SGE shadow.x, tex.x, temp.z;
  #endif

  #if USE_SPECULAR || USE_SPECULAR_MAP
    MAD color.rgb, texdiffuse, sundiffuse, specular;
    MUL temp.rgb, texdiffuse, ambient;
    #if USE_INSTANCING && USE_AO
      MUL temp.rgb, ao, temp;
    #endif
    MAD color.rgb, color, shadow.x, temp;
  #else
    MAD temp.rgb, sundiffuse, shadow.x, ambient;
    #if USE_INSTANCING && USE_AO
      MUL temp.rgb, ao, temp;
    #endif
    MUL color.rgb, texdiffuse, temp;
  #endif
  
#else
  #if USE_SPECULAR || USE_SPECULAR_MAP
    MAD temp.rgb, fragment.color, 2.0, ambient;
    #if USE_INSTANCING && USE_AO
      MUL temp.rgb, ao, temp;
    #endif
    MAD color.rgb, texdiffuse, temp, specular;
  #else
    MAD temp.rgb, fragment.color, 2.0, ambient;
    #if USE_INSTANCING && USE_AO
      MUL temp.rgb, ao, temp;
    #endif
    MUL color.rgb, texdiffuse, temp;
  #endif
#endif

#if !IGNORE_LOS
  // Multiply everything by the LOS texture
  TEX tex.a, v_los, texture[2], 2D;
  MUL color.rgb, color, tex.a;
#endif

MUL result.color.rgb, color, shadingColor;

END
