| 分类   | 类型                       | 大小               | 描述         |
| ---- | ------------------------ | ---------------- | ---------- |
| 数值类型 | TINYINT(tinyint)         | 1 byte           | 小整数值       |
| 数值类型 | SMALLINT(smallint)       | 2 bytes          | 大整数值       |
| 数值类型 | MEDIUMINT(mediumint)     | 3 bytes          | 大整数值       |
| 数值类型 | INT或INTEGER(int或integer) | 4 bytes          | 大整数值       |
| 数值类型 | BIGINT(bigint)           | 8 bytes          | 极大整数值      |
| 数值类型 | FLOAT(float)             | 4 bytes          | 单精度浮点数值    |
| 数值类型 | DOUBLE(double)           | 8 bytes          | 双精度浮点数值    |
| 数值类型 | DECIMAL(decimal)         | 依赖于M(精度)和D(标度)的值 | 小数值(精确定点数) |
***无符号(UNSIGNED)(unsigned)***
***有符号(SIGNED)(signed)***

|分类|类型|大小|描述|
|---|---|---|---|
|字符串类型|CHAR(char)|0 - 255 bytes|定长字符串|
|字符串类型|VARCHAR(varchar)|0 - 65535 bytes|变长字符串|
|字符串类型|TINYBLOB(tinyblob)|0 - 255 bytes|不超过255个字符的二进制数据|
|字符串类型|TINYTEXT(tinytext)|0 - 255bytes|短文本字符串|
|字符串类型|BLOB(blob)|0 - 65 535 bytes|二进制形式的长文本数据|
|字符串类型|TEXT(text)|0 - 65 535 bytes|长文本数据|
|字符串类型|MEDIUMBLOB(mediumblob)|0 - 16 777 215 bytes|二进制形式的中等长度文本数据|
|字符串类型|MEDIUMTEXT(mediumtext)|0 - 16 777 215 bytes|中等长度文本数据|
|字符串类型|LONGBLOB(longblob)|0 - 4 294 967 295 bytes|二进制形式的极大文本数据|
|字符串类型|LONGTEXT(longtext)|0 - 4 294 967 295 bytes|极大文本数据|
使用方式：
	varchar(长度)
