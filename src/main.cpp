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

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>

using namespace boost::program_options;

int main(int argc, char * argv[])
{
    uint64_t shardid;
    options_description desc("Options");
    desc.add_options()
        (",s", value<string>()->required(), "Set your bot's shard id");

    variables_map vm;
    try
    {
        store(parse_command_line(argc, argv, desc), vm);

        if (vm.count("s"))
        {
            shardid = vm["-s"].as<uint64_t>();
        }

        notify(vm);
    }
    catch (boost::program_options::error & e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return -1;
    }


    try
    {
        Bot bot;

        bot.shardid = shardid;
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
