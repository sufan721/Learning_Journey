# Redis 事务与 Lua 脚本

> 如果你抱着 MySQL 事务的思维来学 Redis 事务，你一定会失望。Redis 的事务是"打了激素的批处理"，不是"穿了防弹衣的 ACID"。

---

## Redis 事务基础

### 核心命令

| 命令 | 作用 |
|------|------|
| `MULTI` | 开启事务（进入命令队列模式） |
| `EXEC` | 执行所有队列中的命令 |
| `DISCARD` | 放弃事务（清空队列） |
| `WATCH key` | 监视 Key，如果 EXEC 之前 Key 被修改，事务自动放弃 |

### 事务执行流程

```
Client                          Redis
  │                                │
  ├─ MULTI ──────────────────────>│  进入事务模式
  ├─ SET user:1:name "Jack" ────>│  命令入队（不执行）
  ├─ INCR user:1:visits ────────>│  命令入队（不执行）
  ├─ SET user:1:status "active" ─>│  命令入队（不执行）
  ├─ EXEC ───────────────────────>│  一次性执行所有命令
  │<── ["OK", 1, "OK"] ──────────┤  返回每个命令的结果
```

> Redis 事务的本质是 **"把一堆命令打包，一次发送，顺序执行"**。中间不会被其他客户端插队，这点保证了**隔离性**。但别高兴太早，往下看。

---

## Redis 事务的致命缺陷

### 1. 没有回滚！没有回滚！

```bash
MULTI
SET key1 "value1"
LPUSH key1 "this will fail"  # key1 是 String，不能 LPUSH
SET key2 "value2"
EXEC
# 结果: ["OK", (error), "OK"]
# key1 的值还是被改了！key2 也被改了！
# 出错的命令失败了，但前后的命令照样执行
```

> 这是 Redis 事务最大的反直觉设计。SQL 数据库中一个语句失败整个事务回滚，Redis 是**出错了就当没看见，继续执行后面的**。antirez 的解释是："Redis 事务失败通常是因为编程错误，应该在开发阶段被发现，不值得为这个加回滚机制。" —— 听起来有道理，但你对接的支付回调在 `MULTI/EXEC` 里因为类型错误静默跳过了，你就知道什么叫"编程错误的代价"。

### 2. 不支持隔离级别

Redis 没有"读已提交"或"可重复读"的概念。`MULTI` 里的命令在入队时**不会真正执行**，你没法在事务里根据前一个命令的结果决定下一个命令。

```bash
MULTI
SET counter 10
GET counter        # 返回的是 "QUEUED"，不是 10！
# 你无法根据 GET 的结果来决定下一步做什么
EXEC
```

> Redis 事务是**盲执行**——入队时看不到执行结果，只能一股脑扔进去。

### 3. WATCH 的乐观锁：有总比没有好

`WATCH` 实现了乐观锁，但机制很原始：

```bash
WATCH mykey           # 监视 mykey
val = GET mykey       # 读到 val = 10
MULTI
SET mykey 11          # 想把 10 改成 11
EXEC                  # 如果 mykey 在 WATCH 之后被改过，EXEC 返回 nil（事务被丢弃）
```

```java
// Java 实现 Redis 乐观锁
public boolean transfer(String from, String to, int amount) {
    // CAS 循环，直到成功
    while (true) {
        redis.watch(from, to);
        int fromBalance = Integer.parseInt(redis.get(from));
        if (fromBalance < amount) {
            redis.unwatch();
            return false;
        }
        redis.multi();
        redis.decrBy(from, amount);
        redis.incrBy(to, amount);
        if (redis.exec() != null) {
            return true; // 成功
        }
        // exec 返回 null，说明 key 被改过，重试
    }
}
```

> Redis 的 WATCH + MULTI/EXEC 实现的乐观锁，在高并发下会疯狂自旋重试。**你以为你在用乐观锁，实际上你在写一个无限重试的死循环。** 高并发场景下，老老实实用 Lua 脚本，或者 Redisson 的分布式锁。

---

## Lua 脚本：Redis 事务的真正解药

> Lua 脚本是 Redis 给事务缺陷打的最大补丁。它把多条命令变成一个**原子执行单元**，中间不会有任何其他命令插入进来 —— 这才是你想象中的"事务"该有的样子。

### 为什么用 Lua 脚本？

