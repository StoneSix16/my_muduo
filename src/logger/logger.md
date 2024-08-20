# logger

日志模块, 实现可异步的日志功能. 该模块定义了固定大小的缓冲区类`FixedBuffer`, 在此基础上实现了日志流`LogStream`和日志输出`Logging`. 另外设计了针对文件的日志输出`logFile`及异步实现`AsyncLogging`

## FixedBuffer

FixedBuffer类是一个以缓冲区大小为模板参数的模板类, 实例化了大小为4KB和4MB的模板. FixedBuffer包含一个字符数组和当前位置的指针, 支持对数组的读写.

## LogStream

LogStream类使用4KB的FixedBuffer封装, 并重载了流运算符, 并实现了Fmt和MonoFmt类, 使流运算支持格式化字符串.

### 支持的操作

通过`append(const char* data, int len)` 添加字符串

通过`buffer()` 获取缓冲区

通过`resetBuffer()` 重置缓冲区

### 流运算

流运算支持字符和数的输出, 数(整形和浮点)的最大支持长度为48, 更长的数不会被存入缓冲区.

Fmt格式化类通过`Fmt(const char* fmt, T value)`将一个值格式化输出. MonoFmt格式化类通过`MonoFmt(const char* data, int len)`将`data`以长度`len`输出, 多余的长度用空格填充.

## Logger

Logger类由一个Impl类和一些静态函数组成. Impl类负责将日志内容写入stream对应的缓冲区. 静态函数用于设置两个全局变量`g_output`和`g_flush`, 其负责输出和同步缓冲区内容. 另外, 设置了一些宏用于日志输出.

要输出日志时, Logger会新建一个实例并暴露Impl实例中的stream接口, 日志内容写入stream对应的缓冲区. 离开作用域后, Logger实例析构时, 在析构函数中调用`g_output(const char*, int len)`执行输出操作.

## LogFile

LogFile类负责创建日志文件并写入内容. LogFile需要一个辅助类FileUtil打开/关闭/写入文件.

### FileUtil

辅助类, 通过构造函数`FileUtil(const std::string& name)`创建一个文件并绑定一个64KB大小的缓冲区. 

通过`append(const char*, int)/flush()`可以写入/刷新文件缓冲区, 通过`writtenBytes`可以获取已经写入文件的字节数.

### LogFile逻辑

LogFile使用三个变量`startOfPeriod`, `lastRoll`, `lastFlush`, 分别记录最后一次日志文件创建的天数, 最后一次日志文件创建的时间, 最后一次刷新缓冲区的时间; 使用三个变量`rollSize`, `flushInterval`, `checkEvryN` 控制创建日志文件, 刷新缓冲区和检查; 一个FileUtil实例`file`指向需要写入的文件.

通过`append(const char*, int len)`写入日志内容时, 会先判断写入内容是否多于rollSize, 据此决定是否创建新的日志文件. 当写入次数超过`checkEveryN`时, 检查是否已过最后一次创建文件的天数或刷新缓冲区的间隔, 据此执行相关的操作. 写入操作会对文件加锁, 以保证线程安全.

## AsyncLogging

异步日志分为前后端, 其他线程的写入操作在前端缓冲区进行, 后端另开一个线程, 负责获取前端缓冲区的内容并通过一个logFile实例写入日志文件.

### 支持的操作

构造AsyncLogging实例后, 需要用`start()/stop()`启动/停止后端线程. 启动时, 用信号量保证后端线程开始执行回调函数后从start()返回. 停止时, 唤醒后端线程并等待其返回. 

后端线程检查缓冲区, 若为空则阻塞一段时间, 直到超时或被前端唤醒(利用条件变量). 后端会与前端交换缓冲区来获取前端内容, 避免大量拷贝操作.

通过`append(const char* data, int len)`将数据写入日志文件. 由于默认Logger将日志内容输出到stdout, 如果要使用异步日志, 需要重新设置`Logger::g_output`重定向输出. 写入操作使用互斥量以保证线程安全. 如果前端缓冲区已满, 则唤醒阻塞的后端线程.