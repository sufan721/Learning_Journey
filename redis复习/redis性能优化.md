# Redis 性能优化

> **金句开篇：** Redis 单机能跑到 10 万 QPS，但写出只有 1000 QPS 的 Redis 代码是很多人的日常。性能优化不是加机器，是别把 Ferrari 开成拖拉机。

---

## 性能优化的三个维度

```
         CPU
        /    \
   内存 ────── 网络
        \    /
        磁盘
```

Redis 的性能瓶颈通常在这三个地方：
1. **网络** —— 往返次数太多（RTT 放大效应）
2. **CPU** —— 慢命令、Lua 脚本阻塞、fork 子进程
3. **内存** —— 大 Key、内存碎片、编码不当

> **骚话：** 90% 的 Redis 性能问题不是 Redis 的锅，是**用的人写烂了**。就像你开法拉利挂一档踩到红区怪发动机不行——不是车的问题，是你的问题。

---

## 一、慢查询：找到那颗老鼠屎

### 配置慢查询日志

```conf
# 超过 10 毫秒的命令记入慢查询日志
slowlog-log-slower-than 10000

# 慢查询日志最多存 128 条（FIFO 队列，满了就覆盖）
slowlog-max-len 128
```

### 查看慢查询

```bash
# 查看最近 10 条慢查询
SLOWLOG GET 10

# 每条记录包含:
# 1) 自增 ID
# 2) 执行时刻的 Unix 时间戳
# 3) 执行耗时（微秒）
# 4) 命令和参数
# 5) 客户端 IP:Port
# 6) 客户端名称

# 查看当前有多少条
SLOWLOG LEN

# 清空慢查询
SLOWLOG RESET
```

### 常见慢命令黑名单

| 命令 | 复杂度 | 为什么慢 | 替代方案 |
|------|--------|---------|---------|
| `KEYS *` | O(n) | 全表扫描，阻塞！ | `SCAN` 渐进遍历 |
| `SMEMBERS` | O(n) | 大 Set 全量返回 | `SSCAN` |
| `HGETALL` | O(n) | 大 Hash 全量返回 | `HSCAN` |
| `ZRANGE key 0 -1` | O(log n + m) | 大 ZSet 全量返回 | 分页取 |
| `SORT` | O(n + m log m) | 排序开销大 | 用 ZSet 天然排序 |
| `FLUSHDB/FLUSHALL` | O(n) | 清空数据，同步阻塞 | 4.0+ 用 `FLUSHDB ASYNC` |
| `DEL bigkey` | O(n) | 释放大量内存阻塞 | `UNLINK` 异步删除 |
| Lua 脚本（长） | 取决于脚本 | Lua 执行期间 Redis 完全阻塞 | 拆短或用应用层逻辑 |

> **金句：** `KEYS *` 是 Redis 界的 `SELECT * FROM table`——在开发环境没人管你，在生产环境那就是在杀人。**慢查询日志是你的早报警系统，每天看一眼，胜过事后改三天。**

---

## 二、Pipeline：用一次网络往返干 N 件事

### 没有 Pipeline 的悲剧

```
客户端                 Redis
  │──── SET k1 v1 ────>│
  │<─── OK ────────────┤  (RTT: 0.5ms)
  │──── SET k2 v2 ────>│
  │<─── OK ────────────┤  (RTT: 0.5ms)
  │──── GET k1 ───────>│
  │<─── "v1" ──────────┤  (RTT: 0.5ms)
  ... 100 个命令 = 100 × RTT = 50ms

  实际 Redis 处理每个命令只需要 **微秒级**，
  99% 的时间都花在了网络往返上！
```

### 有了 Pipeline 之后

```
客户端                 Redis
  │─ SET k1 v1 ───────────>│  (一次发送所有命令)
  │─ SET k2 v2 ───────────>│
  │─ GET k1 ──────────────>│
  │─ GET k2 ──────────────>│
  │                         │  Redis 批量处理
  │<── OK ─────────────────┤
  │<── OK ─────────────────┤
  │<── "v1" ───────────────┤
  │<── "v2" ───────────────┤  (一次返回所有结果)

  100 个命令 = 1 × RTT + 处理时间 ≈ 0.5ms
  快了 100 倍！
```

