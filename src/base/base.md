# base

base定义了线程/线程池类, 时间戳类, 并提供Callback.h和noncopyable.h等头文件供其他模块使用.

## 线程/线程池

在线程/线程池类的设计中, 为了让线程的回调函数可以包含该线程/线程池实例的信息, 且避免传递太多参数. 一个技巧是创建一个成员函数`threadFunc(Args)`, 并利用`std::bind(&threadFunc, this, Args)`传递一个包含了相应信息的函数对象.

### 线程

Thread类是对c++11中std::thread的简单封装.

#### 成员

Thread类成员包括一个指向std::thread实例的指针`thread`, 线程的`tid`和`name`, 线程的回调函数`func`以及线程的状态`started/joined`.

Thread类会使用一个静态变量`numCreated`记录创建过的线程数量.

#### 成员函数

Thread的构造函数接受一个回调函数与一个线程名, 并提供`start()/join()`用于控制线程的执行. 同时定义了相关函数获取Thread实例的相关信息. 

Thread实例对应的std::thread实例会在`start()`而非构造函数中被创建. 因此`tid`只有在`start()`执行后有效. `start()`利用信号量机制保证线程实例创建完毕并成功获取到`tid`后返回.

### 线程池

ThreadPool类创建指定数量的线程和一个任务队列, 每个线程会在空闲时尝试获取任务.

#### 成员

ThreadPool包含一个线程指针的std::vector`threads`, 有一个可设置的Task成员`threadInitCallback`用于在线程执行回调函数前被调用. 

ThreadPool包含一个装载函数对象`Task`的std::deque`queue`, 以及其最大长度`maxQueueSize`, 和用于同步的互斥量`mutex`和条件变量`notEmpty/notFull`.

最后, ThreadPool有名称`name`和状态`running`

#### 成员函数

ThreadPool的构造函数接受一个名称. 但在启动ThreadPool实例前, 含需要通过`setMaxQueueSize()`和`setMaxQueueSize()`设置相应变量.

`mutex`保证访问时互斥, 且当队列空/满时, 相关的条件变量会使得线程被挂起, 直到队列可用.

通过`start(int numThreads)`启动实例, 实例会构造`numThreads`个线程. 每个线程的回调函数都是尝试获取Task并执行. 如果`numThreads`为0, 则threadPool的创建线程执行Task. 

通过`stop()`停止实例, 该调用会唤醒所有线程并等待join.

通过`add(Task task)`添加实例. 

### 当前线程

使用线程变量缓存每个线程的tid. 使用`__builtin_expect(long expr, long likely)`优化分支预测. 如果未缓存, 则通过系统调用`SYS_gettid`获取tid.

## 时间戳

存储一个时间戳，单位为微秒

### 支持的操作

通过`now()/invalid()`获取当前/非法时间戳

通过`toString()/toFormattedString(bool showMicroSec)`将时间戳转成字符串

通过`timeDifference(Timestamp high, Timestamp low)`计算两个时间戳相差的秒数. 通过`addTime(Timestamp time, double seconds)`获取一个增加`seconds`秒后的时间戳

## 其他头文件

### noncopyable.h

创建了一个noncopyable类, 该类禁用了拷贝函数. 通过继承noncopyable以禁用该类的拷贝功能.

### Callback.h

声明了一系列可能被多个文件使用的回调函数类


