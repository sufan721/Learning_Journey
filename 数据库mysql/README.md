# MySQL 数据库学习指南

## 📚 项目简介

本目录用于记录 MySQL 数据库的学习笔记、SQL 语句示例和最佳实践。从基础语法到高级优化，系统地掌握关系型数据库的核心知识。

## 📂 目录结构

```
数据库mysql/
├── 基础语法/           # MySQL 基础 SQL 语法
├── 数据类型/           # 数据类型详解
├── 表设计/             # 表结构设计与规范
├── 查询优化/           # SQL 查询性能优化
├── 索引/               # 索引原理与应用
├── 事务与锁/           # 事务处理和并发控制
├── 备份恢复/           # 数据库备份与恢复
├── 实战项目/           # 实际应用场景
└── README.md           # 本文件
```

## ✅ 已学习内容

- [x] **基础 SQL 语句** - SELECT, INSERT, UPDATE, DELETE
- [x] **数据定义语言** - CREATE, ALTER, DROP
- [x] **数据类型** - INT, VARCHAR, DATE, DECIMAL 等常用类型
- [x] **基本查询** - WHERE, ORDER BY, GROUP BY, HAVING

## 🚀 学习路线

### 第一阶段：基础入门
- SQL 基本语法
- 数据类型和表操作
- 简单查询和过滤
- 基本函数使用

### 第二阶段：进阶查询
- 多表联接（JOIN）
- 子查询和嵌套查询
- 聚合函数和分组统计
- 视图和存储过程

### 第三阶段：优化与管理
- 索引设计与优化
- 查询性能分析
- 事务和锁机制
- 数据备份与恢复

### 第四阶段：实战应用
- 数据库设计最佳实践
- 项目实际应用
- 性能调优案例
- 常见问题解决

## 🔧 常用 MySQL 命令

### 连接数据库
```bash
mysql -u root -p
mysql -h localhost -u root -p database_name
```

### 基本操作
```sql
-- 创建数据库
CREATE DATABASE test_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 使用数据库
USE test_db;

-- 创建表
CREATE TABLE users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(100) UNIQUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 插入数据
INSERT INTO users (name, email) VALUES ('张三', 'zhangsan@example.com');

-- 查询数据
SELECT * FROM users WHERE name = '张三';

-- 更新数据
UPDATE users SET email = 'new_email@example.com' WHERE id = 1;

-- 删除数据
DELETE FROM users WHERE id = 1;
```

## 📊 性能优化要点

- **索引优化** - 合理使用单列索引、复合索引
- **查询优化** - 避免全表扫描，使用 EXPLAIN 分析
- **表设计** - 范式化设计，避免冗余
- **事务处理** - 合理控制事务大小，避免死锁

## 💡 常用工具

- **navicat** - 图形化数据库管理工具
- **workbench** - MySQL 官方管理工具
- **phpmyadmin** - Web 管理界面
- **命令行** - mysql-cli

## 📝 学习资源推荐

- MySQL 官方文档
- 《MySQL 必知必会》
- 《高性能 MySQL》
- LeetCode 数据库题目

## 🎯 学习进度

- [x] 基础语法学习
- [ ] 深入索引机制
- [ ] 实战项目完成
- [ ] MySQL 刷题讲解

## 📌 笔记规范

所有笔记文件命名规范：
- 使用中文或英文，保持一致
- 日期格式：YYYY-MM-DD
- 文件名示例：`2026-04-19-索引优化.md`

## 🔗 相关资源

- [MySQL 官网](https://www.mysql.com/)
- [MySQL 文档](https://dev.mysql.com/doc/)

---

**最后更新**: 2026-04-19  
**学习状态**: 持续学习中 📚
```

现在您的 `数据库mysql` 文件夹中已有一份详细的 README.md，包含：

✨ **主要内容：**
- MySQL 学习的完整学习路线（4个阶段）
- 常用命令和代码示例
- 性能优化要点
- 学习进度追踪
- 推荐学习资源

您可以根据实际学习进度随时更新其中的内容！