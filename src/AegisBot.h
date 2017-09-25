//
// AegisBot.h
// aegisbot
//
// Copyright (c) 2017 Zero (zero at xandium dot net)
//
// This file is part of aegisbot.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <cstdio>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <functional>
#include <memory>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/URI.h>
#include <Poco/StreamCopier.h>
#include <Poco/Dynamic/Var.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/bind/placeholders.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread/future.hpp>

#include <boost/log/core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/trivial.hpp>


#include "ABCache.h"

#include "../lib/json/src/json.hpp"

#include "Guild.h"
#include "Member.h"
#include "Channel.h"
#include "Role.h"
#include "RateLimits.h"
#include "../lib/fmt/fmt/ostream.h"
#include "exceptions.h"


#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

using json = nlohmann::json;
using namespace Poco::Net;
using Poco::Dynamic::Var;
using Poco::URI;

class Guild;
class AegisModule;

#ifdef _DEBUG
#define DEBUG_OUTPUT
#define _TRACE
#endif

enum severity_level
{
    trace,
    debug,
    normal,
    warning,
    error,
    critical
};

class AegisBot
{
public:
    Guild & getGuild(uint64_t id);
    static Member & getMember(uint64_t id);
    static Channel & getChannel(uint64_t id);
    static Guild & addGuild(uint64_t id);

    static void setupLogging(severity_level logfile, severity_level logconsole);

    static boost::optional<std::string> call(std::string url, std::string obj = "", RateLimits * endpoint = nullptr, std::string method = "GET", std::string query = "");

    static void setupCache(ABCache * in);
    static void startShards();
    static AegisBot & getShard(uint16_t shard) { return *shards[shard]; };
    static void threads();
    static void cleanup();
    static void configure(bool overwritecache = false);
    static std::string gatewayurl;
    static bool isrunning;
    static bool active;
    //default commands for guilds
    static std::recursive_mutex m;
    static std::vector<std::thread> threadPool;
    static std::thread workthread;
    static std::string token;
    static std::chrono::steady_clock::time_point starttime;
    static ABCache * cache;
    static std::string username;
    static bool rate_global;
    static uint16_t discriminator;
    static std::string avatar;
    static uint64_t userId;
    static bool mfa_enabled;
    static std::string mention;
    static std::map<uint64_t, std::function<void(json &)>> ws_callbacks;
    static std::map<std::string, ABMessageCallback> cmdlist;
    static std::map<std::string, ABMessageCallback> defaultcmdlist;
    static std::map<std::string, ABMessageCallback> admincmdlist;
    static uint64_t ownerid;
    static uint64_t master_server;
    static uint64_t master_channel;
    //static std::map<string, <>> baseModules;

    static void AddCallback(std::string name, std::function<void(json &)> fn);

    //stats
    static uint64_t eventsSeen;
    static uint64_t apiCalls;

    //Private chat tracking
    struct PrivateChat
    {
        uint64_t id;
        uint64_t last_message_id;
        std::vector<uint64_t> recipients;
        RateLimits ratelimits;
    };
    static std::map<uint64_t, PrivateChat> private_channels;
    static std::map<uint64_t, Channel*> channellist;
    static std::map<uint64_t, Member*> memberlist;

    //Guild tracking (Servers)
    static std::map<uint64_t, Guild*> guildlist;

    static std::vector<AegisBot*> shards;
    static boost::asio::io_service io_service;

    static std::map<std::string, uint64_t> eventCount;

    struct
    {
        uint32_t guilds;
        uint32_t members;
        uint32_t channels;
        uint32_t messages;
    } counters = {0,0,0,0};
    uint32_t connectguilds = 0;
    uint32_t shardloaded = 0;
    static std::map<int, bool> shardready;
    static bool botready;

    Member * self = nullptr;

    static uint16_t shardidmax;
    uint16_t shardid;

    AegisBot();
    ~AegisBot();

    bool initialize(uint64_t shardid);

    uint32_t _rate_limit = 120;
    uint32_t _rate_remaining = 60;
    uint32_t _rate_reset = 0;

    std::recursive_mutex wsq;
    void wssend(std::string obj);

    RateLimits ratelimits;

    //Authorization
    uint64_t sequence = 0;
    std::string sessionId;


    void run();
    void log(std::string message, severity_level level = severity_level::normal);
    uint64_t convertDateToInt64(std::string timestamp);


    //////////////////////////////////////////////////////////////////////////
    /*
    * Author:  David Robert Nadeau
    * Site:    http://NadeauSoftware.com/
    * License: Creative Commons Attribution 3.0 Unported License
    *          http://creativecommons.org/licenses/by/3.0/deed.en_US
    */
    /**
    * Returns the peak (maximum so far) resident set size (physical
    * memory use) measured in bytes, or zero if the value cannot be
    * determined on this OS.
    */
    static size_t getPeakRSS()
    {
#if defined(_WIN32)
        /* Windows -------------------------------------------------- */
        PROCESS_MEMORY_COUNTERS info;
        GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
        return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
        /* AIX and Solaris ------------------------------------------ */
        struct psinfo psinfo;
        int fd = -1;
        if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
            return (size_t)0L;		/* Can't open? */
        if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo))
        {
            close(fd);
            return (size_t)0L;		/* Can't read? */
        }
        close(fd);
        return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
        /* BSD, Linux, and OSX -------------------------------------- */
        struct rusage rusage;
        getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
        return (size_t)rusage.ru_maxrss;
