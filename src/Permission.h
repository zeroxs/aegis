//
// Permission.h
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
#include <exception>

struct Override
{
    uint64_t id;
    enum ORType
    {
        USER,
        ROLE
    };
    ORType type;
    uint64_t allow;
    uint64_t deny;
};

class Permission
{
public:
    inline void updatePerms(uint64_t p) { _permissions = p; }
    inline uint64_t getPerms()          { return _permissions; }
    inline bool canInvite()             { return _permissions & 0x1; }
    inline bool canKick()               { return _permissions & 0x2; }
    inline bool canBan()                { return _permissions & 0x4; }
    inline bool isAdmin()               { return _permissions & 0x8; }
    inline bool canManageChannels()     { return _permissions & 0x10; }
    inline bool canManageGuild()        { return _permissions & 0x20; }
    inline bool canAddReactions()       { return _permissions & 0x40; }
    inline bool canViewAuditLogs()      { return _permissions & 0x80; }
    inline bool canReadMessages()       { return _permissions & 0x400; }
    inline bool canSendMessages()       { return _permissions & 0x800; }
    inline bool canTTS()                { return _permissions & 0x1000; }
    inline bool canManageMessages()     { return _permissions & 0x2000; }
    inline bool canEmbed()              { return _permissions & 0x4000; }
    inline bool canAttachFiles()        { return _permissions & 0x8000; }
    inline bool canReadHistory()        { return _permissions & 0x10000; }
    inline bool canMentionEveryone()    { return _permissions & 0x20000; }
    inline bool canExternalEmoiji()     { return _permissions & 0x40000; }
    inline bool canChangeName()         { return _permissions & 0x4000000; }
    inline bool canManageNames()        { return _permissions & 0x8000000; }
    inline bool canManageRoles()        { return _permissions & 0x10000000; }
    inline bool canManageWebhooks()     { return _permissions & 0x20000000; }
    inline bool canManageEmojis()       { return _permissions & 0x40000000; }

    inline bool canVoiceConnect()       { return _permissions & 0x100000; }
    inline bool canVoiceMute()          { return _permissions & 0x400000; }
    inline bool canVoiceSpeak()         { return _permissions & 0x200000; }
    inline bool canVoiceDeafen()        { return _permissions & 0x800000; }
    inline bool canVoiceMove()          { return _permissions & 0x1000000; }
    inline bool canVoiceActivity()      { return _permissions & 0x2000000; }

private:
    uint64_t _permissions = 0;
};

class no_permission : public std::exception
{
public:
    no_permission(const char * e)
        : _data(e)
    {
    }

    virtual const char* what() const throw()
    {
        return _data;
    }

private:
    const char * _data;
};
