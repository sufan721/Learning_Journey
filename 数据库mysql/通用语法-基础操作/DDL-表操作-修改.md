### 添加字段：
```
alter table 表名 add 字段名 类型 [comment 注释][约束];
```
### 修改字段名和字段类型：
```
alter table 表名 change 旧字段 新字段 类型[comment 注释];
```
### 修改数据类型：
```mysql
alter table 表名 modify 字段名 新数据类型;
```
### 删除字段：
```
alter table 表名 drop 字段名;
```
### 修改表名：
```
alter table 表名 rename to 新表名;
```
