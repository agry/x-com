#include "framework/renderer_interface.h"
#include "framework/logger.h"
#include "framework/image.h"
#include "framework/palette.h"
#include <memory>
#include <array>

#include "framework/render/gl_3_0.cpp"

namespace {

using namespace OpenApoc;

class Program
{
	public:
		GLuint prog;
		static GLuint CreateShader(GLenum type, const UString source)
		{
			GLuint shader = gl::CreateShader(type);
			auto sourceString = source.str();
			const GLchar *string = sourceString.c_str();
			GLint stringLength = sourceString.length();
			gl::ShaderSource(shader, 1, &string, &stringLength);
			gl::CompileShader(shader);
			GLint compileStatus;
			gl::GetShaderiv(shader, gl::COMPILE_STATUS, &compileStatus);
			if (compileStatus == gl::TRUE_)
				return shader;

			GLint logLength;
			gl::GetShaderiv(shader, gl::INFO_LOG_LENGTH, &logLength);

			std::unique_ptr<char[]> log(new char[logLength]);
			gl::GetShaderInfoLog(shader, logLength, NULL, log.get());

			LogError("Shader compile error: %s", log.get());

			gl::DeleteShader(shader);
			return 0;
		}
		Program(const UString vertexSource, const UString fragmentSource)
			: prog(0)
		{
			GLuint vShader = CreateShader(gl::VERTEX_SHADER, vertexSource);
			if (!vShader)
			{
				LogError("Failed to compile vertex shader");
				return;
			}
			GLuint fShader = CreateShader(gl::FRAGMENT_SHADER, fragmentSource);
			if (!fShader)
			{
				LogError("Failed to compile fragment shader");
				gl::DeleteShader(vShader);
				return;
			}

			prog = gl::CreateProgram();
			gl::AttachShader(prog, vShader);
			gl::AttachShader(prog, fShader);

			gl::DeleteShader(vShader);
			gl::DeleteShader(fShader);

			gl::LinkProgram(prog);

			GLint linkStatus;
			gl::GetProgramiv(prog, gl::LINK_STATUS, &linkStatus);
			if (linkStatus == gl::TRUE_)
				return;

			GLint logLength;
			gl::GetProgramiv(prog, gl::INFO_LOG_LENGTH, &logLength);

			std::unique_ptr<char[]> log(new char[logLength]);
			gl::GetProgramInfoLog(prog, logLength, NULL, log.get());

			LogError("Program link error: %s", log.get());

			gl::DeleteProgram(prog);
			prog = 0;
			return;

		}

		void Uniform(GLuint loc, Colour c)
		{
			gl::Uniform4f(loc, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
		}

		void Uniform(GLuint loc, Vec2<float> v)
		{
			gl::Uniform2f(loc, v.x, v.y);
		}
		void Uniform(GLuint loc, Vec2<int> v)
		{
			//FIXME: Float conversion
			gl::Uniform2f(loc, v.x, v.y);
		}
		void Uniform(GLuint loc, float v)
		{
			gl::Uniform1f(loc, v);
		}
		void Uniform(GLuint loc, int v)
		{
			gl::Uniform1i(loc, v);
		}

		void Uniform(GLuint loc, bool v)
		{
			gl::Uniform1f(loc, (v ? 1.0f : 0.0f));
		}

		virtual ~Program()
		{
			if (prog)
				gl::DeleteProgram(prog);
		};
};

class SpriteProgram : public Program
{
	protected:
		SpriteProgram(const UString vertexSource, const UString fragmentSource)
			: Program(vertexSource, fragmentSource)
			{
			}
	public:
		GLuint posLoc;
		GLuint sizeLoc;
		GLuint offsetLoc;
		GLuint screenSizeLoc;
		GLuint texLoc;
		GLuint flipYLoc;
};
const char* RGBProgram_vertexSource = {
	"#version 130\n"
	"in vec2 position;\n"
	"uniform vec2 size;\n"
	"uniform vec2 offset;\n"
	"out vec2 texcoord;\n"
	"uniform vec2 screenSize;\n"
	"uniform bool flipY;\n"
	"void main() {\n"
	"  texcoord = position;\n"
	"  vec2 tmpPos = position;\n"
	"  tmpPos *= size;\n"
	"  tmpPos += offset;\n"
	"  tmpPos /= screenSize;\n"
	"  tmpPos -= vec2(0.5,0.5);\n"
	"  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
	"  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
	"}\n"
};
const char* RGBProgram_fragmentSource = {
	"#version 130\n"
	"in vec2 texcoord;\n"
	"uniform sampler2D tex;\n"
	"out vec4 out_colour;\n"
	"void main() {\n"
	" out_colour = texture2D(tex, texcoord);\n"
	"}\n"
};
class RGBProgram : public SpriteProgram
{
	private:
		
