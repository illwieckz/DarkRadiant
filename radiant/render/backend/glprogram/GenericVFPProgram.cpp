#include "GenericVFPProgram.h"

#include "igl.h"
#include "../GLProgramFactory.h"
#include <boost/algorithm/string/predicate.hpp>

namespace render
{

GenericVFPProgram::GenericVFPProgram(const std::string& vertexProgramFilename,
                                     const std::string& fragmentProgramFilename) :
    _program(0),
    _vertexProgramFilename(vertexProgramFilename),
    _fragmentProgramFilename(fragmentProgramFilename)
{}

void GenericVFPProgram::create()
{
    _program = GLProgramFactory::createGLSLProgram(
        _vertexProgramFilename, _fragmentProgramFilename, 
        GLProgramFactory::ProgramType::GameSpecific);
}

void GenericVFPProgram::destroy()
{
    glDeleteProgram(_program);

    GlobalOpenGL().assertNoErrors();
}

void GenericVFPProgram::enable()
{
    glUseProgram(_program);

    glEnableVertexAttribArrayARB(ATTR_TEXCOORD);
    glEnableVertexAttribArrayARB(ATTR_TANGENT);
    glEnableVertexAttribArrayARB(ATTR_BITANGENT);
    glEnableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL().assertNoErrors();
}

void GenericVFPProgram::disable()
{
    glUseProgram(0);

    glDisableVertexAttribArrayARB(ATTR_TEXCOORD);
    glDisableVertexAttribArrayARB(ATTR_TANGENT);
    glDisableVertexAttribArrayARB(ATTR_BITANGENT);
    glDisableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL().assertNoErrors();
}

void GenericVFPProgram::applyRenderParams(const Vector3& viewer,
                                          const Matrix4& localToWorld,
                                          const Params& lightParams)
{
    // TODO
}

} // namespace