> **骚话：** Pipeline 是 Redis 性能优化的鼻祖级技巧。不是 Redis 慢，是你的网络延迟在谋杀 QPS。**在同一个机房内 RTT 是 0.1ms，跨机房可能是 10ms——同样是 1000 条命令，前者 100ms 搞定，后者 10 秒。你的代码一样，性能差 100 倍。**

### Pipeline Java 实现

```java
// Jedis 同步 Pipeline
Pipeline pipeline = jedis.pipelined();
for (int i = 0; i < 1000; i++) {
    pipeline.set("key:" + i, "value:" + i);
}
pipeline.sync(); // 一次性发送，等待全部返回

// Spring Data Redis 通过 executePipelined
List<Object> results = redisTemplate.executePipelined(
    new SessionCallback<Object>() {
        @Override
        public Object execute(RedisOperations ops) {
            for (int i = 0; i < 1000; i++) {
                ops.opsForValue().set("key:" + i, "value:" + i);
            }
            return null;
        }
    }
);
```

### Pipeline 使用注意事项

| 注意 | 说明 |
|------|------|
| **不要一次塞太多** | 一次 Pipeline 塞 10 万条，会撑爆内存和网络 Buffer，拆成 100 条一批 |
| **不保证原子性** | Pipeline 不是事务！其他客户端的命令可能在你的 Pipeline 中间执行 |
| **失败处理** | Pipeline 里一条命令失败，不影响其他命令（和事务不一样） |
| **Cluster 环境** | Pipeline 只能对同一个节点发，跨 slot 的 Key 需要自己分组 |

---

## 三、大 Key：Redis 性能的头号杀手

### 大 Key 的定义

| 类型 | 大 Key 阈值 | 危险等级 |
|------|-----------|---------|
| String | > 10KB | 🟡 中等 |
| String | > 10MB | 🔴 严重 |
| Hash/List/Set/ZSet | 元素数 > 5000 | 🟡 中等 |
| Hash/List/Set/ZSet | 元素数 > 50000 | 🔴 严重 |

### 大 Key 的危害

```bash
# 1. 引起阻塞
DEL bigkey         # O(n) 释放内存，阻塞主线程
HGETALL big_hash   # 一次返回 10 万元素，网络风暴

# 2. 内存不均匀
# Cluster 里一个 Master 因为大 Key 占了 80% 内存，其他 Master 闲着

# 3. 主从同步慢
# 大 Key 导致 RDB 文件巨大，fork + 传输全部变慢

# 4. 过期/淘汰卡顿
# 大 Key 过期时，Redis 同步删除 → 阻塞
```

### 如何扫描大 Key

```bash
# 自带工具：扫描整个 Redis 实例
redis-cli --bigkeys

# 返回示例：
# Biggest string found: "user:session:10086" has 5242880 bytes
# Biggest hash   found: "order:cache" has 12345 fields

# 注意: --bigkeys 是 O(n) 扫描，生产环境在低峰期跑！
# 它只返回每种类型最大的那个 Key，不是全部大 Key 列表
```

```bash
# 更细致：用 MEMORY USAGE 检查单个 Key
MEMORY USAGE user:session:10086
# 返回: 5242890 (字节)

# 检查 Key 的类型和大小
DEBUG OBJECT user:session:10086
# 返回: Value at:... refcount:1 encoding:raw serializedlength:5242880 lru:...
```

### 大 Key 的处置方案

| 方案 | 做法 | 适用场景 |
|------|------|---------|
| **拆分** | `user:1001:info` → `user:1001:name`, `user:1001:age`... | Hash 大对象 |
| **分桶** | `timeline:1001` → `timeline:1001:page1`, `:page2`... | List 时间线 |
| **异步删除** | `UNLINK` 替代 `DEL` | 删除大 Key 必须用 |
| **压缩** | Value 用 Snappy/Gzip 压缩后再存 | String 大于 10KB |
| **换个存储** | 超过 10MB 或 10 万元素，放 HBase/MongoDB/OSS | 不是 Redis 该存的东西 |

