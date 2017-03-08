//
// Channel.h
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

#include "Permission.h"
#include <boost/shared_ptr.hpp>
#include <string>
#include "RateLimits.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>

class ABMessage;
class Guild;
class Channel;
class Member;
class AegisBot;
using boost::shared_ptr;
using std::string;
struct ABMessage;

typedef std::function<void(boost::shared_ptr<ABMessage>)> ABMessageCallback;

struct ABMessage
{
    string content;
    string cmd;
    boost::shared_ptr<Guild> guild = nullptr;
    boost::shared_ptr<Channel> channel = nullptr;
    boost::shared_ptr<Member> member = nullptr;
    string method;
    string endpoint;
    string query;
    ABMessageCallback callback;
};

enum class ChannelType
{
    TEXT = 0,
    VOICE = 2
};

class Channel : public Permission, public boost::enable_shared_from_this<Channel>
{
public:
    Channel(AegisBot & bot, shared_ptr<Guild> guild)
        : bot(bot)
        , guild(guild)
    {

    };
    ~Channel() {};

    AegisBot & bot;

    void belongs_to(shared_ptr<Guild> g) { guild = g; }
    shared_ptr<Guild> belongs_to() { return guild; }

    void getMessages(uint64_t messageid, ABMessageCallback callback = ABMessageCallback());
    void sendMessage(string content, ABMessageCallback callback = ABMessageCallback());
    void bulkDelete(std::vector<string> messages, ABMessageCallback callback = ABMessageCallback());


    uint64_t id = 0;
    uint64_t last_message_id = 0;
    string name;
    string topic;
    uint32_t position = 0;
    ChannelType type = ChannelType::TEXT;// 0 = text, 2 = voice

    uint32_t bitrate = 0;
    uint32_t user_limit = 0;

    RateLimits ratelimits;

private:
    shared_ptr<Guild> guild;
};

