//
// Guild.h
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

#include "AegisBot.h"
#include <map>
#include "Member.h"
#include <boost/shared_ptr.hpp>
#include "Permission.h"
#include "Channel.h"
#include "Role.h"
#include "../lib/json/src/json.hpp"
#include "RateLimits.h"
#include <boost/enable_shared_from_this.hpp>

using boost::shared_ptr;
class AegisBot;

using std::string;
using json = nlohmann::json;

class Guild : public Permission, public boost::enable_shared_from_this<Guild>
{
public:
    Guild(AegisBot & b);
    ~Guild();

    void processMessage(json obj);
    void addCommand(string command, ABMessageCallback callback);

    AegisBot & bot;

    std::map<uint64_t, shared_ptr<Member>> clientlist;
    std::map<uint64_t, shared_ptr<Channel>> channellist;
    std::map<uint64_t, shared_ptr<Role>> rolelist;

    RateLimits ratelimits;

    string prefix = "?";

    uint64_t id = 0;
    string name;
    string icon;
    string splash;
    uint64_t owner_id = 0;
    string region;
    uint64_t afk_channel_id = 0;
    uint32_t afk_timeout = 0;//in seconds
    bool embed_enabled = false;
    uint64_t embed_channel_id = 0;
    uint32_t verification_level = 0;
    uint32_t default_message_notifications = 0;
    uint32_t mfa_level = 0;
    string joined_at;
    bool large = false;
    bool unavailable = true;
    uint32_t member_count = 0;
    //string voice_states;//this is really an array

    //extendable command list. this list allows you to place c++ functions into matching to a command
    //for more than just simple responses
    std::map<std::string, std::function<void(boost::shared_ptr<ABMessage>)>> cmdlist = {};
};

