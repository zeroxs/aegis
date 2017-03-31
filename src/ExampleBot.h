//
// ExampleBot.h
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

#include "AegisModule.h"

class ABMessage;
class AegisBot;


class ExampleBot : public AegisModule
{
public:
    ExampleBot(AegisBot & bot, Guild & guild);
    ~ExampleBot();

    virtual AegisModule * CreateModule(AegisBot & bot, Guild & guild)
    {
        return reinterpret_cast<AegisModule*>(new ExampleBot(bot, guild));
    }
    virtual void DestroyModule(AegisModule * mem)
    {
        delete reinterpret_cast<ExampleBot*>(mem);
    }

    void initialize();
    void remove();

    void echo(ABMessage & message);
    void this_is_a_class_function(ABMessage & message);


};

