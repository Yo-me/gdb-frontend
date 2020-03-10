#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <string>

class Logger
{
    public:
        typedef enum
        {
            LOG_LEVEL_ERROR = 1,
            LOG_LEVEL_WARNING = 2,
            LOG_LEVEL_INFO = 4
        } T_LogLevel;

        Logger(unsigned int level = LOG_LEVEL_ERROR);

        void logError(const std::string &message);
        void logWarning(const std::string &message);
        void logInfo(const std::string &message);
        void setLogLevel(unsigned int level);

    private:
        unsigned int m_logLevel;
        void log(Logger::T_LogLevel level, const std::string &message);
};
#endif