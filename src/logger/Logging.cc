#include "logger/Logging.h"
#include "base/CurrentThread.h"


__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* Utils::strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}



Logger::LogLevel initLogLevel()
{
    if (::getenv("MUDUO_LOG_TRACE"))
        return Logger::TRACE;
    else if (::getenv("MUDUO_LOG_DEBUG"))
        return Logger::DEBUG;
    else
        return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

void defaultOutput(const char* msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
    //FIXME check n
    (void)n;
}

void defaultFlush()
{
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput; 
Logger::FlushFunc g_flush = defaultFlush; 

Logger::Logger(SourceFile file, int line):
    impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level):
    impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char *func):
    impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, bool toAbort):
    impl_(toAbort?FATAL:ERROR, errno, file, line)
{
}

Logger::~Logger()
{
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());
  g_output(buf.data(), buf.length());
  if (impl_.level_ == FATAL)
  {
    g_flush();
    abort();
  }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}

Logger::Impl::Impl(LogLevel level, int old_errno, const SourceFile &file, int line):
    time_(Timestamp::now()),
    level_(level),
    stream_(),
    line_(line),
    basename_(file)
{
    formatTime();
    stream_ << Fmt("%10d", CurrentThread::tid());
    stream_ << Fmt("%6s", LogLevelName[level]);
    if(old_errno != 0)
    {
        stream_ << Utils::strerror_tl(old_errno) << " (errno=" << old_errno << ") ";
    }
}

void Logger::Impl::formatTime()
{
    std::string formatted = time_.toFormattedString();
    snprintf(t_time, sizeof(t_time), "%s", formatted.c_str());
    t_lastSecond = time_.secondsSinceEpoch();
}

void Logger::Impl::finish()
{
    stream_ << " - " << MonoFmt(basename_.data_, basename_.size_) << ':' << line_ << '\n';
}
