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


ExampleBot::ExampleBot(AegisBot & bot, Guild & guild)
    : AegisModule(bot, guild)
{
    name = "example";
}


ExampleBot::~ExampleBot()
{
}

void ExampleBot::initialize()
{
    guild.addCommand("echo", std::bind(&ExampleBot::echo, this, std::placeholders::_1));
    guild.addCommand("this_is_a_class_function", std::bind(&ExampleBot::this_is_a_class_function, this, std::placeholders::_1));
}

void ExampleBot::remove()
{
    guild.removeCommand("echo");
    guild.removeCommand("this_is_a_class_function");
}

void ExampleBot::echo(ABMessage & message)
{
    std::cout << "Message callback triggered on channel[" << message.channel().name << "] from [" << message.member().name << "]" << std::endl;

    message.channel().sendMessage(message.content.substr(message.cmd.size() + message.channel().guild().prefix.size()), [](ABMessage & message)
}

void ExampleBot::this_is_a_class_function(ABMessage & message)
{
    message.channel().sendMessage("return from this_is_a_class_function()");
}

