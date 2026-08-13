# RabbitMQ 核心概念

> 很多人把 RabbitMQ 装好、跑起来、消息能收发，就觉得"我会了"。直到某天消息丢了、队列炸了、集群脑裂了，才意识到自己只是在用 RabbitMQ 的"Hello World"，而从没真正理解它肚子里那套 AMQP 协议在想什么。

---

## 一、为什么要用消息队列？

先搞清楚一个问题：**没有 MQ 你的系统能不能跑？** 能。那为什么还要加它？

| 作用 | 一句话 | 没有 MQ 的样子 |
|------|--------|---------------|
| **异步** | 把"等结果"变成"先记账" | 用户点支付，转圈 30 秒等短信/邮件/积分全处理完 |
| **解耦** | 两个系统不直接调用，通过消息通信 | 订单系统直接调库存、物流、积分、通知...改一个全崩 |
| **削峰** | 高峰期请求排队，不会打崩下游 | 秒杀 10 万 QPS 直接冲垮 MySQL |
| **柔性** | 下游挂了？消息存着不动，恢复后再处理 | 下游挂了 = 上游调用失败 → 用户看到 500 |

> 很多人一听说"秒杀"就上 MQ，然后所有接口都用 MQ，连"改个用户名"都要扔进队列——这叫**为了用而用**。MQ 解决的是**异步、解耦、削峰**，如果接口本身就是同步的（比如用户注册后必须立刻返回结果给前端），强行上 MQ 等于给自己造了一个延迟 + 失败重试 + 幂等性的麻烦。**架构不是堆中间件，是选对工具。**

---

## 二、AMQP 协议与 RabbitMQ 的架构

RabbitMQ 实现了 **AMQP 0-9-1** 协议（Advanced Message Queuing Protocol）。理解 AMQP 的核心模型，才真正理解了 RabbitMQ。

### 核心组件

```
                     RabbitMQ Broker
┌──────────────────────────────────────────────────────┐
│  ┌──────────────┐                                    │
│  │  Connection   │  ← TCP 长连接（一条连接 64 个端口）  │
│  │  ┌──────────┐ │                                    │
│  │  │ Channel  │ │  ← 轻量级"虚拟连接"，复用 TCP 连接    │
│  │  │ Channel  │ │                                    │
│  │  │ Channel  │ │     ┌──────────────┐               │
│  │  └──────────┘ │     │   Exchange   │               │
│  └───────────────┘     │  (交换机)     │               │
│                        │              │               │
│  Producer ───────────> │  type:       │               │
│  发送消息 + routingKey  │  direct      │               │
│                        │  topic       │               │
│                        │  fanout      │               │
│                        │  headers     │               │
│                        └──────┬───────┘               │
│                               │                       │
│                          Binding Key                  │
│                          (绑定规则)                    │
│                               │                       │
│                        ┌──────▼───────┐               │
│                        │   Queue      │               │
│                        │  (队列)       │               │
│                        │  消息暂存区   │               │
│                        └──────┬───────┘               │
│                               │                       │
│                               └──────────────────> Consumer
│                                                       │
│  ┌──────────────┐                                    │
│  │ Virtual Host │  ← 逻辑隔离（类似 MySQL 的 Database）│
│  └──────────────┘                                    │
└──────────────────────────────────────────────────────┘
```

> 为什么有 Channel 这个概念，直接拿 Connection 发消息不行吗？
>
> 因为 Connection 是 **TCP 连接**，建立和销毁代价大。如果每个生产者/消费者线程都建一条 TCP 连接，RabbitMQ 的端口很快就被打满（默认 5672 端口只能监听一个 socket，连接上限受操作系统文件描述符限制）。Channel 是在一条 TCP 连接上**复用的虚拟连接**，创建和销毁几乎是免费的。**这就跟 HTTP/1.1 的 Keep-Alive 一个道理——一次握手，多次通信。**

---

### 四大核心概念的职责

| 组件 | 做什么的 | 类比 |
|------|---------|------|
| **Producer** | 发消息的 | 寄件人 |
| **Exchange** | 根据规则分发消息 | 快递分拣中心 |
| **Queue** | 存消息的，等待消费者 | 收件人的信箱 |
| **Consumer** | 消费消息的 | 收件人 |
| **Binding** | 定义 Exchange 和 Queue 的关系 | 快递路线 |
| **Routing Key** | Producer 发消息时指定的"地址" | 快递单上的地址 |
| **Binding Key** | Exchange 分发消息的规则 | 分拣规则 |

