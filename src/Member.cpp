//
// Member.cpp
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

#include "Member.h"
#include "Guild.h"
#include "AegisBot.h"



Member::Member(AegisBot & bot, uint64_t id, string name, uint16_t discriminator, string avatar)
    : bot(bot)
    , id(id)
    , name(name)
    , discriminator(discriminator)
    , avatar(avatar)
{
}


Member::~Member()
{
}

std::vector<boost::shared_ptr<Guild>> Member::guilds()
{
    //TODO: Performance test this some time
    //should we keep a cache local to each user of what guilds we can see them on?
    //or just check the lists for results
    std::vector<boost::shared_ptr<Guild>> result;
    for (auto & guild : bot.guildlist)
    {
        if (guild.second->clientlist.count(id) > 0)
        {
            result.push_back(guild.second);
        }
    }
    return result;
}
