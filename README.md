# glshader
glsl include

## 简介
glsl include 的实现，使用 C++ 标准库（c++98），只实现了磁盘文件的支持，优点是，可以将错误代码正确定位到相关文件。

## 例子
<pre><code>
glshader shader;
shader.load("../assets/shader/test.frag");
GLuint id = shader.compile(GL_FRAGMENT_SHADER, true); // 开启调试显示
</pre></code>

### 生成的调试信息，就是解析 include 组合之后的代码<br>
   1:<br>
   2: #version 330 core<br>
   3:<br>
   4: //#include "../assets/shader/typedef.glsl"<br>
   5:<br>
   6: //#version 330 core<br>
   ...<br>
   213:<br>
   214:     VERTEX v;<br>
   215:<br>
   216:     FragColor = frag_color * vec4(light_color, 1.0) * texture_color * get_color(v);<br>
   217: }<br>
   218: // end ../assets/shader/test.frag<br>
 
### 错误日志
  glsl> ERROR: Fragment Shader: "../assets/shader/test.frag"<br>
        0:11:  'aa' : syntax error syntax error<br>
