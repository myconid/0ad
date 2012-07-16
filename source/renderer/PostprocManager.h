#ifndef INCLUDED_POSTPROCMANAGER
#define INCLUDED_POSTPROCMANAGER


#include "graphics/ShaderTechnique.h"

class PostprocManager
{
public:
	
	GLuint pingFbo, pongFbo;
	GLuint colourTex1, colourTex2;
	GLuint depthTex, stencilTex;
	GLuint losTex;
	
	GLuint bloomTex;
	
	bool whichBuffer;
	
	std::vector<std::vector<CShaderTechniquePtr> > renderStages;
	
	int width, height;
	
	bool isInitialised;
	
	PostprocManager();
	~PostprocManager();
	
	void Initialize();
	
	void Cleanup();
	
	void RecreateBuffers();
	
	void LoadEffect(const char* name, int stage);
	
	void ApplyEffect(CShaderTechniquePtr &shaderTech1);
	void ApplyPostproc(int stage);
	void ApplyLos();
	
	void CaptureRenderOutput();
	
	void PostprocStage1();
	void PostprocStage2();
	
	void ReleaseRenderOutput();
	
	
	

};


#endif //INCLUDED_POSTPROCMANAGER
