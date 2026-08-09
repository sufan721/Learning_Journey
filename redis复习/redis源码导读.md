# Redis 源码导读

> **金句开篇：** Redis 的源码大概是这个世界上性价比最高的 C 语言教材——几万行代码，没有复杂的依赖，读起来像在读一本精心编排的技术小说。antirez 写了一手好 C，每一行都能教你点什么。

---

## 为什么读 Redis 源码？

> **骚话：** 面试官问你"读过什么开源项目源码吗？"——你说 Redis，面试官的眼睛会亮。不是因为 Redis 有多难，而是因为**真正读完的人极少**，大多数止步于博客解析和别人的读后感。

| 读源码的收获 | 你能学到的 |
|-------------|-----------|
| **工程结构** | 一个单线程事件驱动型服务器怎么设计 |
| **数据结构实现** | SDS、字典、跳表、压缩列表的工业级实现 |
| **内存管理** | jemalloc、引用计数、共享对象池 |
| **网络编程** | epoll/select/kqueue 多路复用的优雅抽象 |
| **C 语言技巧** | 宏的魔法、函数指针做多态、Preprocessor 的艺术 |

---

## 源码全景图：文件结构

```
redis/src/
├── server.c/h        ← 主循环、命令表、配置（6000+ 行，核心中的核心）
├── ae.c/h            ← 事件循环（epoll/kqueue/select 的抽象封装）
├── networking.c      ← 网络读写、客户端管理
├── object.c          ← redisObject 的创建/管理
├── db.c              ← 数据库层：KEY 的 CRUD、过期策略
├── sds.c/h           ← 动态字符串（比 C 原生 char* 强 100 倍）
├── dict.c/h          ← 哈希表（渐进式 rehash，Redis 中最精妙的数据结构）
├── t_string.c        ← String 命令实现
├── t_list.c          ← List 命令实现
├── t_hash.c          ← Hash 命令实现
├── t_set.c           ← Set 命令实现
├── t_zset.c          ← ZSet 命令实现（跳表就在这里！）
├── ziplist.c         ← 压缩列表（3.2 前用，7.0 被 listpack 逐渐替代）
├── quicklist.c       ← 快排列表（3.2+ List 的默认实现）
├── listpack.c        ← 紧凑列表（7.0+，ziplist 的继任者）
├── intset.c          ← 整数集合（小 Set 的编码）
├── rdb.c             ← RDB 持久化
├── aof.c             ← AOF 持久化
├── replication.c     ← 主从复制
├── sentinel.c        ← 哨兵
├── cluster.c         ← Cluster 集群
├── expire.c          ← 过期删除策略
├── lazyfree.c        ← 异步删除（UNLINK）
├── bio.c             ← 后台线程（close fd、AOF fsync、UNLINK）
├── script.c          ← Lua 脚本引擎集成
├── t_stream.c        ← Stream 类型
├── geo.c             ← GEO 类型
├── hyperloglog.c     ← HyperLogLog 类型
├── bitops.c          ← BitMap 位操作
├── util.c            ← 工具函数
└── siphash.c         ← 哈希算法
```

> **阅读建议：** 不要从头到尾按顺序读。**先读 ae.c → server.c 主循环 → networking.c → sds.c → dict.c → t_zset.c 跳表**，这个顺序能让你最快理解"Redis 是怎么跑起来的"。

---

## 一、事件循环（ae.c）— Redis 的心脏

Redis 是事件驱动型服务器，所有操作都由事件循环驱动：

```
int main() {
    initServer();          // 初始化：创建 Socket、注册定时任务
    aeMain(server.el);     // 进入事件循环（死循环，直到关闭）
}

aeMain():
  while (!stop) {
    1. aeProcessEvents()    // 处理文件事件（网络 IO）+ 时间事件（定时任务）
       │
       ├─ 调用 epoll_wait/select/kqueue，等待事件
       ├─ 有事件到达 → 调用对应的回调函数
       │   ├─ 客户端连接 → acceptTcpHandler()
       │   ├─ 客户端命令 → readQueryFromClient()
       │   └─ 写响应完成 → sendReplyToClient()
       │
       └─ 检查时间事件：serverCron() 定时执行
          (每 100ms 执行一次: 过期 Key 清理、AOF 重写、主从同步...)
  }
```

### ae.c 的精妙之处：多路复用的统一抽象

