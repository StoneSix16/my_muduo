# my_muduo

一个参考muduo实现的网络编程库, 去除boost依赖, 并利用c++11标准库替代部分实现.

## 目录结构

- `example/` 编写的用例
- `src/` 源文件
    - `base/` 基本组件, 如线程, 线程池, 时间戳等
    - `logger/` 异步日志
    - `event/` 事件处理与循环线程
    - `net/` 网络连接实现

## 构建

```bash
cd
cmake .
make install 
# .h保存在include下, .so保存在lib下, 按需求使用.
```

## TodoList

- 编写build.sh
- 完善文档
- 补充测试样例, 添加压力测试
- 增加对UDP连接的支持
- 优化定时器队列
- 实现http
- 实现客户端