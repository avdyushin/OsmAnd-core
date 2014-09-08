#include "GPUAPI_OpenGL2plus.h"

#include <cassert>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QStringList>
#include <QRegExp>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include "restore_internal_warnings.h"

#include "MapRendererTypes.h"
#include "IMapRasterBitmapTileProvider.h"
#include "MapSymbol.h"
#include "RasterMapSymbol.h"
#include "Logging.h"

#undef GL_CHECK_RESULT
#undef GL_GET_RESULT
#undef GL_GET_AND_CHECK_RESULT
#if OSMAND_DEBUG
#   define GL_CHECK_RESULT validateResult()
#   define GL_GET_RESULT validateResult()
#   define GL_GET_AND_CHECK_RESULT validateResult()
#else
#   define GL_CHECK_RESULT
#   define GL_GET_RESULT glGetError()
#   define GL_GET_AND_CHECK_RESULT glGetError()
#endif

OsmAnd::GPUAPI_OpenGL2plus::GPUAPI_OpenGL2plus()
    : _isSupported_GREMEDY_string_marker(false)
    , _isSupported_ARB_sampler_objects(false)
    , _isSupported_samplerObjects(false)
    , _isSupported_ARB_vertex_array_object(false)
    , _isSupported_APPLE_vertex_array_object(false)
    , _isSupported_ARB_texture_storage(false)
    , _isSupported_ARB_texture_float(false)
    , _isSupported_ATI_texture_float(false)
    , _isSupported_ARB_texture_rg(false)
    , isSupported_GREMEDY_string_marker(_isSupported_GREMEDY_string_marker)
    , isSupported_ARB_sampler_objects(_isSupported_ARB_sampler_objects)
    , isSupported_samplerObjects(_isSupported_samplerObjects)
    , isSupported_ARB_vertex_array_object(_isSupported_ARB_vertex_array_object)
    , isSupported_APPLE_vertex_array_object(_isSupported_APPLE_vertex_array_object)
    , isSupported_ARB_texture_storage(_isSupported_ARB_texture_storage)
    , isSupported_ARB_texture_float(_isSupported_ARB_texture_float)
    , isSupported_ATI_texture_float(_isSupported_ATI_texture_float)
    , isSupported_ARB_texture_rg(_isSupported_ARB_texture_rg)
{
}

OsmAnd::GPUAPI_OpenGL2plus::~GPUAPI_OpenGL2plus()
{
}

GLenum OsmAnd::GPUAPI_OpenGL2plus::validateResult()
{
    GL_CHECK_PRESENT(glGetError);

    auto result = glGetError();
    if (result == GL_NO_ERROR)
        return result;

    LogPrintf(LogSeverityLevel::Error, "OpenGL error 0x%08x : %s", result, gluErrorString(result));

    return result;
}

