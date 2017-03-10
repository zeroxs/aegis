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
#include "ABRedisCache.h"
#include <boost/lexical_cast.hpp>
#include <json.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include "rss.h"

uint64_t maxshard = 0;
uint64_t shardid = 0;

//Example usage
class Bot
{
public:
    Bot() {}
    ~Bot() {}

    void echo(boost::shared_ptr<ABMessage> message)
    {
        std::cout << "Message callback triggered on channel[" << message->channel->name << "] from [" << message->member->name << "]" << std::endl;
        message->channel->sendMessage(message->content.substr(message->cmd.size() + message->guild->prefix.size()));
    }

    void rates2(boost::shared_ptr<ABMessage> message)
    {
        uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        message->channel->sendMessage(Poco::format("Content: %s\nLimit: %u\nRemain: %u\nReset: %u\nEpoch: %u\nDiff: %u\n0x%X", message->content, message->channel->ratelimits.rateLimit()
        , message->channel->ratelimits.rateRemaining(), message->channel->ratelimits.rateReset(), epoch, message->channel->ratelimits.rateReset() - epoch, &message->channel->ratelimits));
    }

    void callback(boost::shared_ptr<ABMessage> message)
    {
        //if (message)
            //sendMessage("data", );
    }

    void this_is_a_class_function(boost::shared_ptr<ABMessage> message)
    {
        message->channel->sendMessage("return from this_is_a_class_function()");
    }


};
bool parsecli(int argc, char * argv[]);

void this_is_a_global_function(boost::shared_ptr<ABMessage> message)
{
    message->channel->sendMessage("return from this_is_a_global_function()");
}

