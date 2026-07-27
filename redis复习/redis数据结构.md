

redis的key 一般为string ，而value能多种多样

| String    |
| --------- |
| Hash      |
| List      |
| Set       |
| SortedSet |
| GEO       |
| BitMap    |
| HyperLog  |



### String

string类型，也就说字符串类型，是Redis中最简单的存储类型
其中value是字符串，根据字符串的格式不同，又可以分为三类：
- string：普通字符串
- int：整数类型，可以自增，自减操作
- float：浮点类型，可以自增，自减操作
不管哪种格式，底层都是字节数组形式存储，只不过编码方式不同


### Key的层级结构
在实际的业务中，我们可能会有key相等的情况，所以为了解决这个问题

Redis的key允许多个单词形成层级关系，多个单词之间用 **':'** 进行隔开 一般为
==项目名:业务名:类型:id==
当然格式非固定可以根据自己的需求来添加或删除词条 

如果value对象为json的话，可以将json转化为json字符串进行存储  

### Hash类型

hash类型，其的value是一个无序字典，类似于HashMap

当存储类型为String而存储的对象为json字符串时，那么修改将变得困难，主要原因是每次都要将json字符串转为json再修改完后有需转换为json字符串，开销大。 

hash类型的出现就是为了解决这个问题 ，他的key和String一样，但是value存储的方式不同，可以分开来进行存储

| KEY          | VALUE |       |
| ------------ | ----- | ----- |
|              | field | value |
| heima:user:1 | name  | Jack  |
|              | age   | 21    |

### List
Redis中的List和c++的list差不多，可以看作一个双向链表，既可以支持正向检索和反向检索

特征：
- 有序
- 元素可以重复
- 插入和删除快
- 查询速度一般


### Set

Redis中的Set和Java中的HashSet类似，可以看作是一个value为空的HashMap，因为也是一个Hash表，因此具备与HashSet类似的特征，更重要的他不在是以元素为单位，而是一个集合，可以向这个集合中 添加/删除 元素


- 无序
- 元素不可重复
- 查找快
- 支持交集，并集，差集等功能


### SortedSet
SortedSet 一种可排序的set集合，他的本质并非红黑树，  而是由跳表加hash表实现 。每一个元素都带有一个score属性，可以基于score属性对元素进行排序

- 可排序
- 元素不重复
- 查询速度快
当然因为其可排序特性，经常用于实现排行榜这样的功能




