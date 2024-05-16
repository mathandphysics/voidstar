#pragma once

#include <vector>


struct FramebufferSpecification
{
	// Need to create additional stuct members for different kinds of attachments and their properties...
	unsigned int width = 0, height = 0;
	unsigned int samples = 1;
	unsigned int numColouredAttachments = 1;
};


class Framebuffer
{
	public:
		Framebuffer();
		Framebuffer(const FramebufferSpecification& spec);
		~Framebuffer();

		void SetSpecification(const FramebufferSpecification& spec);
		void Initialize();

		void Bind();
		void Unbind();

		void Validate();
		void Resize(unsigned int width, unsigned int height);

		std::vector<unsigned int>& GetColourAttachments();

	private:
		unsigned int m_RendererID = 0;
		FramebufferSpecification m_Specification;

		std::vector<unsigned int> m_ColourAttachments;
		unsigned int m_DepthAttachment = 0;
};