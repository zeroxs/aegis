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

#include "Bot.h"
#include "ABRedisCache.h"
#include <boost/lexical_cast.hpp>

int main(int argc, char * argv[])
{
    uint64_t shardid = 0;

    if (argc == 1)
    {
        //error
        std::cout << "No shard id min/max passed (pass 0 for single instance)" << std::endl;
        return -1;
    }

    shardid = boost::lexical_cast<uint64_t>(argv[1]);

    if (shardid != 0)
    {
        if (argc <= 2)
        {
            //error
            std::cout << "No shard id max passed." << std::endl;
            return -1;
        }
    }

    uint64_t maxshard;
    maxshard = boost::lexical_cast<uint64_t>(argv[2]);

    if (maxshard <= shardid)
    {
        //error
        std::cout << "Shard id must be less than the max." << std::endl;
        return -1;
    }

    try
    {
        Bot bot(shardid, maxshard);

        ABRedisCache cache(bot.io_service);
        cache.address = "localhost";
        cache.port = 6379;
        cache.password = "";
        bot.setup_cache(&cache);

        bot.initialize();

        boost::asio::io_service::work work(bot.io_service);

        boost::asio::signal_set signals(bot.io_service, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code &error, int signal_number)
        {
            if (!error)
            {
                std::cerr << (signal_number==SIGINT?"SIGINT":"SIGTERM") << "received. Shutting down.\n";
                bot.io_service.stop();
            }
        });

        std::vector<std::thread> threadPool;
        for (size_t t = 0; t < std::thread::hardware_concurrency() * 2; t++)
        {
            threadPool.push_back(std::thread([&bot]() { bot.io_service.run(); }));
        }
        for (std::thread& t : threadPool)
        {
            t.join();
        }
    }
    catch (std::exception & e)
    {
        return -1;
    }
    return 0;
}
