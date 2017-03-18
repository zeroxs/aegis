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

std::mutex Member::m;

Member::Member(uint64_t id, string name, uint16_t discriminator, string avatar)
    : id(id)
    , name(name)
    , discriminator(discriminator)
    , avatar(avatar)
{
}

Member::~Member()
{
}

std::vector<Guild*> Member::getGuilds()
{
    //TODO: Performance test this some time
    std::vector<Guild*> result;
    std::lock_guard<std::recursive_mutex> lock(AegisBot::m);
    for (auto & guild : AegisBot::guildlist)
    {
        if (guild.second->memberlist.count(id) > 0)
        {
            result.push_back(guild.second);
        }
    }
    return result;
}

boost::optional<string> Member::getName(uint64_t guildid)
{
    if (guilds[guildid].nickname.length() > 0)
    {
        return guilds[guildid].nickname;
    }
    return boost::none;
}

string Member::getFullName()
{
    std::stringstream fullname;
    fullname << name;
    fullname << "#";
    fullname << this->discriminator;
    return fullname.str();
}

