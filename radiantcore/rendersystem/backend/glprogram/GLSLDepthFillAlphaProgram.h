#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class GLSLDepthFillAlphaProgram :
    public GLSLProgramBase
{
private:
    GLint _locAlphaTest;
    GLint _locObjectTransform;
    
public:
    void create() override;
    void enable() override;
    void disable() override;

    void setObjectTransform(const Matrix4& transform);

    void applyAlphaTest(float alphaTest);
};

}
