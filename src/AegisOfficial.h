//
// AegisOfficial.h
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
#include <stdint.h>
#include <vector>
#include <boost/asio/steady_timer.hpp>
#include "AegisModule.h"

class ABMessage;
class Channel;
class AegisBot;

using std::string;
using std::shared_ptr;

class AegisOfficial : public AegisModule
{
public:
    AegisOfficial(AegisBot & bot, shared_ptr<Guild> guild);
    ~AegisOfficial() {};

    void initialize();
    void remove();

    string uptime();

    void createVoice(shared_ptr<ABMessage> message);
    string getparams(shared_ptr<ABMessage> message);
    void moveAfterCreate(shared_ptr<ABMessage> message, uint64_t member_id);
    void info(shared_ptr<ABMessage> message);
    void clearChat(shared_ptr<ABMessage> message);
    void clean(shared_ptr<ABMessage> message);

};