bool OsmAnd::GPUAPI_OpenGL2plus::initialize()
{
    bool ok;

    ok = GPUAPI_OpenGL::initialize();
    if (!ok)
        return false;

    if (glewInit() != GLEW_NO_ERROR)
        return false;
    // Silence OpenGL error here, it's inside GLEW, so it's not ours
    (void)glGetError();

    GL_CHECK_PRESENT(glGetError);
    GL_CHECK_PRESENT(glGetString);
    GL_CHECK_PRESENT(glGetFloatv);
    GL_CHECK_PRESENT(glGetIntegerv);
    GL_CHECK_PRESENT(glHint);

    const auto glVersionString = glGetString(GL_VERSION);
    GL_CHECK_RESULT;
    QRegExp glVersionRegExp(QLatin1String("(\\d+).(\\d+)"));
    glVersionRegExp.indexIn(QString(QLatin1String(reinterpret_cast<const char*>(glVersionString))));
    _glVersion = glVersionRegExp.cap(1).toUInt() * 10 + glVersionRegExp.cap(2).toUInt();
    LogPrintf(LogSeverityLevel::Info, "OpenGL version %d [%s]", _glVersion, glVersionString);
    if (_glVersion < 20)
    {
        LogPrintf(LogSeverityLevel::Info, "This OpenGL version is not supported");
        return false;
    }
    //////////////////////////////////////////////////////////////////////////
    //NOTE: For testing, limit GL version to 2.0
    //_glVersion = 20;
    //////////////////////////////////////////////////////////////////////////
    
    const auto glslVersionString = glGetString(GL_SHADING_LANGUAGE_VERSION);
    GL_CHECK_RESULT;
    QRegExp glslVersionRegExp(QLatin1String("(\\d+).(\\d+)"));
    glslVersionRegExp.indexIn(QString(QLatin1String(reinterpret_cast<const char*>(glslVersionString))));
    _glslVersion = glslVersionRegExp.cap(1).toUInt() * 100 + glslVersionRegExp.cap(2).toUInt();
    LogPrintf(LogSeverityLevel::Info, "GLSL version %d [%s]", _glslVersion, glslVersionString);
    //////////////////////////////////////////////////////////////////////////
    //NOTE: For testing, limit GLSL version to 1.30, what corresponds to OpenGL 3.0
    //NOTE: For testing, limit GLSL version to 1.10, what corresponds to OpenGL 2.0
    //_glslVersion = 110;
    //////////////////////////////////////////////////////////////////////////

    if (glVersion >= 30)
    {
        GL_CHECK_PRESENT(glGetStringi);

        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        GL_CHECK_RESULT;
        _extensions.clear();
        for (auto extensionIdx = 0; extensionIdx < numExtensions; extensionIdx++)
        {
            const auto& extension = QLatin1String(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, extensionIdx)));
            GL_CHECK_RESULT;

            _extensions.push_back(extension);
        }
    }
    else
    {
        const auto& extensionsString = QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
        GL_CHECK_RESULT;
        _extensions = extensionsString.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    }
    LogPrintf(LogSeverityLevel::Info, "OpenGL extensions: %s", qPrintable(extensions.join(' ')));

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&_maxTextureSize));
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture size %dx%d", _maxTextureSize, _maxTextureSize);

    GLint maxTextureUnitsInFragmentShader;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInFragmentShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units in fragment shader %d", maxTextureUnitsInFragmentShader);
    assert(maxTextureUnitsInFragmentShader >= RasterMapLayersCount);

    GLint maxTextureUnitsInVertexShader;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxTextureUnitsInVertexShader);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units in vertex shader %d", maxTextureUnitsInVertexShader);
    //////////////////////////////////////////////////////////////////////////
    //NOTE: for testing
    //maxTextureUnitsInVertexShader = 0;
    //////////////////////////////////////////////////////////////////////////
    _isSupported_vertexShaderTextureLookup = (maxTextureUnitsInVertexShader >= 1);

    GLint maxTextureUnitsCombined;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnitsCombined);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal texture units (combined) %d", maxTextureUnitsCombined);

    if (extensions.contains(QLatin1String("GL_ARB_ES2_compatibility")))
    {
        // According to http://www.opengl.org/wiki/GLSL_Uniform ("Implementation limits") , this will give incorrect results for AMD/ATI
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &_maxVertexUniformVectors);
        GL_CHECK_RESULT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL maximal 4-component parameters in vertex shader %d", _maxVertexUniformVectors);

        // According to http://www.opengl.org/wiki/GLSL_Uniform ("Implementation limits") , this will give incorrect results for AMD/ATI
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &_maxFragmentUniformVectors);
        GL_CHECK_RESULT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL maximal 4-component parameters in fragment shader %d", _maxFragmentUniformVectors);
    }

    GLint maxVertexUniformComponents;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal parameters in vertex shader %d", maxVertexUniformComponents);
    _maxVertexUniformVectors = maxVertexUniformComponents / 4; // Workaround for AMD/ATI (see above)

    GLint maxFragmentUniformComponents;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniformComponents);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal parameters in fragment shader %d", maxFragmentUniformComponents);
    _maxFragmentUniformVectors = maxFragmentUniformComponents / 4; // Workaround for AMD/ATI (see above)

    if (glVersion >= 43)
    {
        GLint maxUniformLocations;
        glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &maxUniformLocations);
        GL_CHECK_RESULT;
        LogPrintf(LogSeverityLevel::Info, "OpenGL maximal defined parameters %d", maxUniformLocations);
    }

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &_maxVertexAttribs);
    GL_CHECK_RESULT;
    LogPrintf(LogSeverityLevel::Info, "OpenGL maximal vertex attributes %d", _maxVertexAttribs);

    // textureLod() is supported by GLSL 1.30+ specification (which is supported by OpenGL 3.0+), or if GL_ARB_shader_texture_lod is available
    _isSupported_textureLod = (glslVersion >= 130) || extensions.contains(QLatin1String("GL_ARB_shader_texture_lod"));
    _isSupported_texturesNPOT = (glVersion >= 20); // OpenGL 2.0+ fully supports NPOT textures
    _isSupported_EXT_debug_marker = extensions.contains("GL_EXT_debug_marker");
    _isSupported_GREMEDY_string_marker = extensions.contains("GL_GREMEDY_string_marker");
    // http://www.opengl.org/sdk/docs/man/html/glGenSamplers.xhtml are supported only if OpenGL 3.3+ or GL_ARB_sampler_objects is available
    _isSupported_ARB_sampler_objects = extensions.contains(QLatin1String("GL_ARB_sampler_objects"));
    _isSupported_samplerObjects = (glVersion >= 33) || _isSupported_ARB_sampler_objects;
    _isSupported_ARB_vertex_array_object = extensions.contains(QLatin1String("GL_ARB_vertex_array_object"));
    _isSupported_APPLE_vertex_array_object = extensions.contains(QLatin1String("GL_APPLE_vertex_array_object"));
    _isSupported_vertex_array_object = (glVersion >= 30 || isSupported_ARB_vertex_array_object || isSupported_APPLE_vertex_array_object);
    //////////////////////////////////////////////////////////////////////////
    //NOTE: for testing
    //_isSupported_vertex_array_object = false;
    //////////////////////////////////////////////////////////////////////////

    // glTexStorage2D is supported in OpenGL 4.2+ or if GL_ARB_texture_storage is available
    // https://www.opengl.org/sdk/docs/man/html/glTexStorage2D.xhtml
    _isSupported_ARB_texture_storage = extensions.contains(QLatin1String("GL_ARB_texture_storage"));
    _isSupported_texture_storage = (glVersion >= 42) || isSupported_ARB_texture_storage;

    _isSupported_ARB_texture_float = extensions.contains(QLatin1String("GL_ARB_texture_float"));
    _isSupported_ATI_texture_float = extensions.contains(QLatin1String("GL_ATI_texture_float"));
    _isSupported_texture_float = isSupported_ARB_texture_float || isSupported_ATI_texture_float;

    _isSupported_texture_rg = _isSupported_ARB_texture_rg = extensions.contains(QLatin1String("GL_ARB_texture_rg"));

    GLint compressedFormatsLength = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &compressedFormatsLength);
    GL_CHECK_RESULT;
    _compressedFormats.resize(compressedFormatsLength);
    if (compressedFormatsLength > 0)
    {
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, _compressedFormats.data());
        GL_CHECK_RESULT;
    }
    _isSupported_8bitPaletteRGBA8 = extensions.contains("GL_OES_compressed_paletted_texture") || compressedFormats.contains(GL_PALETTE8_RGBA8_OES);
    LogPrintf(LogSeverityLevel::Info, "OpenGL 8-bit palette RGBA8 textures: %s", isSupported_8bitPaletteRGBA8 ? "supported" : "not supported");

    if (isSupported_samplerObjects)
    {
        GL_CHECK_PRESENT(glGenSamplers);
        GL_CHECK_PRESENT(glSamplerParameteri);
        GL_CHECK_PRESENT(glSamplerParameterf);

        // Allocate samplers
        glGenSamplers(SamplerTypesCount, _textureSamplers.data());
        GL_CHECK_RESULT;
        GLuint sampler;

        // ElevationDataTile sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::ElevationDataTile)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL_CHECK_RESULT;

        // BitmapTile_Bilinear sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::BitmapTile_Bilinear)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;

        // BitmapTile_BilinearMipmap sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::BitmapTile_BilinearMipmap)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;

        // BitmapTile_TrilinearMipmap sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::BitmapTile_TrilinearMipmap)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;

        // Symbol sampler
        sampler = _textureSamplers[static_cast<int>(SamplerType::Symbol)];
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_RESULT;
    }

    return true;
}

