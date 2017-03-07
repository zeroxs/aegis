//
// ABRedisCache.cpp
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

#ifdef USE_REDIS

#include "ABRedisCache.h"
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>


ABRedisCache::ABRedisCache(boost::asio::io_service & io_service)
    : redis(io_service)
{
    address = "127.0.0.1";
    port = 6379;
}


ABRedisCache::~ABRedisCache()
{
}

void ABRedisCache::initialize()
{
    boost::asio::ip::address connectaddress = boost::asio::ip::address::from_string(address);
    string errmsg;
    if (!redis.connect(connectaddress, port, errmsg))
    {
        std::cerr << "Can't connect to redis: " << errmsg << std::endl;
    }
    else
    {
        std::cerr << "Redis connected" << std::endl;
    }

    RedisValue result;
    if (password != "")
    {
        result = redis.command("AUTH", { password });
        if (result.isError())
        {
            std::cerr << "AUTH error: " << result.toString() << "\n";
            return;
        }
    }
}

string ABRedisCache::get(string key, bool useprefix)
{
    RedisValue result;
    if (useprefix)
        result = redis.command("GET", { prefix + key });
    else
        result = redis.command("GET", { key });

    if (result.isOk())
    {
        return result.toString();
    }
    else
    {
        return "";
    }
}

bool ABRedisCache::put(string key, string value, bool useprefix)
{
    RedisValue result;
    if (useprefix)
        result = redis.command("SET", { prefix + key });
    else
        result = redis.command("SET", { key });

    if (result.isOk())
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ABRedisCache::expire(string key, int64_t value, bool useprefix)
{
    RedisValue result;
    if (useprefix)
        result = redis.command("EXPIRE", { prefix + key, boost::lexical_cast<std::string>(value) });
    else
        result = redis.command("EXPIRE", { key, boost::lexical_cast<std::string>(value) });
}
#endif
