### 给指定字段添加数据：
```mysql
 insert into 表名(字段1，字段2...) values(值1，值2...);
```
### 给全部字段添加数据：
```mysql
insert into 表名 values(值1，值2...);
```
### 批量添加操作：
```mysql
insert into 表名(字段1，字段2...) values(值1，值2...),(值1，值2...)...；
insert into 表名 values(值1，值2...),(值1，值2...),...;
```