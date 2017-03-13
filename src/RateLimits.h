//
// RateLimits.h
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

#include <stdint.h>
#include <queue>
#include <tuple>
#include <string>
#include <functional>
#include <chrono>
#include <mutex>
#include <iostream>
#include <memory>

class ABMessage;

using std::shared_ptr;

class RateLimits
{
public:
    RateLimits() {};
    ~RateLimits() {};


    //If you have rate remaining, return 0. If you have 0 remaining, returns time until reset to wait
    uint32_t shouldWait() const
    {
        if (_rate_remaining == 0) return _rate_reset;
        if (_retry_after > 0) return _retry_after;
    }

    void rateRemaining(uint32_t rate) { _rate_remaining = rate; }
    uint32_t rateRemaining()
    {
        uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (_rate_reset < epoch)
        {
            _rate_reset = 0;
            _rate_remaining = _rate_limit;
        }
        return _rate_remaining;
    }
    void rateReset(uint32_t rate) { _rate_reset = rate; }
    uint32_t rateReset() { return _rate_reset; }
    void rateLimit(uint32_t rate) { _rate_limit = rate; }
    uint32_t rateLimit() { return _rate_limit; }
    void rateRetry(uint32_t rate) { _retry_after = rate; }
    uint32_t rateRetry() { return _retry_after; }
    void addFailure() { failures++; _lastfailure = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
    void resetFailure() { failures = 0; _lastfailure = 0; }
    bool isFailureTime()
    {
        if (failures > 10)
        {
            uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (_lastfailure + 30 < epoch)
            {
                return true;
            }
        }
        if (failures > 3)
        {
            uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (_lastfailure + 5 < epoch)
            {
                return true;
            }
        }
        return false;
    }

    void setRates(uint32_t limit, uint32_t remaining, uint32_t reset, uint32_t retry)
    {
        _rate_limit = limit;
        _rate_remaining = remaining;
        _rate_reset = reset;
        _retry_after = retry;
    }

    shared_ptr<ABMessage> getMessage()
    {
        std::lock_guard<std::mutex> lock(m);
        if (outqueue.size() > 0)
        {
            shared_ptr<ABMessage> t = outqueue.front();
            outqueue.pop();
            return t;
        }
        return nullptr;
    }

    void putMessage(shared_ptr<ABMessage> message)
    {
        std::lock_guard<std::mutex> lock(m);
        outqueue.push(message);
    }

    std::queue<shared_ptr<ABMessage>> outqueue;

    static bool rate_global;

    uint16_t failures = 0;
private:
    uint32_t _rate_limit = 10;
    uint32_t _rate_remaining = 10;
    uint32_t _rate_reset = 0;
    uint32_t _retry_after = 0;
    std::mutex m;
    uint16_t _lastfailure = 0;
};

