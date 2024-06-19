#include "Framebuffer.h"

#include "Renderer.h"

#include <iostream>
#include <vector>

Framebuffer::Framebuffer()
{
}

Framebuffer::Framebuffer(const FramebufferSpecification& spec)
{
	m_Specification = spec;
	Initialize();
}

Framebuffer::~Framebuffer()
{
	GLCall(glDeleteFramebuffers(1, &m_RendererID));
	GLCall(glDeleteTextures(static_cast<int>(m_ColourAttachments.size()), m_ColourAttachments.data()));
	GLCall(glDeleteTextures(1, &m_DepthAttachment));
#ifndef NDEBUG
	std::cout << "Deleted framebuffer " << m_RendererID << "." << std::endl;
#endif
}

void Framebuffer::SetSpecification(const FramebufferSpecification& spec)
{
	m_Specification = spec;
	Initialize();
}

void Framebuffer::Initialize()
{
	if (m_RendererID)
	{
		GLCall(glDeleteFramebuffers(1, &m_RendererID));
		GLCall(glDeleteTextures(static_cast<int>(m_ColourAttachments.size()), m_ColourAttachments.data()));
		GLCall(glDeleteTextures(1, &m_DepthAttachment));

		m_ColourAttachments.clear();
		m_DepthAttachment = 0;
	}

	GLCall(glGenFramebuffers(1, &m_RendererID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));
	m_ColourAttachments.assign(m_Specification.numColouredAttachments, 0);

#ifndef NDEBUG
	std::cout << "Creating framebuffer " << m_RendererID << " of size : " << m_Specification.width
			  << " x " << m_Specification.height << std::endl;
#endif

	GLCall(glGenTextures(m_Specification.numColouredAttachments, m_ColourAttachments.data()));
	std::vector<unsigned int> attachments;
	for (unsigned int i = 0; i < m_Specification.numColouredAttachments; i++)
	{
		GLCall(glBindTexture(GL_TEXTURE_2D, m_ColourAttachments[i]));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Specification.width, m_Specification.height, 0, GL_RGBA, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_ColourAttachments[i], 0);
		attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
	}

	GLCall(glDrawBuffers(m_Specification.numColouredAttachments, attachments.data()));

	Validate();
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Framebuffer::Bind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));
}

void Framebuffer::Unbind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Framebuffer::Validate() const
{
	ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

void Framebuffer::Resize(unsigned int width, unsigned int height)
{
}

std::vector<unsigned int>& Framebuffer::GetColourAttachments()
{
	return m_ColourAttachments;
}
