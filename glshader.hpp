/*
 Copyright (c) 2005-2020 sdragonx (mail:sdragonx@foxmail.com)

 glshader.hpp

 2022-01-24 14:06:16

*/
#ifndef GLSHADER_HPP_20220124140616
#define GLSHADER_HPP_20220124140616

#if defined(__glew_h__) || defined(__GLEW_H__)
#include <GL/glew.h>
#elif defined(GLAD_GL_H_)
#include <glad/gl4.6.h>
#else
#include <GL/GL.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

namespace cgl {
namespace graphics {

// ����ַ���ǰ��ո�
inline std::string trim(const std::string& str, const std::string& spaces = " \t\r\n")
{
    size_t a = str.find_first_not_of(spaces);
    size_t b = str.find_last_not_of(spaces) + 1;
    if (b <= a) {
        return std::string();
    }
    return str.substr(a, b - a);
}

/* ��ȡ�ļ�����ָ��
 *
 * ��� shader ����Դ���棬���� Android �İ����棬�����Զ���һ�����غ���
 * int myLoadFile(std::stringstream& stm, const std::string& filename, void* arg)
 * {
 *     // load string to stream
 *     std::string str = my_read_data(filename);
 *     stm.str(str);
 *     return 0; // success
 * }
 */
typedef int(*PFN_GLSHADER_LOAD_FILE)(std::stringstream& stm, const std::string& filename, void* arg);

//
// shader Դ��
//

class glshader
{
public:
    // source ������
    enum line_mode
    {
        GLSL_CODE,              // ���룬����ע�͡����е�
        GLSL_VERSION,           // �汾��Ϣ
        GLSL_INCLUDE,           // ���á������� "" ���� <>����ֻ������Դ��Ŀ¼
    };

    // Դ����
    struct glsl_line
    {
        line_mode mode;         // ��ģʽ
        std::string source;     // Դ

        glsl_line(line_mode m = GLSL_CODE, const std::string& str = std::string()) :
            mode(m),
            source(str)
        {

        }
    };

    // Դ��
    struct glsl_source
    {
        std::string filename;
        std::string version;
        bool included;
        std::vector<glsl_line> lines;

        glsl_source()
        {
            included = false;
        }
    };

private:
    // shader Դ����
    struct shader_line
    {
        const glsl_source* source;
        size_t line;

        shader_line(const glsl_source* src, size_t n) : source(src), line(n)
        {
        }
    };

    typedef std::map<std::string, glsl_source>::iterator iterator;

    GLuint m_id;                                    // ������ shader ID
    std::map<std::string, glsl_source> m_sources;   // ���ص�����Դ��
    std::vector<shader_line> m_lines;               // shader ����Դ��ṹ
    glsl_source* m_main;                            // ��Դ��

    PFN_GLSHADER_LOAD_FILE m_pfn_load_file;         // �Զ����ȡ�ļ�����
    void* m_arg;                                    // �Զ����ȡ�ļ������Ķ������

public:
    glshader() : m_id(GL_NONE), m_sources(), m_lines(), m_main()
        , m_pfn_load_file(nullptr), m_arg(nullptr)
    {

    }

    ~glshader()
    {
        this->dispose();
    }

    // ���� shader ID
    GLuint id()const
    {
        return m_id;
    }

    operator GLuint()const
    {
        return m_id;
    }

    // �ж��Ƿ�Ϊ��
    bool is_null()const
    {
        return m_id == GL_NONE;
    }

    // �����Զ����ļ����غ���
    void set_load_function(PFN_GLSHADER_LOAD_FILE pfn, void* arg)
    {
        m_pfn_load_file = pfn;
        m_arg = arg;
    }

    // ��ȡ�Զ����ļ����غ���
    PFN_GLSHADER_LOAD_FILE load_function()const
    {
        return m_pfn_load_file;
    }

    // ��ȡ�Զ����ȡ����
    void* load_arg()const
    {
        return m_arg;
    }

    // ����Դ�� gles
    int load(GLenum type, const std::string& filename, bool debug = false)
    {
        this->dispose();

        // ������Դ��
        if (this->load_source(filename) != 0) {
            this->dispose();
            return -1;
        }

        // ��¼��Դ��
        m_main = this->find_source(filename);

        // Ԥ����
        if (this->process_sources(filename) != 0) {
            return -1;
        }

        m_id = compile(type, debug);

        return m_id == 0 ? -1 : 0;
    }

