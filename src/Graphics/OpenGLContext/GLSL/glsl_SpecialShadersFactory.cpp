#include <assert.h>
#include <N64.h>
#include <FrameBuffer.h>
#include <gDP.h>
#include <GBI.h>
#include <Config.h>
#include <PaletteTexture.h>
#include <ZlutTexture.h>
#include <Graphics/Parameters.h>
#include <Graphics/ObjectHandle.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/OpenGLContext/opengl_CachedFunctions.h>
#include <Config.h>
#include "glsl_SpecialShadersFactory.h"
#include "glsl_ShaderPart.h"
#include "glsl_FXAA.h"
#include "glsl_Utils.h"

static float gSepiaColor[4] = {0};

extern "C"
{
	void gfx_filter_sepia_set(u8 r, u8 g, u8 b, u8 a) noexcept
	{
		gSepiaColor[0] = r / 255.0f;
		gSepiaColor[1] = g / 255.0f;
		gSepiaColor[2] = b / 255.0f;
		gSepiaColor[3] = a / 255.0f;
	}

	void gfx_filter_sepia_disable() noexcept
	{
		gSepiaColor[3] = 0.0f;
	}

	bool gfx_filter_sepia_enabled()
	{
		return gSepiaColor[3] > 0.0f;
	}

	float* gfx_filter_sepia_get()
	{
		return gSepiaColor;
	}
}

namespace glsl {

	/*---------------VertexShaderPart-------------*/

	class VertexShaderRectNocolor : public ShaderPart
	{
	public:
		VertexShaderRectNocolor(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN highp vec4 aRectPosition;									\n"
				"void main()                                                    \n"
				"{                                                              \n"
				"  gl_Position = aRectPosition;									\n"
				"  gl_Position.xy += uVertexOffset * vec2(gl_Position.w);		\n"
				"  gl_Position.xy -= vec2(0.5*screenSizeDims) * gl_Position.ww;	\n"
				"  gl_Position.xy /= vec2(0.5*screenSizeDims);					\n"
				"}                                                              \n"
				;
		}
	};

	class VertexShaderTexturedRect : public ShaderPart
	{
	public:
		VertexShaderTexturedRect(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN highp vec4 aRectPosition;	\n"
				"IN highp vec2 aTexCoord0;		\n"
				"OUT mediump vec2 vTexCoord0;	\n"
				"void main()					\n"
				"{								\n"
				"  gl_Position = aRectPosition;	\n"
				"  vTexCoord0 = aTexCoord0;		\n"
				"}								\n"
			;
		}
	};

	/*---------------ShadowMapShaderPart-------------*/

	class ShadowMapFragmentShader : public ShaderPart
	{
	public:
		ShadowMapFragmentShader(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"uniform lowp usampler2D uZlutImage;\n"
				"uniform lowp usampler2D uTlutImage;\n"
				"uniform sampler2D uDepthImage;		\n"
				"uniform lowp vec4 uFogColor;								\n"
				;

			if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
				if (_glinfo.imageTextures && !_glinfo.n64DepthWithFbFetch)
					m_part += "layout(binding = 2, r32f) highp uniform restrict readonly image2D uDepthImageZ;		\n";

				if (_glinfo.n64DepthWithFbFetch) {
					m_part +=
						"layout(location = 0) OUT lowp vec4 fragColor;	\n"
						"layout(location = 1) inout highp vec4 depthZ;	\n"
						;
				} else
					m_part += "OUT lowp vec4 fragColor;									\n";
			} else
				m_part += "OUT lowp vec4 fragColor;	\n";

			m_part +=
				"lowp float get_alpha()										\n"
				"{															\n"
				;

			if (config.frameBufferEmulation.N64DepthCompare == Config::dcDisable) {
				if (_glinfo.fetch_depth) {
					m_part +=
						"  highp float bufZ = gl_LastFragDepthARM;	\n"
						;
				} else {
					m_part +=
						"  mediump ivec2 coord = ivec2(gl_FragCoord.xy);	\n"
						"  highp float bufZ = texelFetch(uDepthImage,coord, 0).r;	\n"
						;
				}
			} else {
				// Either _glinfo.imageTextures or _glinfo.ext_fetch must be enabled when N64DepthCompare != 0
				// see GLInfo::init()
				if (_glinfo.n64DepthWithFbFetch) {
					m_part +=
						"  highp float bufZ = depthZ.r;	\n"
						;
				} else if (_glinfo.imageTextures) {
					m_part +=
						"  mediump ivec2 coord = ivec2(gl_FragCoord.xy);	\n"
						"  highp float bufZ = imageLoad(uDepthImageZ,coord).r;	\n"
						;
				}
			}