| 需求 | MULTI/EXEC | Lua 脚本 |
|------|-----------|---------|
| 原子性 | 不被打断 | **不被打断** |
| 根据中间结果做判断 | ❌ 不支持 | ✅ 完全支持 |
| 减少网络往返 | ❌ N 条命令 = N 次网络交互 | ✅ 1 次网络往返 |
| 复用 | ❌ | ✅ `SCRIPT LOAD` + `EVALSHA` |
| Cluster 环境 | ❌ 要求同 slot | ❌ 同样要求同 slot |

### 基本用法

```bash
# EVAL script numkeys key [key ...] arg [arg ...]
# numkeys: 有多少个 key 参数
# key:     Redis Key
# arg:     额外的参数

# 例：限制用户访问频率
EVAL "
  local count = redis.call('INCR', KEYS[1])
  if count == 1 then
    redis.call('EXPIRE', KEYS[1], ARGV[1])
  end
  if count > tonumber(ARGV[2]) then
    return 0  -- 超限
  end
  return 1     -- 放行
" 1 user:rate:limit:10086 60 10
#  ↑ numkeys=1
#  KEYS[1] = user:rate:limit:10086
#  ARGV[1] = 60 (窗口秒数)
#  ARGV[2] = 10 (最大次数)
```

### Lua 脚本实战经典场景

#### 1. 限流器（滑动窗口）

```lua
-- 滑动窗口限流
-- KEYS[1]: 限流 Key
-- ARGV[1]: 窗口大小（秒）
-- ARGV[2]: 最大请求数
local now = redis.call('TIME')[1]  -- 当前秒级时间戳
local window = tonumber(ARGV[1])
local max_req = tonumber(ARGV[2])

-- 移除窗口外的旧数据
redis.call('ZREMRANGEBYSCORE', KEYS[1], 0, now - window)

-- 统计窗口内的请求数
local count = redis.call('ZCARD', KEYS[1])
if count >= max_req then
    return 0  -- 拒绝
end

-- 记录本次请求
redis.call('ZADD', KEYS[1], now, now .. '-' .. math.random())
redis.call('EXPIRE', KEYS[1], window + 1)
return 1  -- 放行
```

> 三年前你还在用 `INCR + EXPIRE` 做固定窗口限流，缺点是在窗口交界处流量翻倍。三年后你写出了滑动窗口 Lua 脚本，终于可以在简历上写"精通分布式限流"了。

#### 2. 分布式锁（正确版）

```lua
-- 加锁 + 原子设置过期时间
-- KEYS[1]: 锁的 Key
-- ARGV[1]: 唯一标识（UUID，防止误删别人的锁）
-- ARGV[2]: 过期时间（秒）
if redis.call('SET', KEYS[1], ARGV[1], 'NX', 'EX', ARGV[2]) then
    return 1  -- 获取锁成功
end
return 0      -- 锁已被占用
```

```lua
-- 解锁（只有锁的持有者才能解）
-- KEYS[1]: 锁的 Key
-- ARGV[1]: 唯一标识
if redis.call('GET', KEYS[1]) == ARGV[1] then
    return redis.call('DEL', KEYS[1])
end
return 0  -- 不是你的锁，别碰
```

> 面试八股文的分布式锁是用 `SETNX` 实现的，生产环境的分布式锁是用这个 Lua 脚本实现的 —— 区别在于**释放锁时得验证是不是你加的锁**。不验证的话，你的锁过期了，别人获取了锁，结果你去 DEL 了别人的锁。**这不是 bug，这是生产事故。**

#### 3. 库存扣减（防超卖）

```lua
-- KEYS[1]: 库存 Key
-- ARGV[1]: 扣减数量
local stock = tonumber(redis.call('GET', KEYS[1]) or 0)
if stock < tonumber(ARGV[1]) then
    return -1  -- 库存不足
end
redis.call('DECRBY', KEYS[1], ARGV[1])
return stock - tonumber(ARGV[1])  -- 返回剩余库存
```

### EVALSHA：让脚本飞起来

```bash
# 第一次：用 EVAL 执行（脚本内容每次都要发过去，浪费带宽）
EVAL "return redis.call('GET', KEYS[1])" 1 mykey

# 优化：先加载脚本，用 SHA1 引用
SCRIPT LOAD "return redis.call('GET', KEYS[1])"
# 返回: "e0e06f8b2f0e4c8b9e4f0e4c8b9e4f0e4c8b9e4f"  (SHA1)

# 之后都用 EVALSHA 调用（只传 SHA1，省带宽）
EVALSHA e0e06f8b2f0e4c8b9e4f0e4c8b9e4f0e4c8b9e4f 1 mykey
```

