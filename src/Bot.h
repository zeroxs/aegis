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

#ifdef USE_REDIS
#include <redisclient/redissyncclient.h>
#elif USE_SCYLLA//TODO?
#endif

#include "../lib/json/src/json.hpp"

#include "Guild.h"
#include "Member.h"
#include "Channel.h"

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

using boost::shared_ptr;
using namespace redisclient;

class Guild;

#define DEBUG_OUTPUT

enum EndpointHint
{
    GUILD,
    CHANNEL,
    MAX
};

class Bot
{
public:
    Bot();
    ~Bot();

    void keepalive(const boost::system::error_code& error, const uint64_t ms);
    void onMessage(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg);
    void onConnect(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void userMessage(json & obj);
    bool initialize();
    void processReady(json & d);

    string call(string url, string obj = "", EndpointHint endpointHint = EndpointHint::CHANNEL, string method = "GET", string query = "", shared_ptr<boost::asio::steady_timer> timer = nullptr);

    bool rateLimitCheck(uint64_t epoch, EndpointHint endpointHint = EndpointHint::CHANNEL);
    template <typename T, typename... _BoundArgs>
    void createTimer(uint64_t t, shared_ptr<boost::asio::steady_timer> timer, T f, _BoundArgs&&... __args);

    struct
    {
        //Rate limits
        uint32_t rate_limit = 10;
        uint32_t rate_remaining = 10;
        uint64_t rate_reset = 0;
        uint64_t retry_after = 0;
    } rateLimits[EndpointHint::MAX];

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
    string sessionId;
    string gatewayurl;



    //Bot specific data
    string username;
    bool rate_global = false;
    uint16_t discriminator;
    string avatar;
    uint64_t userId;
    bool mfa_enabled = false;

    //Redis configuration
    string redisaddress;
    string redispass;
    RedisSyncClient redis;

    shared_ptr<Guild> loadGuild(json & obj);
    shared_ptr<Guild> loadGuildFromCache(json & obj);
    shared_ptr<Member> loadMember(json & obj);
    shared_ptr<Member> loadMemberFromCache(json & obj);

    void sendMessageEmbed(string content, uint64_t channel, json & embed);
    void sendMessage(string content, uint64_t channel, shared_ptr<boost::asio::steady_timer> timer = nullptr);
    void bulkDelete(uint64_t channel, std::vector<string> messages);
    void getMessages(uint64_t channel, uint64_t messageid);

    //debug
    std::vector<string> tempmessages;

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

};