```java
// 大 Hash 拆分示例：用户信息 JSON → 分散到多个 Field
// 之前（大 String）:
redis.set("user:1001", hugeJsonString);  // 50KB JSON，改一个字段全量更新

// 之后（Hash）:
redis.hset("user:1001", "name", "Jack");
redis.hset("user:1001", "age", "21");
redis.hset("user:1001", "preferences", preferencesJson);  // 就算这个很大，也是独立的
```

> **批判时刻：** 大 Key 的罪魁祸首往往是"图省事"——把一整个对象序列化成 JSON 塞进一个 String。**"我先这样存，以后出问题再优化" = "我先把定时炸弹塞进去，以后再拆弹。"**

---

## 四、内存优化：省下的都是钱

### 1. 小对象的编码优化

Redis 对小对象有特殊编码，省内存但要在 `redis.conf` 里设好阈值：

```conf
# Hash 用 ziplist/listpack 编码的条件
hash-max-ziplist-entries 512   # 字段 ≤ 512 个
hash-max-ziplist-value 64      # 每个值 ≤ 64 字节

# ZSet 的类似配置
zset-max-ziplist-entries 128
zset-max-ziplist-value 64

# Set 的 intset 编码
set-max-intset-entries 512     # 全是整数且 ≤ 512 个元素
```

> **骚话：** 小对象优化是 Redis 白送的午餐——写代码时不用管，配置文件设好了就自动生效。**但别为了走 ziplist 而刻意把大 Hash 拆小，那是本末倒置。**

### 2. 内存碎片的整治

```bash
# 查看内存碎片率
INFO memory
# mem_fragmentation_ratio: 内存碎片率
# = used_memory_rss / used_memory

# 碎片率 > 1.5: 碎片严重，需要整理
# 碎片率 < 1: 内存被 swap 了，危险！
```

```conf
# 自动碎片整理（4.0+）
activedefrag yes
active-defrag-ignore-bytes 100mb   # 碎片 > 100MB 才触发
active-defrag-threshold-lower 10   # 碎片率 > 10% 才触发
```

### 3. 谁在吃内存？

```bash
# 查看全局内存使用
INFO memory
# used_memory: 当前占用
# used_memory_rss: 操作系统视角（含碎片）
# maxmemory: 上限

# 查看单个 Key 占用多少内存
MEMORY USAGE keyname

# 抽样查看 Key 的内存分布（7.0+）
MEMORY STATS
```

### 4. 内存优化清单

| 优化点 | 做法 | 预期收益 |
|--------|------|---------|
| 缩短 Key 名 | `user:1001:n` 替代 `user:1001:name` | 5-20% |
| 用 Hash 替代多个 String | 10 个 `user:1001:*` → 1 个 Hash | 30-50% |
| 用整数替代字符串 | `HSET user:1001 status 1` 不是 `"active"` | 视情况 |
| 共享对象池 | Redis 自动对 0-9999 的整数做共享 | 自动生效 |
| 过期时间必设 | 不设 TTL = 永远不删 | 预防泄漏 |

---

## 五、CPU 优化

### 1. 禁用高危命令

```conf
# 把危险命令改名，让偷懒的同事写不出来
rename-command KEYS ""
rename-command FLUSHDB ""
rename-command FLUSHALL ""
rename-command CONFIG "CONFIG_9f8a7b6c"  # 改名也可以
```

### 2. 正确使用 SCAN

```bash
# 别用 KEYS，用 SCAN！SCAN 是游标遍历，每次只返回少量 Key
SCAN 0 MATCH user:* COUNT 100
# 返回: (下一次游标, [匹配的 Key 列表])
# 游标到 0 表示遍历结束
```

```java
// Spring Data Redis 的 SCAN
ScanOptions options = ScanOptions.scanOptions()
    .match("user:*")
    .count(100)
    .build();
Cursor<String> cursor = redisTemplate.scan(options);
while (cursor.hasNext()) {
    String key = cursor.next();
    // 处理 key...
}
```

> **金句：** `SCAN` 不是一次返回全部，是游标遍历。所以遍历过程中 Key 可能被增删——你得到的是**近似结果**，不是精确快照。**可以接受近似就用 SCAN，必须精确就做全量快照（RDB）后离线分析。**

---

## 六、连接管理

### 连接数带来的开销

