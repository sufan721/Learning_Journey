# Redis 数据结构详解

> **金句开篇：** Redis 有 8 种数据结构，面试官最爱问前 5 种，实际工作 80% 的人只用前 2 种。剩下 3 种高级货（GEO/BitMap/HyperLogLog）是你拉开差距的武器。

---

## 总览：Key 与 Value

Redis 的 Key 一般为 String，而 Value 能多种多样：

| 类型 | 一句话概括 | 实战场景 |
|------|----------|---------|
| **String** | 万物皆字符串 | 缓存、计数器、分布式锁 |
| **Hash** | 对象存储的甜蜜点 | 用户信息、购物车 |
| **List** | 有序可重复的双向链表 | 消息队列、时间线 |
| **Set** | 无序不重复的集合 | 标签、点赞用户、共同好友 |
| **SortedSet** | 带着分数的有序集合 | 排行榜、延迟队列 |
| **GEO** | 地理位置计算 | 附近的人、门店搜索 |
| **BitMap** | 位运算，极致省内存 | 签到打卡、布隆过滤 |
| **HyperLogLog** | 基数统计，误差换空间 | UV 统计、去重计数 |

> **批判时刻：** 很多人面试时把 Redis 的 8 种数据结构背得滚瓜烂熟，一到实际项目就只会写 `redis.set(key, JSON.stringify(data))`。**Redis 不是 String 数据库，你把它当 JSON 垃圾桶用，那和 Memcached 有什么区别？**

---

## Key 的层级结构

在实际业务中，Key 可能重名，所以 Redis 支持层级结构，用 `:` 分隔：

```
项目名:业务名:类型:id
```

例如：`mall:user:info:10001`、`mall:order:detail:20240803001`

格式非固定，可根据需求调整。核心原则：**见名知意、方便批量操作。**

> **骚话：** Key 命名不规范，同事两行泪。你在 Redis 里执行 `KEYS *` 的时候，运维的心脏也在骤停 —— `KEYS *` 是 O(n) 的阻塞命令，生产环境禁用，别问为什么，问就是被 Leader 骂过。

### 常用 Key 操作命令

| 命令 | 作用 | 注意 |
|------|------|------|
| `KEYS pattern` | 按模式查 Key | **禁用！** 阻塞整个 Redis |
| `SCAN cursor MATCH pattern` | 渐进式遍历 | 生产环境用它替代 KEYS |
| `EXISTS key` | 判断 Key 是否存在 | — |
| `DEL key` | 删除 Key | 阻塞，大 Key 小心 |
| `UNLINK key` | 异步删除 Key | 4.0+ 推荐，大 Key 用这个 |
| `EXPIRE key seconds` | 设置过期时间 | **不设 TTL 就是内存泄漏！** |
| `TTL key` | 查看剩余过期时间 | -1 未设置，-2 已过期 |
| `TYPE key` | 查看 Value 类型 | — |

> **批判时刻：** 生产环境 Redis 实例里有 50% 的 Key 没有设 TTL，结果内存越用越多，最后 OOM 被系统杀掉。运维重启后数据全丢（因为没开持久化），然后甩锅给 Redis"不稳定"。**Redis 不背这锅，是写代码的人没长脑子。**

---

## String

String 是 Redis 中最简单的存储类型。Value 是字符串，根据格式不同分为三类：

- **string**：普通字符串
- **int**：整数类型，支持自增自减操作（`INCR`/`DECR`）
- **float**：浮点类型，支持自增自减操作

不管哪种格式，底层都是**字节数组**形式存储，只是编码方式不同。String 内部有三种编码：

| 编码 | 条件 | 特点 |
|------|------|------|
| `int` | 值是整数且不超过 8 字节 | 直接用 long 存，最省内存 |
| `embstr` | 字符串 ≤ 44 字节 | 一次分配内存，object + SDS 连续存放 |
| `raw` | 字符串 > 44 字节 | 两次分配内存，object 和 SDS 分开存放 |

> **面试骚操作：** `embstr` 的 44 字节阈值是怎么来的？因为 Redis 的 `jemalloc` 分配器在 64 字节内部分配性能最优：`redisObject` 占 16 字节 + `sdshdr8` 占 3 字节 + 结尾 `\0` 占 1 字节 = 20 字节开销，64 - 20 = 44。**这题背下来，面试官会觉得你读过源码。**

