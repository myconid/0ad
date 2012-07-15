#include "precompiled.h"

#include "lib/ogl.h"
#include "maths/MathUtil.h"

#include "gui/GUIutil.h"
#include "lib/bits.h"
#include "ps/CLogger.h"
#include "ps/World.h"

#include "graphics/LightEnv.h"
#include "graphics/ShaderManager.h"

#include "renderer/PostprocManager.h"
#include "renderer/Renderer.h"


CPostprocManager::CPostprocManager()
	: m_IsInitialised(false), m_PingFbo(0), m_PongFbo(0), m_ColourTex1(0), m_ColourTex2(0), 
	  m_DepthTex(0), m_BloomFbo(0), m_BloomTex1(0), m_BloomTex2(0), m_WhichBuffer(true)
{
}

CPostprocManager::~CPostprocManager()
{
	Cleanup();
}

void CPostprocManager::Initialize()
{
	RecreateBuffers();
	m_IsInitialised = true;
	
	renderStages = std::vector<std::vector<CShaderTechniquePtr> >(5);
	
	//LoadEffect("ssao", 1);		

	
	//LoadEffect("fog", 1);
	
	
	LoadEffect("hdr",   1);
	//LoadEffect("bloom", 1);	
	
	//LoadEffect("los", 4);
}

void CPostprocManager::Cleanup()
{
	if (m_IsInitialised)
	{
		if (m_PingFbo) pglDeleteFramebuffersEXT(1, &m_PingFbo);
		if (m_PongFbo) pglDeleteFramebuffersEXT(1, &m_PongFbo);
		if (m_BloomFbo) pglDeleteFramebuffersEXT(1, &m_BloomFbo);
		
		if (m_ColourTex1) glDeleteTextures(1, &m_ColourTex1);
		if (m_ColourTex2) glDeleteTextures(1, &m_ColourTex2);
		if (m_DepthTex) glDeleteTextures(1, &m_DepthTex);
		
		if (m_BloomTex1) glDeleteTextures(1, &m_BloomTex1);
		if (m_BloomTex2) glDeleteTextures(1, &m_BloomTex2);
	}
}


void CPostprocManager::RecreateBuffers()
{
	Cleanup();
		
	
	m_Width = g_Renderer.GetWidth();
	m_Height = g_Renderer.GetHeight();
	
	
	#define GEN_BUFFER_RGBA(name, w, h) \
		glGenTextures(1, (GLuint*)&name); \
		glBindTexture(GL_TEXTURE_2D, name); \
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, \
			GL_UNSIGNED_BYTE, 0); \
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); \
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); \
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); \
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	GEN_BUFFER_RGBA(m_ColourTex1, m_Width, m_Height);
	GEN_BUFFER_RGBA(m_ColourTex2, m_Width, m_Height);
	
	GEN_BUFFER_RGBA(m_BloomTex1, m_Width / 4, m_Height / 4);
	GEN_BUFFER_RGBA(m_BloomTex2, m_Width / 4, m_Height / 4);
	
	#undef GEN_BUFFER_RGBA
	
	glGenTextures(1, (GLuint*)&m_DepthTex);
	glBindTexture(GL_TEXTURE_2D, m_DepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Width, m_Height,
		      0, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
			GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	pglGenFramebuffersEXT(1, &m_PingFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_ColourTex1, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex, 0);
	
	GLenum status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Framebuffer object incomplete (A): 0x%04X", status);
	}
	
	pglGenFramebuffersEXT(1, &m_PongFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_ColourTex2, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex, 0);
	
	status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Framebuffer object incomplete (B): 0x%04X", status);
	}
	
	pglGenFramebuffersEXT(1, &m_BloomFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_BloomTex1, 0);
	
	status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Framebuffer object incomplete (B): 0x%04X", status);
	}

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