	public:
		RGBProgram()
			: SpriteProgram(RGBProgram_vertexSource, RGBProgram_fragmentSource)
			{
				this->posLoc = gl::GetAttribLocation(this->prog, "position");
				this->sizeLoc = gl::GetUniformLocation(this->prog, "size");
				this->offsetLoc = gl::GetUniformLocation(this->prog, "offset");
				this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
				this->texLoc = gl::GetUniformLocation(this->prog, "tex");
				this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
			}
		void setUniforms(Vec2<float> offset, Vec2<float> size, Vec2<int> screenSize, bool flipY, GLint texUnit = 0)
		{
			this->Uniform(this->offsetLoc, offset);
			this->Uniform(this->sizeLoc, size);
			this->Uniform(this->screenSizeLoc, screenSize);
			this->Uniform(this->texLoc, texUnit);
			this->Uniform(this->flipYLoc, flipY);
		}
};
const char* PaletteProgram_vertexSource = {
	"#version 130\n"
	"in vec2 position;\n"
	"uniform vec2 size;\n"
	"uniform vec2 offset;\n"
	"out vec2 texcoord;\n"
	"uniform vec2 screenSize;\n"
	"uniform bool flipY;\n"
	"void main() {\n"
	"  texcoord = position;\n"
	"  vec2 tmpPos = position;\n"
	"  tmpPos *= size;\n"
	"  tmpPos += offset;\n"
	"  tmpPos /= screenSize;\n"
	"  tmpPos -= vec2(0.5,0.5);\n"
	"  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
	"  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
	"}\n"
};
const char* PaletteProgram_fragmentSource = {
	"#version 130\n"
	"in vec2 texcoord;\n"
	"uniform sampler2D tex;\n"
	"uniform sampler2D pal;\n"
	"out vec4 out_colour;\n"
	"void main() {\n"
	" out_colour = texture2D(pal, vec2(texture2D(tex,texcoord).r,0));\n"
	"}\n"
};
class PaletteProgram : public SpriteProgram
{
	private:	
	public:
		GLuint palLoc;
		PaletteProgram()
			: SpriteProgram(PaletteProgram_vertexSource, PaletteProgram_fragmentSource)
			{
				this->posLoc = gl::GetAttribLocation(this->prog, "position");
				this->sizeLoc = gl::GetUniformLocation(this->prog, "size");
				this->offsetLoc = gl::GetUniformLocation(this->prog, "offset");
				this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
				this->texLoc = gl::GetUniformLocation(this->prog, "tex");
				this->palLoc = gl::GetUniformLocation(this->prog, "pal");
				this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
			}
		void setUniforms(Vec2<float> offset, Vec2<float> size, Vec2<int> screenSize, bool flipY, GLint texUnit = 0, GLint palUnit = 1)
		{
			this->Uniform(this->offsetLoc, offset);
			this->Uniform(this->sizeLoc, size);
			this->Uniform(this->screenSizeLoc, screenSize);
			this->Uniform(this->texLoc, texUnit);
			this->Uniform(this->palLoc, palUnit);
			this->Uniform(this->flipYLoc, flipY);
		}
};

const char* PaletteSetProgram_vertexSource = {
	"#version 130\n"
	"in vec2 position;\n"
	"in vec2 texcoord_in;\n"
	"in int sprite_in;\n"
	"out vec2 texcoord;\n"
	"flat out int sprite;\n"
	"uniform vec2 screenSize;\n"
	"uniform bool flipY;\n"
	"void main() {\n"
	"  texcoord = texcoord_in;\n"
	"  sprite = sprite_in;\n"
	"  vec2 tmpPos = position;\n"
	"  tmpPos /= screenSize;\n"
	"  tmpPos -= vec2(0.5,0.5);\n"
	"  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
	"  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
	"}\n"
};
const char* PaletteSetProgram_fragmentSource = {
	"#version 130\n"
	"in vec2 texcoord;\n"
	"flat in int sprite;\n"
	"uniform isampler2DArray tex;\n"
	"uniform sampler2D pal;\n"
	"out vec4 out_colour;\n"
	"void main() {\n"
	" int idx = texelFetch(tex, ivec3(texcoord.x, texcoord.y, sprite), 0).r;\n"
	" out_colour = texelFetch(pal, ivec2(idx,0), 0);\n"
	"}\n"
};
class PaletteSetProgram : public Program
{
	private:
		
		
	public:
		GLuint posLoc;
		GLuint texcoordLoc;
		GLuint spriteLoc;
		GLuint screenSizeLoc;
		GLuint texLoc;
		GLuint palLoc;
		GLuint flipYLoc;
		PaletteSetProgram()
			: Program(PaletteSetProgram_vertexSource, PaletteSetProgram_fragmentSource)
			{
				this->posLoc = gl::GetAttribLocation(this->prog, "position");
				this->texcoordLoc = gl::GetAttribLocation(this->prog, "texcoord_in");
				this->spriteLoc = gl::GetAttribLocation(this->prog, "sprite_in");

				this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
				this->texLoc = gl::GetUniformLocation(this->prog, "tex");
				this->palLoc = gl::GetUniformLocation(this->prog, "pal");
				this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
			}
		void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0, GLint palUnit = 1)
		{
			this->Uniform(this->screenSizeLoc, screenSize);
			this->Uniform(this->texLoc, texUnit);
			this->Uniform(this->palLoc, palUnit);
			this->Uniform(this->flipYLoc, flipY);
		}
		class VertexDef
		{
		public:
			VertexDef(Vec2<float> pos, Vec2<float> texcoords, int sprite)
				: pos(pos), texcoords(texcoords), sprite(sprite){}
			VertexDef(){}

