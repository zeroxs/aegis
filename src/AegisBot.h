//
// Bot.h
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

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/bind/placeholders.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>

#include "ABCache.h"

#include "../lib/json/src/json.hpp"

#include "Guild.h"
#include "Member.h"
#include "Channel.h"
#include "RateLimits.h"

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

class Guild;

#ifdef _DEBUG
#define DEBUG_OUTPUT
#define _TRACE
#endif

class AegisBot
{
public:
    static AegisBot & CreateInstance() { _instance = new AegisBot(); return *_instance; }
    static AegisBot & GetSingleton() { return *_instance; }
    static void DestroyInstance() { delete _instance; }
    static boost::shared_ptr<Guild> CreateGuild(uint64_t id);
private:
    AegisBot();
    static AegisBot * _instance;
public:
    ~AegisBot();

    void setup_cache(ABCache * in);
    void loadConfigs();

    void keepalive(const boost::system::error_code& error, const uint64_t ms);
    void onMessage(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg);
    void onConnect(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void userMessage(json & obj);
    bool initialize(uint64_t shardid, uint64_t maxshard);
    void processReady(json & d);
    void connectWS();

    bool call(string url, string * obj = nullptr, RateLimits * endpoint = nullptr, string method = "GET", string query = "");

    template <typename T, typename... _BoundArgs>
    void createTimer(uint64_t t, shared_ptr<boost::asio::steady_timer> timer, T f, _BoundArgs&&... __args);

    void addCommand(string command, ABMessageCallback callback)
    {
        defaultcmdlist[command] = ABCallbackPair(ABCallbackOptions(), callback);
    }

    void addCommand(string command, ABCallbackPair callback)
    {
        defaultcmdlist[command] = callback;
    }

    FormattingChannel * pFC;
    FormattingChannel * pFCf;
    Logger * log;
    Logger * logf;

    //Websockets
    boost::asio::io_service io_service;
    websocketpp::client<websocketpp::config::asio_tls_client> ws;
    websocketpp::config::asio_client::message_type::ptr message;
    websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection;
    boost::asio::steady_timer keepalive_timer_;

    RateLimits ratelimits;

    //Private chat tracking
    struct PrivateChat
    {
        uint64_t id;
        uint64_t last_message_id;
        std::map<uint64_t, shared_ptr<Member>> recipients;
    };
    std::map<uint64_t, shared_ptr<PrivateChat>> private_channels;
    std::map<uint64_t, shared_ptr<Channel>> channellist;
    std::map<uint64_t, shared_ptr<Member>> globalusers;

    //Guild tracking (Servers)
    std::map<uint64_t, shared_ptr<Guild>> guildlist;

    //Authorization
    uint64_t sequence = 0;
    string token;
    uint64_t shardid = 0;
    uint64_t shardidmax = 0;
    string sessionId;
    string gatewayurl;

    std::chrono::steady_clock::time_point starttime;

    string uptime()
    {
        std::stringstream ss;
        std::chrono::steady_clock::time_point timenow = std::chrono::steady_clock::now();

        uint32_t days = std::chrono::duration_cast<std::chrono::duration<int, std::ratio<5184000>>>(timenow - starttime).count();
        uint32_t hours = std::chrono::duration_cast<std::chrono::hours>(timenow - starttime).count() - days * 5184000;
        uint32_t minutes = std::chrono::duration_cast<std::chrono::minutes>(timenow - starttime).count() - hours * 3600 - days * 5184000;
        int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(timenow - starttime).count() - minutes * 60 - hours * 3600 - days * 5184000;
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

    ABCache * cache;

    //Bot specific data
    string username;
    bool rate_global = false;
    uint16_t discriminator;
    string avatar;
    uint64_t userId;
    bool mfa_enabled = false;
    bool isrunning = true;

    shared_ptr<Guild> loadGuild(json & obj);
    shared_ptr<Guild> loadGuildFromCache(json & obj);
    shared_ptr<Member> loadMember(json & obj);
    shared_ptr<Member> loadMemberFromCache(json & obj);

    void run();

    //default commands for guilds
    std::map<std::string, ABCallbackPair> defaultcmdlist = {};

    std::mutex m;

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

private:
    //internal function for sending messages
    void _sendMessage(string content, uint64_t channel);

};

