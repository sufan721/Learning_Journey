### docker run
	-d 让容器后台运行
	--name 给容器命名
	-e 环境变量
	-p 主机端口 : 容器内端口
	镜像结构名：
		镜像名 : 版本
#### 示例
安装最新版**nginx**
```bash
docker run -d --name nginx -p 80:80  nginx
```

---
他和git一样分为镜像仓库和本地镜像，
从镜像仓库到本地镜像 ：docker pull 
查看本地镜像 ： docker images
 删除本地镜像： docker rmi
 从dockerfile来自己构建镜像： docker build
 保存到本地为压缩文件： docker save
 加载本地压缩文件：docker load
 发布本地镜像到镜像仓库： docker push
 如果容器已经存在由停止转为启动： docker start
 查看容器是否存在 docker ps
 删除容器docker rm
 查看容器日志 docker logs
 进入容器 decker exec
查看容器信息 docker inspect 容器名


![ docker命令](文档图片/docker-命令.png )


### 测试案例
#### 需求：
- 在Docker Hub中搜索nginx镜像，并查看镜像的名称
- 拉取Nginx镜像
- 查看镜像列表
- 创建并运行Nginx容器
- 查看容器
- 停止容器
- 再次启动容器
- 进入Nginx容器
- 删除容器

```
#如果不会--help
docker pull nginx
docker images
docker save -o Nginx.tar nginx:latest
docker rmi nginx 
docker load -i nginx.tar
docker -d --name Nginx -p 80:80 nginx
docker ps 
docker exec -it nginx bash
	exit
docker stop nginx
docker rm nginx
```