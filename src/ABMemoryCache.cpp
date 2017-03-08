//
// ABMemoryCache.cpp
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

#ifdef USE_MEMORY

#include "ABMemoryCache.h"
#include <boost/lexical_cast.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>


ABMemoryCache::ABMemoryCache(boost::asio::io_service & io_service)
    : io_service(io_service)
{
    address = "127.0.0.1";
    port = 6379;
}


ABMemoryCache::~ABMemoryCache()
{
}

bool ABMemoryCache::initialize()
{
    //nothing to do here
}

string ABMemoryCache::get(string key, bool useprefix)
{
    if (useprefix)
        return memdata[prefix + key];
    else
        return memdata[key];
}


bool ABMemoryCache::put(string key, string value, bool useprefix)
{
    if (useprefix)
        memdata[prefix + key] = value;
    else
        memdata[key] = value;
}

void ABMemoryCache::expire(string key, int64_t value, bool useprefix)
{
    if (value == 0)
    {
        auto it = memdata.find(prefix + key);
        if (it != memdata.end())
            memdata.erase(it);
        return;
    }

    std::shared_ptr<boost::asio::steady_timer> timer = std::make_shared<std::shared_ptr<boost::asio::steady_timer>::element_type>(io_service);

    timer->expires_from_now(std::chrono::milliseconds(value));
    timer->async_wait(std::bind(&ABMemoryCache::expire, this, key, 0, useprefix));
}
#endif
