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

#include <Poco/Logger.h>
#include <Poco/PatternFormatter.h>
#include <Poco/FormattingChannel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/Message.h>
#include <Poco/File.h>

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

#include "ABCache.h"

#include "../lib/json/src/json.hpp"

#include "Guild.h"
#include "Member.h"
#include "Channel.h"
#include "RateLimits.h"



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

using Poco::PatternFormatter;
using Poco::FormattingChannel;
using Poco::ConsoleChannel;
using Poco::FileChannel;
using Poco::Message;
using Poco::Logger;
using Poco::File;

using std::string;
using std::shared_ptr;
using std::make_shared;

class Guild;

#ifdef _DEBUG
#define DEBUG_OUTPUT
#define _TRACE
#endif

class AegisBot
{
public:
    shared_ptr<Guild> CreateGuild(uint64_t id);
    static std::pair<bool, string> call(string url, string obj = "", RateLimits * endpoint = nullptr, string method = "GET", string query = "");

    static FormattingChannel * pFC;
    static FormattingChannel * pFCf;
    static Logger * log;
    static Logger * logf;

    static void setupCache(ABCache * in);
    static void loadConfigs();
    static void startShards();
    static void threads();
    static string gatewayurl;
    static std::vector<shared_ptr<AegisBot>> bots;
    static bool isrunning;
    static bool active;
    //default commands for guilds
    static std::recursive_mutex m;
    static std::vector<std::thread> threadPool;
    static std::thread workthread;
    static string token;
    static std::chrono::steady_clock::time_point starttime;
    static ABCache * cache;
    static string username;
    static bool rate_global;
    static uint16_t discriminator;
    static string avatar;
    static uint64_t userId;
    static bool mfa_enabled;
    static string mention;
    //Private chat tracking
    struct PrivateChat
    {
        uint64_t id;
        uint64_t last_message_id;
        std::map<uint64_t, shared_ptr<Member>> recipients;
    };
    static std::map<uint64_t, shared_ptr<PrivateChat>> private_channels;
    static std::map<uint64_t, shared_ptr<Channel>> channellist;
    static std::map<uint64_t, shared_ptr<Member>> globalusers;

    //Guild tracking (Servers)
    static std::map<uint64_t, shared_ptr<Guild>> guildlist;

    static std::vector<shared_ptr<AegisBot>> shards;
    static boost::asio::io_service io_service;

    static uint16_t shardidmax;
    uint16_t shardid;

    AegisBot();
    ~AegisBot();

    void keepalive(const boost::system::error_code& error, const uint64_t ms);
    void onMessage(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg);
    void onConnect(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void userMessage(json & obj);
    bool initialize(uint64_t shardid);
    void processReady(json & d);
    void connectWS();

    template <typename T, typename... _BoundArgs>
    void createTimer(uint64_t t, shared_ptr<boost::asio::steady_timer> timer, T f, _BoundArgs&&... __args);

    //Websockets
    websocketpp::client<websocketpp::config::asio_tls_client> ws;
    websocketpp::config::asio_client::message_type::ptr message;
    websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection;
    boost::asio::steady_timer keepalive_timer_;
    websocketpp::connection_hdl hdl;

    uint32_t _rate_limit = 120;
    uint32_t _rate_remaining = 60;
    uint32_t _rate_reset = 0;

    std::recursive_mutex wsq;
    void wssend(string obj);

    RateLimits ratelimits;

    //Authorization
    uint64_t sequence = 0;
    string sessionId;

    shared_ptr<Guild> loadGuild(json & obj);
    shared_ptr<Guild> loadGuildFromCache(json & obj);
    shared_ptr<Member> loadMember(json & obj);
    shared_ptr<Member> loadMemberFromCache(json & obj);

    void run();

    /*TODO:
    * Add sharding support. Have bot start a new process per shard and enable
    * * synchronizing between them to overcome the DM issue where only shard0
    * * receives DMs. This could be done by AMQP or Redis pub/sub.
    * Data dump to Redis of all cached data to allow for a restart for code
    * * changes while still making use of the gateway resume function?
    * Code eval. Compile a source with a template that could return the result
    * * of a function call as a string. This would just be to show off and
    * * mimic the ability of some languages to JIT eval. Could be extended
    * * to allow deferring execution of some commands to an 'updated' version
    * * without having to restart the bot.
    * Add zlib support
    */










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
};