```c
// ae.h 的核心结构
typedef struct aeEventLoop {
    aeFileEvent *events;   // 文件事件数组（fd → event 映射）
    aeFiredEvent *fired;   // 已触发的事件（epoll_wait 的返回）
    aeTimeEvent *timeEventHead;  // 时间事件链表
    void *apidata;         // 平台相关数据（epoll_fd / kqueue_fd / select 数据）
} aeEventLoop;

// 统一的接口，隐藏了底层差异
aeCreateFileEvent(el, fd, AE_READABLE, callback, clientData);
// 内部会根据编译平台决定用 epoll / kqueue / select
```

```c
// ae.c 的平台选择（预编译指令）
#ifdef HAVE_EPOLL
    #include "ae_epoll.c"     // Linux
#elif defined(HAVE_KQUEUE)
    #include "ae_kqueue.c"    // BSD / macOS
#else
    #include "ae_select.c"    // Windows / 其他
#endif
```

> **骚话：** ae.c 总共不到 500 行，却实现了对 epoll/kqueue/select 三种多路复用机制的统一抽象。**这告诉你一个道理：好的设计不是复杂，是 "刚好够用的抽象"。** nginx、libevent、libuv 都有类似的设计——把底层差异封装掉，上层代码一次编写到处运行。

### serverCron：每秒 10 次的"管家线程"

```c
// serverCron() 在事件循环中被定期调用
void serverCron() {
    // 1. 过期 Key 主动清理（每次随机抽 20 个，过期就删）
    activeExpireCycle();

    // 2. 触发 BGSAVE / AOF rewrite（如果满足条件）
    // 3. 主从重连 / 同步
    // 4. 集群 Gossip 消息
    // 5. 统计信息更新
    // 6. 客户端超时关闭
    // 7. 内存碎片整理（如果开了 activedefrag）

    // 所有这些在事件循环的间隙执行，不会阻塞主线程太久
}
```

> **金句：** serverCron 是 Redis 的后勤总管。你设的 TTL 能过期、AOF 能自动重写、主从能自动重连——都靠它。**它不处理用户请求，但没有它 Redis 就是个静态数据结构。**

---

## 二、SDS（Simple Dynamic String）— 更好的字符串

C 语言原生的 `char*` 有三大死穴：
1. 获取长度是 O(n)（要遍历到 `\0`）
2. 拼接字符串容易缓冲区溢出 (`strcat` 不检查大小)
3. 存二进制数据会被 `\0` 截断

SDS 把这些全解决了：

```c
// sds.h - SDS 的不同头部（省内存的极致操作）
struct __attribute__ ((__packed__)) sdshdr5 {  // 长度 < 2^5
    unsigned char flags;  // 低 3 位存类型，高 5 位存长度
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {  // 长度 < 2^8
    uint8_t len;         // 已用长度
    uint8_t alloc;       // 已分配长度（不含头部和 \0）
    unsigned char flags;
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 { // 长度 < 2^16
    uint16_t len;
    uint16_t alloc;
    unsigned char flags;
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 { // 长度 < 2^32
    uint32_t len;
    uint32_t alloc;
    unsigned char flags;
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 { // 长度 < 2^64
    uint64_t len;
    uint64_t alloc;
    unsigned char flags;
    char buf[];
};
```

### SDS 的核心设计

```
sdshdr8 的内存布局:

 ┌──────┬──────┬───────┬──────────────────────────┬───┐
 │ len  │alloc │ flags │        buf[]              │\0 │
 │  5   │  16  │  0x01 │  "h e l l o"             │   │
 └──────┴──────┴───────┴──────────────────────────┴───┘
                         ↑
                       指针 s 指向这里（不是头部）
                       
 所以 sds 可以直接当作 char* 传给任何 C 函数！
```

> **骚话：** SDS 最骚的地方在于——返回给外部的指针指向的是 `buf`，不是头部。所以你可以把 SDS 当成 `char*` 直接传给 `printf` 或者任何 C 函数。**看起来是原生字符串，背后却藏了一个完整的内存管理系统。这叫"鸭子类型在 C 语言中的极致体现"。**

### SDS 的优化细节

| 特性 | C 原生 char* | SDS |
|------|-------------|-----|
| 获取长度 | O(n) 遍历 | O(1) 读 len 字段 |
| 缓冲区溢出 | 可能 | 自动扩容，永远不会溢出 |
| 内存预分配 | 无 | 扩容时多分配一倍（减少 realloc 次数） |
| 惰性释放 | 无 | 缩短时不立即释放（下次扩容直接用） |
| 二进制安全 | ❌ 遇到 `\0` 就截断 | ✅ len 记录长度，不依赖 `\0` |