			m_part +=
				"  highp int iZ = bufZ > 0.999 ? 262143 : int(floor(bufZ * 262143.0));\n"
				"  mediump int y0 = clamp(iZ/512, 0, 511);					\n"
				"  mediump int x0 = iZ - 512*y0;							\n"
				"  highp uint iN64z = texelFetch(uZlutImage,ivec2(x0,y0), 0).r;		\n"
				"  highp float n64z = clamp(float(iN64z)/65532.0, 0.0, 1.0);\n"
				"  highp int index = min(255, int(n64z*255.0));				\n"
				"  highp uint iAlpha = texelFetch(uTlutImage,ivec2(index,0), 0).r;\n"
				"  return float(iAlpha>>8)/255.0;							\n"
				"}															\n"
				"void main()												\n"
				"{															\n"
				"  fragColor = vec4(uFogColor.rgb, get_alpha());			\n"
				"}															\n"
				;
		}
	};

	/*---------------TexrectDrawerShaderPart-------------*/

	class TexrectDrawerTex3PointFilter : public ShaderPart
	{
	public:
		TexrectDrawerTex3PointFilter(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"#if (__VERSION__ > 120)																						\n"
					"# define IN in																									\n"
					"# define OUT out																								\n"
					"#else																											\n"
					"# define IN varying																							\n"
					"# define OUT																									\n"
					"#ifndef GL_FRAGMENT_PRECISION_HIGH																				\n"
					"# define highp mediump																							\n"
					"#endif																											\n"
					"#endif // __VERSION __																							\n"
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);												\n"
					"uniform lowp int uEnableAlphaTest;																				\n"
					"uniform mediump vec2 uTextureSize;																				\n"
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					"#define TEX_OFFSET(off) texture2D(tex, texCoord - (off)/texSize)												\n"
					"lowp vec4 texFilter(in sampler2D tex, in mediump vec2 texCoord)												\n"
					"{																												\n"
					"  lowp vec4 c = texture2D(tex, texCoord);																		\n"
					"  if (c == uTestColor) discard;																				\n"
					"  if (uEnableAlphaTest != 0 && !(c.a > 0.0)) discard;															\n"
					"  mediump vec2 texSize = uTextureSize;																			\n"
					"																												\n"
					"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\n"
					"  offset -= step(1.0, offset.x + offset.y);																	\n"
					"  lowp vec4 zero = vec4(0.0);					 																\n"
					"  lowp vec4 c0 = TEX_OFFSET(offset);																			\n"
					"  c0 = c * vec4(equal(c0, uTestColor)) + c0 * vec4(notEqual(c0, uTestColor));									\n"
					"  lowp vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y));										\n"
					"  c0 = c * vec4(equal(c1, uTestColor)) + c1 * vec4(notEqual(c1, uTestColor));									\n"
					"  lowp vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)));										\n"
					"  c2 = c * vec4(equal(c2, uTestColor)) + c2 * vec4(notEqual(c2, uTestColor));									\n"
					"  return c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);													\n"
					"}																												\n"
					"																												\n"
				;
			} else {
				m_part =
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					"#define TEX_OFFSET(off, tex, texCoord, texSize) texture(tex, texCoord - (off)/texSize)							\n"
					"#define TEX_FILTER(name, tex, texCoord)																		\\\n"
					"{																												\\\n"
					"  lowp vec4 c = texture(tex, texCoord);		 																\\\n"
					"  if (c == uTestColor) discard;																				\\\n"
					"  if (uEnableAlphaTest == 1 && !(c.a > 0.0)) discard;															\\\n"
					"  mediump vec2 texSize = vec2(textureSize(tex,0));																\\\n"
					"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\\\n"
					"  offset -= step(1.0, offset.x + offset.y);																	\\\n"
					"  lowp vec4 zero = vec4(0.0);					 																\\\n"
					"  lowp vec4 c0 = TEX_OFFSET(offset, tex, texCoord, texSize);													\\\n"
					"  c0 = c * vec4(equal(c0, uTestColor)) + c0 * vec4(notEqual(c0, uTestColor));									\\\n"
					"  lowp vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord, texSize);				\\\n"
					"  c1 = c * vec4(equal(c1, uTestColor)) + c1 * vec4(notEqual(c1, uTestColor));									\\\n"
					"  lowp vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord, texSize);				\\\n"
					"  c2 = c * vec4(equal(c2, uTestColor)) + c2 * vec4(notEqual(c2, uTestColor));									\\\n"
					"  name = c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);													\\\n"
					"}																												\\\n"
					"																											    \n"
					;
			}
		}
	};

	class TexrectDrawerTexBilinearFilter : public ShaderPart
	{
	public:
		TexrectDrawerTexBilinearFilter(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"#if (__VERSION__ > 120)																						\n"
					"# define IN in																									\n"
					"# define OUT out																								\n"
					"#else																											\n"
					"# define IN varying																							\n"
					"# define OUT																									\n"
					"#ifndef GL_FRAGMENT_PRECISION_HIGH																				\n"
					"# define highp mediump																							\n"
					"#endif																											\n"
					"#endif // __VERSION __																							\n"
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);												\n"
					"uniform lowp int uEnableAlphaTest;																				\n"
					"uniform mediump vec2 uTextureSize;																				\n"
					"#define TEX_OFFSET(off) texture2D(tex, texCoord - (off)/texSize)												\n"
					"lowp vec4 texFilter(in sampler2D tex, in mediump vec2 texCoord)												\n"
					"{																												\n"
					"  lowp vec4 c = texture2D(tex, texCoord);																		\n"
					"  if (c == uTestColor) discard;																				\n"
					"  if (uEnableAlphaTest != 0 && !(c.a > 0.0)) discard;															\n"
					"  mediump vec2 texSize = uTextureSize;																			\n"
					"																												\n"
					"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\n"
					"  offset -= step(1.0, offset.x + offset.y);																	\n"
					"  lowp vec4 zero = vec4(0.0);																					\n"
					"																												\n"
					"  lowp vec4 p0q0 = TEX_OFFSET(offset);																			\n"
					"  p0q0 = c * vec4(equal(p0q0, uTestColor)) + p0q0 * vec4(notEqual(p0q0, uTestColor));							\n"
					"  lowp vec4 p1q0 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y));										\n"
					"  p1q0 = c * vec4(equal(p1q0, uTestColor)) + p1q0 * vec4(notEqual(p1q0, uTestColor));							\n"
					"																												\n"
					"  lowp vec4 p0q1 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)));				                        \n"
					"  p0q1 = c * vec4(equal(p0q1, uTestColor)) + p0q1 * vec4(notEqual(p0q1, uTestColor));							\n"
					"  lowp vec4 p1q1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)));						\n"
					"  p1q1 = c * vec4(equal(p1q1, uTestColor)) + p1q1 * vec4(notEqual(p1q1, uTestColor));							\n"
					"																												\n"
					"  mediump vec2 interpolationFactor = abs(offset);																\n"
					"  lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); // Interpolates top row in X direction.		\n"
					"  lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); // Interpolates bottom row in X direction.	\n"
					"  return mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); // Interpolate in Y direction.					\n"
					"}																												\n"
					;
			}
			else {
				m_part =
					"#define TEX_OFFSET(off, tex, texCoord, texSize) texture(tex, texCoord - (off)/texSize)							\n"
					"#define TEX_FILTER(name, tex, texCoord)																		\\\n"
					"{																												\\\n"
					"  lowp vec4 c = texture(tex, texCoord);																		\\\n"
					"  if (c == uTestColor) discard;																				\\\n"
					"  if (uEnableAlphaTest == 1 && !(c.a > 0.0)) discard;															\\\n"
					"  mediump vec2 texSize = vec2(textureSize(tex,0));																\\\n"
					"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\\\n"
					"  offset -= step(1.0, offset.x + offset.y);																	\\\n"
					"  lowp vec4 zero = vec4(0.0);																					\\\n"
					"																												\\\n"
					"  lowp vec4 p0q0 = TEX_OFFSET(offset, tex, texCoord, texSize);													\\\n"
					"  p0q0 = c * vec4(equal(p0q0, uTestColor)) + p0q0 * vec4(notEqual(p0q0, uTestColor));							\\\n"
					"  lowp vec4 p1q0 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord, texSize);				\\\n"
					"  p1q0 = c * vec4(equal(p1q0, uTestColor)) + p1q0 * vec4(notEqual(p1q0, uTestColor));							\\\n"
					"																												\\\n"
					"  lowp vec4 p0q1 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord, texSize);				\\\n"
					"  p0q1 = c * vec4(equal(p0q1, uTestColor)) + p0q1 * vec4(notEqual(p0q1, uTestColor));							\\\n"
					"  lowp vec4 p1q1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)), tex, texCoord, texSize);	\\\n"
					"  p1q1 = c * vec4(equal(p1q1, uTestColor)) + p1q1 * vec4(notEqual(p1q1, uTestColor));							\\\n"
					"																												\\\n"
					"  mediump vec2 interpolationFactor = abs(offset);																\\\n"
					"  lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); 											\\\n" // Interpolates top row in X direction.
					"  lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); 											\\\n" // Interpolates bottom row in X direction.
					"  name = mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); 												\\\n" // Interpolate in Y direction.
					"}																												\\\n"
					"																												\n"
				;
			}
		}
	};

	class TexrectDrawerFragmentDraw : public ShaderPart
	{
	public:
		TexrectDrawerFragmentDraw(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"uniform sampler2D uTex0;													\n"
					"IN mediump vec2 vTexCoord0;												\n"
					"OUT lowp vec4 fragColor;													\n"
					"void main()																\n"
					"{																			\n"
					"  fragColor = texFilter(uTex0, vTexCoord0);								\n"
					"  gl_FragColor = fragColor;												\n"
					"}																			\n"
				;
			} else {
				m_part =
					"uniform sampler2D uTex0;													\n"
					"uniform lowp int uEnableAlphaTest;											\n"
					"uniform highp float uPrimDepth;											\n"
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);			\n"
					"in mediump vec2 vTexCoord0;												\n"
					"out lowp vec4 fragColor;													\n"
					"void main()																\n"
					"{																			\n"
					"  TEX_FILTER(fragColor, uTex0, vTexCoord0);								\n"
					;
				if (config.generalEmulation.enableFragmentDepthWrite != 0 &&
					config.frameBufferEmulation.N64DepthCompare == Config::dcDisable) {
					m_part +=
						"  gl_FragDepth = uPrimDepth;											\n"
						;
				}
				m_part +=
					"}																			\n"
				;
			}
		}
	};

	class TexrectDrawerFragmentClear : public ShaderPart
	{
	public:
		TexrectDrawerFragmentClear(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);	\n"
					"void main()														\n"
					"{																	\n"
					"  gl_FragColor = uTestColor;										\n"
					"}																	\n"
				;
			} else {
				m_part =
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);	\n"
					"out lowp vec4 fragColor;													\n"
					"void main()																\n"
					"{																			\n"
					"  fragColor = uTestColor;													\n"
					"}																			\n"
				;
			}
		}
	};

	/*---------------Hybrid texture filter-------------*/

	static
		std::string const& getHybridTextureFilter()
	{
		static std::string strFilter =
			"uniform sampler2D uTex0;                                                         \n"
			"                                                                                 \n"
			"ivec2 get_texture_size()                                                         \n"
			"{                                                                                \n"
			"    return textureSize(uTex0, 0);                                                \n"
			"}                                                                                \n"
			"                                                                                 \n"
			"mediump vec2 norm2denorm(mediump vec2 uv)                                        \n"
			"{                                                                                \n"
			"    return uv * vec2(get_texture_size()) - 0.5;                                  \n"
			"}                                                                                \n"
			"                                                                                 \n"
			"mediump vec2 denorm2norm(mediump vec2 denorm_uv)                                 \n"
			"{                                                                                \n"
			"    return (denorm_uv + 0.5) / vec2(get_texture_size());                         \n"
			"}                                                                                \n"
			"                                                                                 \n"
			"mediump vec4 hybridFilter(mediump vec2 uv)                                       \n"
			"{                                                                                \n"
			"    mediump vec2 denorm_uv = norm2denorm(uv);                                    \n"
			"    mediump vec2 low_corner = floor(denorm_uv);                                  \n"
			"    mediump vec2 ratio = denorm_uv - low_corner;                                 \n"
			"                                                                                 \n"
				/*
				* 'radius' is the distance from the edge where interpolation happens.
				* It's calculated based on  how big a fragment is in denormalized
				* texture coordinates.
				*
				* E.g.: If a texel maps to 5 fragments, then each fragment is
				* 1/5 texels big. So the smooth transition should be between one and
				* two fragments big, since there are enough fragments to show the full
				* color of the texel.
				*
				* If a fragment is larger than one texel, we don't care, we're already
				* sampling the wrong texels, and should be using mipmaps instead.
				*/

				// Here, fwidth() is used to estimte how much denorm_uv changes per fragment.
				// But we divide it by 2, since fwidth() is adding abs(dx) + abs(dy).
			"    mediump vec2 fragment_size  = fwidth(denorm_uv) / 2.0;                       \n"
			"                                                                                 \n"
			"    mediump float is_frag_gt1, radius;                                           \n"
				// Do nothing if fragment is greater than 1 texel
				// Don't make the transition more than one fragment (+/- 0.5 fragment)
			"    is_frag_gt1 = step(1.0, fragment_size.s);                                    \n"
			"    radius = min(fragment_size.s, 0.5);                                          \n"
			"    ratio.s = ratio.s * is_frag_gt1 + smoothstep(0.5 - radius,                   \n"
			"        0.5 + radius,	ratio.s) * (1.0 - is_frag_gt1);                           \n"
			"    is_frag_gt1 = step(1.0, fragment_size.t);                                    \n"
			"    radius = min(fragment_size.t, 0.5);                                          \n"
			"    ratio.t = ratio.t * is_frag_gt1 + smoothstep(0.5 - radius,                   \n"
			"        0.5 + radius,	ratio.t) * (1.0 - is_frag_gt1);                           \n"
			"                                                                                 \n"
				// now bump the coord to the texel using ratio
			"    mediump vec2 new_denorm_uv = low_corner + ratio;                             \n"
			"    mediump vec2 new_uv = denorm2norm(new_denorm_uv);                            \n"
			"    return texture2D(uTex0, new_uv);                                             \n"
			"}                                                                                \n"
			;
		return strFilter;
	}

	/*---------------TexrectCopyShaderPart-------------*/

	class TexrectUpscaleCopy : public ShaderPart
	{
	public:
		TexrectUpscaleCopy(const opengl::GLInfo & _glinfo)
		{
			if (config.generalEmulation.enableHybridFilter) {
				m_part = getHybridTextureFilter();
				m_part +=
					"IN mediump vec2 vTexCoord0;						\n"
					"OUT lowp vec4 fragColor;							\n"
					"													\n"
					"void main()										\n"
					"{													\n"
					"	fragColor = hybridFilter(vTexCoord0);	        \n"
					;
			} else {
				m_part =
					"IN mediump vec2 vTexCoord0;							\n"
					"uniform sampler2D uTex0;								\n"
					"OUT lowp vec4 fragColor;								\n"
					"														\n"
					"void main()											\n"
					"{														\n"
					"	fragColor = texture2D(uTex0, vTexCoord0);			\n"
					;
			}
		}
	};

	class TexrectDownscaleCopy : public ShaderPart
	{
	public:
		TexrectDownscaleCopy(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN mediump vec2 vTexCoord0;							\n"
				"uniform sampler2D uTex0;								\n"
				"OUT lowp vec4 fragColor;								\n"
				"														\n"
				"void main()											\n"
				"{														\n"
				"	fragColor = texture2D(uTex0, vTexCoord0);			\n"
				;
		}
	};

	/*---------------TexrectDepthCopyShaderPart-------------*/

	class TexrectColorAndDepthUpscaleCopy : public ShaderPart
	{
	public:
		TexrectColorAndDepthUpscaleCopy(const opengl::GLInfo & _glinfo)
		{
			if (config.generalEmulation.enableHybridFilter) {
				m_part = getHybridTextureFilter();
				m_part +=
					"IN mediump vec2 vTexCoord0;						\n"
					"uniform sampler2D uTex1;							\n"
					"OUT lowp vec4 fragColor;							\n"
					"													\n"
					"void main()										\n"
					"{													\n"
					"	fragColor = hybridFilter(vTexCoord0);	        \n"
					"	gl_FragDepth = texture2D(uTex1, vTexCoord0).r;	\n"
					;
			} else {
				m_part =
					"IN mediump vec2 vTexCoord0;							\n"
					"uniform sampler2D uTex0;								\n"
					"uniform sampler2D uTex1;								\n"
					"OUT lowp vec4 fragColor;								\n"
					"														\n"
					"void main()											\n"
					"{														\n"
					"	fragColor = texture2D(uTex0, vTexCoord0);			\n"
					"	gl_FragDepth = texture2D(uTex1, vTexCoord0).r;		\n"
					;
			}
		}
	};

	class TexrectColorAndDepthDownscaleCopy : public ShaderPart
	{
	public:
		TexrectColorAndDepthDownscaleCopy(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN mediump vec2 vTexCoord0;							\n"
				"uniform sampler2D uTex0;								\n"
				"uniform sampler2D uTex1;								\n"
				"OUT lowp vec4 fragColor;								\n"
				"														\n"
				"void main()											\n"
				"{														\n"
				"	fragColor = texture2D(uTex0, vTexCoord0);			\n"
				"	gl_FragDepth = texture2D(uTex1, vTexCoord0).r;		\n"
				;
		}
	};

	/*---------------PostProcessorShaderPart-------------*/

	class GammaCorrection : public ShaderPart
	{
	public:
		GammaCorrection(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN mediump vec2 vTexCoord0;													\n"
				"uniform sampler2D uTex0;													\n"
				"uniform lowp float uGammaCorrectionLevel;									\n"
				"OUT lowp vec4 fragColor;													\n"
				"void main()																\n"
				"{																			\n"
				"    fragColor = texture2D(uTex0, vTexCoord0);								\n"
				"    fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / uGammaCorrectionLevel));	\n"
				;
		}
	};

	class Sepia : public ShaderPart
	{
		public:
		Sepia(const opengl::GLInfo& _glinfo)
		{
			m_part = "IN mediump vec2 vTexCoord0;													\n"
				 "uniform sampler2D uTex0;													\n"
				 "uniform lowp vec4 uColor;									\n"
				 "OUT lowp vec4 fragColor;													\n"
				 "void main()																\n"
				 "{																			\n"
				 "    fragColor = texture2D(uTex0, vTexCoord0);								\n"
				 "    float gray = (fragColor.r + fragColor.g + fragColor.b) / 3.0;                     \n"
				 "    vec3 sepia = vec3(gray);                     \n"
				 "    sepia *= vec3(uColor.r, uColor.g, uColor.b);                     \n"
				 "    fragColor.rgb = mix(fragColor.rgb, sepia, uColor.a);								\n";
		}
	};

	/*---------------TextDrawerShaderPart-------------*/

	class TextDraw : public ShaderPart
	{
	public:
		TextDraw(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN mediump vec2 vTexCoord0;	\n"
				"uniform sampler2D uTex0;			\n"
				"uniform lowp vec4 uColor;		\n"
				"OUT lowp vec4 fragColor;			\n"
				"															\n"
				"void main()																														\n"
				"{																																			\n"
				"  fragColor = pow(texture2D(uTex0, vTexCoord0).r, 1.0 / 1.8) * uColor;	\n"
			;
		}
	};

	/*---------------SpecialShader-------------*/

	template<class VertexBody, class FragmentBody, class Base = graphics::ShaderProgram>
	class SpecialShader : public Base
	{
	public:
		SpecialShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd = nullptr)
			: m_program(0)
			, m_useProgram(_useProgram)
		{
			VertexBody vertexBody(_glinfo);
			FragmentBody fragmentBody(_glinfo);

			std::stringstream ssVertexShader;
			_vertexHeader->write(ssVertexShader);
			vertexBody.write(ssVertexShader);

			std::stringstream ssFragmentShader;
			_fragmentHeader->write(ssFragmentShader);
			fragmentBody.write(ssFragmentShader);
			if (_fragmentEnd != nullptr)
				_fragmentEnd->write(ssFragmentShader);

			m_program =
				graphics::ObjectHandle(Utils::createRectShaderProgram(ssVertexShader.str().data(), ssFragmentShader.str().data()));
		}

		~SpecialShader()
		{
			m_useProgram->useProgram(graphics::ObjectHandle::null);
			glDeleteProgram(GLuint(m_program));
		}

		void activate() override {
			m_useProgram->useProgram(m_program);
			gDP.changed |= CHANGED_COMBINE;
		}

	protected:
		graphics::ObjectHandle m_program;
		opengl::CachedUseProgram * m_useProgram;
	};

	/*---------------ShadowMapShader-------------*/

	typedef SpecialShader<VertexShaderRectNocolor, ShadowMapFragmentShader> ShadowMapShaderBase;

	class ShadowMapShader : public ShadowMapShaderBase
	{
	public:
		ShadowMapShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader)
			: ShadowMapShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader)
			, m_locFog(-1)
			, m_locZlut(-1)
			, m_locTlut(-1)
			, m_locDepthImage(-1)
		{
			m_useProgram->useProgram(m_program);
			m_locFog = glGetUniformLocation(GLuint(m_program), "uFogColor");
			m_locZlut = glGetUniformLocation(GLuint(m_program), "uZlutImage");
			m_locTlut = glGetUniformLocation(GLuint(m_program), "uTlutImage");
			m_locDepthImage = glGetUniformLocation(GLuint(m_program), "uDepthImage");
			m_locVertexOffset = glGetUniformLocation(GLuint(m_program), "uVertexOffset");

			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}

		void activate() override {
			ShadowMapShaderBase::activate();
			glUniform4fv(m_locFog, 1, &gDP.fogColor.r);
			glUniform1i(m_locZlut, int(graphics::textureIndices::ZLUTTex));
			glUniform1i(m_locTlut, int(graphics::textureIndices::PaletteTex));
			glUniform1i(m_locDepthImage, 0);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			const bool isNativeRes = config.frameBufferEmulation.nativeResFactor == 1 && config.video.multisampling == 0;
			const float vertexOffset = isNativeRes ? 0.5f : 0.0f;
			glUniform2f(m_locVertexOffset, vertexOffset, vertexOffset);
			g_paletteTexture.update();
		}

	private:
		int m_locFog;
		int m_locZlut;
		int m_locTlut;
		int m_locDepthImage;
		int m_locVertexOffset;
	};

	/*---------------FXAAShader-------------*/

	typedef SpecialShader<FXAAVertexShader, FXAAFragmentShader> FXAAShaderBase;

	class FXAAShader : public FXAAShaderBase
	{
	public:
		FXAAShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: FXAAShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			m_textureSizeLoc = glGetUniformLocation(GLuint(m_program), "uTextureSize");
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}

		void activate() override {
			FXAAShaderBase::activate();
#ifndef NATIVE
			FrameBuffer * pBuffer = frameBufferList().findBuffer(*REG.VI_ORIGIN & 0xffffff);
#else
			FrameBuffer* pBuffer = frameBufferList().findBuffer(*REG.VI_ORIGIN);
#endif
			if (pBuffer != nullptr && pBuffer->m_pTexture != nullptr &&
				(m_width != pBuffer->m_pTexture->width || m_height != pBuffer->m_pTexture->height)) {
				m_width = pBuffer->m_pTexture->width;
				m_height = pBuffer->m_pTexture->height;
				glUniform2f(m_textureSizeLoc, GLfloat(m_width), GLfloat(m_height));
			}
		}

	private:
		int m_textureSizeLoc = -1;
		u16 m_width = 0;
		u16 m_height = 0;
	};

	/*---------------TexrectDrawerShader-------------*/

	class TexrectDrawerShaderDraw : public graphics::TexrectDrawerShaderProgram
	{
	public:
		TexrectDrawerShaderDraw(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader)
			: m_program(0)
			, m_useProgram(_useProgram)
			, m_depth(0)
		{
			VertexShaderTexturedRect vertexBody(_glinfo);
			std::stringstream ssVertexShader;
			_vertexHeader->write(ssVertexShader);
			vertexBody.write(ssVertexShader);

			std::stringstream ssFragmentShader;
			_fragmentHeader->write(ssFragmentShader);

			if (config.texture.bilinearMode == BILINEAR_STANDARD) {
				TexrectDrawerTexBilinearFilter filter(_glinfo);
				filter.write(ssFragmentShader);
			} else {
				TexrectDrawerTex3PointFilter filter(_glinfo);
				filter.write(ssFragmentShader);
			}

			TexrectDrawerFragmentDraw fragmentMain(_glinfo);
			fragmentMain.write(ssFragmentShader);

			m_program =
				graphics::ObjectHandle(Utils::createRectShaderProgram(ssVertexShader.str().data(), ssFragmentShader.str().data()));

			m_useProgram->useProgram(m_program);
			GLint loc = glGetUniformLocation(GLuint(m_program), "uTex0");
			assert(loc >= 0);
			glUniform1i(loc, 0);
			m_textureSizeLoc = glGetUniformLocation(GLuint(m_program), "uTextureSize");
			m_enableAlphaTestLoc = glGetUniformLocation(GLuint(m_program), "uEnableAlphaTest");
			m_primDepthLoc = glGetUniformLocation(GLuint(m_program), "uPrimDepth");
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}

		~TexrectDrawerShaderDraw()
		{
			m_useProgram->useProgram(graphics::ObjectHandle::null);
			glDeleteProgram(GLuint(m_program));
		}

		void activate() override
		{
			m_useProgram->useProgram(m_program);
			if (m_primDepthLoc >= 0) {
				const GLfloat depth = gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z : 0.0f;
				if (depth != m_depth) {
					m_depth = depth;
					glUniform1f(m_primDepthLoc, m_depth);
				}
			}
			gDP.changed |= CHANGED_COMBINE;
		}

		void setTextureSize(u32 _width, u32 _height) override
		{
			if (m_textureSizeLoc < 0)
				return;
			m_useProgram->useProgram(m_program);
			glUniform2f(m_textureSizeLoc, (GLfloat)_width, (GLfloat)_height);
			gDP.changed |= CHANGED_COMBINE;
		}

		void setEnableAlphaTest(int _enable) override
		{
			m_useProgram->useProgram(m_program);
			glUniform1i(m_enableAlphaTestLoc, _enable);
			gDP.changed |= CHANGED_COMBINE;
		}

	protected:
		graphics::ObjectHandle m_program;
		opengl::CachedUseProgram * m_useProgram;
		GLint m_enableAlphaTestLoc;
		GLint m_textureSizeLoc;
		GLint m_primDepthLoc;
		GLfloat m_depth;
	};

	typedef SpecialShader<VertexShaderTexturedRect, TexrectDrawerFragmentClear> TexrectDrawerShaderClear;

	/*---------------TexrectCopyShader-------------*/

	typedef SpecialShader<VertexShaderTexturedRect, TexrectUpscaleCopy> TexrectUpscaleCopyShaderBase;

	class TexrectUpscaleCopyShader : public TexrectUpscaleCopyShaderBase
	{
	public:
		TexrectUpscaleCopyShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: TexrectUpscaleCopyShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	typedef SpecialShader<VertexShaderTexturedRect, TexrectDownscaleCopy> TexrectDownscaleCopyShaderBase;

	class TexrectDownscaleCopyShader : public TexrectDownscaleCopyShaderBase
	{
	public:
		TexrectDownscaleCopyShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: TexrectDownscaleCopyShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	/*---------------TexrectColorAndDepthCopyShader-------------*/

	typedef SpecialShader<VertexShaderTexturedRect, TexrectColorAndDepthUpscaleCopy> TexrectColorAndDepthUpscaleCopyShaderBase;

	class TexrectColorAndDepthUpscaleCopyShader : public TexrectColorAndDepthUpscaleCopyShaderBase
	{
	public:
		TexrectColorAndDepthUpscaleCopyShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: TexrectColorAndDepthUpscaleCopyShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc0 = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc0, 0);
			const int texLoc1 = glGetUniformLocation(GLuint(m_program), "uTex1");
			glUniform1i(texLoc1, 1);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	typedef SpecialShader<VertexShaderTexturedRect, TexrectColorAndDepthDownscaleCopy> TexrectColorAndDepthDownscaleCopyShaderBase;

	class TexrectColorAndDepthDownscaleCopyShader : public TexrectColorAndDepthDownscaleCopyShaderBase
	{
	public:
		TexrectColorAndDepthDownscaleCopyShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: TexrectColorAndDepthDownscaleCopyShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc0 = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc0, 0);
			const int texLoc1 = glGetUniformLocation(GLuint(m_program), "uTex1");
			glUniform1i(texLoc1, 1);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	/*---------------PostProcessorShader-------------*/

	typedef SpecialShader<VertexShaderTexturedRect, GammaCorrection> GammaCorrectionShaderBase;

	class GammaCorrectionShader : public GammaCorrectionShaderBase
	{
	public:
		GammaCorrectionShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: GammaCorrectionShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			const int levelLoc = glGetUniformLocation(GLuint(m_program), "uGammaCorrectionLevel");
			assert(levelLoc >= 0);
			const f32 gammaLevel = (config.gammaCorrection.force != 0) ? config.gammaCorrection.level : 2.0f;
			glUniform1f(levelLoc, gammaLevel);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	typedef SpecialShader<VertexShaderTexturedRect, Sepia, graphics::ColorShaderProgram> SepiaShaderBase;

	class SepiaShader : public SepiaShaderBase
	{
		public:
		SepiaShader(const opengl::GLInfo& _glinfo, opengl::CachedUseProgram* _useProgram, const ShaderPart* _vertexHeader, const ShaderPart* _fragmentHeader, const ShaderPart* _fragmentEnd) :
		    SepiaShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			m_colorLoc = glGetUniformLocation(GLuint(m_program), "uColor");
			glUniform4fv(m_colorLoc, 1, gSepiaColor);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
			gfx_filter_sepia_disable();
			memset(m_lastColor, 0, sizeof(m_lastColor));
		}

		void setColor(float* _color) override
		{
			if(memcmp(_color, m_lastColor, sizeof(m_lastColor)))
			{
				m_useProgram->useProgram(m_program);
				glUniform4fv(m_colorLoc, 1, _color);
				m_useProgram->useProgram(graphics::ObjectHandle::null);
				memcpy(m_lastColor, _color, sizeof(m_lastColor));
			}
		}

		private:
		int m_colorLoc;
		float m_lastColor[4];
	};

	/*---------------TexrectDrawerShader-------------*/

	typedef SpecialShader<VertexShaderTexturedRect, TextDraw, graphics::TextDrawerShaderProgram> TextDrawerShaderBase;

	class TextDrawerShader : public TextDrawerShaderBase
	{
	public:
		TextDrawerShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: TextDrawerShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			m_colorLoc = glGetUniformLocation(GLuint(m_program), "uColor");
			glUniform4fv(m_colorLoc, 1, config.font.colorf);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}

		void setTextColor(float * _color) override {
			m_useProgram->useProgram(m_program);
			glUniform4fv(m_colorLoc, 1, _color);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}

	private:
		int m_colorLoc;
	};

	/*---------------SpecialShadersFactory-------------*/

	SpecialShadersFactory::SpecialShadersFactory(const opengl::GLInfo & _glinfo,
												opengl::CachedUseProgram * _useProgram,
												const ShaderPart * _vertexHeader,
												const ShaderPart * _fragmentHeader,
												const ShaderPart * _fragmentEnd)
		: m_glinfo(_glinfo)
		, m_vertexHeader(_vertexHeader)
		, m_fragmentHeader(_fragmentHeader)
		, m_fragmentEnd(_fragmentEnd)
		, m_useProgram(_useProgram)
	{
	}

	graphics::ShaderProgram * SpecialShadersFactory::createShadowMapShader() const
	{
		if (m_glinfo.isGLES2)
			return nullptr;

		return new ShadowMapShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader);
	}

	graphics::TexrectDrawerShaderProgram * SpecialShadersFactory::createTexrectDrawerDrawShader() const
	{
		return new TexrectDrawerShaderDraw(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createTexrectDrawerClearShader() const
	{
		return new TexrectDrawerShaderClear(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createTexrectUpscaleCopyShader() const
	{
		return new TexrectUpscaleCopyShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createTexrectColorAndDepthUpscaleCopyShader() const
	{
		if (m_glinfo.isGLES2)
			return nullptr;

		return new TexrectColorAndDepthUpscaleCopyShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createTexrectDownscaleCopyShader() const
	{
		return new TexrectDownscaleCopyShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createTexrectColorAndDepthDownscaleCopyShader() const
	{
		if (m_glinfo.isGLES2)
			return nullptr;

		return new TexrectColorAndDepthDownscaleCopyShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createGammaCorrectionShader() const
	{
		return new GammaCorrectionShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ColorShaderProgram* SpecialShadersFactory::createSepiaShader() const
	{
		return new SepiaShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createFXAAShader() const
	{
		return new FXAAShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::TextDrawerShaderProgram * SpecialShadersFactory::createTextDrawerShader() const
	{
		return new TextDrawerShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

}
