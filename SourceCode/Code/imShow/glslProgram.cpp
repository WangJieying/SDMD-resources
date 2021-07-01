#include <GL/glew.h>
#include <GL/freeglut.h>

#include "include/messages.h"
#include "include/glslProgram.h"

using namespace std;

GLSLProgram::GLSLProgram(string vertex, string fragment) {
    has_geometry_shader = false;
    file_vertex = vertex;
    file_frag = fragment;

    init();
}

GLSLProgram::GLSLProgram(string vertex, string fragment, string geometry,
                         GLint input, GLint output, GLint vertices) {
    has_geometry_shader = true;
    file_vertex = vertex;
    file_frag = fragment;
    file_geom = geometry;
    geom_input = input;
    geom_output = output;
    geom_vertices = vertices;

    init();
}

/* Refactor? (3x the same thing, for 3 different programs) */
GLSLProgram::~GLSLProgram() {
    glDetachShader(prog, shader_vertex->shader);
    delete shader_vertex;
    if (has_geometry_shader) {
        glDetachShader(prog, shader_geom->shader);
        delete shader_geom;
    }
    glDetachShader(prog, shader_frag->shader);
    delete shader_frag;
    glDeleteProgram(prog);
}

void GLSLProgram::init() {
    shader_vertex = new GLSLShader(file_vertex, GL_VERTEX_SHADER);
    if (has_geometry_shader)
        shader_geom = new GLSLShader(file_geom, GL_GEOMETRY_SHADER_EXT);
    shader_frag = new GLSLShader(file_frag, GL_FRAGMENT_SHADER);
}

void GLSLProgram::compile() {
    if (shader_vertex == NULL || shader_frag == NULL) {
        PRINT(MSG_ERROR, "Vertex shader did not initialize properly. Exiting.\n");
    }
    shader_vertex->compile();
    if (has_geometry_shader) shader_geom->compile();
    shader_frag->compile();
}

void GLSLProgram::attachShaders() {
    prog = glCreateProgram();

    glAttachShader(prog, shader_vertex->shader);
    if (has_geometry_shader) glAttachShader(prog, shader_geom->shader);
    glAttachShader(prog, shader_frag->shader);
}

void GLSLProgram::link() {
    GLint link_status;
    GLint length;
    GLchar info [1000];

    if (has_geometry_shader) {
        glProgramParameteriEXT(prog, GL_GEOMETRY_INPUT_TYPE_EXT, geom_input);
        glProgramParameteriEXT(prog, GL_GEOMETRY_OUTPUT_TYPE_EXT, geom_output);
        glProgramParameteriEXT(prog, GL_GEOMETRY_VERTICES_OUT_EXT, geom_vertices);
    }

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
    PRINT(MSG_VERBOSE, "Program link status: %d\n", link_status);
    if (link_status == GL_TRUE) {
        PRINT(MSG_VERBOSE, "Shader program linked succesfully\n");
    } else {
        PRINT(MSG_ERROR, "Error while linking shader program!\n");
        glGetShaderInfoLog(prog, 1000, &length, info);
        PRINT(MSG_ERROR, "%s\n", info);
        exit(1);
    }
}

void GLSLProgram::compileAttachLink() {
    compile();
    attachShaders();
    link();
}

void GLSLProgram::reload() {
}

void GLSLProgram::bind() {
    glUseProgram(prog);
}

void GLSLProgram::unbind() {
    glUseProgram(0);
}

void GLSLProgram::bindUniforms(int count, string *names) {
    int i;
    GLint binding;
    string name;

    for (i = 0; i < count; i++) {
        name = names[i];
        binding = bindUniform(name);
        PRINT(MSG_ERROR, "%d\n", binding);
        if (binding != -1) {
            PRINT(MSG_VERBOSE, "Bound uniform '%s'\n", name.c_str());
            uniforms[name] = binding;
        } else {
            PRINT(MSG_ERROR, "Unable to bind uniform '%s'\n", name.c_str());
        }
    }
}

GLuint GLSLProgram::uniform(string name) {
    return uniforms[name];
}

GLint GLSLProgram::bindUniform(string name) {
    return glGetUniformLocation(prog, name.c_str());
}

void GLSLProgram::bindAttributes(int count, string *names) {
    int i;
    GLint binding;
    string name;

    for (i = 0; i < count; i++) {
        name = names[i];
        binding = bindAttribute(name);
        if (binding != -1) {
            PRINT(MSG_VERBOSE, "Bound attribute '%s'\n", name.c_str());
            attributes[name] = binding;
        } else {
            PRINT(MSG_ERROR, "Unable to bind attribute '%s'\n", name.c_str());
        }
    }
}

GLuint GLSLProgram::attribute(string name) {
    return attributes[name];
}

GLint GLSLProgram::bindAttribute(string name) {
    return glGetAttribLocation(prog, name.c_str());
}
