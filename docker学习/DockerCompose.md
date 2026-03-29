现在的docker还有一个问题，那就是在多个关联的容器中，需要一个一个去部署，不方便 -> DockerCompose出现了，

DockerCompose可以通过一个单独的docker-compose.yml 模板文件来自定义一组相关联的应用容器，帮助我们解决多个相互关联的Docker的快速部署
示例
```
version: '3.8'
services:
# MySQL 8.0 数据库
  mysql:
    image: mysql:8.0  
    container_name: mysql
    restart: always
    ports: - "3306:3306"
    environment:
    MYSQL_ROOT_PASSWORD: root123456
    # root 密码
    MYSQL_DATABASE: mydb
    # 自动创建的数据库
    volumes: - ./mysql/data:/var/lib/mysql
      # 数据持久化
      networks: - app-network
# Redis 缓存
  redis:
    image: redis:7-alpine
    container_name: redis
    restart: always
    ports: - "6379:6379"
    volumes: - ./redis/data:/data # 数据持久化
    command: redis-server --appendonly yes # 开启持久化
    networks:
    - app-network
# Nginx 网页服务
  nginx:
    image: nginx:stable-alpine
    container_name: nginx
    restart: always
    ports:
    - "80:80"
    - "443:443"
    volumes:
    - ./nginx/html:/usr/share/nginx/html # 网站文件
    - ./nginx/conf:/etc/nginx/conf.d # 配置文件
    networks:
    - app-network
    # 统一网络，服务之间可以用服务名互相访问
    networks: app-network:
    driver: bridge
```


![ dockercompose的使用方法](文档图片/docker_compose.png )