> `EVALSHA` 每次调用节省的几十字节带宽，在高并发下放大成每秒几 MB。**优化就是这些细节堆起来的，别只会堆机器。**

### Lua 脚本的三大坑

| 坑 | 说明 | 应对 |
|------|------|------|
| **阻塞** | Lua 执行期间 Redis 完全阻塞，其他命令全部等待 | 脚本不要有死循环，不要处理大量数据，不要超过 5 秒 |
| **Cluster 限制** | 脚本中操作的所有 Key 必须在同一 slot | 用 Hash Tag `{user}:name` `{user}:age` |
| **随机性** | Lua 中 `math.random` 会导致脚本在 AOF/master-slave 复制中不一致 | 用 `redis.call('TIME')` 传入时间作为种子 |
| **脚本缓存** | 脚本一旦被执行，Redis 会缓存它，占用内存 | `SCRIPT FLUSH` 清理缓存 |
| **事务幻觉** | 脚本内部出错后仍会部分执行（和事务一样不回滚） | 先校验再操作，把危险操作放前面 |

```lua
-- 随机性的正确做法: 把随机种子从外部传入
-- 不要用 math.random() !!
-- 通过 ARGV 把随机值传进来
```

> Lua 脚本的最大风险就是**阻塞**。一个写过长的 Lua 脚本（比如遍历一个 List 的 100 万个元素），能让 Redis 卡住几十秒。哨兵检测到超时 → 触发故障转移 → Master 切换 → 集群震荡。**一个烂的 Lua 脚本 = 一次人为的 Redis 宕机。**

### Spring Boot 中使用 Lua

```java
// 加载脚本
private final DefaultRedisScript<Long> rateLimitScript;

{
    rateLimitScript = new DefaultRedisScript<>();
    rateLimitScript.setLocation(new ClassPathResource("scripts/rate_limit.lua"));
    rateLimitScript.setResultType(Long.class);
}

// 执行
List<String> keys = List.of("rate:limit:user:" + userId);
Long result = redisTemplate.execute(rateLimitScript, keys, "60", "10");
```

---

## Redis 事务 vs MySQL 事务 vs Lua 脚本

| 维度 | MySQL 事务 | Redis MULTI/EXEC | Redis Lua |
|------|-----------|-----------------|-----------|
| 原子性 | ✅ 全成功或全回滚 | ⚠️ 不被打断，但出错不回滚 | ⚠️ 不被打断，但出错不回滚 |
| 隔离性 | ✅ 多级别（RU/RC/RR/Serializable） | ✅ 串行执行（最强隔离） | ✅ 串行执行 |
| 持久性 | ✅ WAL + fsync | ❌ 依赖持久化配置 | ❌ 依赖持久化配置 |
| 回滚 | ✅ ROLLBACK | ❌ 没有 ROLLBACK | ❌ 没有 ROLLBACK |
| 条件判断 | ✅ WHERE / IF | ❌ 只能盲执行 | ✅ 完全支持 |
| 适用场景 | 核心业务数据 | 简单批处理 | **复杂原子操作** |

> Redis 的事务不是真正的 ACID 事务，它是一个**"可以保证不被插队的批量命令执行器"**。如果你需要回滚能力、需要根据中间结果做判断、需要真正的 ACID——请用关系型数据库，或者接受最终一致性。**在 Redis 里追求 ACID，就像在麦当劳点法式大餐：不是人家没有，是你来错了地方。**

---

## 总结：什么时候用哪个？

```
需要"多条命令不被打断"？
  ├── 逻辑简单，不需要根据中间结果判断 → MULTI/EXEC 够用
  └── 逻辑复杂，需要条件判断/循环 → Lua 脚本

需要"出错回滚"？
  └── 别用 Redis！去用 MySQL 主库。

高并发分布式锁/限流/扣库存？
  └── Lua 脚本是唯一正确的选择。
```

> 八股文告诉你 `MULTI/EXEC` 是 Redis 的事务，实战告诉你 Lua 脚本才是 Redis 事务的亲爹。**面试一个答，生产一个写，两者并行不悖，这叫生存智慧。**