    // �ͷ�
    void dispose()
    {
        if (m_id != GL_NONE) {
            glDeleteShader(m_id);
            m_id = GL_NONE;
        }
        m_sources.clear();
        m_lines.clear();
        m_main = NULL;
    }

    void attach(GLuint program)
    {
        glAttachShader(program, m_id);
    }

    void detach(GLuint program)
    {
        glDetachShader(program, m_id);
    }

protected:
    // ���� source
    glsl_source* create_source(const std::string& filename)
    {
        glsl_source* source = &m_sources.insert(std::make_pair(filename, glsl_source())).first->second;
        source->filename = filename;
        return source;
    }

    // ���� source
    glsl_source* find_source(const std::string& filename)
    {
        iterator itr = m_sources.find(filename);
        if (itr != m_sources.end()) {
            return &itr->second;
        }
        return NULL;
    }

    // �����ļ����ַ�����
    int load_file(std::stringstream& stm, const std::string& filename)
    {
        std::ifstream in;
        in.open(filename.c_str());
        if (in.is_open()) {
            stm << in.rdbuf();
            in.close();
            return 0;
        }
        return -1;
    }

    // ���� glsl �� map ��
    int load_source(const std::string& filename)
    {
        using namespace std;

        glsl_source* source;

        // ����Ƿ��Ѿ����ع�
        source = find_source(filename);
        if (source) {
            return 0;
        }

        // �����ļ�
        std::stringstream in;
        int n = -1;

        if (m_pfn_load_file) {// �Զ�����غ���
            n = m_pfn_load_file(in, filename, m_arg);
        }
        else {// Ĭ�ϴ��̼��غ���
            n = this->load_file(in, filename);
        }

        if (n != 0) {
            cout << "glsl> file open failed: " << filename << endl;
            return -1;
        }
        else {
            cout << "glsl> load source \"" << filename << "\"" << endl;
        }

        // ���� map ����
        source = create_source(filename);

        // ����Ŀ¼
        string dir = filename.substr(0, filename.find_last_of("/\\") + 1);

        string line;
        string tag;
        size_t pos;

        // �����ļ�
        while (!in.eof()) {
            getline(in, line);

            if (line.empty()) {
                source->lines.push_back(glsl_line(GLSL_CODE, line));
                continue;
            }

            pos = line.find_first_not_of(" \t");

            if (pos != string::npos && line[pos] == '#') {
                stringstream stm(line);
                stm >> tag;

                if (tag == "#version") {
                    stm >> tag;
                    source->version = tag;   // ���汾
                    stm >> tag;
                    if (!tag.empty() && tag[0] != '/') {    // �汾������
                        source->version.push_back(' ');
                        source->version.append(tag);
                    }
                    //cout << "glsl> version " << source->version << endl;
                    source->lines.push_back(glsl_line(GLSL_VERSION, source->version));
                }
                else if (tag == "#include") {
                    stm >> tag;
                    tag = tag.substr(0, tag.find('/')); // ����ע��
                    tag = trim(tag, " \t\r\n\"<>");
                    //cout << "glsl> include \"" << tag << "\"" << endl;

                    // ���� include �ļ�
                    string tempFile = dir + tag;
                    if (this->load_source(tempFile) != 0) {
                        return -1;
                    }

                    source->lines.push_back(glsl_line(GLSL_INCLUDE, tempFile));
                }
                else {
                    source->lines.push_back(glsl_line(GLSL_CODE, line));
                }
            }
            else {
                source->lines.push_back(glsl_line(GLSL_CODE, line));
            }
        }// end while

         // ����ļ���β��ʶ����ϴ�����֤���������ļ�����
        tag = "// end ";
        tag.append(filename);
        source->lines.push_back(glsl_line(GLSL_CODE, tag));

        return 0;
    }

    // Ԥ�������
    int process_sources(const std::string filename)
    {
        glsl_source* source = this->find_source(filename);

        if (!source) {
            return -1;
        }

        if (source->included) {
            return 0;
        }

        // �жϰ汾
        if (source != m_main) {
            if (source->version != m_main->version) {
                std::cout << "glsl> different source code versions: " << filename << std::endl;
                return -1;
            }
        }

        for (size_t i = 0; i < source->lines.size(); ++i) {
            const glsl_line& line = source->lines[i];

            switch (line.mode) {
            case GLSL_CODE:
                m_lines.push_back(shader_line(source, i));
                break;
            case GLSL_VERSION:
                m_lines.push_back(shader_line(source, i));
                break;
            case GLSL_INCLUDE:
                m_lines.push_back(shader_line(source, i));
                process_sources(line.source);
                break;
            default:
                break;
            }
        }

        // ����Ѿ�����
        source->included = true;

        return 0;
    }

