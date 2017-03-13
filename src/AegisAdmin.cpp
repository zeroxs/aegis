//
// AegisAdmin.cpp
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

#include "AegisAdmin.h"
#include "Channel.h"
#include "Member.h"
#include "Guild.h"
#include "AegisBot.h"
#include <json.hpp>
#include <chrono>
#include "AegisBot.h"


AegisAdmin::AegisAdmin(AegisBot & bot, shared_ptr<Guild> guild)
    : AegisModule(bot, guild)
{
    name = "default";
}

void AegisAdmin::initialize()
{
    auto g = guild.lock();
    if (!g)
        return;
    g->addCommand("reload", std::bind(&AegisAdmin::reload, this, std::placeholders::_1));
}

void AegisAdmin::remove()
{
    auto g = guild.lock();
    if (!g)
        return;
    g->removeCommand("reload");
}

void AegisAdmin::reload(shared_ptr<ABMessage> message)
{
    if (message->member->id)
    {
        bot.loadConfigs();
        message->channel->sendMessage("Configs reloaded.");
    }
    else
    {
        message->channel->sendMessage("Not authorized.");
    }
}
