#include "Texture.h"

#include "Renderer.h"

#include <iostream>

#include "vendor/stb_image/stb_image.h"


Image::Image()
	: m_FilePath(), m_Width(0), m_Height(0), m_numChannels(0), m_Format(0), m_FileBuffer(NULL)
{
}

Image::Image(const std::string& path, bool flip)
	: m_FilePath(path), m_Width(0), m_Height(0), m_numChannels(0), m_Format(0), m_FileBuffer(NULL)
{
	Load(flip);
}

Image::~Image()
{
	Unload();
}

void Image::SetPath(const std::string& path, bool flip)
{
	m_FilePath = path;
	Load(flip);
}

void Image::Load(bool flip)
{
	if (!m_FileBuffer)
	{
		stbi_set_flip_vertically_on_load(flip);

		m_FileBuffer = stbi_load(m_FilePath.c_str(), &m_Width, &m_Height, &m_numChannels, 0);
		if (m_FileBuffer)
		{
			if (m_numChannels == 1)
			{
				m_Format = GL_RED;
			}
			else if (m_numChannels == 3)
			{
				m_Format = GL_RGB;
			}
			else if (m_numChannels == 4)
			{
				m_Format = GL_RGBA;
			}
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << m_FilePath << std::endl;
		}
	}
	else
	{
		std::cout << "Image " << m_FilePath << " already loaded!" << std::endl;
	}
}

void Image::Unload()
{
	if (m_FileBuffer)
	{
		stbi_image_free(m_FileBuffer);
	}
	m_FileBuffer = NULL;
}




Texture2D::Texture2D()
	: m_RendererID(0), m_FilePath(), m_Image()
{
}

Texture2D::Texture2D(const std::string& path, bool flip)
	: m_RendererID(0), m_FilePath(path), m_Image(path, flip)
{
	CreateTexture();
}

Texture2D::~Texture2D()
{
	GLCall(glDeleteTextures(1, &m_RendererID));
}

void Texture2D::Bind(unsigned int slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
}

void Texture2D::Unbind() const
{
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void Texture2D::GenTexture()
{
	assert(m_RendererID == 0);
	GLCall(glGenTextures(1, &m_RendererID));
}

void Texture2D::SetGLParameters()
{
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

int Texture2D::GetWidth()
{
	return m_Image.GetWidth();
}

int Texture2D::GetHeight()
{
	return m_Image.GetHeight();
}

void Texture2D::CreateTexture()
{
	GenTexture();
	Bind();
	SetGLParameters();

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, m_Image.GetFormat(), m_Image.GetWidth(), m_Image.GetHeight(),
		0, m_Image.GetFormat(), GL_UNSIGNED_BYTE, m_Image.GetBuffer()));

	Unbind();
	m_Image.Unload();
}




TextureCubeMap::TextureCubeMap()
{
}

TextureCubeMap::TextureCubeMap(const std::vector<std::string> paths)
	: m_RendererID(0), m_Paths(paths)
	// Necessary face ordering in paths list: xpos, xneg, ypos, yneg, zpos, zneg
{
	SetCubeMap(paths);
}

TextureCubeMap::~TextureCubeMap()
{
	GLCall(glDeleteTextures(1, &m_RendererID));
}

void TextureCubeMap::SetCubeMap(const std::vector<std::string> paths)
{
	assert(m_RendererID == 0);
	m_Paths = paths;

	GenTexture();
	Bind();

	for (int i = 0; i < 6; i++)
	{
		m_Images.push_back(Image());
		m_Images[i].SetPath(m_Paths[i], 0);
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_Images[i].GetFormat(), m_Images[i].GetWidth(),
			m_Images[i].GetHeight(), 0, m_Images[i].GetFormat(), GL_UNSIGNED_BYTE, m_Images[i].GetBuffer()));
		m_Images[i].Unload();
	}

	Unbind();

	SetGLParameters();
}

void TextureCubeMap::GenTexture()
{
	assert(m_RendererID == 0);
	GLCall(glGenTextures(1, &m_RendererID));
}

void TextureCubeMap::SetGLParameters()
{
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
}

void TextureCubeMap::Bind(unsigned int slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID));
}

void TextureCubeMap::Unbind() const
{
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}