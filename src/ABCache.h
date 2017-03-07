//
// ABCache.h
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

#pragma once

#include <string>
#include <iostream>


using std::string;


class ABCache
{
public:
    ABCache() {};
    virtual ~ABCache() {};

    //get key entry - set no prefix to false for entries like global configs that do not
    //need to be attached to a specific shard
    virtual string get(string key, bool useprefix = true) = 0;
    virtual bool put(string key, string value, bool useprefix = true) = 0;
    //may not have a portable function in other database solutions
    //for Redis, -1 is infinite, 0 is delete now, >= 1 is seconds until expiry
    //databases that do not support entry expiration could have a timer
    //created that deletes it, though that is subject to 'leaks' in case of
    //application termination before it finishes
    virtual void expire(string key, int64_t value = 0, bool useprefix = true) = 0;

    string address;
    uint16_t port;
    string password;

    //prefix for all keys
    string prefix;
};