> RabbitMQ 的架构精髓在于 **"Producer 不直接往 Queue 里发消息"**。消息先到 Exchange，Exchange 根据 Binding 规则决定投给哪个 Queue。**这个间接层，是 RabbitMQ 比 Redis List 做消息队列灵活一万倍的原因。**

---

### Virtual Host：被低估的隔离机制

```bash
# vhost 就是 RabbitMQ 内部的"数据库"概念
# 每个 vhost 有独立的 Exchange、Queue、Binding、权限

# 创建 vhost
rabbitmqctl add_vhost /order-system
# 给用户授权
rabbitmqctl set_permissions -p /order-system admin ".*" ".*" ".*"
```

| 特性 | 说明 |
|------|------|
| 资源隔离 | 不同 vhost 的 Exchange/Queue 互不可见 |
| 权限隔离 | 用户只在自己有权限的 vhost 里操作 |
| 默认 `/` vhost | 安装自带，千万别用在生产上 —— 给它取个有意义的名字 |

> 见过最离谱的操作：所有项目共用一个 vhost，Exchange 命名乱得一塌糊涂，A 项目的人不小心删了 B 项目的 Queue，两小时内没人发现，因为日志太多了找不到源头。**vhost 不要钱，为什么不分开？**

---

## 三、Exchange 的四种类型

Exchange 是 RabbitMQ 的灵魂。四种类型，从简单到复杂各有各的用武之地。

### 1. Direct Exchange（直连交换机）

```
Binding: QueueA ← binding key = "order"
         QueueB ← binding key = "payment"

Producer 发送: routing key = "order"  → 投递到 QueueA
Producer 发送: routing key = "payment" → 投递到 QueueB
Producer 发送: routing key = "xxx"    → 丢弃!
```

**Routing Key 必须和 Binding Key 完全匹配。** 最精确、最直接、最常用。

### 2. Fanout Exchange（扇出/广播交换机）

```
Binding: QueueA ← (忽略 binding key)
         QueueB ← (忽略 binding key)
         QueueC ← (忽略 binding key)

Producer 发送 → 三个 Queue 都收到!
```

**忽略 Routing Key，消息广播到所有绑定的 Queue。** 适合"通知所有人"的场景。

### 3. Topic Exchange（主题交换机）

```
Binding: QueueA ← binding key = "order.*"       (匹配 order.create / order.cancel)
         QueueB ← binding key = "order.#"       (匹配 order.* 和 order.create.success)
         QueueC ← binding key = "*.error"       (匹配 order.error / payment.error)

* → 匹配恰好一个单词
# → 匹配零个或多个单词
```

### 4. Headers Exchange（头交换机）

不看 Routing Key，看消息的 **Header 属性**。效率较低，一般用 Topic 就够了。

> 90% 的场景 Direct 和 Topic 就够了。Fanout 是"全员通知"，Headers 你大概率一辈子用不到。千万别为了显得高大上，把四种 Exchange 全弄一遍——**简单是架构的通行证，复杂是维护的墓志铭。**

---

## 四、Connection 和 Channel 的深度理解

### 连接数吓死人

```java
// ❌ 错误姿势：每个消息都建新连接
public void send(String msg) {
    Connection conn = factory.newConnection();  // TCP 三次握手
    Channel ch = conn.createChannel();
    ch.basicPublish(EXCHANGE, KEY, null, msg.getBytes());
    conn.close();                               // TCP 四次挥手
}

// ✅ 正确姿势：连接复用，Channel 池化
// Connection 全局一个，Channel 线程内复用
```

### Channel 不是线程安全的

**一个 Channel 只能在同一时刻被一个线程使用。** 多线程并发发消息？每个线程用自己独立的 Channel，共享同一个 Connection。

> 很多人借了一个 Channel，在多线程里共享，然后报出诡异的 `AlreadyClosedException` 或者消息串位。**RabbitMQ 的 Channel 设计是"单线程独享"的，你把它当线程池里的公共资源用，活该半夜查日志。**

---

## 五、消息流转的 10 秒历程

一条消息从出生到消亡：

