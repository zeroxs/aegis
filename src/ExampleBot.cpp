//
// ExampleBot.h
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

#include "ExampleBot.h"
#include "Channel.h"
#include "Member.h"
#include "Guild.h"
#include "AegisBot.h"


ExampleBot::ExampleBot(AegisBot & bot, shared_ptr<Guild> guild)
    : AegisModule(bot, guild)
{
    name = "example";
}


ExampleBot::~ExampleBot()
{
}

void ExampleBot::initialize()
{
    auto g = guild.lock();
    if (!g)
        return;
    g->addCommand("echo", std::bind(&ExampleBot::echo, this, std::placeholders::_1));
    g->addCommand("rates2", std::bind(&ExampleBot::rates2, this, std::placeholders::_1));
    g->addCommand("this_is_a_class_function", std::bind(&ExampleBot::this_is_a_class_function, this, std::placeholders::_1));
}

void ExampleBot::remove()
{
    auto g = guild.lock();
    if (!g)
        return;
    g->removeCommand("echo");
    g->removeCommand("rates2");
    g->removeCommand("this_is_a_class_function");
}

void ExampleBot::echo(shared_ptr<ABMessage> message)
{
    std::cout << "Message callback triggered on channel[" << message->channel->name << "] from [" << message->member->name << "]" << std::endl;
    message->channel->sendMessage(message->content.substr(message->cmd.size() + message->guild->prefix.size()));
}

void ExampleBot::rates2(shared_ptr<ABMessage> message)
{
    uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    message->channel->sendMessage(Poco::format("Content: %s\nLimit: %u\nRemain: %u\nReset: %u\nEpoch: %u\nDiff: %u\n0x%X", message->content, message->channel->ratelimits.rateLimit()
                                               , message->channel->ratelimits.rateRemaining(), message->channel->ratelimits.rateReset(), epoch, message->channel->ratelimits.rateReset() - epoch, &message->channel->ratelimits));
}

void ExampleBot::this_is_a_class_function(shared_ptr<ABMessage> message)
{
    message->channel->sendMessage("return from this_is_a_class_function()");
}