### 常用命令

| 命令 | 作用 |
|------|------|
| `SET key value [EX seconds]` | 设置值 + 过期时间（一步到位） |
| `GET key` | 获取值 |
| `INCR key` / `DECR key` | 自增/自减 1 |
| `INCRBY key n` | 自增 n |
| `SETNX key value` | 不存在才设置（分布式锁核心） |
| `SETEX key seconds value` | 设置 + 过期（原子操作） |
| `MSET` / `MGET` | 批量操作 |

> **骚话：** 分布式锁八股文第一题就是 `SETNX`，但现在真实项目里都用 `SET key value NX EX seconds`，因为它是**原子操作**，不会出现"设了锁没设过期→死锁→半夜报警"的经典事故。

---

## Hash

Hash 类型的 Value 是一个无序字典，类似 Java 的 `HashMap<String, String>`。

当用 String 存 JSON 对象时，修改一个字段需要：**"取出 JSON→解析→修改→序列化→写回"**，不仅代码丑，开销也大。Hash 就是为了解决这个问题 —— **每个字段独立存储，改哪个动哪个。**

### Hash 结构示意

| KEY | field | value |
|-----|-------|-------|
| `user:1001` | `name` | Jack |
|  | `age` | 21 |
|  | `city` | 深圳 |

### Hash 的两种编码

| 编码 | 条件 | 底层 |
|------|------|------|
| `ziplist` | 字段少且短（默认 field≤512, value≤64B） | 压缩列表，连续内存，省空间 |
| `hashtable` | 字段多或长 | 真正的哈希表，查询 O(1) |

> **临界知识：** `ziplist` 虽省内存，但插入时可能触发连锁更新（连续内存重新分配），大 Hash 别指望它。Redis 7.0 已经用 `listpack` 替代了 `ziplist`，解决了连锁更新问题。**如果你还在背 ziplist 的连锁更新当面试亮点，最好补一句"7.0 已修复"。**

### 常用命令

| 命令 | 作用 |
|------|------|
| `HSET key field value` | 设置字段 |
| `HGET key field` | 获取字段 |
| `HMSET key f1 v1 f2 v2` | 批量设置 |
| `HGETALL key` | 获取所有字段 | **大 Hash 慎用!** |
| `HDEL key field` | 删除字段 |
| `HLEN key` | 字段数量 |
| `HEXISTS key field` | 是否存在 |
| `HINCRBY key field n` | 数值自增 |

---

## List

Redis 的 List 是一个**双向链表**，支持正向和反向检索。底层在元素少时用 `quicklist`（3.2 之后统一的实现，结合了 `ziplist` 和 `linkedlist` 的优点），而非单纯的链表。

### 特征

- ✅ 有序
- ✅ 元素可重复
- ✅ 插入和删除快（O(1)）
- ⚠️ 索引查询 O(n)，不是数组随机访问

### 常用命令

| 命令 | 作用 |
|------|------|
| `LPUSH key v1 v2 ...` | 头部插入 |
| `RPUSH key v1 v2 ...` | 尾部插入 |
| `LPOP key` / `RPOP key` | 弹出 |
| `LRANGE key start stop` | 范围查询 |
| `BRPOP key timeout` | 阻塞弹出（消息队列核心） |
| `LLEN key` | 长度 |
| `LTRIM key start stop` | 裁剪（只保留指定范围） |

> **经典面试题：** Redis List 做消息队列有什么问题？
> 1. **不支持消费者组**（一个消息只能被一个消费者消费）
> 2. **不支持 ACK 确认机制**（消息弹出就没了，消费者挂了消息直接丢）
> 3. **没有持久化消费进度**
>
> 所以 Redis 5.0 推出了 **Stream**，这才是正经消息队列。**List 做 MQ 只适合"丢了也不心疼"的场景，别拿它扛核心业务。**

> **骚话：** `LRANGE list 0 -1` 在生产环境相当于 `KEYS *` 的表弟，一样阻塞。大 List 全量查询请用 `LINDEX` 分段取，或者换数据结构。

---

## Set

Redis 的 Set 是一个 **value 为 null 的 Hash 表**，因此具备与 HashSet 类似的特性。它不再以元素为单位，而是一个集合，支持集合间的运算。