```c
// SDS 扩容的预分配策略
sds sdsMakeRoomFor(sds s, size_t addlen) {
    // ...
    if (newlen < SDS_MAX_PREALLOC) // 1MB
        newlen *= 2;               // 翻倍扩容
    else
        newlen += SDS_MAX_PREALLOC; // 超过 1MB，每次只加 1MB
    // ...
}
```

> **金句：** SDS 的内存预分配策略和 Java 的 ArrayList 扩容、Go 的 Slice 扩容如出一辙。**好的数据结构设计跨越语言——本质都是"用空间换时间，用惰性换吞吐"。**

---

## 三、字典（dict.c）— 渐进式 Rehash 的教科书实现

Redis 的 Hash 表底层就是 `dict`，这是整个 Redis 中使用最广泛的数据结构（Hash 类型用它、全局 Key 空间用它、过期 Key 集合用它……）。

### dict 的结构

```c
// dict.h
typedef struct dict {
    dictType *type;          // 类型特定函数（hash 函数、key 比较...）
    void *privdata;
    dictht ht[2];            // 两个哈希表！核心就在这里
    long rehashidx;          // rehash 进度（-1 = 不在 rehash）
    int16_t pauserehash;     // rehash 暂停计数器
} dict;

typedef struct dictht {
    dictEntry **table;       // 哈希表数组
    unsigned long size;      // 数组大小（总是 2^n）
    unsigned long sizemask;  // size - 1（取模用, hash & sizemask 比 % 快）
    unsigned long used;      // 已用节点数
} dictht;

typedef struct dictEntry {
    void *key;
    union { void *val; uint64_t u64; int64_t s64; } v;
    struct dictEntry *next;  // 链表法解决哈希冲突
} dictEntry;
```

### 渐进式 Rehash：Redis 最精妙的设计

```
为什么需要两个 ht[2]？

Redis 单线程，不能一次性把整个哈希表 rehash（阻塞太久）。
所以它"一次搬一点，边用边搬"：

初始状态:
  ht[0] → [entry1]→[entry2]→[entry3]→[entry4]  (size=4)
  ht[1] → 空
  rehashidx = -1

开始 rehash:
  ht[0] → 旧表 (正在被搬走)
  ht[1] → 新表 [entry1]→[entry2]→...  (size=8, 扩容一倍)
  rehashidx = 2  (表示前 2 个槽位已搬完)

每次对 dict 做增删改查时:
  顺便搬一个槽位: rehash(ht[0], ht[1], rehashidx++)
  搬完所有槽位后:
    ht[1] → ht[0]
    ht[0] = 空
    rehashidx = -1
```

> **金句：** 渐进式 Rehash 是 Redis 给所有写高并发系统的人上的最生动一课——**一个长操作不要一次性做完，切成小块，每次做一点，用户完全感受不到。** 这和前端的时间切片（Time Slicing）、操作系统的分时调度是同一种智慧。

### Rehash 的触发条件

```c
// dict.c - _dictExpandIfNeeded()
if (d->ht[0].used >= d->ht[0].size &&
    (dict_can_resize || d->ht[0].used / d->ht[0].size > dict_force_resize_ratio))
{
    return dictExpand(d, d->ht[0].used * 2); // 扩容一倍
}

// 负载因子 = used / size
// 一般情况下 > 1 就扩容
// 有 BGSAVE/BGWRITEAOF 子进程时 > 5 才扩容（减少 Copy-On-Write 开销！）
```

> **骚话：** 注意到没有？有 fork 子进程时，rehash 阈值从 1 提到 5。**为什么？因为 rehash 会大量写内存，导致 Copy-On-Write 页复制，内存翻倍。** 这行代码告诉你：Redis 的作者不只是写数据结构，他在写一整套"和生产环境博弈"的策略。

---

## 四、跳表（t_zset.c）— SortedSet 的核心引擎

### 跳表数据结构

```c
// server.h
typedef struct zskiplist {
    struct zskiplistNode *header, *tail;
    unsigned long length;        // 节点总数
    int level;                   // 当前最高层数
} zskiplist;

typedef struct zskiplistNode {
    sds ele;                     // 元素值
    double score;                // 分数
    struct zskiplistNode *backward;  // 后退指针（双向）
    struct zskiplistLevel {
        struct zskiplistNode *forward; // 前进指针
        unsigned long span;            // 跨度（跳过了多少个节点）
    } level[];                   // 柔性数组，每个节点有不同的层数
} zskiplistNode;
```

### 层数怎么决定？— 随机！