int main(int argc, char * argv[])
{
    if (!parsecli(argc, argv))
        return -1;

    try
    {
        //create our Bot object and cache and configure the basic settings
        AegisBot & bot = AegisBot::CreateInstance();

#ifdef USE_REDIS
        ABRedisCache cache(bot.io_service);
        cache.address = "127.0.0.1";
        cache.port = 6379;
        cache.password = "";
#endif
        if (!cache.initialize())
        {
            //error with cache
            return -1;
        }
        bot.setup_cache(&cache);

        bot.loadConfigs();

        //this is temporary
#ifdef USE_MEMORY
        string token = "yourtokenhere";
#endif


        //register some callbacks

        //we can register a callback to a lambda function, or our own class, or even global space functions
        //note, these are global functions and will exist in all guilds the bot is connected to.

        //create our class that contains some functions (optional)
        Bot ourfunctionclass;

#define MAKE_CALLBACK(a) ABCallbackPair(ABCallbackOptions(), a)
        bot.defaultcmdlist["this_is_a_global_command"] = ABCallbackPair(ABCallbackOptions(), std::bind(&this_is_a_global_function, std::placeholders::_1));
        bot.defaultcmdlist["this_is_a_class_command"] = ABCallbackPair(ABCallbackOptions(), std::bind(&Bot::this_is_a_class_function, &ourfunctionclass, std::placeholders::_1));
        bot.defaultcmdlist["this_is_a_lambda_command"] = ABCallbackPair(ABCallbackOptions(), [](shared_ptr<ABMessage> message)
        {
            message->channel->sendMessage("return from this_is_a_lambda_function()");
        });

        //set up a single guild with the bot (official use will have this be dynamic and loaded from a DB)
        auto myguild = AegisBot::CreateGuild(287048029524066334LL);

        //this allows the addition of extra commands specific to a single guild and not accessible by other guilds
        myguild->prefix = "?";
        bot.defaultcmdlist["echo"] = ABCallbackPair(ABCallbackOptions(), std::bind(&Bot::echo, &ourfunctionclass, std::placeholders::_1));


        //example usage of callbacks within a callback
        //this deletes the last 100 messages in the channel this is called from
        bot.addCommand("delete_history", [&bot](shared_ptr<ABMessage> message)
        {
            message->channel->getMessages(message->message_id, [&bot](shared_ptr<ABMessage> message)
            {
                json arr = json::parse(message->content);

                std::vector<string> tempmessages;

                for (auto & m : arr)
                {
                    string entry = m["id"];
                    poco_trace_f1(*(bot.log), "Message entry: %s", entry);
                    tempmessages.push_back(entry);
                }

                message->channel->bulkDelete(tempmessages);
            });
        });


        bot.addCommand("timer", [&bot](shared_ptr<ABMessage> message)
        {
            uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

            std::vector<string> tokens;
            boost::split(tokens, message->content, boost::is_any_of(" "));

            if (tokens.size() < 3)
            {
                //not enough tokens
                message->channel->sendMessage(Poco::format("Error parsing. Format is %s%s time-in-seconds message", message->guild->prefix, message->cmd));
                return;
            }

            int64_t time = std::stoll(tokens[1]);
            if (time > 1000 * 60 * 60)//1 hour max timer
            {
                //fix this mess, have Channel class have a sendMessage function instead of guild
                message->channel->sendMessage(Poco::format("Timer too large `%Ld`", time));
                return;
            }
            message->channel->sendMessage(Poco::format("Timer started `%Ld`", time));
            //this WILL instantly fire and possibly crash since t loses scope. leaving it here just because.
            //add a timers vector to the bot that tracks users, channels, reasons, and times and have main loop
            //process it instead
            std::thread t([&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(time));
                bot.io_service.post([message, time]()
                {
                    message->channel->sendMessage(Poco::format("Response to `%s`", message->content));
                });
            });
        });
        bot.addCommand("rates", [&bot](shared_ptr<ABMessage> message)
        {
            std::cout << "Rates Command()" << std::endl;
            uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            message->channel->sendMessage(Poco::format("Content: %s\nLimit: %u\nRemain: %u\nReset: %u\nEpoch: %u\nDiff: %u", message->content, message->channel->ratelimits.rateLimit()
                , message->channel->ratelimits.rateRemaining(), message->channel->ratelimits.rateReset(), epoch, message->channel->ratelimits.rateReset() - epoch));
        });
        bot.addCommand("clean_channel", [&bot](shared_ptr<ABMessage> message)
        {
            message->channel->bulkDelete(bot.tempmessages);
        });
        bot.addCommand("get_history", [&bot](shared_ptr<ABMessage> message)
        {
            message->channel->getMessages(message->channel->id, [&bot](shared_ptr<ABMessage> message)
            {
                json arr = json::parse(message->content);

                for (auto & m : arr)
                {
                    string entry = m["id"];
                    poco_trace_f1(*(bot.log), "Message entry: %s", entry);
                    bot.tempmessages.push_back(entry);
                }
            });
        });

        bot.addCommand("reload", [&bot](shared_ptr<ABMessage> message)
        {
            bot.loadConfigs();
            message->channel->sendMessage("Configs reloaded.");
        });

        bot.addCommand("info", [&bot](shared_ptr<ABMessage> message)
        {
            uint64_t timenow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            uint64_t guild_count = bot.guildlist.size();
            uint64_t member_count = bot.globalusers.size();
            uint64_t channel_count = bot.channellist.size();
            uint64_t channel_text_count = 0;
            uint64_t channel_voice_count = 0;
            {
                std::lock_guard<std::mutex> lock(bot.m);
                for (auto & channel : bot.channellist)
                {
                    if (channel.second->type == ChannelType::TEXT)
                        channel_text_count++;
                    else
                        channel_voice_count++;
                }
            }
            std::stringstream members;
            members << member_count << "\n0 Online\n0 Offline\nstuff";

            std::stringstream channels;
            channels << channel_count << " total\n" << channel_text_count << " text\n" << channel_voice_count << " voice";

            std::stringstream guilds;
            guilds << guild_count;

            message->content = "";
            string uptime = bot.uptime();
            string stats;
            stats = Poco::format("Memory usage: %.2fMB\nMax Memory: %.2fMB", double(getCurrentRSS()) / (1024 * 1024), double(getPeakRSS()) / (1024 * 1024));
            json t = {
                {"title", "AegisBot"},
                {"description", "[Latest bot source](https://github.com/zeroxs/aegisbot)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)"},
                {"color", 10599460},
                {"fields", 
                    json::array(
                        {
                            { { "name", "Members" },{ "value", members.str() }, { "inline", true } },
                            { { "name", "Channels" },{ "value", channels.str() },{ "inline", true } },
                            { { "name", "Uptime test" },{ "value", uptime },{ "inline", true } },
                            { { "name", "Guilds" },{ "value", guilds.str() },{ "inline", true } }
                        }
                    )
                },
                {"footer", {{"icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png"},{"text", "Made in c++ running aegisbot library"}}}
            };
            t["description"] = stats;
            message->channel->sendMessageEmbed(json(), t);
        });





        //////////////////////////////////////////////////////////////////////////
        //find a better way to do this
        //ultimately, a master application would spin these bots up passing params
        //as the shard ids. alternatively, could make this bot do some calls to
        //load more bot instances at this point and have those ignore this part
        string res;
        bot.call("/gateway/bot", &res, nullptr, "GET", "");
        json ret = json::parse(res);
        bot.gatewayurl = ret["url"];

        std::cout << "Recommended shard count: " << ret["shards"] << std::endl;
        std::cout << "Current set shard count: " << maxshard << std::endl;
        std::cout << "My shard id: " << shardid << std::endl;

        bot.initialize(shardid, maxshard);
        bot.run();

        for (std::thread& t : bot.threadPool)
        {
            t.join();
        }
    }
    catch (std::exception & e)
    {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}

bool parsecli(int argc, char * argv[])
{
    if (argc == 1)
    {
        //error
        std::cout << "No shard id min/max passed (pass 0 for single instance)" << std::endl;
        return false;
    }

    shardid = std::atoll(argv[1]);

    if (shardid != 0)
    {
        if (argc <= 2)
        {
            //error
            std::cout << "No shard id max passed." << std::endl;
            return false;
        }
    }

    maxshard = std::atoll(argv[2]);

    if (maxshard <= shardid)
    {
        //error
        std::cout << "Shard id must be less than the max." << std::endl;
        return false;
    }
    return true;
}
