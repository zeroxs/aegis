//
// main.cpp
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

#include "AegisBot.h"
#if defined USE_REDIS
#include "ABRedisCache.h"
#elif defined USE_MEMORY
#include "ABMemoryCache.h"
#endif
#include <boost/lexical_cast.hpp>
#include <json.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/tuple/tuple.hpp>

#ifdef WIN32
#define WIN32_PAUSE system("pause");
#else
#define WIN32_PAUSE
#endif

#define _DEBUGTOKEN


std::string uptime();

int main(int argc, char * argv[])
{
    srand(static_cast<int32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()));

#ifdef WIN32
    Poco::Crypto::OpenSSLInitializer::initialize();
#endif

#if defined( _DEBUGTOKEN )
    AegisBot::tokenstr = "config:token-test";
#elif defined( SELFBOT )
    AegisBot::tokenstr = "config:token-self";
#else
    AegisBot::tokenstr = "config:token-prod";
#endif

    try
    {
        boost::asio::io_service::work work(AegisBot::io_service);

        AegisBot::setupLogging();
        
#if defined USE_REDIS
        ABRedisCache cache(AegisBot::io_service);
#ifdef WIN32
        cache.address = "192.168.184.136";
#else
        cache.address = "127.0.0.1";
#endif
        cache.port = 6379;
        cache.password = "";
#elif defined USE_MEMORY
        ABMemoryCache cache(AegisBot::io_service);
#endif

        if (!cache.initialize())
        {
            //error with cache
            std::cerr << "Unable to initialize cache" << std::endl;
            WIN32_PAUSE
            return -1;
        }
        AegisBot::setupCache(&cache);
        //cache.put("config:token", "Mjg4MDYzMTYzNzI5OTY5MTUy.DC61AQ.s8gzSsMJbMl0eeh3gX3EDtu-6Sw");

        //create our Bot object and cache and configure the basic settings
        AegisBot::startShards();


        //Grab shard0 for setting up single/few server bot
        auto & bot = AegisBot::getShard(0);

        //add unique commands to a specific guild. no other guilds can access these
        Guild & myguild = bot.getGuild(287048029524066334LL);
        Guild & dbots = bot.getGuild(110373943822540800LL);
        Guild & dapi = bot.getGuild(81384788765712384LL);
        



        auto SetGame = [&bot](ABMessage & message)
        {
            std::string gamestr = message.content.substr(message.channel().guild().prefix.size() + 8);
            AegisBot::io_service.post([game = std::move(gamestr), &bot = message.channel().guild().bot]()
            {
                json obj;
                obj["op"] = 3;
                obj["d"]["idle_since"] = nullptr;

                obj["d"]["game"] = { { "name", game } };

                bot.wssend(obj.dump());
            });
        };

        auto Events = [&bot](ABMessage & message)
        {
            uint64_t eventsseen = 0;
            std::stringstream ss;

            ss << "```Total: {0}";

            for (auto & evt : AegisBot::eventCount)
            {
                ss << " [" << evt.first << "]:" << evt.second;
                eventsseen += evt.second;
            }

            ss << "```";

            message.channel().sendMessage(fmt::format(ss.str(), eventsseen));
        };

        auto Info = [&bot](ABMessage & message)
        {
            uint64_t guild_count = AegisBot::guildlist.size();
            uint64_t member_count = AegisBot::memberlist.size();
            uint64_t member_count_unique = 0;
            uint64_t member_online_count = 0;
            uint64_t member_dnd_count = 0;
            uint64_t channel_count = AegisBot::channellist.size();
            uint64_t channel_text_count = 0;
            uint64_t channel_voice_count = 0;
            uint64_t member_count_active = 0;

            uint64_t eventsseen = 0;

            {
                std::lock_guard<std::recursive_mutex> lock(AegisBot::m);

                for (auto & bot : AegisBot::shards)
                    eventsseen += bot->sequence;

                for (auto & member : AegisBot::memberlist)
                {
                    if (member.second->status == MEMBER_STATUS::ONLINE)
                        member_online_count++;
                    else if (member.second->status == MEMBER_STATUS::DND)
                        member_dnd_count++;
                }

                for (auto & channel : AegisBot::channellist)
                {
                    if (channel.second->type == ChannelType::TEXT)
                        channel_text_count++;
                    else
                        channel_voice_count++;
                }

                for (auto & guild : AegisBot::guildlist)
                    member_count_active += guild.second->memberlist.size();

                member_count = message.bot.memberlist.size();
            }

            std::string members = fmt::format("{0} seen\n{1} total\n{2} unique\n{3} online\n{4} dnd", member_count, member_count_active, member_count_unique, member_online_count, member_dnd_count);
            std::string channels = fmt::format("{0} total\n{1} text\n{2} voice", channel_count, channel_text_count, channel_voice_count);
            std::string guilds = fmt::format("{0}", guild_count);
            std::string events = fmt::format("{0}", eventsseen);
            std::string misc = fmt::format("I am shard {0} of {1}", message.channel().guild().bot.shardid + 1, message.channel().guild().bot.shardidmax);

            fmt::MemoryWriter w;
            w << "[Latest bot source](https://github.com/zeroxs/aegis)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: "
                << double(AegisBot::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
                << double(AegisBot::getPeakRSS()) / (1024 * 1024) << "MB";
            std::string stats = w.str();

            message.content = "";
            json t = {
                { "title", "AegisBot" },
                { "description", stats },
                { "color", rand() % 0xFFFFFF },//10599460 },
                { "fields",
                json::array(
            {
                { { "name", "Members" },{ "value", members },{ "inline", true } },
                { { "name", "Channels" },{ "value", channels },{ "inline", true } },
                { { "name", "Uptime" },{ "value", uptime() },{ "inline", true } },
                { { "name", "Guilds" },{ "value", guilds },{ "inline", true } },
                { { "name", "Events Seen" },{ "value", events },{ "inline", true } },
                { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
                { { "name", "misc" },{ "value", misc },{ "inline", false } }
            }
                    )
                },
                { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", "Made in c++ running aegis library" } } }
            };
            message.channel().sendMessageEmbed(json(), t);
        };

//         myguild.addCommand("reset", [&bot](ABMessage & message)
//         {
//             message.channel().guild().bot.ws.close(message.channel().guild().bot.hdl, 1002, "");
//         });
        



        myguild.addCommand("custom_command", [&bot](ABMessage & message)
        {
            message.channel().sendMessage("unique command for this guild.", [](ABMessage & message)
            {
                //continuation from whatever that is
            });
        });

        myguild.addCommand("echo", [&bot](ABMessage & message)
        {
            message.channel().sendMessage(message.content.substr(message.cmd.size() + message.channel().guild().prefix.size()));
        });

        myguild.addCommand("events", Events);
        dbots.addCommand("events", Events);
        dapi.addCommand("events", Events);

        myguild.addCommand("info", Info);
        dbots.addCommand("info", Info);
        dapi.addCommand("info", Info);

        myguild.addCommand("clearchat", [&bot](ABMessage & message)
        {
            message.channel().getMessages(message.message_id, [](ABMessage & message)
            {
                try
                {
                    int64_t epoch = ((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - (14 * 24 * 60 * 60 * 1000)) - 1420070400000) << 22;
                    std::vector<std::string> delmessages;
                    for (auto & m : message.obj)
                    {
                        if (std::stoull(m["id"].get<std::string>()) > epoch)
                        {
                            delmessages.push_back(m["id"].get<std::string>());
                        }
                    }
                    message.channel().bulkDelete(delmessages);
                }
                catch (std::exception & e)
                {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            });
        });

        myguild.addCommand("disc", [&bot](ABMessage & message)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, message.content, boost::is_any_of(" "));

            fmt::MemoryWriter w;
            int i = 0;

            for (auto & m : AegisBot::memberlist)
            {
                if (m.second->discriminator == std::stoi(tokens[1]))
                {
                    ++i;
                    w << m.second->getFullName() << "\n";
                }
            }
            if (i > 0)
                message.channel().sendMessage(w.str());
            else
                message.channel().sendMessage("None");
        });

        myguild.addCommand("serverlist", [&bot](ABMessage & message)
        {
            fmt::MemoryWriter w;
            for (auto & g : message.channel().guild().bot.guildlist)
            {
                w << "*" << g.second->name << "*  :  " << g.second->id << "\n";
            }


            json t = {
                { "title", "Server List" },
                { "description", w.str() },
                { "color", 10599460 }
            };
            message.channel().sendMessageEmbed(json(), t);
        });

        myguild.addCommand("leaveserver", [&bot](ABMessage & message)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, message.content, boost::is_any_of(" "));

            uint64_t guildid = std::stoull(tokens[1]);

            if (tokens.size() >= 2)
            {
                try
                {
                    std::string guildname = message.bot.getGuild(guildid).name;
                    message.channel().sendMessage(fmt::format("Leaving **{1}** [{0}]", tokens[1], message.bot.getGuild(guildid).name));
                    message.bot.getGuild(guildid).leave(std::bind([](ABMessage & message, std::string id, std::string guildname, Channel * channel)
                    {
                        channel->sendMessage(fmt::format("Successfully left **{1}** [{0}]", id, guildname));
                    }, std::placeholders::_1, tokens[1], guildname, &message.channel()));
                }
                catch (std::domain_error & e)
                {
                    message.channel().sendMessage(std::string(e.what()));
                }
            }
            else
            {
                message.channel().sendMessage("Invalid guild id.");
            }
        });

        myguild.addCommand("serverinfo", [&bot](ABMessage & message)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, message.content, boost::is_any_of(" "));

            if (tokens.size() >= 2)
            {
                uint64_t guildid = std::stoull(tokens[1]);
              
                Guild & guild = message.bot.getGuild(guildid);
                std::string title;

                title = fmt::format("Server: **{0}**", guild.name);

                uint64_t botcount = 0;
                for (auto & m : guild.memberlist)
                    if (m.second.first->isbot)
                        botcount++;
                //TODO: may break until full member chunk is obtained on connect

                //TODO:2 this should be getMember
                Member & owner = guild.bot.getMember(guild.owner_id);
                fmt::MemoryWriter w;
                w << "Owner name: " << owner.getFullName() << " [" << owner.getName(guild.id).value_or("") << "]\n"
                    << "Owner id: " << guild.owner_id << "\n"
                    << "Member count: " << guild.memberlist.size() << "\n"
                    << "Channel count: " << guild.channellist.size() << "\n"
                    << "Bot count: " << botcount << "\n"
                    << "";


                json t = {
                    { "title", title },
                    { "description", w.str() },
                    { "color", 10599460 }
                };
                message.channel().sendMessageEmbed(json(), t);
            }
            else
            {
                message.channel().sendMessage("Invalid guild id.");
            }
        });

        myguild.addCommand("setgame", SetGame);
        dbots.addCommand("setgame", SetGame);

        dbots.activeChannels.push_back(132632676225122304LL);
        dbots.activeChannels.push_back(113743192305827841LL);
        dapi.activeChannels.push_back(81402706320699392LL);

        myguild.activeChannels.push_back(288707540844412928LL);


        AegisBot::threads();
        AegisBot::workthread.join();
    }
    catch (std::exception & e)
    {
        std::cout << e.what() << std::endl;
        WIN32_PAUSE
        return -1;
    }
    return 0;
}


std::string uptime()
{
    std::stringstream ss;
    std::chrono::steady_clock::time_point timenow = std::chrono::steady_clock::now();

    int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(timenow - AegisBot::starttime).count();

    uint32_t seconds = (ms / 1000) % 60;
    uint32_t minutes = (((ms / 1000) - seconds) / 60) % 60;
    uint32_t hours = (((((ms / 1000) - seconds) / 60) - minutes) / 60) % 24;
    uint32_t days = (((((((ms / 1000) - seconds) / 60) - minutes) / 60) - hours) / 24);

    if (days > 0)
        ss << days << "d ";
    if (hours > 0)
        ss << hours << "h ";
    if (minutes > 0)
        ss << minutes << "m ";
    if (seconds > 0)
        ss << seconds << "s";
    return ss.str();
}

