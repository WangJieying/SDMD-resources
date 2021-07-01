#ifndef __GLSLPROGRAM_H
#define __GLSLPROGRAM_H

#include <string>
#include <map>
#include "glslShader.h"

using namespace std;

class GLSLProgram
{
        string file_vertex, file_geom, file_frag;
        char *src_vertex, *src_geom, *src_frag;
        bool has_geometry_shader;

        GLSLShader *shader_vertex, *shader_geom, *shader_frag;
        GLint geom_input, geom_output, geom_vertices;

        map<string, GLuint> uniforms;
        map<string, GLuint> attributes;

    public:
        GLuint prog;

        GLSLProgram(string vertex, string fragment);
        GLSLProgram(string vertex, string fragment, string geometry,
                    GLint geom_input, GLint geom_output, GLint geom_vertices);
        ~GLSLProgram();

        void compile();
        void attachShaders();
        void link();
        void compileAttachLink();
        void reload();
        void bind();
        void unbind();

        void bindUniforms(int count, string *names);
        GLuint uniform(string name);
        void bindAttributes(int count, string *names);
        GLuint attribute(string name);

    private:
        char* loadSourceFromFile(string file);
        void init();

        GLint bindUniform(string name);
        GLint bindAttribute(string name);
};

#endif
