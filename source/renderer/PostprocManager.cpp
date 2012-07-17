#include "precompiled.h"

#include "lib/ogl.h"
#include "maths/MathUtil.h"

#include "gui/GUIutil.h"
#include "lib/bits.h"
#include "ps/CLogger.h"

#include "graphics/ShaderManager.h"
#include "renderer/Renderer.h"

#include "renderer/PostprocManager.h"


PostprocManager::PostprocManager()
{
	isInitialised = false;
	pingFbo = 0;
	pongFbo = 0;
	colourTex1 = 0;
	colourTex2 = 0;
	depthTex = 0;
	stencilTex = 0;
	losTex = 0;	
	whichBuffer = true;
}


PostprocManager::~PostprocManager()
{
	Cleanup();
}

void PostprocManager::Initialize()
{
	RecreateBuffers();
	isInitialised = true;
	
	renderStages = std::vector<std::vector<CShaderTechniquePtr> >(5);
	
	//LoadEffect("ssao", 1);		

	
	//LoadEffect("fog", 1);
	LoadEffect("hdr",   1);
	//LoadEffect("bloom", 1);	
	
	//LoadEffect("los", 4);
}

void PostprocManager::Cleanup()
{
	if (isInitialised)
	{
		if (pingFbo) pglDeleteFramebuffersEXT(1, &pingFbo);
		if (pongFbo) pglDeleteFramebuffersEXT(1, &pongFbo);
		if (colourTex1) glDeleteTextures(1, &colourTex1);
		if (colourTex2) glDeleteTextures(1, &colourTex2);
		if (depthTex) glDeleteTextures(1, &depthTex);
		if (losTex) glDeleteTextures(1, &losTex);
	}
}


void PostprocManager::RecreateBuffers()
{
	Cleanup();
		
	
	width = g_Renderer.GetWidth();
	height = g_Renderer.GetHeight();

	
	glGenTextures(1, (GLuint*)&colourTex1);
	glBindTexture(GL_TEXTURE_2D, colourTex1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
		
	glGenTextures(1, (GLuint*)&colourTex2);
	glBindTexture(GL_TEXTURE_2D, colourTex2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	
	glGenTextures(1, (GLuint*)&stencilTex);
	glBindTexture(GL_TEXTURE_2D, stencilTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height,
		      0, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8,NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
			GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	
	/**glGenTextures(1, (GLuint*)&losTex);
	glBindTexture(GL_TEXTURE_2D, losTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
		  0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);*/

	/*glGenTextures(1, (GLuint*)&lumaTex);
	glBindTexture(GL_TEXTURE_2D, lumaTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16, width, height,
		  0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);*/
	
	glBindTexture(GL_TEXTURE_2D, 0);

	pglGenFramebuffersEXT(1, &pingFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, colourTex1, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, losTex, 0);
	//pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lumaTex, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencilTex, 0);
	
	GLenum status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Framebuffer object incomplete (A): 0x%04X", status);
	}
	
	pglGenFramebuffersEXT(1, &pongFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, colourTex2, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, losTex, 0);
	//pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lumaTex, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencilTex, 0);
	
	status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Framebuffer object incomplete (B): 0x%04X", status);
	}
	

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


void PostprocManager::CaptureRenderOutput()
{
	/*glBindTexture(GL_TEXTURE_2D, colourTex1);	
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, width, height, 0);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glCopyTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT, 0, 0, width, height, 0);*/
	//glBindTexture(GL_TEXTURE_2D, lumaTex);
	//glCopyTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE16, 0, 0, width, height, 0);
	
	
	// clear both FBOs and leave pingFbo selected for rendering; 
	// whichBuffer stays true at this point
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pongFbo);	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	pglDrawBuffers(1, buffers);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pingFbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	pglDrawBuffers(1, buffers);
	//glClear(GL_COLOR_BUFFER_BIT);
	
	whichBuffer = true;
}


void PostprocManager::ReleaseRenderOutput()
{
	//ApplyLos();
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	// we blit to screen from the previous buffer active
	if (whichBuffer)
		pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, pingFbo);
	else
		pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, pongFbo);
	
	pglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	pglBlitFramebufferEXT(0, 0, width, height, 0, 0, width, height, 
			      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);

	
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
}

