# Redis Stream

> **金句开篇：** 在 Stream 出现之前，用 Redis 做消息队列就像用手刹漂移——能漂，但翻车是迟早的事。Stream 让 Redis 终于有了一个"正经的"消息队列数据结构。

---

## 为什么需要 Stream？

### 旧方案的尴尬

在 Redis 5.0（2018）之前，做消息队列有两套"土方案"：

**方案 A：List + BRPOP/LPUSH**

```
生产者: LPUSH queue "msg"
消费者: BRPOP queue 0  (阻塞等待消息)

致命问题：
  ❌ 没有 ACK 机制 —— 消息弹出来就没了，消费者崩了消息直接丢
  ❌ 不能重复消费 —— 弹出了就没了
  ❌ 没有消费者组 —— 一条消息只能被一个消费者消费（不支持广播）
```

**方案 B：Pub/Sub**

```
生产者: PUBLISH channel "msg"
消费者: SUBSCRIBE channel

致命问题：
  ❌ 消息不持久化 —— 如果没人订阅，消息直接丢弃
  ❌ 没有 ACK —— 收到了就是收到了，处理失败也没人知道
  ❌ 消费者断线期间的消息全丢 —— "先发后接" = 永远收不到
```

> **批判时刻：** 用 List 做 MQ 的人分两种：一种是"这丢了无所谓"（比如发个验证码），另一种是"我不知道消息会丢"（比如用 List 做订单状态流转）。**前者是工程取舍，后者是定时炸弹。**

### Stream 的答案

Stream 借鉴了 Kafka 的设计思想，提供：

| 能力 | List | Pub/Sub | **Stream** |
|------|------|---------|------------|
| 消息持久化 | ✅ | ❌ | ✅ |
| ACK 确认 | ❌ | ❌ | ✅ |
| 消费者组 | ❌ | ❌ | ✅ |
| 重复消费 | ❌ | ❌ | ✅ |
| 消息回溯 | ❌ | ❌ | ✅ |
| 阻塞读取 | ✅ | ✅ | ✅ |

---

## Stream 核心概念

### 数据结构

```
Stream Key: "orders"
│
├── 消息结构: [ID, {field:value, ...}]
│   ID 格式: <timestamp>-<sequence>
│   例如: 1691234567890-0
│
├── 消费者组 (Consumer Group):
│   ├── group: "order-processor"
│   ├── 消费者: consumer-1, consumer-2, consumer-3
│   └── PEL (Pending Entries List): 已投递但未 ACK 的消息
│
└── 消息是有序的、可追加的、不可变的（类似 Kafka 的 append-only log）
```

### 消息 ID 的讲究

```
1691234567890-0
│             │
│             └── 同一毫秒内的序列号（从 0 开始递增）
└── 毫秒级时间戳

自动生成: XADD key * field value  →  自动分配 ID
手动指定: XADD key 1000-0 field value  →  手动指定 ID
最小化:    XADD key 0-1 field value  →  ID 最小但不推荐
```

> **骚话：** Stream 的消息 ID 就是时间戳+序号。这意味着你天然可以**按时间范围查消息**——这是 List 做不到的。**但别把这个当成时序数据库用，那是另一种东西。**

---

## 核心命令速查

### 消息的增删改查

| 命令 | 作用 |
|------|------|
| `XADD key ID field value [field value ...]` | 追加消息（`*` = 自动 ID） |
| `XREAD COUNT n STREAMS key ID` | 读取消息（`0` = 从头读，`$` = 只读新消息） |
| `XRANGE key start end [COUNT n]` | 范围查询（`- +` = 全部） |
| `XREVRANGE key end start` | 反向范围查询 |
| `XLEN key` | 消息数量 |
| `XDEL key ID` | 删除消息（逻辑删除） |
| `XTRIM key MAXLEN ~ n` | 按长度裁剪（`~` = 近似，性能更好） |

### 消息的读取模式