    // ���� shader ��������
    static const char* shader_type_to_string(GLenum type)
    {
        switch (type) {
        case GL_FRAGMENT_SHADER:
            return "fragment shader";
        case GL_VERTEX_SHADER:
            return "vertex shader";
        case GL_GEOMETRY_SHADER:
            return "geometry shader";
        case GL_TESS_EVALUATION_SHADER:
            return "tess evaluation shader";
        case GL_TESS_CONTROL_SHADER:
            return "tess control shader";
        case GL_COMPUTE_SHADER:
            return "compute shader";
        default:
            return "unknown shader";
        }
    }

    // create a shader object, load the shader source, and compile the shader.
    GLuint compile_shader(GLenum type, const char *source)
    {
        GLuint shader;

        // create the shader object
        shader = glCreateShader(type);

        if (!shader) {
            printf("glCreateShader failed.\n");
            return GL_NONE;
        }

        // load the shader source
        glShaderSource(shader, 1, &source, NULL);

        // Compile the shader
        glCompileShader(shader);

        // check the compile status
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen > 1) {
                std::vector<char> log(infoLen); // ������־����
                glGetShaderInfoLog(shader, infoLen, NULL, &log[0]);
                this->report(type, &log[0]);
            }

            glDeleteShader(shader);
            return GL_NONE;
        }

        return shader;
    }

    // �������
    void report(GLenum type, const char* log)
    {
        using namespace std;

        stringstream in(log);
        string line;
        string tag;
        string pos;
        string a, b;
        size_t x, y;

        while (!in.eof()) {
            getline(in, line);

            if (line.empty()) {
                cout << endl;
                continue;
            }

            stringstream stm(line);

            stm >> tag;
            if (tag == "ERROR:" || tag == "WARNING:") {
                // ��������λ��
                stm >> pos;
                x = pos.find(':');
                a = pos.substr(0, x);
                b = pos.substr(x + 1, pos.length() - x - 2);

                x = atoi(a.c_str());
                y = atoi(b.c_str());

                // ���������Դ�룬y ֵ��һ����ʼ���� 0
                if (m_lines[y].source != m_main) {
                    --y;
                }

                const shader_line& shaderLine = m_lines[y];

                // �����к�����ʼ 1
                //cout << "================================" << endl;
                cout << "glsl> " << tag << ' ' << shader_type_to_string(type)
                    << ": \"" << shaderLine.source->filename << "\"" << endl;
                cout << "      " << x << ":" << shaderLine.line + 1
                    << ": " << line.substr(stm.tellg()) << endl;
            }
            else {
                cout << "glsl> " << log;
            }
        }
    }

    // ���룬���� shader ID
    GLuint compile(GLenum type, bool debug = false)
    {
        using namespace std;

        // �Ƿ��Ѿ�����汾���
        bool tag_version = false;
        std::string code;

        for (size_t i = 0; i < m_lines.size(); ++i) {
            size_t id = m_lines[i].line;
            const glsl_line& line = m_lines[i].source->lines[id];

            switch (line.mode) {
            case GLSL_CODE:
                code.append(line.source);
                code.push_back('\n');
                if (debug) cout << i + 1 << ": " << line.source << endl;
                break;
            case GLSL_VERSION:
                if (tag_version) {
                    code.append("//");
                    if (debug) cout << i + 1 << ": //#version " << line.source << endl;
                }
                else {
                    tag_version = true;
                    if (debug) cout << i + 1 << ": #version " << line.source << endl;
                }
                code.append("#version ");
                code.append(line.source);
                code.push_back('\n');
                break;
            case GLSL_INCLUDE:
                code.append("//");
                code.append(line.source);
                code.push_back('\n');

                if (debug) cout << i + 1 << ": //#include \""<< line.source <<"\"" << endl;
                break;
            default:
                break;
            }
        }

        // ����
        return compile_shader(type, code.c_str());
    }
};

}// end namespace graphics
}// end namespace cgl

#endif// GLSHADER_HPP_20220124140616
