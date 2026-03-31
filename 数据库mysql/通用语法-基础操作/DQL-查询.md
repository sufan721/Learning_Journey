### 查询：
```
基本查询：
	SELECT(select)
		字段列表
	FROM(from)
		表名列表
条件查询：
	WHERE(where)
		条件列表
聚合函数
	count 计数
	max 最大值
	min 最小值
	avg 平均值
	sum 求和		

分组查询：
	GROUP BY (group by)
		group by后面的是通过什么进行分组
		分组字段列表
	HAVING (having)
		分组后条件列表
排序查询：
	ORDER BY (order by)
		排序字段列表
分页查询：
	LIMIT (limit)
		分页参数
```

### 基本查询：
```mysql
select 字段... from 表名;
select * from 表名;
起别名
select 字段 as 别名 from 表名;
去除重复记录
select distinct 字段列表 from 表名;
```

### 条件查询：
```mysql
select 字段列表 from 表名 where 条件列表;
```

### 聚合函数：
```mysql
select 聚合函数(字段列表)from 表名 
count 计数
max 最大值
min 最小值
avg 平均值
sum 求和
```

### 分组查询：
```mysql
select 字段列表 from 表名[where 条件]group by 分组字段名 [having 分组后过滤条件];
```

### 排序查询：
```mysql
selcet 字段列表 from 表名 order by 字段1 排序方式1,字段2 排序方式2;
排序方式：
	ASC：升序
	DESC：降序	
	
	select customer_number
	from Orders
	group by customer_number
	order by count(order_number) desc
	limit 1;
```

### 分页查询：
```mysql
select 字段列表 from 表名 limit 起始索引, 查询记录数;
like 模糊搜索
注：
	·起始索引从0开始，起始索引=(查询页码-1)*每页显示记录数:
	·分页查询是数据库的方言，不同的数据库有不同的实现，MySQL中是LIMIT。
	·如果查询的是第一页数据，起始索引可以省略，直接简写为limit 10。
```