#include "LogFile.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>


Utils::FileUtil::FileUtil(const std::string &fileName):
    fp_(fopen(fileName.c_str(), "ae")),
    writtenBytes_(0)
{
    assert(fp_);
    ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

Utils::FileUtil::~FileUtil()
{
    ::fclose(fp_);
}

void Utils::FileUtil::append(const char *data, size_t len)
{
    size_t written = 0;
    while(written != len)
    {
        size_t remain = len - written;
        size_t n = write(data+written, remain);
        if(n != remain)
        {
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "AppendFile::append() failed %s\n", Utils::strerror_tl(err));
                break;
            }
        }
        written += n;
    }
    writtenBytes_ += written;
}

void Utils::FileUtil::flush()
{
    ::fflush(fp_);
}

size_t Utils::FileUtil::write(const char *data, size_t len)
{
    return ::fwrite_unlocked(data, 1, len, fp_);
}


logFile::logFile(const std::string &name, off_t rollSize, int flushInterval, int checkEveryN):
    basename_(name),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN_),
    count_(0),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
    rollFile();
}

void logFile::append(const char *data, int len)
{   
    std::lock_guard<std::mutex> lock(mutex_);
    append_unclocked(data, len);
}

void logFile::flush()
{
    file_->flush();
}

bool logFile::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    // 获取今天0点时间
    time_t start = now/kRollPerSeconds_*kRollPerSeconds_;

    if(now > lastRoll_)
    {
        lastRoll_ = now;
        startOfPeriod_ = start;
        lastFlush_ = now;
        file_.reset(new Utils::FileUtil(filename));
        return true;
    }
    return false;
}

std::string logFile::getLogFileName(const std::string &basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    localtime_r(now, &tm);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
    filename += timebuf;

    filename += ".log";

    return filename;
}

void logFile::append_unclocked(const char* logline, int len)
{
    file_->append(logline, len);

    // 如果内容或添加次数超过阈值，则滚动日志或刷新日志
    // 如果超过了刷新间隔，则刷新
    if(file_->writtenBytes() > rollSize_)
    {
        rollFile();
    }
    else
    {
        ++count_;
        if(count_ >= checkEveryN_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            if(thisPeriod_ != startOfPeriod_)
            {
                rollFile();
            }
            else if(now - lastFlush_ > flushInterval_)
            {
                lastFlush_ = now;
                flush();
            }
        }
    }
}
