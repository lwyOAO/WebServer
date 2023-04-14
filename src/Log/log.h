#ifndef LWY_LOGGER_H
#define LWY_LOGGER_H

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
#include <map>
#include <mutex>
#include <sys/time.h>
#include "yaml-cpp/yaml.h"

/**
 * 流式输出的实现思路：重载<<运算符，使之记录消息的时间戳，然后因为使用是通过宏定义
*/
namespace Lwy
{
    // 日志级别
    class LogLevel
    {
    public:
        typedef std::shared_ptr<LogLevel> ptr;
        enum Level
        {
            // UNKNOW级别
            UNKNOW = 0,
            // DEBUG级别
            DEBUG = 1,
            // INFO级别
            INFO = 2,
            // WARN级别
            WARN = 3,
            // ERROR级别
            ERROR = 4,
            // FATAL级别
            FATAL = 5
        };

        static std::string ToString(Level level);
        static Level FromString(const std::string &level);
    };

    //日志信息
    class LogMsg {
    public:
        typedef std::shared_ptr<LogMsg> ptr;
        LogMsg(const std::string& msg, LogLevel::Level& level, std::string&, int&);
        time_t& getRawTime() { return m_rawTime;}
        struct tm* getPtime() {return m_ptime;}
        LogLevel::Level& getLevel() { return m_level;}
        std::string& getMsg() { return m_msg;}
        struct timeval& getUtime() { return m_utime;}
        std::string getFileName() { return m_fileName;}
        int getLine() { return m_line;}
    private:
        time_t m_rawTime; // 系统当前日历时间 UTC 1970-01-01 00：00：00开始的unix时间戳
        struct tm *m_ptime; // 本地时间，从1970年起始的时间戳转换为1900年起始的时间数据结构
        LogLevel::Level m_level; //日志级别
        std::string m_msg;  //日志信息
        struct timeval m_utime;
        std::string m_fileName;
        int m_line;
    };

    /***
     * 日志格式
     * 日期 %d  本地时间 %X  年份 %Y  月份 %M  日 %D  时 %h  分 %m  秒 %s  毫秒 %q
     * 行号 %L  错误级别 %p  换行 %n  累计毫秒数 %k  文件名 %F  事件 %e  日志器名称 %c
     * 线程id %t  协程id %T
    */
    class Layout
    {
    public:
        typedef std::vector<std::pair<void (*)(std::ostream &, const std::string &, int), int>> funcs;
        typedef std::shared_ptr<Layout> ptr;
        Layout(const std::string &pattern);
        Layout(const std::string&& pattern);
        void init(); // 解析pattern生成样式
        std::vector<std::pair<char, int>>& getLayout() { return m_layout;}

        private : std::string m_pattern;
        std::vector<std::pair<char, int>> m_layout;
    };

    class Logger;
    // 日志输出器
    class Appender
    {
    protected:
        std::string m_name;
        Layout::ptr m_layout;
        Logger* m_logger;
        std::mutex mtx;

    public:
        typedef std::shared_ptr<Appender> ptr;
        virtual ~Appender() {}

        virtual void output(const LogMsg::ptr& msg, std::ostream&);
        void setName(const std::string &);
        void setLayout(const Layout::ptr&);
        void setLogger(Logger* logger) { m_logger = logger;}
    };

    // 终端日志输出器
    class ConsoleAppender : public Appender
    {
    public:
        typedef std::shared_ptr<ConsoleAppender> ptr;
        void output(const LogMsg::ptr &msg, std::ostream& os = std::cout) override { Appender::output(msg, std::cout); }
    };

    // 文件日志输出器
    class FileAppender : public Appender
    {
    private:
        std::string m_file;
        std::ofstream m_os;

    public:
        typedef std::shared_ptr<FileAppender> ptr;
        FileAppender(const std::string &);
        void output(const LogMsg::ptr &msg, std::ostream& os = std::cout) override { Appender::output(msg, m_os); }
    };

    //配置器
    class Configurer {
    public:
        typedef std::shared_ptr<Configurer>  ptr;
        struct appenders{
            std::string type;
            std::string name;
            std::string file;
        };
        struct config {
            std::string name;
            LogLevel::Level level;
            std::vector<struct appenders> m_appender;
            std::string layout_name;
            std::string pattern;
        };
        Configurer(){}
        Configurer(const std::string&);
        void init();
        std::vector<struct config> getConfig() const { return m_configs;}
    private :
        std::vector<struct config> m_configs;
        std::string m_path;
        YAML::Node m_node;
    };

    // 日志器
    class Logger
    {
    public:
        typedef std::shared_ptr<Logger> ptr;

        static Logger::ptr instance;
        static Logger::ptr& getInstance(){
            return instance;
        }

       	Logger(const std::string &name, LogLevel::Level);
        Logger(const std::string &name, LogLevel::Level level, Appender::ptr appender, const std::string &lay);
        Logger(const Logger&);

        static ptr getInstance(const std::string &);
        std::string getName();
        Appender::ptr& getAppender() {
            return m_appenders[0];
        }
        std::stringstream& getOs() {
            return m_os;
        }

        void setAppenders(Appender::ptr);
        void setLevel(const std::string &);
        void setLayout(const Layout::ptr&);
        LogLevel::Level getLevel() {
            return m_level;
        }

    private:
        std::stringstream m_os;
        std::string m_name;
        LogLevel::Level m_level;
        Layout::ptr m_layout;
        std::vector<Appender::ptr> m_appenders;
        std::mutex m_mutex;  //互斥量导致复制构造函数被删除
    };

    class LogManager {
    public:
        typedef std::shared_ptr<LogManager> ptr;
        LogManager(const std::string&, const Configurer::ptr&);

        Logger::ptr FindLogger(const std::string&);
    private:
        std::vector<Logger::ptr> m_logger;
        std::string m_name;
        Configurer::ptr m_config;
    };

    class Temp {
    public:
        typedef std::shared_ptr<Temp> ptr;
        Temp(const Logger::ptr& logger, const LogLevel::Level level,const std::string& fileName, int line)
            : m_logger(logger), m_level(level),m_fileName(fileName), m_line(line)
        {
        }

        ~Temp();
        std::stringstream& getOs() {
            return m_logger->getOs();
        }
    private:
        Logger::ptr m_logger;
        LogLevel::Level m_level;
        std::string m_fileName;
        int m_line;
    };

    /**
     * @brief 使用流式方式将日志级别level的日志写入到logger
     * @details 构造一个临时对象，包裹包含日志器和日志事件，在对象析构时调用日志器写日志事件
     */
#define LOG_LEVEL(logger, level)     \
    if (level >= logger->getLevel()) \
    Lwy::Temp::ptr(new Lwy::Temp(logger, level, __FILE__, __LINE__))->getOs()

#define LOG_FATAL(logger) LOG_LEVEL(logger, Lwy::LogLevel::FATAL)

#define LOG_ERROR(logger) LOG_LEVEL(logger, Lwy::LogLevel::ERROR)

#define LOG_WARN(logger) LOG_LEVEL(logger, Lwy::LogLevel::WARN)

#define LOG_INFO(logger) LOG_LEVEL(logger, Lwy::LogLevel::INFO)

#define LOG_DEBUG(logger) LOG_LEVEL(logger, Lwy::LogLevel::UNKNOW)

#define INS() Lwy::Logger::getInstance()

}

#endif