```
每个 Redis 连接 ≈ 10KB 内存（TCP Buffer + Redis Client Buffer）
10000 个连接 ≈ 100MB 内存

更致命的是: Redis 单线程处理命令时，连接多了不直接导致变慢，
但每个连接的握手、保活、断开都有开销
```

### 最佳实践

```java
// 使用连接池，不要每次请求都 new 连接！
// Jedis 连接池示例
JedisPoolConfig config = new JedisPoolConfig();
config.setMaxTotal(20);        // 最大连接数
config.setMaxIdle(10);         // 最大空闲连接
config.setMinIdle(5);          // 最小空闲连接
config.setMaxWaitMillis(3000); // 获取连接最大等待时间

JedisPool pool = new JedisPool(config, "localhost", 6379);
// 全局一个 Pool，用完归还，别每次 new
```

```conf
# Redis 服务端
maxclients 10000           # 最大连接数
timeout 300                # 空闲连接超时（秒）
tcp-keepalive 300          # TCP keepalive
```

> **批判时刻：** 见过最离谱的项目：每次请求都 `new Jedis("localhost", 6379)`，用完不关。10 分钟后服务器连接数到上限，Redis 拒绝新连接。**连接池是基础知识，不是高级优化。不知道用连接池的人，先回去看 JDBC 那一章。**

---

## 七、性能压测：知道你的天花板在哪

### 自带压测工具

```bash
# redis-benchmark 基本用法
redis-benchmark -h 127.0.0.1 -p 6379 -c 100 -n 100000

# 参数说明:
# -c 100:   100 个并发连接
# -n 100000: 总共发 100000 个请求
# -t set,get: 只测试 SET 和 GET
# -d 100:   数据大小 100 字节
# -q:       静默模式（只显示 QPS）
# --csv:    输出 CSV 格式

# 实战示例
redis-benchmark -h 127.0.0.1 -p 6379 \
  -c 500 -n 1000000 -t set,get \
  -d 256 --csv
```

### 生产级压测的注意事项

| 注意点 | 说明 |
|--------|------|
| **必须在同机房测** | 跨机房测的是网络延迟，不是 Redis 性能 |
| **逐步提高并发** | 从 50 → 100 → 500 → 1000，找拐点 |
| **关注 P99** | 平均值没意义，P99 延迟才是用户体验 |
| **Pipeline + 并发** | 真实场景是 Pipeline × 多连接，混合压测 |
| **数据量** | 空 Redis 测出来的 QPS 没意义，装几 GB 数据再测 |

> **骚话：** `redis-benchmark` 跑出来的 QPS 是"理论最高值"，相当于汽车的实验室油耗——真实路况永远达不到。**生产环境的真实 QPS 通常只有 benchmark 的 50%-70%，因为你会有慢命令、大 Key、主从同步、AOF 刷盘这些"生活成本"。**

---

## 性能优化总结：诊断一图流

```
Redis 慢了？
  │
  ├── 1. SLOWLOG GET 10 → 有慢命令？
  │   ├── 是 → 找到慢命令 → 改造或替换
  │   └── 否 → 下一步
  │
  ├── 2. redis-cli --bigkeys → 有大 Key？
  │   ├── 是 → 拆分 / UNLINK / 迁移到其他存储
  │   └── 否 → 下一步
  │
  ├── 3. INFO stats → 检查 total_commands_processed / instantaneous_ops_per_sec
  │   └── QPS 接近单机极限 → 上 Cluster
  │
  ├── 4. INFO memory → mem_fragmentation_ratio > 1.5？
  │   ├── 是 → 开 activedefrag
  │   └── 否 → 下一步
  │
  ├── 5. 业务代码里有 KEYS / SMEMBERS / HGETALL？
  │   └── 是 → 换成 SCAN / SSCAN / HSCAN
  │
  └── 6. 你的代码有没有用 Pipeline？
      └── 没有 → 加 Pipeline，尤其是批量写操作
```

> **终极金句：** 性能优化有一铁律——**先测量，再优化。** 不要凭感觉猜哪里慢，SLOWLOG 和 INFO 是你的 CT 扫描仪，数据不会说谎。**猜出来的优化叫玄学，测出来的优化叫工程。**
