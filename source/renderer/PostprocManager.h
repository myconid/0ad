#ifndef INCLUDED_POSTPROCMANAGER
#define INCLUDED_POSTPROCMANAGER


#include "graphics/ShaderTechnique.h"

class CPostprocManager
{
public:
	
	GLuint m_PingFbo, m_PongFbo;
	GLuint m_ColourTex1, m_ColourTex2;
	GLuint m_DepthTex;
	GLuint m_BloomFbo, m_BloomTex1, m_BloomTex2;
	
	bool m_WhichBuffer;
	
	std::vector<std::vector<CShaderTechniquePtr> > renderStages;
	
	int m_Width, m_Height;
	
	bool m_IsInitialised;
	
	CPostprocManager();
	~CPostprocManager();
	
	void Initialize();
	
	void Cleanup();
	
	void RecreateBuffers();
	
	void LoadEffect(const char* name, int stage);
	
	void ApplyBloom();
	
	void ApplyEffect(CShaderTechniquePtr &shaderTech1);
	void ApplyPostproc(int stage);
	void ApplyLos();
	
	void CaptureRenderOutput();
	
	void PostprocStage1();
	void PostprocStage2();
	
	void ReleaseRenderOutput();
	
	
	

};


#endif //INCLUDED_POSTPROCMANAGER
