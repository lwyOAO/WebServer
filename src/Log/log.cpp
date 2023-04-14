#include "log.h"
#include <cctype>
#include <ctime>
#include <thread>

namespace Lwy
{

    std::string LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(C)         \
    case LogLevel::C: \
        return #C;    \
        break;
            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
        }
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }

        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);

        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL);

        return LogLevel::UNKNOW;
#undef XX
    }

    LogMsg::LogMsg(const std::string &msg, LogLevel::Level& level, std::string& fileName, int& line)
        : m_level(level), m_msg(msg), m_fileName(fileName), m_line(line) {
        /* 获取时间，理论到us */
        gettimeofday(&m_utime, NULL);
        // 获取系统当前日历时间 UTC 1970-01-01 00：00：00开始的unix时间戳
        time(&m_rawTime);
        // 将日历时间转换为本地时间，从1970年起始的时间戳转换为1900年起始的时间数据结构
        m_ptime = localtime(&m_rawTime);
    }

    void Appender::setName(const std::string &name)
    {
        if (!name.empty())
            m_name = name;
    }

    void Appender::setLayout(const Layout::ptr &layout)
    {
        m_layout = layout;
    }

    void Appender::output(const LogMsg::ptr &msg, std::ostream & m_os)
    {
        using std::cout;
        std::lock_guard<std::mutex> lck(mtx);

        for (auto i = m_layout->getLayout().begin(); i != m_layout->getLayout().end(); ++i)
        {
            switch (i->first)
            {
#define XX(c)      \
    case c:        \
        m_os << c; \
        break;

                XX('#');
                XX('%');
                XX('(');
                XX(')');
                XX('{');
                XX('}');
                XX('*');
                XX('&');
                XX('-');
                XX('+');
                XX('$');
                XX('@');
                XX(' ');
                XX(':');
                XX('[');
                XX(']');
                XX('\t');
                XX('<');
                XX('>');

            case 'd':
            {
                tm* ptime = msg->getPtime();
                m_os << ptime->tm_year + 1900 << "-" << ptime->tm_mon << "-" << ptime->tm_mday
                   << " " << ptime->tm_hour << ":" << ptime->tm_min << ":" << ptime->tm_sec;
                break;
            }
            case 'F':
                m_os << msg->getFileName();
                break;
            case 'L':
                m_os << msg->getLine();
                break;
            case 'e':
                m_os << msg->getMsg();
                break;
            case 'p':
            {
                switch(msg->getLevel()){
                    case 0:
                        m_os << "UNKNOW";
                        break;
                    case 1:
                        m_os << "DEBUG";
                        break;
                    case 2:
                        m_os << "INFO";
                        break;
                    case 3:
                        m_os << "WARN";
                        break;
                    case 4:
                        m_os << "ERROR";
                        break;
                    case 5:
                        m_os << "FATAL";
                        break;
                    default:
                        m_os << "???";
                        break;
                }
                break;
            }

            case 'c':
                m_os << m_logger->getName();
                break;
            case 't':
                m_os << std::this_thread::get_id();
                break;
            case 'n':
                m_os << std::endl;
                break;
            case 'T':
                m_os << ' ';
                break;
            case 'Y':
                m_os << msg->getPtime()->tm_year + 1900;
                break;
            case 'M':
                m_os << msg->getPtime()->tm_mon;
                break;
            case 'D':
                m_os << msg->getPtime()->tm_mday;
                break;
            case 'h':
                m_os << msg->getPtime()->tm_hour;
                break;
            case 'm':
                m_os << msg->getPtime()->tm_min;
                break;
            case 's':
                m_os << msg->getPtime()->tm_sec;
            case 'q':
                m_os << msg->getUtime().tv_usec / 1000;
            default:
                break;
            }
        }
    }

    FileAppender::FileAppender(const std::string &file_name) : m_file(file_name)
    {
        if (!m_file.empty())
        {
            m_os.open(m_file, std::ios::app);
        }
        else
        {
            m_os.open("log.txt", std::ios::app);
        }
    }

    Layout::Layout(const std::string &pattern) : m_pattern(pattern)
    {
        if (pattern.empty())
        {
            std::cout << "use default layout" << std::endl;
        }
        else
            init();
    }

    Layout::Layout(const std::string &&pattern) : m_pattern(pattern)
    {
    }

    void Layout::init()
    {
        if (m_pattern.empty())
            return;

        std::vector<std::pair<char, int>> &str = m_layout;
        std::string DateFormat = "";
        size_t i = 0, n = 0;
        std::string snum = "";
        int num = 0;
        bool leftBracket = false;
        while (i < m_pattern.size())
        {
            // 匹配 %p 类型
            if (m_pattern[i] == '%' && isalpha(m_pattern[i + 1]))
            {
                // 匹配 %d{xxxxxxx}
                if (m_pattern[i + 1] == 'd' && m_pattern[i + 2] == '{')
                {
                    leftBracket = true;
                    i += 3;
                }
                else
                {
                    str.push_back({m_pattern[i + 1], 0});
                    i += 2;
                }
            }
            // 匹配 %% 类型
            else if (m_pattern[i] == '%' && m_pattern[i + 1] == '%')
            {
                str.push_back({m_pattern[i + 1], 0});
                i += 2;
            }
            // 匹配 %-4p 类型
            else if (m_pattern[i] == '%' && !isalpha(m_pattern[i + 1]))
            {
                n = i + 1;
                while (!isalpha(m_pattern[n]))
                {
                    ++n;
                }
                snum = (std::string)m_pattern.substr(n, n - i - 1);
                if (!snum.empty())
                    num = std::stoi(snum);
                str.push_back({m_pattern[n], num});

                i = n + 1;
            }
            // 匹配 %d{} 中的 '}'
            else if (m_pattern[i] == '}')
            {
                if (leftBracket == true)
                {
                    ++i;
                    leftBracket = false;
                }
                else
                {
                    str.push_back({m_pattern[i], 0});
                    ++i;
                }
            }
            // 匹配 {}、# 等符号
            else
            {
                str.push_back({m_pattern[i], 0});
                ++i;
            }
        }
    }

    Configurer::Configurer(const std::string &path) : m_path(path)
    {
        if (!m_path.empty())
        {
            try
            {
                m_node = YAML::LoadFile(m_path);
            }
            catch (YAML::BadFile &e)
            {
                std::cout << "read file error!" << std::endl;
                exit(-1);
            }

            init();
        }
    }

    void Configurer::init()
    {
        if (!m_node.IsDefined())
        {
            std::cout << "error: logger is not define." << std::endl;
            return;
        }
        if (m_node["loggers"].IsSequence())
        {
            for (size_t i = 0; i < m_node.size(); ++i)
            {
                YAML::Node node = m_node["loggers"][i];
                struct config CFG;
                if (node["name"].IsDefined())
                {
                    CFG.name = node["name"].as<std::string>();
                }
                else
                {
                    CFG.name = "default_" + std::to_string(i);
                }
                if (node["level"].IsDefined())
                {
                    CFG.level = LogLevel::FromString(node["level"].as<std::string>());
                }
                else
                {
                    CFG.level = LogLevel::DEBUG;
                }
                if (node["appenders"].IsDefined())
                {
                    if (node["appenders"].IsSequence())
                    {
                        for (size_t i = 0; i < node["appenders"].size(); ++i)
                        {
                            YAML::Node appNode = node["appenders"][i];
                            struct appenders app;
                            if (appNode["name"].IsDefined())
                            {
                                app.name = appNode["name"].as<std::string>();
                            }
                            else
                            {
                                app.name = "defaultAppender_" + std::to_string(i);
                            }
                            if (appNode["type"].IsDefined())
                            {
                                app.type = appNode["type"].as<std::string>();
                                if (app.type == "FileAppender")
                                {
                                    if (appNode["file"].IsDefined())
                                    {
                                        app.file = appNode["file"].as<std::string>();
                                    }
                                    else
                                    {
                                        app.file = "./default_log.txt";
                                    }
                                }
                            }
                            else
                            {
                                app.type = "debug";
                            }
                            CFG.m_appender.push_back(app);
                        }
                    }
                }
                else
                {
                    struct appenders app;
                    app.name = "defaultAppender";
                    app.type = "debug";
                    CFG.m_appender.push_back(app);
                }
                if (node["layout"].IsDefined())
                {
                    if (node["layout"]["name"].IsDefined())
                    {
                        CFG.layout_name = node["layout"]["name"].as<std::string>();
                    }
                    else
                    {
                        CFG.layout_name = "defaultLayout";
                    }
                    if (node["layout"]["pattern"].IsDefined())
                    {
                        CFG.pattern = node["layout"]["pattern"].as<std::string>();
                    }
                    else
                    {
                        CFG.pattern = "%d   [%F:%L]  %t:%T  %p  %m%n";
                    }
                }
                m_configs.push_back(CFG);
            }
        }
    }

    Logger::Logger(const std::string &name, LogLevel::Level level = LogLevel::DEBUG)
        : m_name(name), m_level(level)
    {
    }

    Logger::Logger(const Logger & logger) {

    }
    void Logger::setAppenders(Appender::ptr appender)
    {
        appender->setLayout(m_layout);
        std::lock_guard<std::mutex> locker(m_mutex);
        m_appenders.push_back(appender);
        appender->setLogger(this);
    }

    Logger::Logger(const std::string &name, LogLevel::Level level, Appender::ptr appender, const std::string &lay)
        : m_name(name), m_level(level)
    {
        m_layout = std::make_shared<Layout>(lay);
        setAppenders(appender);
    }

    Logger::ptr Logger::instance = std::make_shared<Logger>("default", LogLevel::DEBUG, Appender::ptr(new FileAppender("log.txt")), "%d [%p]  %t  {%F:%L}  <%c>  %e%n");

    void Logger::setLevel(const std::string &level)
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_level = LogLevel::FromString(level);
    }

    void Logger::setLayout(const Layout::ptr &layout)
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_layout = layout;
    }

    std::string Logger::getName()
    {
        return m_name;
    }

    LogManager::LogManager(const std::string &name, const Configurer::ptr &config)
        : m_name(name), m_config(config)
    {
        std::vector<Configurer::config> con = config->getConfig();
        for (size_t i = 0; i < con.size(); ++i)
        {
            Logger::ptr logger(new Logger(con[i].name, con[i].level));
            Layout::ptr layout(new Layout(con[i].pattern));
            logger->setLayout(layout);
            for (size_t j = 0; j < con[i].m_appender.size(); ++j)
            {
                std::vector<Configurer::appenders> app = con[i].m_appender;
                Appender::ptr appender;
                if (app[j].type == "ConsoleAppender")
                {
                    appender = Appender::ptr(new ConsoleAppender());
                }
                else if (app[j].type == "FileAppender")
                {
                    appender = Appender::ptr(new FileAppender(app[j].file));
                }
                appender->setName(app[j].name);
                logger->setAppenders(appender);
            }
            m_logger.push_back(logger);
        }
    }

    Logger::ptr LogManager::FindLogger(const std::string &name)
    {
        for (auto &i : m_logger)
        {
            if (i->getName() == name)
            {
                return i;
            }
        }
        Logger::ptr temp(std::make_shared<Logger>("root"));
        return temp;
    }

    Temp::~Temp()
    {
        LogMsg::ptr msg(std::make_shared<LogMsg>(m_logger->getOs().str(), m_level, m_fileName, m_line));
        m_logger->getAppender()->output(msg, std::cout);
        m_logger->getOs().clear();
        m_logger->getOs().str("");
    }
}