```c
// t_zset.c
int zslRandomLevel(void) {
    int level = 1;
    // 每次有 1/4 概率提升一层 (ZSKIPLIST_P = 0.25)
    while ((random() & 0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
        level += 1;
    return (level < ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
    // ZSKIPLIST_MAXLEVEL = 32，最多 32 层
}
```

> **骚话：** 跳表的层数是**扔硬币**决定的。每层 1/4 概率提升。理论上 32 层可以支撑 2^32 个节点。**不用旋转、不用平衡因子、不用染色——一个随机函数就替代了红黑树的复杂逻辑。这是随机算法的工程胜利。**

### 跳表插入 vs 红黑树

```
跳表 vs 红黑树对比：

跳表:
  ✅ 实现简单（~200 行 vs 红黑树 ~500 行）
  ✅ 支持范围查找（按顺序遍历，像链表一样自然）
  ✅ 实现无锁并发更容易（只有指针修改）
  ✅ 平均复杂度 O(log n)，概率保证
  
  ❌ 不保证绝对平衡（但概率上几乎和红黑树一样）
  ❌ 比红黑树多占一点内存（每层多一个 forward 指针）

红黑树:
  ✅ 严格平衡，保证最坏 O(log n)
  ❌ 实现复杂、旋转难调试
  ❌ 范围查找不如链表自然
```

---

## 五、Quicklist — 链表的工程优化

Redis 3.2 之前，List 有两种编码：`ziplist`（内存紧凑但插入慢）和 `linkedlist`（插入快但内存浪费）。**Quicklist 把两者融合了。**

```
Quicklist 的结构:

quicklist:
┌───┬───┬───┬───┬───┬───┐
│ H │ E │ C │ E │ C │ T │
│ E ｜ N ｜ O ｜ N ｜ O ｜ A │
│ A ｜ T ｜ U ｜ T ｜ U ｜ I │
│ D ｜ R ｜ N ｜ R ｜ N ｜ L │
│   ｜ Y ｜ T ｜ Y ｜ T ｜   │
└───┴───┴───┴───┴───┴───┘
   │              │
   ▼              ▼
  ziplist       ziplist    ← 每个节点是一个小的 ziplist
 (3 个元素)    (2 个元素)
 
→ 头尾两端同时操作 ziplist，中间节点做一个链表
→ 内存紧凑（ziplist） + 快速插入删除（链表）= Quicklist
```

```c
// quicklist.h
typedef struct quicklist {
    quicklistNode *head;
    quicklistNode *tail;
    unsigned long count;    // 所有 ziplist 中的总元素数
    unsigned long len;      // quicklistNode 的数量
    int fill : 16;          // 每个 ziplist 最多存多少元素
    unsigned int compress : 16; // 中间节点压缩深度
} quicklist;

typedef struct quicklistNode {
    struct quicklistNode *prev;
    struct quicklistNode *next;
    unsigned char *zl;       // 指向 ziplist 或 listpack
    unsigned int sz;         // zl 的字节数
    unsigned int count : 16; // ziplist 中的元素数
    unsigned int encoding : 2; // 1=ziplist, 2=listpack
    unsigned int container : 2; // NONE=1
    unsigned int recompress : 1;
} quicklistNode;
```

> **金句：** Quicklist 的设计哲学是**"中庸之道"**——不要极端地选择纯链表或纯数组，而是在中间找一个平衡点。**LinkedList 太费内存，Array 太慢插入，Quicklist 把两个的缺点都规避了。工程就是做 Trade-off**

---

## 六、内存管理的智慧

### redisObject：Redis 所有对象的基类

```c
// server.h
typedef struct redisObject {
    unsigned type:4;        // 类型: STRING/LIST/HASH/SET/ZSET
    unsigned encoding:4;    // 编码: INT/EMBSTR/RAW/ZIPLIST/SKIPLIST...
    unsigned lru:24;        // LRU 时间 (秒) 或 LFU 访问计数
    int refcount;           // 引用计数（共享对象的关键）
    void *ptr;              // 指向实际数据
} redisObject;              // 总共 16 字节
```

### 引用计数与共享对象

```c
// Redis 启动时会创建 0~9999 的整数共享对象
// 所有用到 0~9999 整数的地方都直接引用这些预创建的对象

for (j = 0; j < OBJ_SHARED_INTEGERS; j++) {
    shared.integers[j] = createObject(OBJ_STRING, (void*)(long)j);
    shared.integers[j]->refcount = OBJ_SHARED_REFCOUNT; // 永不释放
}

// 所以当你 SET key 42 或者 SET key 9999 时，
// Redis 不需要分配新内存，直接指向共享对象！
```