bool OsmAnd::GPUAPI_OpenGL2plus::release()
{
    bool ok;

    if (isSupported_samplerObjects)
    {
        GL_CHECK_PRESENT(glDeleteSamplers);
        if (_textureSamplers[0] != 0)
        {
            glDeleteSamplers(1, _textureSamplers.data());
            GL_CHECK_RESULT;
            _textureSamplers.fill(0);
        }
    }

    ok = GPUAPI_OpenGL::release();
    if (!ok)
        return false;

    return true;
}

OsmAnd::GPUAPI_OpenGL2plus::TextureFormat OsmAnd::GPUAPI_OpenGL2plus::getTextureSizedFormat(const SkBitmap::Config skBitmapConfig) const
{
    GLenum textureFormat = GL_INVALID_ENUM;

    switch (skBitmapConfig)
    {
        case SkBitmap::Config::kARGB_8888_Config:
            textureFormat = GL_RGBA8;
            break;

        case SkBitmap::Config::kARGB_4444_Config:
            textureFormat = GL_RGBA4;
            break;

        case SkBitmap::Config::kRGB_565_Config:
            textureFormat = GL_RGB565;
            break;

        default:
            assert(false);
            return static_cast<TextureFormat>(GL_INVALID_ENUM);
    }

    return static_cast<TextureFormat>(textureFormat);
}