void CPostprocManager::ApplyBloom()
{
	glDisable(GL_BLEND);
	
	GLint originalFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &originalFBO);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_BloomTex1, 0);
	
	CShaderDefines defines;
	defines.Add("BLOOM_NOP", "1");
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("bloom"),
			g_Renderer.GetSystemShaderDefines(), defines);
	
	tech->BeginPass();
	CShaderProgramPtr shader = tech->GetShader();
	
	GLuint renderedTex = m_WhichBuffer ? m_ColourTex1 : m_ColourTex2;
	
	glBindTexture(GL_TEXTURE_2D, renderedTex);
	pglGenerateMipmapEXT(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	shader->BindTexture("renderedTex", renderedTex);
	
	glPushAttrib(GL_VIEWPORT_BIT); 
	glViewport(0, 0, m_Width / 4, m_Height / 4);
	
	glBegin(GL_QUADS);
	    glColor4f(1.f, 1.f, 1.f, 1.f);
	    glTexCoord2f(1.0, 1.0);	glVertex2f(1,1);
	    glTexCoord2f(0.0, 1.0);	glVertex2f(-1,1);	    
	    glTexCoord2f(0.0, 0.0);	glVertex2f(-1,-1);
	    glTexCoord2f(1.0, 0.0);	glVertex2f(1,-1);
	glEnd();
	
	glPopAttrib(); 
	tech->EndPass();
	
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_BloomTex2, 0);
	
	CShaderDefines defines2;
	defines2.Add("BLOOM_PASS_H", "1");
	tech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("bloom"),
			g_Renderer.GetSystemShaderDefines(), defines2);
	
	tech->BeginPass();
	shader = tech->GetShader();
	shader->BindTexture("renderedTex", m_BloomTex1);
	shader->Uniform("texSize", m_Width / 4, m_Height / 4, 0.0f, 0.0f);
	
	glPushAttrib(GL_VIEWPORT_BIT); 
	glViewport(0, 0, m_Width / 4, m_Height / 4);
	
	glBegin(GL_QUADS);
	    glColor4f(1.f, 1.f, 1.f, 1.f);
	    glTexCoord2f(1.0, 1.0);	glVertex2f(1,1);
	    glTexCoord2f(0.0, 1.0);	glVertex2f(-1,1);
	    glTexCoord2f(0.0, 0.0);	glVertex2f(-1,-1);
	    glTexCoord2f(1.0, 0.0);	glVertex2f(1,-1);
	glEnd();
	
	glPopAttrib(); 
	tech->EndPass();
	

	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_BloomFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_BloomTex1, 0);
	
	CShaderDefines defines3;
	defines3.Add("BLOOM_PASS_V", "1");
	tech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("bloom"),
			g_Renderer.GetSystemShaderDefines(), defines3);
	
	tech->BeginPass();
	shader = tech->GetShader();
	shader->BindTexture("renderedTex", m_BloomTex2);
	shader->Uniform("texSize", m_Width / 4, m_Height / 4, 0.0f, 0.0f);
	
	glPushAttrib(GL_VIEWPORT_BIT); 
	glViewport(0, 0, m_Width / 4, m_Height / 4);
	
	glBegin(GL_QUADS);
	    glColor4f(1.f, 1.f, 1.f, 1.f);
	    glTexCoord2f(1.0, 1.0);	glVertex2f(1,1);
	    glTexCoord2f(0.0, 1.0);	glVertex2f(-1,1);
	    glTexCoord2f(0.0, 0.0);	glVertex2f(-1,-1);
	    glTexCoord2f(1.0, 0.0);	glVertex2f(1,-1);
	glEnd();
	
	glPopAttrib(); 
	tech->EndPass();
	
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, originalFBO);
}

void CPostprocManager::CaptureRenderOutput()
{
	/*glBindTexture(GL_TEXTURE_2D, m_ColourTex1);	
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, m_Width, m_Height, 0);
	glBindTexture(GL_TEXTURE_2D, m_DepthTex);
	glCopyTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT, 0, 0, m_Width, m_Height, 0);*/
	//glBindTexture(GL_TEXTURE_2D, lumaTex);
	//glCopyTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE16, 0, 0, m_Width, m_Height, 0);
	
	
	// clear both FBOs and leave m_PingFbo selected for rendering; 
	// m_WhichBuffer stays true at this point
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	pglDrawBuffers(1, buffers);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	pglDrawBuffers(1, buffers);
	//glClear(GL_COLOR_BUFFER_BIT);
	
	m_WhichBuffer = true;
}


