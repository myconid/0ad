#ifndef INCLUDED_POSTPROCMANAGER
#define INCLUDED_POSTPROCMANAGER


#include "graphics/ShaderTechnique.h"

class CPostprocManager
{
public:
	
	GLuint m_PingFbo, m_PongFbo;
	GLuint m_ColourTex1, m_ColourTex2;
	GLuint m_DepthTex;
	GLuint m_BloomFbo, m_BlurTex2a, m_BlurTex2b, m_BlurTex4a, m_BlurTex4b, m_BlurTex8a, m_BlurTex8b;
	
	bool m_WhichBuffer;
	
	CStrW m_PostProcEffect;
	CShaderTechniquePtr m_PostProcTech;
	
	int m_Width, m_Height;
	
	bool m_IsInitialised;
	
	CPostprocManager();
	~CPostprocManager();
	
	void Initialize();
	
	void Cleanup();
	
	void RecreateBuffers();
	
	void LoadEffect(CStrW &name);
	
	void ApplyBloom();
	void ApplyBloomDownscale2x(GLuint inTex, GLuint outTex, int inWidth, int inHeight);
	void ApplyBloomBlur(GLuint inOutTex, GLuint tempTex, int inWidth, int inHeight);
	
	void ApplyEffect(CShaderTechniquePtr &shaderTech1, int pass);
	void ApplyPostproc();
	void ApplyLos();
	
	void CaptureRenderOutput();
	
	void PostprocStage1();
	void PostprocStage2();
	
	void ReleaseRenderOutput();
	
	std::vector<CStrW> GetPostEffects() const;
	
	inline const CStrW& GetPostEffect() const 
	{
		return m_PostProcEffect;
	}
	
	void SetPostEffect(CStrW name);
	

};


#endif //INCLUDED_POSTPROCMANAGER
