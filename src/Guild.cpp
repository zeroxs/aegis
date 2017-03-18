//
// Guild.cpp
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

#include "Guild.h"
#include "AegisBot.h"
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include "AegisAdmin.h"
#include "AegisOfficial.h"
#include "AuctionBot.h"
#include "ExampleBot.h"


Guild::Guild(AegisBot & bot, uint64_t id)
    : bot(bot)
    , id(id)
{

}

Guild::~Guild()
{
    for (auto mod = modules.begin(); mod != modules.end(); ++mod)
    {
        if ((*mod)->name == "default")
            delete static_cast<AegisOfficial*>(*mod);
        else if ((*mod)->name == "auction")
            delete static_cast<AuctionBot*>(*mod);
        else if ((*mod)->name == "example")
            delete static_cast<ExampleBot*>(*mod);
        else if ((*mod)->name == "admin")
            delete static_cast<AegisAdmin*>(*mod);
    }
}

void Guild::processMessage(json obj)
{
    json author = obj["author"];
    uint64_t userid = std::stoll(author["id"].get<string>());

    //if this is my own message, ignore
    if (userid == AegisBot::userId)
        return;

    string avatar = author["avatar"].is_string()?author["avatar"]:"";
    string username = author["username"];
    uint16_t discriminator = std::stoll(author["discriminator"].get<string>());

    uint64_t channel_id = std::stoll(obj["channel_id"].get<string>());
    uint64_t id = std::stoll(obj["id"].get<string>());
    uint64_t nonce = obj["nonce"].is_null()?0:std::stoll(obj["nonce"].get<string>());

    string content = obj["content"];
    bool tts = obj["tts"];
    bool pinned = obj["pinned"];

    //if message came from a bot, ignore
    if (!AegisBot::memberlist.count(userid) || AegisBot::memberlist[userid]->isbot == true)
        return;



#ifdef SELFBOT
    if (userid != ROOTADMIN)
        return;
#endif


    if (userid == ROOTADMIN)
    {

        if (content.substr(0, AegisBot::mention.size()) == AegisBot::mention)
        {

            boost::char_separator<char> sep{ " " };
            boost::tokenizer<boost::char_separator<char>> tok{ content, sep };

            auto token = tok.begin();

            ++token;

            if (token == tok.end()) return;

            string cmd = *(token++);

#ifdef _DEBUG
            if (cmd == "setprefix")
            {
                if (token == tok.end()) return;
                string setprefix = *(token++);
                if (setprefix.size() == 0)
                    channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
                else
                {
                    prefix = setprefix;
                    channellist[channel_id]->sendMessage(Poco::format("Prefix successfully set to `%s`", setprefix));

                    //TODO: set this in a persistent DB to maintain across restarts
                }
                return;
            }
#endif

            if (cmd == "exit")
            {
                //TODO: add some core bot management
                poco_critical_f3(*bot.log, "Bot shutdown g[%Lu] c[%Lu] u[%Lu]", id, channel_id, userid);
                channellist[channel_id]->sendMessage("Bot shutting down.");
                std::this_thread::sleep_for(std::chrono::seconds(2));
                bot.ws.close(bot.hdl, 1001, "");
                bot.io_service.stop();
                bot.isrunning = false;
                bot.active = false;
                return;
            }
        }
    }


    if ((userid == owner_id) || (userid == ROOTADMIN))//for support
    {
        //guild owner is talking, do a check if this is the prefix set up command
        //as that needs to be set up first before the bot will function
        try
        {
            if (content.substr(0, AegisBot::mention.size()) == AegisBot::mention)
            {

                boost::char_separator<char> sep{ " " };
                boost::tokenizer<boost::char_separator<char>> tok{ content, sep };

                auto token = tok.begin();

                token++;
                string cmd = *(token++);

                if (cmd == "setprefix")
                {
                    string setprefix = *(token++);
                    if (setprefix.size() == 0)
                        channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
                    else
                    {
                        prefix = setprefix;
                        channellist[channel_id]->sendMessage(Poco::format("Prefix successfully set to `%s`", setprefix));

                        //TODO: set this in a persistent DB to maintain across restarts
                    }
                }
                else if (cmd == "enablemodule")
                {
                    string modulename = *(token++);

                    if (modulename == "admin" && userid != ROOTADMIN)
                    {
                        channellist[channel_id]->sendMessage("Not authorized.");
                        return;
                    }

                    switch (addModule(modulename))
                    {
                        case -1:
                            channellist[channel_id]->sendMessage(Poco::format("Error adding module [%s] already enabled.", modulename));
                            return;
                        case 0:
                            channellist[channel_id]->sendMessage(Poco::format("Error adding module [%s] does not exist.", modulename));
                            return;
                        case 1:
                            channellist[channel_id]->sendMessage(Poco::format("[%s] successfully enabled.", modulename));
                            return;
                    }
                }
                else if (cmd == "disablemodule")
                {
                    string modulename = *(token++);
                    if (removeModule(modulename))
                    {
                        channellist[channel_id]->sendMessage(Poco::format("Module [%s] successfully disabled.", modulename));
                        return;
                    }
                    else
                    {
                        channellist[channel_id]->sendMessage(Poco::format("Error removing module [%s] not enabled.", modulename));
                        return;
                    }
                }
                else if (cmd == "commands")
                {
                    std::stringstream ss;
                    for (auto & c : cmdlist)
                    {
                        ss << " " << c.first;
                    }
                    ss << " ";
                    channellist[channel_id]->sendMessage(Poco::format("Command list: `%s`.", ss.str()));
                }
                return;
            }
        }
        catch (std::exception&e)
        {
            channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
            return;
        }
    }


    if (content.size() == 1 || content.size() - prefix.size() == 0 || content.substr(0, prefix.size()) != prefix)
        return;
    
    boost::char_separator<char> sep{ " " };
    boost::tokenizer<boost::char_separator<char>> tok{ content, sep };

    //TODO: 
    //boost::tokenizer<boost::char_separator<char>> tok{ content.substr(prefix.size(), sep };
    //vs
    //string cmd = (*(token++)).substr(prefix.size());


    if (tok.begin() == tok.end())
        return;
    auto token = tok.begin();
    string cmd = (*(token++)).substr(prefix.size());

    //check if attachment exists
    if (obj.count("attachments") > 0)
    {
        std::cout << "Attachments check true" << std::endl;
        for (auto & attach : obj["attachments"])
        {
            std::cout << "Attachment found" << obj["url"] << std::endl;
            if (attachmenthandler.second && attachmenthandler.first.enabled)
            {
                ABMessage message(channellist[channel_id], AegisBot::memberlist[userid]);
                message.message_id = id;
                message.obj = obj;
                attachmenthandler.second(message);
                return;
            }
        }
    }

    if (cmdlist.count(cmd))
    {
        //guild owners bypass all restrictions
        if (userid != owner_id)
        {
            //command exists - construct callback object and perform callback
            //but first check if it's enabled
            if (!cmdlist[cmd].first.enabled)
                return;

            //now check access levels
            //a user that does not exist in the access list has a default permission level of 0
            //commands have a default setting of 1 preventing anyone but guild owner from initially
            //running any commands until permissions are set
            if (memberlist[userid].second < cmdlist[cmd].first.level)
            {
                if (!silentperms)
                {
                    channellist[channel_id]->sendMessage("You do not have access to that command.");
                    return;
                }
            }
        }


        ABMessage message(channellist[channel_id], AegisBot::memberlist[userid]);
        message.content = content;
        message.message_id = id;
        message.cmd = cmd;
        cmdlist[cmd].second(message);
        return;
    }
}

