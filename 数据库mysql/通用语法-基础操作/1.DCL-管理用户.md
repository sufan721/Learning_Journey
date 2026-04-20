### 查询用户：
```mysql
use mysql ;
select * from  user;
```
### 创建用户：
```mysql
create user '用户名'@'主机名' identified by '密码';
```
### 修改用户密码：
```mysql
alter user '用户名'@'主机名' identified with mysql_native_password by '新密码';
```
### 删除用户：
```mysql
drop user '用户名'@'主机名';
```