```
1. Producer 创建 Connection → 建立 TCP 连接
2. Producer 创建 Channel → 虚拟连接就绪
3. Producer 发消息：basicPublish(exchange, routingKey, props, body)
4. Exchange 收到消息 → 匹配 Binding → 找到目标 Queue(s)
5. 消息入队 → 持久化(如果配置了) → 等待消费
6. Consumer 的 Channel 拉取(pull)或推送(push)消息
7. Consumer 处理消息
8. Consumer 发送 ACK (确认) 或 NACK (拒绝)
9. RabbitMQ 收到 ACK → 删除消息
10. 如果有死信/重试策略，执行相应的逻辑
```

**消息可以在几乎所有环节丢。** 具体怎么丢的、怎么防，见 [RabbitMQ消息可靠性.md](RabbitMQ消息可靠性.md)。

---

## 六、Spring AMQP 快速起步

```java
// 1. 配置
@Configuration
public class RabbitConfig {
    @Bean
    public Queue orderQueue() {
        return QueueBuilder.durable("order.queue").build();
    }

    @Bean
    public DirectExchange orderExchange() {
        return new DirectExchange("order.exchange");
    }

    @Bean
    public Binding binding() {
        return BindingBuilder
            .bind(orderQueue())
            .to(orderExchange())
            .with("order.create");
    }

    @Bean
    public RabbitTemplate rabbitTemplate(ConnectionFactory factory) {
        RabbitTemplate template = new RabbitTemplate(factory);
        template.setConfirmCallback((data, ack, cause) -> {
            if (!ack) log.error("消息未到达 Exchange: {}", cause);
        });
        template.setReturnsCallback(returned -> {
            log.error("消息未路由到 Queue: {}", returned.getMessage());
        });
        return template;
    }
}

// 2. 生产者
@Service
public class OrderProducer {
    @Autowired private RabbitTemplate rabbitTemplate;

    public void sendOrder(Order order) {
        rabbitTemplate.convertAndSend(
            "order.exchange", "order.create", order
        );
    }
}

// 3. 消费者
@Component
public class OrderConsumer {
    @RabbitListener(queues = "order.queue")
    public void onOrder(Order order) {
        log.info("收到订单: {}", order);
    }
}
```

> Spring Boot 把 RabbitMQ 的样板代码量从 200 行压缩到 20 行。但代价是——很多人连 Exchange 和 Queue 的关系都没搞明白就开始写 `@RabbitListener`，出了问题只会重启。

---

## 七、RabbitMQ 管理面板速览

```bash
# 开启管理插件（Web UI 在 15672 端口）
rabbitmq-plugins enable rabbitmq_management

# 访问 http://localhost:15672
# 默认账号: guest / guest（仅限 localhost 登录！远程必须建新用户）
```

管理面板必看的页面：

| 标签页 | 看什么 | 报警线 |
|--------|--------|--------|
| **Overview** | 连接数、Channel 数、消息速率 | Ready 消息持续增长 → 消费者跟不上 |
| **Queues** | 每个队列的 Ready/Unacked/Total | Unacked 堆积 → 消费者处理太慢 |
| **Exchanges** | Exchange 和 Queue 的绑定关系 | — |
| **Channels** | 连接数和 Channel 数 | Channel 数飙升 → 连接泄漏 |

> 管理面板看到某 Queue 的 Ready 消息 500 万条，Unacked 为 0——这说明**消费者根本没连上来**。见过最搞笑的：运维重启了消费者服务，忘了把消费者进程拉起来，3 天后 Ready 消息 2000 万，内存把整个节点撑爆。**管理面板不是摆设，是监控的第一个入口。**

---

## 八、入门 Checklist

- [ ] 理解了为什么需要 MQ（异步/解耦/削峰，不是"别人用我也用"）
- [ ] 能画出 Exchange → Binding → Queue 的关系图
- [ ] 知道 4 种 Exchange 的区别和适用场景
- [ ] 明白 Connection 和 Channel 的关系（TCP 复用）
- [ ] 能用 Spring Boot 跑通一个 Hello World
- [ ] 知道管理面板在哪，怎么看

> RabbitMQ 的核心模型就三个角色：**Exchange 分消息，Queue 存消息，Binding 连接两者。** 把这三个的关系画清楚了，你就超过了 70% 的 RabbitMQ 用户。**剩下 30% 的人，死在消息可靠性和高可用的坑里——那是后续章节要讲的事了。**
