//
// Channel.cpp
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

#include "Channel.h"
#include "Guild.h"
#include "AegisBot.h"
#include "Member.h"
#include "../lib/fmt/fmt/ostream.h"

ABMessage::ABMessage(Channel * channel)
    : bot(channel->guild().bot)
    , _channel(channel)
    , _member(channel->guild().bot.self)
    , _guild(nullptr)
{

}
ABMessage::ABMessage(Channel * channel, Member * member)
    : bot(channel->guild().bot)
    , _channel(channel)
    , _member(member)
    , _guild(nullptr)
{

}
ABMessage::ABMessage(Guild * guild)
    : bot(guild->bot)
    , _channel(nullptr)
    , _member(nullptr)
    , _guild(guild)
{

}

void Channel::getMessages(uint64_t messageid, ABMessageCallback callback)
{
    if (!canReadHistory())
    {
        if (guild().silentperms)
            return;
        else
            throw no_permission("READ_HISTORY");
    }

    ABMessage message(this);
    message.endpoint = fmt::format("/channels/{0}/messages", id);
    message.method = "GET";
    message.query = fmt::format("?before={0}&limit=100", messageid);
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Channel::sendMessage(string content, ABMessageCallback callback)
{
    //ignore permissions in hopes the bot has it as it uses sendMessage() to report other
    //permission errors and failures if enabled
//     if (!canSendMessages())
//         return;

    json obj;
    if (guild().preventbotparse)
        obj["content"] = "\u200B" + content;
    else
        obj["content"] = content;

    ABMessage message(this);
    message.content = obj.dump();
    message.endpoint = fmt::format("/channels/{0}/messages", id);
    message.method = "POST";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Channel::sendMessageEmbed(json content, json embed, ABMessageCallback callback /*= ABMessageCallback()*/)
{
     if (!canSendMessages())
         return;

    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;

    ABMessage message(this);
    message.content = obj.dump();
    message.endpoint = fmt::format("/channels/{0}/messages", id);
    message.method = "POST";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Channel::bulkDelete(std::vector<string> messages, ABMessageCallback callback)
{
    if (!canManageMessages())
    {
        if (guild().silentperms)
            return;
        else
            throw no_permission("MANAGE_MESSAGES");
    }

    json arr(messages);
    json obj;
    obj["messages"] = arr;

    ABMessage message(this);
    message.content = obj.dump();
    message.endpoint = fmt::format("/channels/{0}/messages/bulk-delete", id);
    message.method = "POST";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Channel::deleteChannel(ABMessageCallback callback)
{
    if (!canManageGuild())
    {
        if (guild().silentperms)
            return;
        else
            throw no_permission("MANAGE_SERVER");
    }

    ABMessage message(this);
    message.endpoint = fmt::format("/channels/{0}", id);
    message.method = "DELETE";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

/*
if (!canEmbed())
{
    if (guild().silentperms)
        return;
    else
        throw no_permission("EMBED_LINKS");
}*/
