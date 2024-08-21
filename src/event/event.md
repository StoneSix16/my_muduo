# event

负责io事件的监听与处理, 事件循环线程与线程池的实现. Poller类实现io事件监听, 底层采用epoll+LT. Channel类实现事件处理, 包括设置回调函数, 设置感兴趣事件, 以及处理事件. EventLoop类实现事件循环, EventLoopThread/EventLoopThreadPool是对事件循环线程的封装.

## EventLoop

主从Reactor模型下, main loop负责接受连接并分发给sub loop. sub loop负责处理多个连接的事件, 一个loop可能负责多个的fd, 处理io事件和额外的计算任务. 一个EventLoop用Channel封装每个需要被处理的fd, 并注册到一个Poller实例上实施监听. Poller会返回触发事件的fd对应的Channel, 由loop调用Channel封装的回调函数进行处理. 此外, 一个loop实例还包含一个定时器队列以执行一些需要定时的任务.

Poller的监听会阻塞线程一段时间后返回监听结果, 如果阻塞期间有事件触发则立刻返回. 为了避免阻塞时间过长导致无法及时处理计算任务, loop会设置一个额外的wakeupFd, 通过在添加计算任务的同时触发wakeupFd的事件, 使循环从阻塞中返回并及时处理.

基于`one loop per thread`的设计模式, 所有io事件和计算任务都需要在loop线程下完成. 在建立连接后, main loop会将连接描述符的Channel封装逻辑交给sub loop执行, 以实现分发.

### 成员

```cpp
/// loop的状态
bool looping_;
std::atomic<bool> quit_; 
bool eventHandling_;
bool callingPendingFunctors_;

int64_t iteration_; /* loop循环次数 */
const pid_t threadId_; /* 创建loop的线程 */
Timestamp pollReturnTime_; /* 监听到事件的时刻 */

/// @note 需要注意声明的顺序。
/// 由于channel在析构时会检查自己是否在对应poller中，故poller需要在channel后被析构
std::unique_ptr<Poller> poller_;

/// 用于唤醒loop线程的eventfd
/// 所有的fd都要用channel封装
int wakeupFd_;
std::unique_ptr<Channel> wakeupChannel_;

std::unique_ptr<TimerQueue> timerQueue_;

/// 触发时间的channels, poll每次返回时更新
ChannelList activeChannels_;  
Channel* currentActiveChannel_;

/// 除io外的计算任务
std::vector<Functor> pendingFunctors_;
mutable std::mutex mutex_;  /* pendingFunctor的互斥锁 */
```

### 支持操作

#### 开启/退出循环

通过`loop()/stop()`开启/关闭循环. 

开启循环时, 置位`looping`, 通过检查`quit`判断是否继续循环, 循环内, 线程调用poller的`poll()`获取触发事件的channel, 遍历channel处理事件后, 执行`pendingFunctors`中的额外计算任务. 循环退出后, 会将`looping`和`quit`复位. `loop()`只能被loop线程调用

关闭循环时, 仅置位`quit`. `stop()`可被其他线程调用, `quit`可能被多个线程同时修改, 使用atomic保持原子性.

#### 唤醒线程

通过`wakeup()`唤醒线程, 可以被其他线程调用

wakeupFd是一个eventfd, 可用于简单的事件通知/处理. 通过对eventfd进行写操作触发可读事件以实现通知, 通过读操作处理通知.

`wakeup()`对wakeupFd执行写操作.

#### 更新/删除channel

通过`updateChannel(Channel*)/removeChannel(Channel*)`修改channel, 通过`hasChannel(Channel*)`判断loop是否负责对应Channel. 均可以被其他线程调用.

loop会调用poller中相应的方法, 取消/更新channel对应的fd在poller下的政策状态. 注意`removeChannel`仅仅取消注册, 不会关闭对应的fd.

#### 添加计算任务

通过`runInLoop(const Functor&)/queueInLoop(const Functor&)`添加额外的计算任务. 均可以被其他线程调用.

