#pragma once

#include <vector>
#include <string>

class Texture
{
public:
	virtual void Bind(unsigned int slot = 0) const = 0;
	virtual void Unbind() const = 0;
};


class Image
{
public:
	Image();
	Image(const std::string& path, bool flip = 0);
	~Image();

	void SetPath(const std::string& path, bool flip = 0);
	void Load(bool flip);
	void Unload();

	int GetWidth() { return m_Width; }
	int GetHeight() { return m_Height; }
	int GetNumChannels() { return m_numChannels; }
	int GetFormat() { return m_Format; }
	unsigned char* GetBuffer() { return m_FileBuffer; }
private:
	std::string m_FilePath;
	int m_Width, m_Height, m_numChannels, m_Format;
	unsigned char* m_FileBuffer;
};


class Texture2D : public Texture
{
public:
	Texture2D();
	Texture2D(const std::string& path, bool flip = 0);
	~Texture2D();

	void GenTexture();
	void SetGLParameters();

	void Bind(unsigned int slot = 0) const override;
	void Unbind() const override;

	int GetWidth();
	int GetHeight();

private:
	void CreateTexture();
	unsigned int m_RendererID;
	std::string m_FilePath;
	Image m_Image;
};


class TextureCubeMap : public Texture
{
public:
	TextureCubeMap();
	TextureCubeMap(const std::vector<std::string> paths);
	~TextureCubeMap();

	void SetCubeMap(const std::vector<std::string> paths);

	void GenTexture();
	void SetGLParameters();

	void Bind(unsigned int slot = 0) const override;
	void Unbind() const override;

private:
	unsigned int m_RendererID = 0;
	std::vector<std::string> m_Paths;
	std::vector<Image> m_Images;
};