void CPostprocManager::ReleaseRenderOutput()
{
	//ApplyLos();
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	// we blit to screen from the previous buffer active
	if (m_WhichBuffer)
		pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_PingFbo);
	else
		pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_PongFbo);
	
	pglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	pglBlitFramebufferEXT(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, 
			      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	pglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);

	
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
}

void CPostprocManager::LoadEffect(const char* name, int stage)
{
	CShaderTechniquePtr shaderTech1 = g_Renderer.GetShaderManager().LoadEffect(name);
	
	renderStages[stage].push_back(shaderTech1);
	
}

void CPostprocManager::ApplyEffect(CShaderTechniquePtr &shaderTech1)
{
	///CaptureRenderOutput();
	
	// select the other FBO for rendering
	if (!m_WhichBuffer)
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	else
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	
	
	
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	
	//glEnable (GL_BLEND);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shaderTech1->BeginPass();
	CShaderProgramPtr shader = shaderTech1->GetShader();
	
	shader->Bind();	
	
	// use the textures from the current FBO as input to the shader
	if (m_WhichBuffer)
		shader->BindTexture("bgl_RenderedTexture", m_ColourTex1);
	else
		shader->BindTexture("bgl_RenderedTexture", m_ColourTex2);
	
	shader->BindTexture("bgl_DepthTexture", m_DepthTex);
	
	//shader->BindTexture("losTexPersp", losTex);
	
	shader->BindTexture("bloomTex", m_BloomTex2);
	
	shader->Uniform("bgl_RenderedTextureWidth", m_Width);
	shader->Uniform("bgl_RenderedTextureHeight", m_Height);
	
	shader->Uniform("brightness", g_LightEnv.m_Brightness);
	shader->Uniform("hdr", g_LightEnv.m_Contrast);
	shader->Uniform("saturation", g_LightEnv.m_Saturation);
	shader->Uniform("bloom", g_LightEnv.m_Bloom);
	
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
	
	m_WhichBuffer = !m_WhichBuffer;
}



void CPostprocManager::ApplyPostproc(int stage)
{
	if (!m_IsInitialised) 
		return;
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
	
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	pglDrawBuffers(1, buffers);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
	pglDrawBuffers(1, buffers);	
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	
	ApplyBloom(); 
	
	
	std::vector<CShaderTechniquePtr> &effects = renderStages[stage];
	
	//std::cout << "EFFECT!!! " << effects.size() << std::endl;
	
	for (size_t i = 0; i < effects.size(); i++)
	{		
		ApplyEffect(effects[i]);
	}
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex, 0);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex, 0);
}


/*void CPostprocManager::ApplyLos()
{
	
	
	CShaderTechniquePtr &shaderTech1 = renderStages[4][0];
		
	//CShaderProgramPtr shader = shaderTech1->GetShader();
	
	
	//ApplyEffect(shaderTech1);
	
	
	
	if (!m_WhichBuffer)
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	else
		pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	
	
	
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	
	//glEnable (GL_BLEND);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	

	shaderTech1->BeginPass();
	CShaderProgramPtr shader = shaderTech1->GetShader();
	
	shader->Bind();	
	
	// use the textures from the current FBO as input to the shader
	if (m_WhichBuffer)
		shader->BindTexture("bgl_RenderedTexture", m_ColourTex1);
	else
		shader->BindTexture("bgl_RenderedTexture", m_ColourTex2);
	
	//shader->BindTexture("bgl_DepthTexture", m_DepthTex);
	
	//shader->BindTexture("losTexPersp", losTex);
	
	shader->Uniform("bgl_RenderedTextureWidth", m_Width);
	shader->Uniform("bgl_RenderedTextureHeight", m_Height);
	
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
	
	m_WhichBuffer = !m_WhichBuffer;
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PongFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, losTex, 0);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_PingFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, losTex, 0);
		
}
*/