为了让容器之间相互访问，我们需要让他连接上网络，但是自动分配的网络，可能会被占用，让原来可以连接通的网络->连接不通，所以我们需要自定义网络，来让容器之间可以互相访问

| 命令                          | 说明           | 文档地址                        |
| --------------------------- | ------------ | --------------------------- |
| `docker network create`     | 创建一个网络       | `docker network create`     |
| `docker network ls`         | 查看所有网络       | `docker network ls`         |
| `docker network rm`         | 删除指定网络       | `docker network rm`         |
| `docker network prune`      | 清除未使用的网络     | `docker network prune`      |
| `docker network connect`    | 使指定容器连接加入某网络 | `docker network connect`    |
| `docker network disconnect` | 使指定容器连接离开某网络 | `docker network disconnect` |
| `docker network inspect`    | 查看网络详细信息     | `docker network inspect`    |

