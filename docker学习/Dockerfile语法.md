##### 镜像结构
镜像中包含了应用程序所需要的运行环境、函数库、配置、以及应用本身等各种文件，这些文件分层打包而成


为了做出一个自己的镜像，我们可以用一个Dockerfile，来告诉docker我们需要执行什么指令

| 指令         | 说明                             | 示例                                                                 |
| ---------- | ------------------------------ | ------------------------------------------------------------------ |
| FROM       | 指定基础镜像                         | `FROM centos:6`                                                    |
| ENV        | 设置环境变量，可在后面指令使用                | `ENV key value`                                                    |
| COPY       | 拷贝本地文件到镜像的指定目录                 | `COPY ./jre11.tar.gz /tmp`                                         |
| RUN        | 执行 Linux 的 shell 命令，一般是安装过程的命令 | `RUN tar -zxvf /tmp/jre11.tar.gz && EXPORTS path=/tmp/jre11:$path` |
| EXPOSE     | 指定容器运行时监听的端口，是给镜像使用者看的         | `EXPOSE 8080`                                                      |
| ENTRYPOINT | 镜像中应用的启动命令，容器运行时调用             | `ENTRYPOINT java -jar xx.jar`                                      |


在编写好了Dockfile后，构建镜像
```bash
docker build -t  Myinmage:1.0 .
```
-t 给镜像起名，不指定版本默认为latest
. 指定Dockfile所在目录，." 为当前目录

