### ls   列出文件信息：
	ls [选项][目录或文件]
	-a： 列出所有文件，包括隐藏文件
	-d：将目录像文件一样显示，而不是显示其下的文件。
	-i：输出文件的 i 节点的索引信息。
	-k：以 k 字节的形式表示文件的大小。
	-l：列出文件的详细信息。
	-t：以时间排序。
	-R：递归列出所有子目录下的文件。
### pwd：显示用户当前的目录
### cd：改变当前工作目录
	回到home目录：
		cd
		cd ~
	切换到上级目录
		cd ..
		cd ../.. 上两级目录
	切换到上个目录
		cd -
	相对路径和绝对路径
		cd Documents         # 相对路径
		cd /home/user/Docs   # 绝对路径
		cd ./subdir         # 当前目录下的子目录
### touch ：更改文件或目录的日期时间，或新建一个不存在的文件。
	 touch [选项] 文件名
		 touch filename.txt # 创建单个文件
		 touch file1.txt file2.txt file3.txt # 创建多个文件
	 -a：只更改访问时间
	 -m：只更改变动时间
	 -c：不创建新文件，只更新时间戳
	 -d：使用指定字符串表示时间    #touch -d "2025-12-25 18:30:30" file.txt
	 -t：使用指定时间戳格式
	 -r：参照其他文件的时间    touch -r a b：b的时间和a相同
### mkdir：创建目录（文件夹）
	mkdir [选项] 目录名 
		mkdir newf #创建单个目录
		mkdir dir1 dir2 dir3 #创建多个目录
	-p：创建多级目录       #mkdir -p /home/user/projects/2025/src/main/java
	-m：设置目录权限
	-v：显示详细信息
		#指令混用
			mkdir -pv dir1/dir2/dir3
			# 输出：
			# mkdir: created directory 'dir1'
			# mkdir: created directory 'dir1/dir2'
			# mkdir: created directory 'dir1/dir2/dir3'
		#批量创建用户
			mkdir -p /home/{alice,bob,charlie}/Documents
### rmdir：删除目录（文件夹）
	rmdir *  # 删除所有空目录
	-p:递归删除空目录（同时删除父目录）
	-v:显示删除过程的详细信息
### chmod：(分级权限)八进制表示方法/符号表示法
	三类权限：
		   符号    数字   对文件       对目录
	读：     r     4    查看内容    列出目录内容
	写：     w     2    修改内容    创建/删除内容
	执行：   x     1    执行文件    进入目录
	
	用户身份：
	用户类型     符号           说明
	所有者：	 u (user)      文件创建者
	所属组：     g (group)     文件所属的用户组
	其他人：     o (others)    其他所有用户
	
	八进制：
		三位数字[所有者][所属组][其他人]
			755： rwx  rx  rx  #文件创作者可读/写/执行，所属的组可以读/执行
	符号表示：
			+  # 添加权限
			-  # 移除权限
			=  # 设置权限（覆盖原有）
	chmod [[所有者][所属组][其他人]] [目录/文件]
	chmod [谁][符号][权限] 
		chmod 755 file.cpp 
			对于file.cpp只有创建者可以修改，其他人只能读和执行
		chmod g+w file.cpp
			对于用户组，添加可以执行的权限
### rm：删除
	rm [选项] 文件名
	-f：强制删除所有即使是只读 
	-i：删除前逐一询问确认
	-r：删除目录及其下所有文件
### man：查指令和系统调用接口
	man [选项] 命令名
	1：命令
	2：系统调用
	3：库函数
	4：特殊文件
	5：文件格式
	6：游戏
	7：附件与变量
	8：系统管理命令    
	-k：搜索关键词
	num：只在num页查找
	-a：所有章节的内容
### 通配符：简单的匹配
	*：匹配任意字符
	?：匹配单个字符
	[]:匹配字符集
### cp：复制
	cp [选项] 源文件/目录   目标文件或目录 ：a->b
	-f ,--force：强行复制文件
	-i ,--interactive：覆盖文件之前询问用户
	-r,-R,--recursive：递归
### mv：移动
	mv [选项] 源文件/目录   目标文件或目录 ：a->b
	-f：强行移动文件
	-i：覆盖文件之前询问用户
	mv [文件] [新名字]       # 重命名
### cat:查看文件内容(cout)
	cat [选项] 文件
	-b:对非空内容编号 
	-n:对输出内容行编号
	-s:不输出多行空格
### less：终端分页/搜索
	less [选项] 文件
	分页：
		f:向前一页
		b：向后一页
		d：向前半页
		u：向后半页
		e：向前一行
		y：向后一行
		g：开头
		G：结尾
	搜索：
		/pattern：向前搜索
		?pattern：向后搜索
### head&tail ：打印
	head/tail -num file
		
	head -10 text.txt:正打印
	tail -10 text.txt：反打印
	|：中间查找
		head -520 text.txt|tail -5
			抽取前520行，在抽取后5行：516，517，518，519，520   
### date：对时间操作
	显示时间：
		data +.. ..
		%H：小时
		%M：分钟
		%S：秒
		%d：日
		%m：月
		%F：相当%Y-%m-%d
		%Y：完整年份
		%X：相当%H:%M:%S
	设定时间：
		-s
	时间戳转换：
		+%s:时间->时间戳
		+%Y:%m:%d **-d @**1599642565：时间戳->时间
### cal：日历
### 重定向
	输出重定向： >  (cin)
		echo "内容" > [目标]
			会清除原有内容
	追加重定向： >>
		echo "内容" >> [目标]
			不会清除原有内容
	输出重定向：<
		
### find：查找
	find [路径] -name 文件 ./：当前路径
### grep：搜寻
	grep [选项] "搜寻内容" 文件
	-i：忽略大小写
	-n：同时输出行号
	-v：反向选择，输出没有"搜寻内容"的那一行
	
### zip/unzip
	zip：压缩成zip
	unzip：解压zip文件 -d 目标路径
	-r：递归处理
### tar：打包和解包文件，并可直接查看内容。
	 tar [-cxtzjvf] 文件与目录 ...
	 -c：建立一个压缩文件的参数指令(create)
	 -x：解开一个压缩文件的参数指令
	 -t：查看 tarfile 里面的文件
	 -z：是否同时具有 gzip 的属性
	 -j：是否同时具有 bzip2 的属性
	 -v：显示压缩/解压过程中处理的文件
	 -f：使用档名
	-C：解压到指定目录
### uname：获取**电脑和操作系统的相关信息**。
	uname [选项]
		-a
### 常用热键：
	[tab]: 命令补全 
	[ctrl] - c ：停掉程序
### vim:
	插入
		->命令： Esc
	底行
		->命令：ESC
	命令
		->插入： 
			:i,a,o
		->保存退出：
			:wq  保存并退出
			:w   保存
			:q!  不保存强制退出
### 常用命令扩展
	安装和登录命令: login, shutdown, halt, reboot, install, mount, umount, chsh, exit, last
	文件处理命令: file, mkdir, grep, dd, find, mv, ls, diff, cat, ln
	系统管理相关命令: df, top, free, quota, at, lp, adduser, groupadd, kill, crontab
	网络操作命令: ifconfig, ip, ping, netstat, telnet, ftp, route, rlogin, rcp, finger, mail, nslookup
	系统安全相关命令: passwd, su, umask, chgrp, chmod, chown, chattr, sudo, ps, who
	其它命令: tar, unzip, gunzip, unarj, mtools, man, unendcode, uudecode