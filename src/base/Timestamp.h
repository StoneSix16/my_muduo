#pragma once

#include <string>
#include <sys/time.h>

class Timestamp
{
public:
    Timestamp()
        : microSecondsSinceEpoch_(0)
    {
    }

    /// 根据时间构造时间戳
    /// @param microSecondsSinceEpochArg
    Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    {
    }

    void swap(Timestamp& that)
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    // 可以执行默认的拷贝与赋值函数

    /// @brief 返回字符串格式时间戳
    /// @return [millisec].[microsec]
    std::string toString() const;

    /// @brief 返回日期格式时间戳
    /// @return "YY/MM/DD H:M:S(.microsec)"
    std::string toFormattedString(bool showMicroSeconds = true) const;

    bool valid() const {return microSecondsSinceEpoch_ > 0;}

    // 用于内部使用
    int64_t microSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}
    time_t secondsSinceEpoch() const 
    { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

    /// @brief 获取当前时间戳
    static Timestamp now();
    /// @brief 构造一个非法的时间戳
    static Timestamp invalid() {return Timestamp();}

    static const int kMicroSecondsPerSecond = 1000*1000;

private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

/// 计算时间戳的差，返回秒
/// @param high, low
/// @return （high-low）seconds
inline double timeDifference(Timestamp high, Timestamp low)
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

/// 对给定时间戳添加 @c 秒
/// @param timestamp, seconds
/// @return  timestamp+seconds Timestamp
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}