			Vec2<float> pos;
			Vec2<float> texcoords;
			int sprite;
		};
		static_assert(sizeof(VertexDef) == 20, "VertexDef should be tightly packed");
		class SpriteDef
		{
		public:
			SpriteDef(Vec2<float> offset, Vec2<float> size, int sprite)
			{
				v[0] = { offset, Vec2<float>{0, 0}, sprite };
				v[1] = { offset + size * Vec2<float>{0, 1}, Vec2<float>{0, 1}, sprite };
				v[2] = { offset + size * Vec2<float>{1, 0}, Vec2<float>{1, 0}, sprite };
				v[3] = { offset + size * Vec2<float>{1, 0}, Vec2<float>{1, 0}, sprite };
			}

			VertexDef v[4];
		};
		static_assert(sizeof(SpriteDef) == sizeof(VertexDef)*4, "SpriteDef should be tightly packed");
};

const char* SolidColourProgram_vertexSource = {
	"#version 130\n"
	"in vec2 position;\n"
	"uniform vec2 size;\n"
	"uniform vec2 offset;\n"
	"uniform vec2 screenSize;\n"
	"uniform bool flipY;\n"
	"void main() {\n"
	"  vec2 tmpPos = position;\n"
	"  tmpPos *= size;\n"
	"  tmpPos += offset;\n"
	"  tmpPos /= screenSize;\n"
	"  tmpPos -= vec2(0.5,0.5);\n"
	"  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
	"  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
	"}\n"
};
const char* SolidColourProgram_fragmentSource = {
	"#version 130\n"
	"uniform vec4 colour;\n"
	"out vec4 out_colour;\n"
	"void main() {\n"
	" out_colour = colour;\n"
	"}\n"
};
class SolidColourProgram : public Program
{
	private:
		
		
	public:
		GLuint posLoc;
		GLuint sizeLoc;
		GLuint offsetLoc;
		GLuint screenSizeLoc;
		GLuint colourLoc;
		GLuint flipYLoc;
		SolidColourProgram()
			: Program(SolidColourProgram_vertexSource, SolidColourProgram_fragmentSource)
			{
				this->posLoc = gl::GetAttribLocation(this->prog, "position");
				this->sizeLoc = gl::GetUniformLocation(this->prog, "size");
				this->offsetLoc = gl::GetUniformLocation(this->prog, "offset");
				this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
				this->colourLoc = gl::GetUniformLocation(this->prog, "colour");
				this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
			}
		void setUniforms(Vec2<float> offset, Vec2<float> size, Vec2<int> screenSize, bool flipY, Colour colour)
		{
			this->Uniform(this->offsetLoc, offset);
			this->Uniform(this->sizeLoc, size);
			this->Uniform(this->screenSizeLoc, screenSize);
			this->Uniform(this->colourLoc, colour);
			this->Uniform(this->flipYLoc, flipY);
		}
};

class IdentityQuad
{
public:
	static void draw(GLuint attribPos)
	{
		static const float vertices[] =
		{
			0.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		};
		gl::EnableVertexAttribArray(attribPos);
		gl::VertexAttribPointer(attribPos, 2, gl::FLOAT, gl::FALSE_, 0, &vertices);
		gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
	}
};

class ActiveTexture
{
	ActiveTexture(const ActiveTexture &) = delete;
public:
	GLenum prevUnit;
	static GLenum getUnitEnum(int unit)
	{
		return gl::TEXTURE0 + unit;
	}