OsmAnd::GPUAPI_OpenGL2plus::TextureFormat OsmAnd::GPUAPI_OpenGL2plus::getTextureSizedFormat_float() const
{
    GLenum textureFormat = GL_INVALID_ENUM;

    if (isSupported_texture_float && isSupported_texture_rg)
        textureFormat = GL_R32F;
    else if (isSupported_texture_rg)
        textureFormat = GL_R8;
    else
        textureFormat = GL_LUMINANCE8_EXT; //NOTE: only available in GL_EXT_texture

    return static_cast<TextureFormat>(textureFormat);
}

bool OsmAnd::GPUAPI_OpenGL2plus::isValidTextureSizedFormat(const TextureFormat textureFormat) const
{
    return (static_cast<GLenum>(textureFormat) != GL_INVALID_ENUM);
}

OsmAnd::GPUAPI_OpenGL2plus::SourceFormat OsmAnd::GPUAPI_OpenGL2plus::getSourceFormat_float() const
{
    SourceFormat sourceFormat;
    sourceFormat.format = GL_INVALID_ENUM;
    sourceFormat.type = GL_INVALID_ENUM;

    if (isSupported_texture_rg)
        sourceFormat.format = GL_RED;
    else
        sourceFormat.format = GL_LUMINANCE;

    if (isSupported_texture_float)
        sourceFormat.type = GL_FLOAT;
    else
        sourceFormat.type = GL_UNSIGNED_BYTE;

    return sourceFormat;
}

bool OsmAnd::GPUAPI_OpenGL2plus::isValidSourceFormat(const SourceFormat sourceFormat) const
{
    return
        sourceFormat.format != GL_INVALID_ENUM &&
        sourceFormat.type != GL_INVALID_ENUM;
}

void OsmAnd::GPUAPI_OpenGL2plus::allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format)
{
    // Use glTexStorage2D if possible
    if (isSupported_texture_storage)
    {
        GL_CHECK_PRESENT(glTexStorage2D);

        GLenum textureFormat = static_cast<GLenum>(format);

        glTexStorage2D(target, levels, textureFormat, width, height);
        GL_CHECK_RESULT;
        return;
    }

    // Fallback to dumb allocation
    GPUAPI_OpenGL::allocateTexture2D(target, levels, width, height, format);
}

void OsmAnd::GPUAPI_OpenGL2plus::uploadDataToTexture2D(
    GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
    const SourceFormat sourceFormat)
{
    GL_CHECK_PRESENT(glPixelStorei);
    GL_CHECK_PRESENT(glTexSubImage2D);

    // GL_UNPACK_ROW_LENGTH is supported from OpenGL 1.1+
    glPixelStorei(GL_UNPACK_ROW_LENGTH, dataRowLengthInElements);
    GL_CHECK_RESULT;

    glTexSubImage2D(target, level,
        xoffset, yoffset, width, height,
        static_cast<GLenum>(sourceFormat.format),
        static_cast<GLenum>(sourceFormat.type),
        data);
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL2plus::setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount)
{
    GL_CHECK_PRESENT(glTexParameteri);

    // GL_TEXTURE_MAX_LEVEL supported from OpenGL 1.2+
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmapLevelsCount);
    GL_CHECK_RESULT;
}

