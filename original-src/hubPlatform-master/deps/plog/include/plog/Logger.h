#pragma once
#include <plog/Appenders/IAppender.h>
#include <plog/Util.h>
#include <vector>

#ifndef PLOG_DEFAULT_INSTANCE
#   define PLOG_DEFAULT_INSTANCE 0
#endif

namespace plog
{
    template<int instance>
    class Logger : public util::Singleton<Logger<instance> >, public IAppender
    {
    public:
        Logger(Severity maxSeverity = none) : m_maxSeverity(maxSeverity)
        {
        }

        Logger& addAppender(IAppender* appender)
        {
            assert(appender != this);
            m_appenders.push_back(appender);
            return *this;
        }

        void removeAppender(IAppender* appender)
        {
            assert(appender != this);

            m_appenders.erase(std::remove(m_appenders.begin(), m_appenders.end(), appender), m_appenders.end());
        }

        Severity getMaxSeverity() const
        {
            return m_maxSeverity;
        }

        void setMaxSeverity(Severity severity)
        {
            m_maxSeverity = severity;
        }

        bool checkSeverity(Severity severity) const
        {
            return severity <= m_maxSeverity;
        }

        virtual void write(const Record& record)
        {
            if (checkSeverity(record.getSeverity()))
            {
                *this += record;
            }
        }

        void operator+=(const Record& record)
        {
            for (std::vector<IAppender*>::iterator it = m_appenders.begin(); it != m_appenders.end(); ++it)
            {
                (*it)->write(record);
            }
        }

    private:
        Severity m_maxSeverity;
        std::vector<IAppender*> m_appenders;
    };

    template<int instance>
    inline Logger<instance>* get()
    {
        return Logger<instance>::getInstance();
    }

    inline Logger<PLOG_DEFAULT_INSTANCE>* get()
    {
        return Logger<PLOG_DEFAULT_INSTANCE>::getInstance();
    }
}
