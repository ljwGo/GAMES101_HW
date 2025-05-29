[toc]

# 0. 序言

​	为了将作业从ubuntu环境移植到window环境，真的是费了一番功夫。

​	CGL是苹果开发的**服务于图形学的简单库**。它包含了glad，glfw，还有一些简单的数学操作代码（如矩阵，向量）。

# 1. glad/glew/glut/freeglut/glfw的联系和区别

​	简单来说，glad是glew的升级。它们用来**获取函数实现（包括一些扩展函数）在内存的位置**。因为opengl是一套标准，真正的实现交由GPU厂商实现，作为二进制保存起来。

​	freeglut是glut的替换版本，用于**窗口控制**，glut很久没有更新了，它的许可证不允许增量开发。opengl原本的函数只操作GPU。

​	同样的，glfw也是轻量级的跨平台窗口控制库。

​	参考[OpenGL之gult/freeglut/glew/glfw/glad的联系与区别_opengl中的glfw,glew,stb,glm是干嘛呢的-CSDN博客](https://blog.csdn.net/qq_38446366/article/details/115328051)

# 2. cmake生成sln项目报freetype字体库缺失

## 2.1 安装vcpkg

参考[Unable to make on Windows 10. Can't find FreeType · Issue #36 · jpbruyere/vkvg (github.com)](https://github.com/jpbruyere/vkvg/issues/36)

​	就像上面提到的那样，自己下载编译freetype，不知道怎么配置cmake。

​	这里使用**vcpkg（c++包管理工具）**来下载freetype。安装vcpkg和使用vcpkg安装freetype的步骤就在上面博客内了。



详细的vcpkg使用方法--https://zhuanlan.zhihu.com/p/87391067

cmake环境搭建--[Windows下CMake编译环境搭建 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/576408483)



## 2.2 cmake兼容旧语法 (启动旧策略)

​	3.1之后就对一些语法进行了改变, CGL使用的2.8版本.

[CMake 报错Warning (dev) at XXX、Policy CMP0054 is not set | 九七的CMake常见问题集锦_cmake warning (dev)-CSDN博客](https://blog.csdn.net/qq_42495740/article/details/118565969)



## 2.2 git秘钥问题

​	不知道为什么git的秘钥失效了（似乎是email值错误导致的）。

​	我重新生成了秘钥，打算替换掉。**期间我修改了秘钥的默认保存路径和名称**，**需要使用ssh-agent将秘钥手动添加到代理中**。

```bash
ssh-agent bash
ssh-add -l查看代理秘钥列表
ssh-add 秘钥路径
```

但这个添加是**临时的**，要持久生效。在.ssh目录下生成config文件，内容如下：

```bash
Host  github.com
  HostName github.com
  IdentityFile ~/.ssh/github_ed25519
```

记得确保name和email是正确的。**同时删除known_hosts文件**

期间还知道有什么**端口屏蔽，DNS污染问题**

可以使用

```
ssh -vT git@github.com
```

来测试是否连通

参考[Github配置SSH密钥连接（附相关问题解决） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/628727065)



## 2.3 powershell兼容问题

​	在使用vcpkg安装freetype时，发现**powershell版本过低**。需要自己安装高版本powershell，并将exe加入到环境变量中（安装很简单）。

​	打开powershell.exe。

* 在powershell中使用$host查看版本
* $PSHOME查看安装路径



## 2.4 使用cmake构建sln项目

​	使用命令构建项目

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake
```



# 3. 解决sln项目连接器提示无法识别的外部符号

​	就是找不到lib或是dll文件。

​	问题是**glew提供的是lib静态库，默认情况使用dll库**，解决方法--**在导入glew.h文件前宏定义GLEW_STATIC指明要使用lib静态库**。

​	importance——**必须在文件中添加#define GLEW_STATIC，而不是visual stadio的资源管理器中**

参考[VS 2010中的Glew :未解析的外部符号__imp__glewInit-腾讯云开发者社区-腾讯云 (tencent.com)](https://cloud.tencent.com/developer/ask/sof/65010)



# 4. cmake构建sln项目中的ALL_BUILD和INSTALL（了解）

参考[cmake编译后的sln中ALL_BUILD和INSTALL项目解析-CSDN博客](https://blog.csdn.net/weixin_44120025/article/details/115196643)