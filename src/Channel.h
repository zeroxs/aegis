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
#include <string>
#include "RateLimits.h"
#include <boost/optional.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

class ABMessage;
class Guild;
class Channel;
class Member;
class AegisBot;
using std::string;
struct ABCallbackOptions
{
    bool enabled = true;// false;
    uint16_t level = 0;//1;
};

//TODO:
typedef std::function<void(ABMessage&)> ABMessageCallback;
typedef std::pair<ABCallbackOptions, std::function<void(ABMessage&)>> ABCallbackPair;


enum class ChannelType
{
    TEXT = 0,
    VOICE = 2
};

class Channel : public Permission
{
public:
    Channel(Guild * guild) : _guild(guild) {}
    ~Channel() {};

    Guild & guild() { return *_guild; }
 
    void getMessages(uint64_t messageid, ABMessageCallback callback = ABMessageCallback());
    void sendMessage(string content, ABMessageCallback callback = ABMessageCallback());
    void sendMessageEmbed(json content, json embed, ABMessageCallback callback = ABMessageCallback());
    void bulkDelete(std::vector<string> messages, ABMessageCallback callback = ABMessageCallback());
    void deleteChannel(ABMessageCallback callback = ABMessageCallback());


    uint64_t id = 0;
    uint64_t last_message_id = 0;
    string name;
    string topic;
    uint32_t position = 0;
    ChannelType type = ChannelType::TEXT;// 0 = text, 2 = voice

    uint32_t bitrate = 0;
    uint32_t user_limit = 0;

    RateLimits ratelimits;

    Permission overrides;

    //TODO: track all the overrides. not just our own. possible use-case?
    //std::pair<Role, Permission> overrides

private:
    Guild * _guild;
};


class ABMessage
{
public:
    ABMessage(Channel * channel);
    ABMessage(Channel * channel, Member * member);
    ABMessage(Guild * guild);
    uint64_t message_id = 0;
    Guild & guild() { return *_guild; }
    Channel & channel() { return *_channel; }
    Member & member() { return *_member; }
    string content;
    string cmd;
    string method;
    string endpoint;
    string query;
    ABMessageCallback callback;
    json obj;
    AegisBot & bot;

private:
    Channel * _channel;
    Member * _member;
    Guild * _guild;
};