如果调用者是loop线程, 则`runInLoop`直接执行回调函数; 否则执行`queueInLoop`逻辑, 将回调函数对象加入`pendingFunctors`

#### 添加定时任务

通过调用`runAt/runAfter/runEvery`添加相关的定时任务, 以上调用会创建一个定时器并加入`timerQueue`中

## Poller

muduo中的Poller是一个抽象基类, 可以选择基于poll/epoll实现的派生类, 这里只实现了epoll的派生类EpollPoller. EpollPoller包含一个描述符`epollFd`, 记录已注册channel的`channelList`, 记录触发事件的`events`.

### 支持操作

#### 更新/删除channel

通过`updateChannel/removeChannel`修改channel对应fd在epoll实例中的注册状态. 应当由loop线程调用.

## Channel

Channel封装一个描述符, 相应的回调函数, 感兴趣的事件等内容. 

### 成员

```cpp
/// 事件类型
enum eventType
{
    kNoneEvent = 0,
    kReadEvent = EPOLLIN | EPOLLPRI,
    kWriteEvent = EPOLLOUT
};

EventLoop* loop_; /* fd所属的事件循环 */
const int fd_;
int events_; /* 感兴趣的事件类型 */
int revents_; /* epoll/poll接收到的事件类型 */
int index_; /* poller的注册情况 */

std::weak_ptr<void> tie_; /* 指向依赖对象（TcpConnection）的指针，必要时转化成共享指针 */
bool tied_;

/// channel状态
bool eventHandling_;
bool addedToLoop_;

// 相应事件的回调函数

ReadEventCallback readCallback_;
EventCallback writeCallback_;
EventCallback closeCallback_;
EventCallback errorCallback_;
```

### 支持操作

#### 设置与调用回调函数

通过`setRead/Write/Close/ErrorCallback`设置相应的回调函数. 通过`handleEvent`根据`revents`调用对应回调函数. 只有loop线程可以调用`handleEvent`.

#### 设置感兴趣事件

通过修改`events`的值设置fd要监听的事件类型. 修改完毕后, 会通过调用loop下的`updateChannel`将更新同步到poller中. 通过`isNoneEvent/isWriting/isReading`查看监听的事件. 

#### 移除Channel

通过调用loop下的`removeChannel`实现, 移除操作只取消fd在poller中的注册. 关闭fd的逻辑要在别处实现.

为了移除Channel, 首先要禁用所有的监听事件, 然后调用`remove`取消注册.

#### tie

通过`tie`使得依赖对象的生命周期比自身更长. 网络连接中, 为了防止sockfd因连接中断在channel处理事件时被关闭, 吊桶该方法绑定对应的connection.

tie构造依赖对象的一个weak_ptr, 在必要时, 将其提升为shared_ptr, 防止其被析构.

## EventLoopThread/ThreadPool

事件循环线程封装了事件循环的创建过程, 并在启动后返回事件循环指针. 线程池包含一个baseloop和一个loopList, 用来管理多个事件循环.

### 循环线程操作

构造完成后, 调用`start`开启循环线程. 线程会创建一个EventLoop实例并调用`loop`开始循环. 线程支持传入回调函数, 该函数会在线程开始创建loop实例前被执行. `start`保证会在loop实例创建成功后返回其地址, 或返回nullptr. 

循环停止后, 线程回将loop指针置空以防止非法访问. 考虑到loop实例可能被多个线程使用, 需要通过mutex保证互斥访问.

### 循环线程池操作

线程池需要传入一个EventLoop指针作为baseloop.

通过`start`启动线程池前, 需要设置线程池中线程数量. 线程池将创建相应数量的事件循环线程, 并获取每个线程下的事件循环指针.

指针数组不直接暴露, 只能通过`getNextLoop`轮询获取, 通过这种方式可以轮询分配连接给sub loop.

为了实现主从Reactor, 在启用所有sub loop后, 还需要启动base loop的循环. 具体的逻辑实现在TcpServer中实现.