void OsmAnd::GPUAPI_OpenGL2plus::glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays)
{
    if (glVersion >= 30 || isSupported_ARB_vertex_array_object)
    {
        GL_CHECK_PRESENT(glGenVertexArrays);

        glGenVertexArrays(n, arrays);
    }
    else if (isSupported_APPLE_vertex_array_object)
    {
        GL_CHECK_PRESENT(glGenVertexArraysAPPLE);

        glGenVertexArraysAPPLE(n, arrays);
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::glBindVertexArray_wrapper(GLuint array)
{
    if (glVersion >= 30 || isSupported_ARB_vertex_array_object)
    {
        GL_CHECK_PRESENT(glBindVertexArray);

        glBindVertexArray(array);
    }
    else if (isSupported_APPLE_vertex_array_object)
    {
        GL_CHECK_PRESENT(glBindVertexArrayAPPLE);

        glBindVertexArrayAPPLE(array);
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays)
{
    if (glVersion >= 30 || isSupported_ARB_vertex_array_object)
    {
        GL_CHECK_PRESENT(glDeleteVertexArrays);

        glDeleteVertexArrays(n, arrays);
    }
    else if (isSupported_APPLE_vertex_array_object)
    {
        GL_CHECK_PRESENT(glDeleteVertexArraysAPPLE);

        glDeleteVertexArraysAPPLE(n, arrays);
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::preprocessShader(QString& code)
{
    QString shaderHeader;
    if (glslVersion >= 330)
    {
        shaderHeader = QString::fromLatin1(
            // Declare version of GLSL used
            "#version 330 core                                                                                                  ""\n"
            "                                                                                                                   ""\n"
            // General definitions
            "#define INPUT in                                                                                                   ""\n"
            "#define PARAM_OUTPUT out                                                                                           ""\n"
            "#define PARAM_INPUT in                                                                                             ""\n"
            "                                                                                                                   ""\n"
            // Features definitions
            "#define VERTEX_TEXTURE_FETCH_SUPPORTED %VertexTextureFetchSupported%                                               ""\n"
            "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D texture                                                                                  ""\n"
            "#define SAMPLE_TEXTURE_2D_LOD textureLod                                                                           ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 130)
    {
        shaderHeader = QString::fromLatin1(
            // Declare version of GLSL used
            "#version 130                                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // General definitions
            "#define INPUT in                                                                                                   ""\n"
            "#define PARAM_OUTPUT out                                                                                           ""\n"
            "#define PARAM_INPUT in                                                                                             ""\n"
            "                                                                                                                   ""\n"
            // Features definitions
            "#define VERTEX_TEXTURE_FETCH_SUPPORTED %VertexTextureFetchSupported%                                               ""\n"
            "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D texture2D                                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D_LOD texture2DLod                                                                         ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 110)
    {
        shaderHeader = QString::fromLatin1(
            // Declare version of GLSL used
            "#version 110                                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // General definitions
            "#define INPUT attribute                                                                                            ""\n"
            "#define PARAM_OUTPUT varying                                                                                       ""\n"
            "#define PARAM_INPUT varying                                                                                        ""\n"
            "                                                                                                                   ""\n"
            // Precision specifying is not supported
            "#define highp                                                                                                      ""\n"
            "#define mediump                                                                                                    ""\n"
            "#define lowp                                                                                                       ""\n"
            "                                                                                                                   ""\n"
            // Features definitions
            "#define VERTEX_TEXTURE_FETCH_SUPPORTED %VertexTextureFetchSupported%                                               ""\n"
            "#define TEXTURE_LOD_SUPPORTED %TextureLodSupported%                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D texture2D                                                                                ""\n"
            "#define SAMPLE_TEXTURE_2D_LOD texture2DLod                                                                         ""\n"
            "                                                                                                                   ""\n");
    }
    else
    {
        assert(false);
    }

    auto shaderSourcePreprocessed = shaderHeader;
    shaderSourcePreprocessed.replace("%VertexTextureFetchSupported%", QString::number(isSupported_vertexShaderTextureLookup ? 1 : 0));
    shaderSourcePreprocessed.replace("%TextureLodSupported%", QString::number(isSupported_textureLod ? 1 : 0));

    code.prepend(shaderSourcePreprocessed);
}

void OsmAnd::GPUAPI_OpenGL2plus::preprocessVertexShader(QString& code)
{
    preprocessShader(code);
}

void OsmAnd::GPUAPI_OpenGL2plus::preprocessFragmentShader(QString& code)
{
    QString commonHeader;
    preprocessShader(commonHeader);

    QString shaderHeader;
    if (glslVersion >= 130)
    {
        shaderHeader = QLatin1String(
            // Fragment shader output declaration
            "#define FRAGMENT_COLOR_OUTPUT out_FragColor                                                                        ""\n"
            "out vec4 out_FragColor;                                                                                            ""\n"
            "                                                                                                                   ""\n");
    }
    else if (glslVersion >= 110)
    {
        shaderHeader = QLatin1String(
            // Make some extensions required
            "#ifdef GL_ARB_shader_texture_lod                                                                                   ""\n"
            "#extension GL_ARB_shader_texture_lod : require                                                                     ""\n"
            "#endif // GL_ARB_shader_texture_lod                                                                                ""\n"
            "                                                                                                                   ""\n"
            // Fragment shader output declaration
            "#define FRAGMENT_COLOR_OUTPUT gl_FragColor                                                                         ""\n"
            "                                                                                                                   ""\n");
    }
    else
    {
        assert(false);
    }

    code.prepend(shaderHeader);
    code.prepend(commonHeader);
}

void OsmAnd::GPUAPI_OpenGL2plus::optimizeVertexShader(QString& code)
{
}

void OsmAnd::GPUAPI_OpenGL2plus::optimizeFragmentShader(QString& code)
{
}

void OsmAnd::GPUAPI_OpenGL2plus::setTextureBlockSampler(const GLenum textureBlock, const SamplerType samplerType)
{
    if (isSupported_samplerObjects)
    {
        GL_CHECK_PRESENT(glBindSampler);
        glBindSampler(textureBlock - GL_TEXTURE0, _textureSamplers[static_cast<int>(samplerType)]);
        GL_CHECK_RESULT;
    }
    else
    {
        // In case sampler objects are not supported, use settings per-texture
        _textureBlocksSamplers[textureBlock] = samplerType;
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::applyTextureBlockToTexture(const GLenum texture, const GLenum textureBlock)
{
    if (isSupported_samplerObjects)
    {
        // In case sampler objects are supported, nothing to do here
    }
    else
    {
        GL_CHECK_PRESENT(glTexParameteri);

        const auto samplerType = _textureBlocksSamplers[textureBlock];
        if (samplerType == SamplerType::ElevationDataTile)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::BitmapTile_Bilinear)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::BitmapTile_BilinearMipmap)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::BitmapTile_TrilinearMipmap)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
        }
        else if (samplerType == SamplerType::Symbol)
        {
            glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
            glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_CHECK_RESULT;
        }
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker)
{
    GL_CHECK_PRESENT(glPushGroupMarkerEXT);
    glPushGroupMarkerEXT(length, marker);
}

void OsmAnd::GPUAPI_OpenGL2plus::glPopGroupMarkerEXT_wrapper()
{
    GL_CHECK_PRESENT(glPopGroupMarkerEXT);
    glPopGroupMarkerEXT();
}

void OsmAnd::GPUAPI_OpenGL2plus::pushDebugGroupMarker(const QString& title)
{
    if (isSupported_GREMEDY_string_marker)
    {
        GL_CHECK_PRESENT(glStringMarkerGREMEDY);

        QString marker;
        {
            QMutexLocker scopedLocker(&_gdebuggerGroupsStackMutex);
            _gdebuggerGroupsStack.push_back(title);
            marker = _gdebuggerGroupsStack.join(QLatin1Char('/'));
        }
        marker = QLatin1String("Group begin '") + marker + QLatin1String("':");
        glStringMarkerGREMEDY(marker.length(), qPrintable(marker));
    }
    GPUAPI_OpenGL::pushDebugGroupMarker(title);
}

void OsmAnd::GPUAPI_OpenGL2plus::popDebugGroupMarker()
{
    GPUAPI_OpenGL::popDebugGroupMarker();
    if (isSupported_GREMEDY_string_marker)
    {
        GL_CHECK_PRESENT(glStringMarkerGREMEDY);

        QString marker;
        {
            QMutexLocker scopedLocker(&_gdebuggerGroupsStackMutex);
            marker = _gdebuggerGroupsStack.join(QLatin1Char('/'));
            _gdebuggerGroupsStack.pop_back();
        }
        marker = QLatin1String("Group end '") + marker + QLatin1String("'.");
        glStringMarkerGREMEDY(marker.length(), qPrintable(marker));
    }
}

void OsmAnd::GPUAPI_OpenGL2plus::glClearDepth_wrapper(const float depth)
{
    GL_CHECK_PRESENT(glClearDepth);

    glClearDepth(depth);
}