```bash
# 1. 简单发布
XADD orders * user_id 1001 amount 99.9 item "机械键盘"
# 返回: "1691234567890-0"（消息 ID）

# 2. 从头读 2 条
XREAD COUNT 2 STREAMS orders 0
# 0 表示从第一条开始读

# 3. 阻塞读新消息（类似 BRPOP）
XREAD BLOCK 5000 COUNT 2 STREAMS orders $
# $ 表示只读最新的；BLOCK 5000 超时 5 秒

# 4. 范围查询（查最近 5 条）
XREVRANGE orders + - COUNT 5
```

### 消费者组命令

| 命令 | 作用 |
|------|------|
| `XGROUP CREATE key group ID` | 创建消费者组（`$`=只消费新消息, `0`=从第一条开始） |
| `XREADGROUP GROUP group consumer STREAMS key >` | 消费者组读消息（`>`=只读未分配给别人的新消息） |
| `XACK key group ID` | ACK 确认 |
| `XPENDING key group` | 查看已投递但未 ACK 的消息（PEL） |
| `XCLAIM key group consumer min-idle-time ID` | 认领超时未 ACK 的消息（转移给别的消费者） |
| `XGROUP DELCONSUMER key group consumer` | 删除组内的消费者 |

---

## 消费者组实战

### 创建消费者组 → 消费 → ACK 完整流程

```bash
# ===== 生产者 =====
XADD orders * user_id 1001 item "键盘" amount 99.9
XADD orders * user_id 1002 item "鼠标" amount 49.9
XADD orders * user_id 1003 item "耳机" amount 199.0

# ===== 消费者组 =====
# 创建消费者组（从 Stream 头开始消费）
XGROUP CREATE orders order-processor 0-0

# 消费者 C1 读取属于自己的新消息（> 表示只读未分配的新消息）
XREADGROUP GROUP order-processor C1 COUNT 2 STREAMS orders >
# C1 拿到了前 2 条消息

# 消费者 C2 读取
XREADGROUP GROUP order-processor C2 COUNT 2 STREAMS orders >
# C2 拿到了第 3 条消息

# 消费者处理完，ACK
XACK orders order-processor 1691234567890-0

# 查看未 ACK 的消息（处理中的消息）
XPENDING orders order-processor
# 返回: (未 ACK 数量, 最小 ID, 最大 ID, 每个消费者的详情)

# 详细查：哪些消费者有未 ACK 的消息？
XPENDING orders order-processor - + 10
# 列出前 10 条 Pending 消息，含: ID, 消费者名, 空闲时间(ms), 被投递次数
```

### 消息转移：拯救"死掉"的消费者

```bash
# C1 挂了，它手里的消息没人 ACK，一直 Pending
# 1. 查看 Pending 中 60 秒没 ACK 的消息
XPENDING orders order-processor - + 10

# 2. 把空闲超过 60 秒的消息转移给活着的 C2
XCLAIM orders order-processor C2 60000 1691234567890-0
# Key    Group    目标消费者  最小空闲时间(ms)  消息 ID

# 3. C2 重新处理这些被转移的消息，然后 ACK
```

> **金句：** Stream 的消费者组 + PEL + XCLAIM 形成了完美的"故障恢复三角"——消息投递了不会丢，消费者挂了能转移，处理完了要确认。**这才是一个消息队列该有的基本素养。**

---

## Stream vs Kafka vs RabbitMQ

> **骚话：** 每当一个新 MQ 出现，大家就会问"它能替代 Kafka 吗？"这个问题本身就问错了——工具不存在替代，只存在适合。

| 维度 | Redis Stream | Kafka | RabbitMQ |
|------|-------------|-------|----------|
| 吞吐量 | 中~高（10 万+/s） | **极高**（百万级/s） | 中（万级/s） |
| 延迟 | **极低**（微秒级） | 低（毫秒级） | 低（毫秒级） |
| 持久化 | 依赖 AOF/RDB | 磁盘顺序写，天然持久 | 磁盘 + 内存 |
| 消息回溯 | ✅ 可以（有 ID 就行） | ✅ 天然支持（offset） | ❌ 消费完就删 |
| ACK | ✅ 手动 | ✅ 手动/自动 | ✅ 手动/自动 |
| 消费者组 | ✅ | ✅ | ❌（用 Exchange 路由模拟） |
| 分区/分片 | ❌ 不支持原生分区 | ✅ Partition | ✅ Queue 级别 |
| 运维复杂度 | **低**（已有 Redis 集群） | **高**（ZooKeeper/KRaft） | 中 |
| 适用规模 | 中小型，快速接入 | **大规模日志/流处理** | 企业级路由 |

