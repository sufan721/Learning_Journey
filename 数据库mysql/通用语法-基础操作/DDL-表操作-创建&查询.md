### 查询：
```
查询当前数据库所有表：
	SHOW TABLES;
	show tables;
查询表结构：
	DESC 表名;
	desc 表名;
查询指定表的建表语句：
	SHOW CREATE TABLE 表名;
	show create table 表名;
```
### 创建：
```
CREATE TABLE 表名(
字段1 字段类型 [COMMENT  字段1注释],
字段2 字段类型 [COMMENT  字段2注释],
字段3 字段类型 [COMMENT  字段3注释],	
......
字段n 字段类型 [COMMENT  字段n注释]
)[COMMENT  表注释];

create table 表名(
字段1 字段类型 [comment  字段1注释],
字段2 字段类型 [comment  字段2注释],
字段3 字段类型 [comment  字段3注释],	
......
字段n 字段类型 [comment  字段n注释]
)[comment  表注释];
```

### 在学习中我遇到的一些注意事项：
```
	表注释和前面的()之间没有空格
```
	