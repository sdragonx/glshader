# glshader
glsl include 的实现。

## 简介
glsl include 的实现，使用 C++ 标准库（c++98），实现了磁盘文件的支持。此库代码简短，并且可以将错误代码正确定位到相关文件。**只支持简单 #include 标记，对于 #define 没有进行解析，通过 #if #else 等预处理命令 include 的方式暂时没有实现**。

## 例子
```
glshader shader;
shader.load("../assets/shader/test.frag");
GLuint id = shader.compile(GL_FRAGMENT_SHADER, true); // 开启调试显示
```

### 生成的调试信息，就是解析 include 组合之后的代码
```
1:
2: #version 330 core
3:
4: //#include "../assets/shader/typedef.glsl"
5:
6: //#version 330 core
...
213:
214:     VERTEX v;
215:
216:     FragColor = frag_color * vec4(light_color, 1.0) * texture_color * get_color(v);
217: }
218: // end ../assets/shader/test.frag
``` 
### 错误日志显示
```
glsl> ERROR: Fragment Shader: "../assets/shader/test.frag"
      0:11:  'aa' : syntax error syntax error
```
