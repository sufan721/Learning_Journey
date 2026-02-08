### 查询
```
#数据库不区分大小写， 但关键字最好大写
查询所有数据库：
	SHOW DATABASES;
	show databases;
查询当前数据库：
	SELECT DATABASES();
	select databases();
创建数据库：
	CREATE DATABASE[IF NOTEXISTS]数据库名[DEFAULT CHARSET字符集][COLLATE 排序规则];
	create database 数据库名；
删除数据库：
	DROP DATABASE「IF EXISTS]数据库名;
	drop datbase 数据库名;
使用：
	USE 数据库名;
	use 数据库名;
```
	
### 作业：创建一个数据库为animal，并查看使用
```
答：
create database animal;
use animal;
select databases();
```
### 在学习中我遇到的一些注意事项：
```
中断和Linux一样ctrl + c
在使用命令时记得末尾分号
初学者不好记的话，看小写英文，，操作对象是单个数据库的话用单数，多个用复数
```