### 特征

- ❌ 无序
- ❌ 元素不可重复
- ✅ 查找快（O(1)）
- ✅ 支持**交集、并集、差集** —— 这才是 Set 的灵魂

### 常用命令

| 命令 | 作用 |
|------|------|
| `SADD key member` | 添加 |
| `SREM key member` | 删除 |
| `SMEMBERS key` | 获取所有成员（**大 Set 慎用！**） |
| `SISMEMBER key member` | 是否存在 |
| `SCARD key` | 集合大小 |
| **`SINTER k1 k2`** | **交集** —— 共同好友 |
| **`SUNION k1 k2`** | **并集** —— 全部好友 |
| **`SDIFF k1 k2`** | **差集** —— k1 有但 k2 没有的好友 |
| `SRANDMEMBER key count` | 随机抽 count 个（抽奖用） |

> **批判时刻：** 很多人把 Set 当 List 用，因为"反正都能存数据"。但 Set 的无序+去重意味着**你插不进去重复数据、取不出指定位置的元素**。数据结构的选型不看你"能存什么"，而看你的**业务需要什么操作**。

---

## SortedSet

SortedSet 是**可排序的 Set**，底层由 **跳表（skipList）+ Hash 表** 实现。每个元素带一个 score 属性，基于 score 排序，score 相同时按 member 字典序排。

> **灵魂拷问：** 为什么不用红黑树？
> 红黑树也能排序，但**跳表实现更简单、支持范围查找、并发友好**。在 Redis 这种追求极致性能的场景下，跳表就是最优解。**算法没有绝对的好坏，只有场景合不合适。**

### SortedSet 跳表结构示意

```
Level 3:  [1] ---------------------> [20] -----------> NULL
Level 2:  [1] --------> [7] -------> [20] --> [30] --> NULL
Level 1:  [1] --> [3] -> [7] --> [9] -> [20] -> [30] -> NULL
原始链表: [1] -> [3] -> [7] -> [9] -> [20] -> [30]
```

跳表通过**多层索引**将查找复杂度从 O(n) 降到 **O(log n)**，同时保持了链表的插入删除便利性。每一层都以概率 p（通常 0.25）决定是否向上提升 —— **用随机性换平衡，比 AVL 树少了旋转的开销。**

### 特征

- ✅ 可排序
- ❌ 元素不重复
- ✅ 查询速度快（O(log n)）
- ✅ 范围查询方便（排行榜天然适配）

### 常用命令

| 命令 | 作用 |
|------|------|
| `ZADD key score member` | 添加元素 + 分数 |
| `ZRANGE key start stop [WITHSCORES]` | 按 score 升序取 |
| `ZREVRANGE key start stop` | 按 score 降序取（排行榜 Top N） |
| `ZRANK key member` | 元素升序排名 |
| `ZSCORE key member` | 获取分数 |
| `ZREM key member` | 移除元素 |
| `ZINCRBY key n member` | 分数 + n |
| `ZINTERSTORE dest numkeys k1 k2` | 交集存到新 key |

> **实战金句：** 排行榜是 SortedSet 的"绝杀"场景。`ZREVRANGE rank 0 9 WITHSCORES` 一行命令取 Top 10 榜单，比你在 MySQL 里 `ORDER BY score DESC LIMIT 10` 快一个数量级。**MySQL 做排行榜是"能做"，Redis 是"擅长做"，区别在于当用户量上来了，前者会把你服务器的 CPU 吃干抹净。**

---

## GEO

GEO 是 Redis 3.2 加入的地理位置数据结构，底层其实复用了 **SortedSet**。它将经纬度通过 **Geohash 算法** 编码成一个 score 值，利用 ZSet 的范围查询能力实现地理搜索。

> **骚话：** GEO 的底层是 SortedSet，这不丢人，反而说明 Redis 的设计哲学：**用已有的轮子造新车，而不是每次从零画图纸。**

### 核心原理：Geohash

Geohash 将二维的经纬度编码成一维字符串（如 `wx4g0bm`），**前缀相同的字符越多，位置越近**（但有边界 case，两个点在 Geohash 边界上会很近但前缀不同，所以要做九宫格搜索）。

### 常用命令