	ActiveTexture(int unit)
	{
		gl::GetIntegerv(gl::ACTIVE_TEXTURE, (GLint*)&prevUnit);
		gl::ActiveTexture(getUnitEnum(unit));
	}
	~ActiveTexture()
	{
		gl::ActiveTexture(prevUnit);
	}
};

class UnpackAlignment
{
	UnpackAlignment(const UnpackAlignment &) = delete;
public:
	GLint prevAlign;
	UnpackAlignment(int align)
	{
		gl::GetIntegerv(gl::UNPACK_ALIGNMENT, &prevAlign);
		gl::PixelStorei(gl::UNPACK_ALIGNMENT, align);
	}
	~UnpackAlignment()
	{
		gl::PixelStorei(gl::UNPACK_ALIGNMENT, prevAlign);
	}
};

class BindTexture
{
	BindTexture(const BindTexture &) = delete;
public:
	GLenum bind;
	GLuint prevID;
	int unit;
	static GLenum getBindEnum(GLenum e)
	{
		switch (e) {
			case gl::TEXTURE_1D: return gl::TEXTURE_BINDING_1D;
			case gl::TEXTURE_2D: return gl::TEXTURE_BINDING_2D;
			case gl::TEXTURE_3D: return gl::TEXTURE_BINDING_3D;
			case gl::TEXTURE_2D_ARRAY: return gl::TEXTURE_BINDING_2D_ARRAY;
			default:
				LogError("Unknown texture enum %d", (int)e);
				return gl::TEXTURE_BINDING_2D;
		}
	}
	BindTexture(GLuint id, GLint unit = 0, GLenum bind = gl::TEXTURE_2D)
		: bind(bind), unit(unit) 
	{
		ActiveTexture a(unit);
		gl::GetIntegerv(getBindEnum(bind), (GLint*)&prevID);
		gl::BindTexture(bind, id);
	}
	~BindTexture()
	{
		ActiveTexture a(unit);
		gl::BindTexture(bind, prevID);
	}
};

template <GLenum param>
class TexParam
{
	TexParam(const TexParam&) = delete;
public:
	GLint prevValue;
	GLuint id;
	GLenum type;

	TexParam(GLuint id, GLint value, GLenum type = gl::TEXTURE_2D)
		: id(id), type(type)
	{
		BindTexture b(id, 0, type);
		gl::GetTexParameteriv(type, param, &prevValue);
		gl::TexParameteri(type, param, value);
	}
	~TexParam()
	{
		BindTexture b(id, 0, type);
		gl::TexParameteri(type, param, prevValue);
	}
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;
public:
	GLuint prevID;
	BindFramebuffer(GLuint id)
	{
		gl::GetIntegerv(gl::DRAW_FRAMEBUFFER_BINDING, (GLint*)&prevID);
		gl::BindFramebuffer(gl::DRAW_FRAMEBUFFER, id);

	}
	~BindFramebuffer()
	{
		gl::BindFramebuffer(gl::DRAW_FRAMEBUFFER, prevID);
	}
};

class FBOData : public RendererImageData
{
public:
	GLuint fbo;
	GLuint tex;
	Vec2<float> size;
	//Constructor /only/ to be used for default surface (FBO ID == 0)
	FBOData(GLuint fbo)
		//FIXME: Check FBO == 0
		//FIXME: Warn if trying to texture from FBO 0
		: fbo(fbo), tex(-1), size(0,0){}

	FBOData(Vec2<int> size)
		:size(size.x, size.y)
	{
		gl::GenTextures(1, &this->tex);
		BindTexture b(this->tex);
		gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA8, size.x, size.y, 0, gl::RGBA, gl::UNSIGNED_BYTE, NULL);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
		gl::GenFramebuffers(1, &this->fbo);
		BindFramebuffer f(this->fbo);

		gl::FramebufferTexture2D(gl::DRAW_FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D, this->tex, 0);
		assert(gl::CheckFramebufferStatus(gl::DRAW_FRAMEBUFFER) == gl::FRAMEBUFFER_COMPLETE);


	}
	virtual ~FBOData()
	{
		if (tex)
			gl::DeleteTextures(1, &tex);
		if (fbo)
			gl::DeleteFramebuffers(1, &fbo);
	}
};

class GLRGBImage : public RendererImageData
{
	public:
		GLuint texID;
		Vec2<float> size;
		std::weak_ptr<RGBImage> parent;
		GLRGBImage(std::shared_ptr<RGBImage> parent)
			: size(parent->size), parent(parent)
		{
			RGBImageLock l(parent, ImageLockUse::Read);
			gl::GenTextures(1, &this->texID);
			BindTexture b(this->texID);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
			gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, parent->size.x, parent->size.y, 0, gl::RGBA, gl::UNSIGNED_BYTE, l.getData());

		}
		virtual ~GLRGBImage()
		{
			gl::DeleteTextures(1, &this->texID);
		}
};