### 选型建议

```
你的消息量每天 < 1000 万条，且已有 Redis？
  → Stream 够了，别引入 Kafka 增加运维复杂度

你的消息量每天 > 1 亿条，需要做流处理/ETL？
  → Kafka，Stream 不是为此设计的

你的消息路由规则非常复杂（topic→多队列→多消费者）？
  → RabbitMQ，它的 Exchange/Routing Key 无人能敌

你只是想做个异步任务队列，丢一两条无所谓？
  → List + BRPOP 仍然是最简单的选择
```

> **批判时刻：** 最怕的是"我们用了 Redis，所以消息队列也用 Redis Stream 吧，统一技术栈"——然后日活上来后消息量爆炸，Stream 的 Key 变成大 Key，内存告急，持久化压力飙升。**技术栈统一不等于技术债清零。在合适的地方用合适的工具，才是架构师的核心能力。**

---

## Stream 的坑与最佳实践

### 1. 消息积压 → 内存爆炸

Stream 的消息是存在**内存**里的（受持久化配置影响），不像 Kafka 是磁盘顺序写。大量积压 → Redis 内存飙到 `maxmemory` → 触发淘汰策略。

```bash
# 必须设置 Stream 的最大长度！
XADD orders MAXLEN ~ 10000 * field value
# 超过 10000 条自动裁剪旧消息（~ 表示近似裁剪，高效）

# 或在创建 Stream 后定期裁剪
XTRIM orders MAXLEN ~ 10000
```

> **金句：** 不给 Stream 设 MAXLEN，就像不给 Key 设 TTL——内存泄漏只是时间问题。

### 2. PEL 膨胀

如果消费者因为 Bug 永远不 ACK，PEL 会越来越大，内存占用越来越高。甚至消费者已经下线了，消息还在 PEL 里挂着。

**监控 PEL 大小：**

```bash
XPENDING orders order-processor
# 返回第一个数字就是未 ACK 数量
# 如果这个数字持续增长 → 你的消费者出问题了
```

### 3. 消息重复消费

Stream 保证 **At-Least-Once**（至少一次），不保证 Exactly-Once。消费者 ACK 之前如果挂了，消息会被 XCLAIM 转移给其他消费者 → 可能被处理两次。

**消费者需要实现幂等：**

```java
// 用消息 ID 做去重
String msgId = message.getId();
if (redis.sismember("processed:msgs", msgId)) {
    redis.xack(streamKey, group, msgId); // 重复消息直接 ACK
    return;
}
processMessage(message);
redis.sadd("processed:msgs", msgId);
redis.xack(streamKey, group, msgId);
```

### 4. 消费者数量多于 Stream 消息

如果你有 5 个消费者，但只有 3 条新消息——只有 3 个消费者能拿到消息，另外 2 个返回空。这是正常的，别以为是 Bug。

> **骚话：** Stream 不是为了替代 Kafka 而生，而是为了给"已经用了 Redis、需要简单 MQ 功能"的人一条活路。**把 Stream 当 Kafka 用是错的，把 Stream 当 List 的升级版才是对的。**

---

## 总结

```
你的场景是什么？

"我只是想异步处理，丢了无所谓"
  → List + BRPOP/LPUSH（最简单）

"我需要可靠消息、消费者组、ACK、消息回溯"
  → Redis Stream（比引入 Kafka 省一个运维团队）

"我需要海量吞吐、磁盘持久化、流计算对接"
  → Kafka / Pulsar（这才是正确选择）

"Spring 项目需要简单延迟/异步，开发量小"
  → Spring Event + @Async（甚至不需要 MQ）
```

> **终极骚话：** 消息队列选型有一个铁律 —— **如果 Redis Stream 够用，就别碰 Kafka。** 你以为你在选技术，实际上你在选运维成本和凌晨被叫起来的概率。**简单是最高级的架构。**