> **骚话：** 共享对象池是一个简单粗暴但极其有效的内存优化——0~9999 的整数每个 Redis 实例只用存一份。**但 Redis 7.0 之后开始移除这个机制，因为它的收益在大多数现代场景下微乎其微，反而增加了代码复杂度。这是工程上的"懂得放弃"。**

---

## 七、如何开始读 Redis 源码

### 推荐的阅读顺序

```
阶段 1: 搞懂"怎么跑起来的"  (1-2 周)
├── ae.c/ae.h          → 事件循环（500 行，必读）
├── server.c           → 主函数 main() + initServer() + serverCron()
└── networking.c       → 客户端连接、命令接收、响应发送

阶段 2: 搞懂"数据怎么存的"  (2-4 周)
├── sds.c/sds.h        → 动态字符串（600 行，强烈推荐）
├── dict.c/dict.h      → 字典 + 渐进式 Rehash（500 行，精读！）
├── object.c           → redisObject
├── ziplist.c          → 压缩列表（先了解结构，再读操作函数）
├── quicklist.c        → List 的底层实现
├── intset.c           → 整数集合
└── t_zset.c           → 跳表（只看 zslInsert/zslDelete 就行）

阶段 3: 搞懂"数据怎么持久化和复制的"  (2-3 周)
├── rdb.c              → RDB 快照
├── aof.c              → AOF 日志
├── replication.c      → 主从复制
└── expire.c           → 过期删除策略

阶段 4: 搞懂"集群怎么工作的"  (3-4 周)
├── sentinel.c         → 哨兵
├── cluster.c          → 集群（2500+ 行，挑战！）
└── t_stream.c         → Stream 类型
```

### 阅读技巧

| 技巧 | 说明 |
|------|------|
| **GDB 调试验证** | `gdb --args redis-server redis.conf`，设断点，单步执行看数据结构变化 |
| **画数据结构图** | 纸和笔是读 C 源码最好的工具——指针指来指去，不画图就是噩梦 |
| **先读注释** | antirez 的注释写得非常好，函数头部注释就是缩略版文档 |
| **只读主干** | 不要每行都读——先看懂核心数据结构和核心函数，边缘情况以后再说 |
| **带着问题读** | "SDS 怎么避免缓冲区溢出？""渐进 Rehash 怎么不影响正常查询？" |

> **金句：** 很多人买了《Redis 设计与实现》就放在那里吃灰，或者在博客上看了几篇"Redis 源码分析"就以为懂了。**源码不会骗你——你读了几行，你就真懂了几分。没读就是没读，面试官总能问出来。**

---

## 源码中那些让人拍案叫绝的细节

### 1. 取模优化

```c
// dict.c 中计算 Key 在哪个槽位：
idx = hash & d->ht[table].sizemask;  // size 总是 2^n, sizemask = size - 1

// hash % size  →  hash & (size - 1)  当 size 是 2 的幂时，等价且更快
// 位运算比取模快一个数量级
```

### 2. 柔性数组（Zero-Length Array）

```c
// SDS header 的 buf[] 和 zskiplistNode 的 level[] 都是柔性数组
// 好处：结构体 + 动态数据连续存放，一次 malloc，缓存友好
struct sdshdr8 {
    uint8_t len;
    uint8_t alloc;
    unsigned char flags;
    char buf[];  // 不占空间，只是一个占位符
};
```

### 3. 惰性删除到处都是

```c
// Key 过期了不会立即删
// 1. 访问时检查过期 → 惰性删除
// 2. serverCron 随机抽 20 个 → 定期删除
// 不主动全量扫描 → 因为 O(n) 会阻塞
```

### 4. 用 union 省内存

```c
// dictEntry 的 value:
union {
    void *val;        // 指向复杂对象
    uint64_t u64;     // 直接存 uint64
    int64_t s64;      // 直接存 int64
} v;
// 如果值是数字，就不需要额外 malloc → 省一次内存分配
```

> **骚话收尾：** Redis 源码中没有一行是多余的——你甚至找不出什么可以删掉的东西。antirez 写代码的风格是 **"能用三行解决的事绝不写十行，但该考虑的边缘情况一个不漏"**。读完 Redis 源码，你就知道了什么叫 **"C 语言优雅起来有多优雅"** 。
>
> 如果你只能读一个开源项目的源码来提升自己，读 Redis。**不是因为 Redis 是"最好的 C 项目"，而是因为它恰好卡在"不太大也不太小、不太简单也不太难"的甜蜜点上。好到值得读，小到读得完。**