class GLPalette : public RendererImageData
{
	public:
		GLuint texID;
		Vec2<float> size;
		std::weak_ptr<Palette> parent;
		GLPalette(std::shared_ptr<Palette> parent)
			: size(Vec2<float>(parent->colours.size(), 1)), parent(parent)
		{
			gl::GenTextures(1, &this->texID);
			BindTexture b(this->texID);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
			gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, parent->colours.size(), 1, 0, gl::RGBA, gl::UNSIGNED_BYTE, parent->colours.data());

		}
		virtual ~GLPalette()
		{
			gl::DeleteTextures(1, &this->texID);
		}
};

class GLPaletteImage : public RendererImageData
{
	public:
		GLuint texID;
		Vec2<float> size;
		std::weak_ptr<PaletteImage> parent;
		GLPaletteImage(std::shared_ptr<PaletteImage> parent)
			: size(parent->size), parent(parent)
		{
			PaletteImageLock l(parent, ImageLockUse::Read);
			gl::GenTextures(1, &this->texID);
			BindTexture b(this->texID);
			UnpackAlignment align(1);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
			gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RED, parent->size.x, parent->size.y, 0, gl::RED, gl::UNSIGNED_BYTE, l.getData());

		}
		virtual ~GLPaletteImage()
		{
			gl::DeleteTextures(1, &this->texID);
		}
};

class GLPaletteSpritesheet : public RendererImageData
{
	public:
		std::weak_ptr<ImageSet> parent;
		Vec2<int> maxSize;
		unsigned numSprites;
		GLuint texID;
		GLPaletteSpritesheet(std::shared_ptr<ImageSet> parent)
			: parent(parent), maxSize(parent->maxSize), numSprites(parent->images.size())
		{
			gl::GenTextures(1, &this->texID);
			BindTexture b(this->texID, 0, gl::TEXTURE_2D_ARRAY);
			UnpackAlignment align(1);
			gl::TexParameteri(gl::TEXTURE_2D_ARRAY, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
			gl::TexParameteri(gl::TEXTURE_2D_ARRAY, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
			gl::TexImage3D(gl::TEXTURE_2D_ARRAY, 0, gl::R8UI, maxSize.x, maxSize.y, numSprites, 0, gl::RED_INTEGER, gl::UNSIGNED_BYTE, NULL);

			std::unique_ptr<char[]> zeros(new char[maxSize.x * maxSize.y]);
			memset(zeros.get(), 1, maxSize.x * maxSize.y);

			for (unsigned int i = 0; i < numSprites; i++)
			{
				std::shared_ptr<PaletteImage> img =
					std::dynamic_pointer_cast<PaletteImage>(parent->images[i]);
				//FIXME: HACK - better way of clearing undefined portions to '0'?
				gl::TexSubImage3D(gl::TEXTURE_2D_ARRAY, 0, 0, 0, i, maxSize.x, maxSize.y, 1, gl::RED_INTEGER, gl::UNSIGNED_BYTE, zeros.get());

				PaletteImageLock l(img, ImageLockUse::Read);

				gl::TexSubImage3D(gl::TEXTURE_2D_ARRAY, 0, 0, 0, i, img->size.x, img->size.y, 1, gl::RED_INTEGER, gl::UNSIGNED_BYTE, l.getData());

			}

		}
		virtual ~GLPaletteSpritesheet()
		{
			gl::DeleteTextures(1, &this->texID);
		}
};

class OGL30Renderer : public Renderer
{
private:
	enum class RendererState
	{
		Idle,
		BatchingSpritesheet,
	};
	RendererState state;
	std::shared_ptr<RGBProgram> rgbProgram;
	std::shared_ptr<SolidColourProgram> colourProgram;
	std::shared_ptr<PaletteProgram> paletteProgram;
	std::shared_ptr<PaletteSetProgram> paletteSetProgram;
	GLuint currentBoundProgram;
	GLuint currentBoundFBO;

	std::shared_ptr<Surface> currentSurface;
	std::shared_ptr<Palette> currentPalette;

	friend class RendererSurfaceBinding;
	virtual void setSurface(std::shared_ptr<Surface> s)
	{
		this->flush();
		this->currentSurface = s;
		if (!s->rendererPrivateData)
			s->rendererPrivateData.reset(new FBOData(s->size));

		FBOData *fbo = static_cast<FBOData*>(s->rendererPrivateData.get());
		gl::BindFramebuffer(gl::FRAMEBUFFER, fbo->fbo);
		this->currentBoundFBO = fbo->fbo;
		gl::Viewport(0, 0, s->size.x, s->size.y);
	};
	virtual std::shared_ptr<Surface> getSurface()
	{
		return currentSurface;
	};
	std::shared_ptr<Surface> defaultSurface;
public:
	OGL30Renderer();
	virtual ~OGL30Renderer();
	virtual void clear(Colour c = Colour{0,0,0,0});
	virtual void setPalette(std::shared_ptr<Palette> p)
	{
		if (!p->rendererPrivateData)
			p->rendererPrivateData.reset(new GLPalette(p));
		this->currentPalette = p;
	}
	
	virtual void draw(std::shared_ptr<Image> i, Vec2<float> position);
	virtual void drawRotated(std::shared_ptr<Image> i, Vec2<float> center, Vec2<float> position, float angle)
	{
		LogError("Unimplemented function");
		std::ignore = i;
		std::ignore = center;
		std::ignore = position;
		std::ignore = angle;
	};
	virtual void drawScaled(std::shared_ptr<Image> image, Vec2<float> position, Vec2<float> size, Scaler scaler = Scaler::Linear)
	{

		if (this->state != RendererState::Idle)
			this->flush();
		std::shared_ptr<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage*>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage);
				image->rendererPrivateData.reset(img);
			}
			DrawRGB(*img, position, size, scaler);
			return;
		}

