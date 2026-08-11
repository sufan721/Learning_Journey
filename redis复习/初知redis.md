# 初识 Redis

> Redis 很快，快到你还没写完一条 SQL，它已经把结果塞你脸上了。

---

## 认识 NoSQL

SQL 是关系型数据库，那么按照英文来讲，NoSQL 数据库就是**非关系型数据库**。

既然是数据库，那么无非就是增删改查，让我们来对比一下 SQL vs NoSQL：

| 维度 | SQL（关系型） | NoSQL（非关系型） |
|------|--------------|-------------------|
| 数据结构 | 结构化（表、行、列，Schema 严格） | 非结构化（KV、文档、图、列族） |
| 数据关联 | 通过外键 JOIN 维护关系 | 没有 JOIN，关系靠代码层维护 |
| 事务 | ACID 完整体支持 | BASE 理论，多数不支持或弱支持 |
| 扩展方式 | 垂直扩展为主（堆硬件） | 水平扩展为主（加节点） |
| 查询语言 | 统一 SQL | 各有各的 API/DSL |
| 典型代表 | MySQL、PostgreSQL、Oracle | Redis、MongoDB、Cassandra、Neo4j |

> 很多人一听"NoSQL"就觉得是"不要 SQL"，于是把所有关系型数据库一把梭成 NoSQL。结果数据一致性炸了，回头又在 NoSQL 上层手写了一套事务管理 —— 这就是典型的"为了用而用"。**技术选型不是追星，别把工具当信仰。**

### NoSQL 的四大门派

| 类型 | 代表 | 典型场景 |
|------|------|---------|
| KV 存储 | **Redis**、Memcached | 缓存、计数器、Session |
| 文档存储 | MongoDB、CouchDB | 灵活的 JSON 数据、CMS |
| 列族存储 | HBase、Cassandra | 海量日志、时序数据 |
| 图存储 | Neo4j、JanusGraph | 社交关系、推荐系统 |

---

## 认识 Redis

### Redis 是什么？

Redis（**Re**mote **Di**ctionary **S**erver）是一个基于内存的键值对数据库，由 Salvatore Sanfilippo（antirez）在 2009 年用 C 语言写出来的。

> antirez 把 Redis 写得如此优雅，以至于大部分"Redis 源码分析"博客看到第三篇就开始抄来抄去，真正读完 `server.c` 的人屈指可数。

### Redis 凭什么快？

Redis 的单机 QPS 能做到 **10 万+**，不是吹的，原因如下：

1. **纯内存操作** —— 绝大多数操作都在内存完成，磁盘只是备胎（持久化时才用）。
2. **单线程模型** —— 别笑！单线程避免了上下文切换、锁竞争这些多线程的破事。**一个线程干翻你一个线程池，靠的是 IO 多路复用。**
3. **IO 多路复用** —— 用 epoll（Linux）/ kqueue（BSD）等机制，一个线程同时监听多个 Socket，谁有数据就处理谁。
4. **高效的数据结构** —— 每种类型都针对特定场景做了极致优化，后面数据结构篇会详细展开。

> Redis 6.0 引入了多线程，很多人惊呼"Redis 终于不单线程了"——醒醒，多线程只用在**网络 IO 的读写**上，命令执行还是单线程。别拿这个去面试装逼，会被打脸。

### Redis 的"副业"们

你以为 Redis 只是个缓存？它其实是个瑞士军刀：

| 场景 | 用到的特性 |
|------|-----------|
| 缓存 | String，设置 TTL |
| 分布式锁 | `SETNX` + 过期时间 |
| 计数器/限流 | String 自增 + 过期 |
| 排行榜 | SortedSet |
| 消息队列 | List（BRPOP）/ Stream |
| 好友关注/共同好友 | Set 交集并集 |
| 附近的人 | GEO |
| 布隆过滤 | BitMap / RedisBloom 模块 |
| UV 统计 | HyperLogLog |
| 分布式 Session | String / Hash |

> 面试必问 Redis，但 80% 的公司只是把它当 Memcached 用。花里胡哨学了 SortedSet、GEO、Stream，入职发现同事只调 `SET` 和 `GET`，连 `EXPIRE` 都不加 —— **缓存雪崩的种子就是这么埋下的。**

### Redis vs Memcached：别再选错了

| 维度 | Redis | Memcached |
|------|-------|-----------|
| 数据结构 | 丰富（String/Hash/List/Set/ZSet...） | 只支持 String |
| 持久化 | 支持 RDB + AOF | 不支持 |
| 集群 | 原生支持 Cluster | 需要客户端分片 |
| 线程 | 单线程→6.0 引入 IO 多线程 | 多线程 |
| Value 上限 | 512MB | 1MB |
| 适用场景 | 复杂业务、持久化需求 | 纯缓存、大对象缓存 |

> **一句定论：** 你今天能叫得出名字的公司，99% 选 Redis。Memcached 只有在"我只需要缓存，别的啥也不要"的时候才考虑，但问题来了 —— 你确定你永远不需要别的？

---

## Redis 安装与启动

```bash
# Docker 启动（最推荐，环境隔离干净）
docker run -d --name redis -p 6379:6379 redis:latest

# 进入客户端
docker exec -it redis redis-cli

# 经典 PING
127.0.0.1:6379> PING
PONG
```

> `PING → PONG`，这是每个 Redis 新手的第一个 Hello World。它告诉你一个道理：**好的设计连心跳都这么简洁。**

---

## 总结

- **NoSQL 不是 SQL 的替代品，而是补充品。** 该用 MySQL 的地方别硬上 MongoDB，该用 Redis 缓存的地方别让 MySQL 硬扛。
- **Redis 的核心竞争力 = 内存 + 单线程命令执行 + IO 多路复用 + 丰富数据结构。**
- **别把 Redis 只当缓存用** —— 它是一个**数据结构服务器**，缓存只是它最出名的副业。

> 用 Redis 的人分三种：
> 1. 真需要高性能的人 —— 他们认真设计 Key、合理设置 TTL、监控内存。
> 2. 为了简历上有 Redis 的人 —— 他们 `SET` 完就忘，离职留下一个内存快被撑爆的实例。
> 3. 还没用 Redis 但面试被问麻了的人 —— 你大概率是第三种，所以正在看这篇笔记。
>
> 争取做第一种。