void Guild::addCommand(string command, ABMessageCallback callback)
{
    cmdlist[command] = ABCallbackPair(ABCallbackOptions(), callback);
}

void Guild::addCommand(string command, ABCallbackPair callback)
{
    cmdlist[command] = callback;
}

void Guild::removeCommand(string command)
{
    if (cmdlist.count(command) > 0)
    {
        cmdlist.erase(command);
    }
}

//TODO: add support for multiple maybe?
void Guild::addAttachmentHandler(ABMessageCallback callback)
{
    attachmenthandler = ABCallbackPair(ABCallbackOptions(), callback);
}

void Guild::addAttachmentHandler(ABCallbackPair callback)
{
    attachmenthandler = callback;
}

void Guild::removeAttachmentHandler()
{
    attachmenthandler = ABCallbackPair();
}

void Guild::modifyMember(json content, uint64_t guildid, uint64_t memberid, ABMessageCallback callback)
{
    //if (!canSendMessages())
    //    return;
    poco_trace(*(AegisBot::log), "modifyMember() goes through");

    ABMessage message(this);
    message.content = content.dump();
    message.endpoint = Poco::format("/guilds/%Lu/members/%Lu", guildid, memberid);
    message.method = "PATCH";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Guild::createVoice(json content, uint64_t guildid, ABMessageCallback callback)
{
    //if (!canSendMessages())
    //    return;
    poco_trace(*(AegisBot::log), "createVoice() goes through");

    ABMessage message(this);
    message.content = content.dump();
    message.endpoint = Poco::format("/guilds/%Lu/channels", guildid);
    message.method = "POST";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Guild::leave(ABMessageCallback callback)
{
    ABMessage message(this);
    message.endpoint = Poco::format("/users/@me/guilds/%Lu", id);
    message.method = "DELETE";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

int Guild::addModule(string modName)
{
    //modules are hardcoded until I design a system to use SOs to load them.

    for (auto & m : modules)
    {
        if (m->name == modName)
        {
            return -1;
        }
    }


    //TODO: throw these into a list elsewhere instead to clean this up
    if (modName == "default")
    {
        AegisOfficial * mod = new AegisOfficial(bot, *this);
        modules.push_back(mod);
        mod->initialize();
        return 1;
    }
    else if (modName == "auction")
    {
        AuctionBot * mod = new AuctionBot(bot, *this);
        modules.push_back(mod);
        mod->initialize();
        return 1;
    }
    else if (modName == "example")
    {
        ExampleBot * mod = new ExampleBot(bot, *this);
        modules.push_back(mod);
        mod->initialize();
        return 1;
    }
    else if (modName == "admin")
    {
        AegisAdmin * mod = new AegisAdmin(bot, *this);
        modules.push_back(mod);
        mod->initialize();
        return 1;
    }
    return 0;
}

bool Guild::removeModule(string modName)
{
    for (auto mod = modules.begin(); mod != modules.end(); ++mod)
    {
        if ((*mod)->name == modName)
        {
            (*mod)->remove();
            modules.erase(mod);
            if (modName == "default")
                delete static_cast<AegisOfficial*>(*mod);
            else if (modName == "auction")
                delete static_cast<AuctionBot*>(*mod);
            else if (modName == "example")
                delete static_cast<ExampleBot*>(*mod);
            else if (modName == "admin")
                delete static_cast<AegisAdmin*>(*mod);
            return true;
        }
    }
    return false;
}