		std::shared_ptr<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
		if (paletteImage)
		{
			GLPaletteImage *img = dynamic_cast<GLPaletteImage*>(paletteImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLPaletteImage(paletteImage);
				image->rendererPrivateData.reset(img);
			}
			if (scaler != Scaler::Nearest)
			{
				//blending indices doesn't make sense. You'll have to render
				//it to an RGB surface then scale that
				LogError("Only nearest scaler is supported on paletted images");
			}
			DrawPalette(*img, position, size);
			return;
		}

		std::shared_ptr<Surface> surface = std::dynamic_pointer_cast<Surface>(image);
		if (surface)
		{
			FBOData *fbo = dynamic_cast<FBOData*>(surface->rendererPrivateData.get());
			if (!fbo)
			{
				fbo = new FBOData(image->size);
				image->rendererPrivateData.reset(fbo);
			}
			DrawSurface(*fbo, position, size, scaler);
			return;
		}
		LogError("Unsupported image type");
	};
	virtual void drawTinted(std::shared_ptr<Image> i, Vec2<float> position, Colour tint)
	{
		LogError("Unimplemented function");
		std::ignore = i;
		std::ignore = position;
		std::ignore = tint;
	};
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c);
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0)
	{
		this->drawLine(position, Vec2<float>{position.x + size.x, position.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x + size.x, position.y}, Vec2<float>{position.x + size.x, position.y + size.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x + size.x, position.y + size.y}, Vec2<float>{position.x, position.y + size.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x, position.y + size.y}, position, c, thickness);
	};
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0)
	{
		//FIXME: Hack - allow axis-aligned lines to be drawn using the drawFilledRect() fn
		if (p1.x == p2.x)
		{
			if (p1.y > p2.y)
				std::swap(p1, p2);
			this->drawFilledRect(p1, Vec2<float>{thickness, p2.y - p1.y}, c);
		}
		else if (p1.y == p2.y)
		{
			if (p1.x > p2.x)
				std::swap(p1, p2);
			this->drawFilledRect(p1, Vec2<float>{p2.x - p1.x, thickness}, c);
		}
		else
		{
			LogError("Unimplemented function {%f,%f},{%f,%f}", p1.x, p1.y, p2.x, p2.y);
		}
	};
	virtual void flush();
	virtual UString getName();
	virtual std::shared_ptr<Surface>getDefaultSurface()
	{
		return this->defaultSurface;
	};

	void BindProgram(std::shared_ptr<Program> p)
	{
		if (this->currentBoundProgram == p->prog)
			return;
		gl::UseProgram(p->prog);
		this->currentBoundProgram = p->prog;
	}


	void DrawRGB(GLRGBImage &img, Vec2<float> offset, Vec2<float> size, Scaler scaler)
	{
		GLenum filter;
		switch (scaler)
		{
			case Scaler::Linear:
				filter = gl::LINEAR;
				break;
			case Scaler::Nearest:
				filter = gl::NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = gl::NEAREST;
				break;
		}
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(offset, size, this->currentSurface->size, flipY);
		BindTexture t(img.texID);
		TexParam<gl::TEXTURE_MAG_FILTER> mag(img.texID, filter);
		TexParam<gl::TEXTURE_MIN_FILTER> min(img.texID, filter);
		IdentityQuad::draw(rgbProgram->posLoc);
	}

	void DrawPalette(GLPaletteImage &img, Vec2<float> offset, Vec2<float> size)
	{
		BindProgram(paletteProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteProgram->setUniforms(offset, size, this->currentSurface->size, flipY);
		BindTexture t(img.texID, 0);

		BindTexture p(static_cast<GLPalette*>(this->currentPalette->rendererPrivateData.get())->texID, 1);

		IdentityQuad::draw(paletteProgram->posLoc);
	}

	void DrawSurface(FBOData &fbo, Vec2<float> offset, Vec2<float> size, Scaler scaler)
	{
		GLenum filter;
		switch (scaler)
		{
			case Scaler::Linear:
				filter = gl::LINEAR;
				break;
			case Scaler::Nearest:
				filter = gl::NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = gl::NEAREST;
				break;
		}
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(offset, size, this->currentSurface->size, flipY);
		BindTexture t(fbo.tex);
		TexParam<gl::TEXTURE_MAG_FILTER> mag(fbo.tex, filter);
		TexParam<gl::TEXTURE_MIN_FILTER> min(fbo.tex, filter);
		IdentityQuad::draw(rgbProgram->posLoc);
	}

	void DrawRect(Vec2<float> offset, Vec2<float> size, Colour c)
	{
		BindProgram(colourProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(offset, size, this->currentSurface->size, flipY, c);
		IdentityQuad::draw(colourProgram->posLoc);
	}

	class BatchedVertex
	{
	public:
		Vec2<float> position;
		Vec2<float> texCoord;
		int spriteIdx;
		BatchedVertex()
		{}
		BatchedVertex(Vec2<float> p, Vec2<float> tc, int i)
			: position(p), texCoord(tc), spriteIdx(i)
			{
			}
	};
	static_assert(sizeof(BatchedVertex) == 20, "BatchedVertex unexpected size");

	class BatchedSprite
	{
	public:
		std::array<BatchedVertex, 4> vertices;
		BatchedSprite(Vec2<float> screenPosition, Vec2<float> spriteSize,
			int spriteIdx)
		{
			Vec2<float> maxTexCoords = spriteSize;
			Vec2<float> maxPosition = screenPosition + spriteSize;
			vertices[0] = BatchedVertex{
				screenPosition,
				Vec2<float>{0,0},
				spriteIdx
			};
			vertices[1] = BatchedVertex{
				Vec2<float>{screenPosition.x, maxPosition.y},
				Vec2<float>{0,maxTexCoords.y},
				spriteIdx
			};
			vertices[2] = BatchedVertex{
				Vec2<float>{maxPosition.x, screenPosition.y},
				Vec2<float>{maxTexCoords.x,0},
				spriteIdx
			};
			vertices[3] = BatchedVertex{
				maxPosition,
				maxTexCoords,
				spriteIdx
			};
		}
	};
	static_assert(sizeof(BatchedSprite) == sizeof(BatchedVertex) * 4, "BatchedSprite unexpected size");

	std::vector<BatchedSprite> batchedSprites;
	unsigned maxBatchedSprites;
	unsigned maxSpritesheetSize;
	std::shared_ptr<GLPaletteSpritesheet> boundSpritesheet;

	std::unique_ptr<GLint[]> firstList;
	std::unique_ptr<GLsizei[]> countList;

	void DrawBatchedSpritesheet()
	{
		BindProgram(paletteSetProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteSetProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(this->boundSpritesheet->texID, 0, gl::TEXTURE_2D_ARRAY);
		BindTexture p(static_cast<GLPalette*>(this->currentPalette->rendererPrivateData.get())->texID, 1);

		gl::EnableVertexAttribArray(paletteSetProgram->posLoc);
		gl::EnableVertexAttribArray(paletteSetProgram->texcoordLoc);
		gl::EnableVertexAttribArray(paletteSetProgram->spriteLoc);

		const char* vertexPtr = (const char*)this->batchedSprites.data();

		gl::VertexAttribPointer(paletteSetProgram->posLoc, 2, gl::FLOAT, gl::FALSE_, sizeof(BatchedVertex), vertexPtr + offsetof(BatchedVertex, position));
		gl::VertexAttribPointer(paletteSetProgram->texcoordLoc, 2, gl::FLOAT, gl::FALSE_, sizeof(BatchedVertex), vertexPtr + offsetof(BatchedVertex, texCoord));
		gl::VertexAttribIPointer(paletteSetProgram->spriteLoc, 1, gl::INT, sizeof(BatchedVertex), vertexPtr + offsetof(BatchedVertex, spriteIdx));

		gl::MultiDrawArrays(gl::TRIANGLE_STRIP, this->firstList.get(), this->countList.get(), this->batchedSprites.size());

		this->batchedSprites.clear();
		this->state = RendererState::Idle;

	}

};


OGL30Renderer::OGL30Renderer()
	: state(RendererState::Idle), rgbProgram(new RGBProgram()), colourProgram(new SolidColourProgram()), paletteProgram(new PaletteProgram()), paletteSetProgram(new PaletteSetProgram()), currentBoundProgram(0)
{
	GLint viewport[4];
	gl::GetIntegerv(gl::VIEWPORT, viewport);
	LogInfo("Viewport {%d,%d,%d,%d}", viewport[0], viewport[1], viewport[2], viewport[3]);
	assert(viewport[0] == 0 && viewport[1] == 0);
	this->defaultSurface = std::make_shared<Surface>(Vec2<int>{viewport[2], viewport[3]});
	this->defaultSurface->rendererPrivateData.reset(new FBOData(0));
	this->currentSurface = this->defaultSurface;

	GLint maxTexArrayLayers;
	gl::GetIntegerv(gl::MAX_ARRAY_TEXTURE_LAYERS, &maxTexArrayLayers);
	LogInfo("MAX_ARRAY_TEXTURE_LAYERS: %d", maxTexArrayLayers);
	this->maxBatchedSprites = 256;
	this->maxSpritesheetSize = maxTexArrayLayers;

	this->firstList.reset(new GLint[this->maxBatchedSprites]);
	this->countList.reset(new GLsizei[this->maxBatchedSprites]);

	for (unsigned int i = 0; i < this->maxBatchedSprites; i++)
	{
		this->firstList[i] = 4*i;
		this->countList[i] = 4;
	}

	GLint maxTexUnits;
	gl::GetIntegerv(gl::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
	LogInfo("MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d", maxTexUnits);
	gl::Enable(gl::BLEND);
	gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
}

OGL30Renderer::~OGL30Renderer()
{

}

void
OGL30Renderer::clear(Colour c)
{
	this->flush();
	gl::ClearColor(c.r/255.0f, c.g/255.0f, c.b/255.0f, c.a/255.0f);
	gl::Clear(gl::COLOR_BUFFER_BIT);
}

void OGL30Renderer::draw(std::shared_ptr<Image> image, Vec2<float> position)
{
	std::shared_ptr<ImageSet> owningSet = image->owningSet.lock();
	if (owningSet)
	{
		if (owningSet->images.size() > maxSpritesheetSize)
		{
			static bool warnonce = false;
			if (!warnonce)
			{
				warnonce = true;
				LogError("Spritesheet size %d would be over max array size %d - falling back to 'slow' path", owningSet->images.size(), maxSpritesheetSize);
			}
		}
		else
		{
			std::shared_ptr<GLPaletteSpritesheet> ss = std::dynamic_pointer_cast<GLPaletteSpritesheet>(owningSet->rendererPrivateData);
			if (!ss)
			{
				ss = std::make_shared<GLPaletteSpritesheet>(owningSet);
				owningSet->rendererPrivateData = ss;
			}
			switch (this->state)
			{
				default:
					this->flush();
				case RendererState::BatchingSpritesheet:
					if (ss != this->boundSpritesheet ||
					    this->batchedSprites.size() >= this->maxBatchedSprites)
					{
						this->flush();
					}
				case RendererState::Idle:
					break;
			}
			this->boundSpritesheet = ss;
			this->state = RendererState::BatchingSpritesheet;
			this->batchedSprites.emplace_back(
					position, Vec2<float>(image->size.x, image->size.y),
					image->indexInSet
				);
			return;
		}
	}
	drawScaled(image, position, image->size, Scaler::Nearest);
}
void OGL30Renderer::drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c)
{
	this->flush();
	DrawRect(position, size, c);
}

void
OGL30Renderer::flush()
{
	switch (this->state)
	{
		case RendererState::Idle:
			break;
		case RendererState::BatchingSpritesheet:
			this->DrawBatchedSpritesheet();
			break;
	}
	this->state = RendererState::Idle;
}

UString
OGL30Renderer::getName()
{
	return "OGL3.0 Renderer";
}

class OGL30RendererFactory : public OpenApoc::RendererFactory
{
bool alreadyInitialised;
bool functionLoadSuccess;
public:
	OGL30RendererFactory()
		: alreadyInitialised(false), functionLoadSuccess(false){}
	virtual OpenApoc::Renderer *create()
	{
		if (!alreadyInitialised)
		{
			alreadyInitialised = true;
			auto success = gl::sys::LoadFunctions();
			if (!success)
				return nullptr;
			if (success.GetNumMissing())
				return nullptr;
			functionLoadSuccess = true;
		}
		if (functionLoadSuccess)
			return new OGL30Renderer();
		return nullptr;
	}
};

OpenApoc::RendererRegister<OGL30RendererFactory> register_at_load_gl_3_0_renderer("GL_3_0");

}; //anonymous namespace