| 命令 | 作用 |
|------|------|
| `GEOADD key lng lat member` | 添加地点 |
| `GEOPOS key member` | 获取坐标 |
| `GEODIST key m1 m2 [m/km]` | 计算两点距离 |
| `GEOSEARCH key FROMMEMBER m BYRADIUS 5 km` | 附近的人（6.2+） |
| `GEORADIUS key lng lat 5 km` | 以坐标为中心搜（旧版） |

> **实战注意：** `GEORADIUS` 是 O(n + log m) 的复杂度，n 是返回的成员数，不要想着 "搜全中国 10km 范围内的门店"，你会得到一个超时报警和 DBA 的白眼。

---

## BitMap

BitMap 不是一种"新类型"，它只是 **String 的位操作视图**。一个 String 最大 512MB = 2^32 bit，可以表示 42 亿个标志位，极致的空间利用率。

### 一句话理解

把 String 当成一个巨大的 bit 数组，每个 bit 代表一个状态（0/1），比如：**用户 ID=1000 的人今天签到了吗？就看第 1000 位是不是 1。**

| 命令 | 作用 |
|------|------|
| `SETBIT key offset 1` | 将第 offset 位设为 1 |
| `GETBIT key offset` | 获取第 offset 位的值 |
| `BITCOUNT key [start end]` | 统计 1 的个数（签到总天数） |
| `BITOP AND/OR/XOR dest k1 k2` | 位运算（查哪些用户连续签到） |
| `BITPOS key 1` | 第一个 1 的位置 |

> **实战金句：** 一个亿级用户的签到系统，用 MySQL 存"哪天签到了"是灾难（1 亿用户 × 365 天 = 365 亿行），用 BitMap 只占 1 亿 bit ≈ 12MB。**12MB vs 几 TB，这不是优化，这是降维打击。**

> **但要批判一下：** BitMap 的 Key 是用户 ID 的数字 offset，如果用户 ID 是用 UUID 或者 Twitter Snowflake 的高位随机部分很大，你的 BitMap 会变成稀疏的"黑洞"——大量 bit 位永远用不上却占着内存。**这个时候就该上 Roaring Bitmap（压缩位图）或者布隆过滤器了。工具要对，姿势要对。**

---

## HyperLogLog

HyperLogLog（HLL）是一种**概率统计型**数据结构，用于基数统计（统计"不重复元素个数"，即 UV 统计）。

### 核心特征

- 每个 HLL Key **只占 12KB**，却能统计 2^64 个不同元素
- **误差率 0.81%**（标准误差），绝大多数场景可接受
- **不支持获取元素本身，只能获取估算的基数** —— 存了就取不回来了

| 命令 | 作用 |
|------|------|
| `PFADD key element` | 添加元素 |
| `PFCOUNT key` | 估算不重复元素个数 |
| `PFMERGE dest k1 k2` | 合并多个 HLL |

> **金句：** 统计 1 亿 UV，Set 要几个 GB，HyperLogLog 只要 **12KB**。用 0.81% 的误差换 99.999% 的内存节省 —— 这不叫妥协，这叫**工程智慧**。

> **批判：** 有些人一听到"有误差"就拒绝 HLL，"我不允许任何数据不精确"——结果上线后 Redis 内存被 Set 撑爆，整个缓存层雪崩。**追求绝对精确是数学的事，工程是做取舍。如果你真需要精确去重且产品要求 UV 一个都不差，请上 Flink + ClickHouse，别在 Redis 里硬撑。**

---

## 数据结构底层实现总结

| 类型 | 3.2 之前 | 3.2 之后 | 7.0 之后 |
|------|---------|---------|---------|
| String | raw / embstr / int | 同前 | 同前 |
| Hash | ziplist / hashtable | ziplist / hashtable | **listpack** / hashtable |
| List | ziplist / linkedlist | quicklist | quicklist |
| Set | intset / hashtable | 同前 | 同前 |
| ZSet | ziplist / skiplist+dict | 同前 | **listpack** / skiplist+dict |

> **金句收尾：** Redis 的数据结构，学一个月能面试，学一年能做项目，学三年才能写出"刚好够用又不出事"的设计。**数据结构本身不难，难的是"知道在什么时候用哪个"，而这恰恰是八股文和实战之间最大的鸿沟。**
>
> **跳过八股文，直接去写代码吧。写崩几次，你自然就记住了。**
