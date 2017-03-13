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

private:
    //internal function for sending messages
    void _sendMessage(string content, uint64_t channel);

};