void PostprocManager::LoadEffect(const char* name, int stage)
{
	CShaderTechniquePtr shaderTech1 = g_Renderer.GetShaderManager().LoadEffect(name);
	
	renderStages[stage].push_back(shaderTech1);
	
}

void PostprocManager::ApplyEffect(CShaderTechniquePtr &shaderTech1)
{
	///CaptureRenderOutput();
	
	// select the other FBO for rendering
	if (!whichBuffer)
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pingFbo);
	else
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pongFbo);
	
	
	
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	
	//glEnable (GL_BLEND);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shaderTech1->BeginPass();
	CShaderProgramPtr shader = shaderTech1->GetShader();
	
	shader->Bind();	
	
	// use the textures from the current FBO as input to the shader
	if (whichBuffer)
		shader->BindTexture("bgl_RenderedTexture", colourTex1);
	else
		shader->BindTexture("bgl_RenderedTexture", colourTex2);
	
	shader->BindTexture("bgl_DepthTexture", stencilTex);
	
	//shader->BindTexture("losTexPersp", losTex);
	
	shader->Uniform("bgl_RenderedTextureWidth", width);
	shader->Uniform("bgl_RenderedTextureHeight", height);
	
	glBegin(GL_QUADS);
	    glColor4f(1.f, 1.f, 1.f, 1.f);
	    glTexCoord2f(1.0, 1.0);	glVertex2f(1,1);
	    glTexCoord2f(0.0, 1.0);	glVertex2f(-1,1);	    
	    glTexCoord2f(0.0, 0.0);	glVertex2f(-1,-1);
	    glTexCoord2f(1.0, 0.0);	glVertex2f(1,-1);
	glEnd();
	
	shader->Unbind();
	
	shaderTech1->EndPass();	
		
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	
	whichBuffer = !whichBuffer;
}



void PostprocManager::ApplyPostproc(int stage)
{
	if (!isInitialised) 
		return;
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
	
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	pglDrawBuffers(1, buffers);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
	pglDrawBuffers(1, buffers);
	
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	
	std::vector<CShaderTechniquePtr> &effects = renderStages[stage];
	
	//std::cout << "EFFECT!!! " << effects.size() << std::endl;
	
	for (size_t i = 0; i < effects.size(); i++)
	{		
		ApplyEffect(effects[i]);
	}
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencilTex, 0);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencilTex, 0);
}


void PostprocManager::ApplyLos()
{
	
	
	CShaderTechniquePtr &shaderTech1 = renderStages[4][0];
		
	//CShaderProgramPtr shader = shaderTech1->GetShader();
	
	
	//ApplyEffect(shaderTech1);
	
	
	
	if (!whichBuffer)
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pingFbo);
	else
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pongFbo);
	
	
	
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	
	//glEnable (GL_BLEND);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	

	shaderTech1->BeginPass();
	CShaderProgramPtr shader = shaderTech1->GetShader();
	
	shader->Bind();	
	
	// use the textures from the current FBO as input to the shader
	if (whichBuffer)
		shader->BindTexture("bgl_RenderedTexture", colourTex1);
	else
		shader->BindTexture("bgl_RenderedTexture", colourTex2);
	
	//shader->BindTexture("bgl_DepthTexture", stencilTex);
	
	shader->BindTexture("losTexPersp", losTex);
	
	shader->Uniform("bgl_RenderedTextureWidth", width);
	shader->Uniform("bgl_RenderedTextureHeight", height);
	
	glBegin(GL_QUADS);
	    glColor4f(1.f, 1.f, 1.f, 1.f);
	    glTexCoord2f(1.0, 1.0);	glVertex2f(1,1);
	    glTexCoord2f(0.0, 1.0);	glVertex2f(-1,1);	    
	    glTexCoord2f(0.0, 0.0);	glVertex2f(-1,-1);
	    glTexCoord2f(1.0, 0.0);	glVertex2f(1,-1);
	glEnd();
	
	shader->Unbind();
	
	shaderTech1->EndPass();	
		
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	
	whichBuffer = !whichBuffer;
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, losTex, 0);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, losTex, 0);
		
}