#else
        return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
        /* Unknown OS ----------------------------------------------- */
        return (size_t)0L;			/* Unsupported. */
#endif
    }

    /**
    * Returns the current resident set size (physical memory use) measured
    * in bytes, or zero if the value cannot be determined on this OS.
    */
    static size_t getCurrentRSS()
    {
#if defined(_WIN32)
        /* Windows -------------------------------------------------- */
        PROCESS_MEMORY_COUNTERS info;
        GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
        return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
        /* OSX ------------------------------------------------------ */
        struct mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
            (task_info_t)&info, &infoCount) != KERN_SUCCESS)
            return (size_t)0L;		/* Can't access? */
        return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
        /* Linux ---------------------------------------------------- */
        long rss = 0L;
        FILE* fp = NULL;
        if ((fp = fopen("/proc/self/statm", "r")) == NULL)
            return (size_t)0L;		/* Can't open? */
        if (fscanf(fp, "%*s%ld", &rss) != 1)
        {
            fclose(fp);
            return (size_t)0L;		/* Can't read? */
        }
        fclose(fp);
        return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
        /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
        return (size_t)0L;			/* Unsupported. */
#endif
    }
    //////////////////////////////////////////////////////////////////////////

    static std::string uptime()
    {
        std::stringstream ss;
        std::chrono::steady_clock::time_point timenow = std::chrono::steady_clock::now();

        int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(timenow - AegisBot::starttime).count();

        uint32_t seconds = (ms / 1000) % 60;
        uint32_t minutes = (((ms / 1000) - seconds) / 60) % 60;
        uint32_t hours = (((((ms / 1000) - seconds) / 60) - minutes) / 60) % 24;
        uint32_t days = (((((((ms / 1000) - seconds) / 60) - minutes) / 60) - hours) / 24);

        if (days > 0)
            ss << days << "d ";
        if (hours > 0)
            ss << hours << "h ";
        if (minutes > 0)
            ss << minutes << "m ";
        if (seconds > 0)
            ss << seconds << "s";
        return ss.str();
    }



    friend Guild;
protected:
    websocketpp::client<websocketpp::config::asio_tls_client> ws;
    websocketpp::connection_hdl hdl;

private:
    void onMessage(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg);
    void onConnect(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void userMessage(json & obj);
    void processReady(json & d);
    void connectWS();
    void pruneMsgHistory(const boost::system::error_code& error);
    void purgeMsgHistory();
    void keepAlive(const boost::system::error_code& error, const uint64_t ms);
    void guildCreate(json & obj);
    void channelCreate(json & channel, uint64_t guild_id);
    void memberCreate(json & member, Guild & guild);
    void memberUpdate(json & member, Guild & guild);
    void loadRole(json & role, Guild & guild);
    void loadEmoji(json & emoji, Guild & guild);
    void loadPresence(json & presence, Guild & guild);

    void channelUpdate(json & obj);
    void channelDelete(json & obj);
    void roleCreate(json & obj);
    void roleDelete(json & obj);
    void roleUpdate(json & obj);

    boost::log::sources::severity_logger< severity_level > slg;

    //Websockets
    websocketpp::config::asio_client::message_type::ptr message;
    websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection;
    boost::asio::steady_timer ratelimit_queue;
    boost::asio::steady_timer keepalive_timer_;

    //better organize timers?
    boost::asio::steady_timer prunemessages;
    uint8_t wsfail = 0;
    uint64_t wsfailtime = 0;
};

//////////////////////////////////////////////////////////////////////////
//?info data test
#if defined(_WIN64)
#define PLATFORM_NAME "Windows x64" // Windows
#elif defined(_WIN32)
#define PLATFORM_NAME "Windows x86" // Windows
#elif defined(__CYGWIN__) && !defined(_WIN32)
#define PLATFORM_NAME "Windows (Cygwin)" // Windows (Cygwin POSIX under Microsoft Window)
#elif defined(__linux__)
#define PLATFORM_NAME "Linux" // Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
#include <sys/param.h>
#if defined(BSD)
#define PLATFORM_NAME "*BSD" // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
#endif
#elif defined(__hpux)
#define PLATFORM_NAME "HP-UX" // HP-UX
#elif defined(_AIX)
#define PLATFORM_NAME "AIX" // IBM AIX
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1
#define PLATFORM_NAME "iOS iPhone simulator" // Apple iOS
#elif TARGET_OS_IPHONE == 1
#define PLATFORM_NAME "iOS iPhone" // Apple iOS
#elif TARGET_OS_MAC == 1
#define PLATFORM_NAME "OSX" // Apple OSX
#endif
#elif defined(__sun) && defined(__SVR4)
#define PLATFORM_NAME "Solaris" // Oracle Solaris, Open Indiana
#else
#define PLATFORM_NAME "undefined"
#endif
//////////////////////////////////////////////////////////////////////////
