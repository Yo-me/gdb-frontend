#include "Logger.hpp"

#include <iostream>

Logger::Logger(unsigned int level)
    :m_logLevel(level)
{}

void Logger::setLogLevel(unsigned int level)
{
    m_logLevel = level;
}

void Logger::logError(const std::string &message)
{
    this->log(LOG_LEVEL_ERROR, message);
}

void Logger::logInfo(const std::string &message)
{
    this->log(LOG_LEVEL_INFO, message);
}

void Logger::logWarning(const std::string &message)
{
    this->log(LOG_LEVEL_WARNING, message);
}

void Logger::log(Logger::T_LogLevel level, const std::string &message)
{
    if(m_logLevel & level)
    {
        switch(level)
        {
            case LOG_LEVEL_ERROR:
                std::cout << "[ERROR] : ";
                break;
            case LOG_LEVEL_INFO:
                std::cout << "[INFO] : ";
                break;
            case LOG_LEVEL_WARNING:
                std::cout << "[WARNING] : ";
                break;
        }

        std::cout << message << std::endl;
    }
}