/* -------------------------------------------------------------------------- */
/* Copyright 2002-2019, OpenNebula Project, OpenNebula Systems                */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

#ifndef MONITOR_MESSAGE_H
#define MONITOR_MESSAGE_H

#include <unistd.h>

#include <string>
#include <iostream>
#include <sstream>

#include "EnumString.h"

void base64_decode(const std::string& in, std::string& out);

int base64_encode(const std::string& in, std::string &out);

int zlib_decompress(const std::string& in, std::string& out);

int zlib_compress(const std::string& in, std::string& out);

/**
 *  This class represents a generic message used by the Monitoring Protocol.
 *  The structure of the message is:
 *
 *  +----+-----+---------+------+
 *  | ID | ' ' | PAYLOAD | '\n' |
 *  +----+-----+---------+------+
 *
 *    ID String (non-blanks) identifying the message type
 *    ' ' A single white space to separate ID and payload
 *    PAYLOAD of the message XML base64 encoded
 *    '\n' End of message delimiter
 *
 */
template<typename E>
class Message
{
public:
    /**
     *  Parse the Message from an input string
     *    @param input string with the message
     */
    int parse_from(const std::string& input);

    /**
     *  Writes this object to the given string
     */
    int write_to(std::string& out) const;

    /**
     *  Writes this object to the given file descriptor
     */
    int write_to(int fd) const;

    /**
     *  Writes this object to the given output stream
     */
    int write_to(std::ostream& oss) const;

    /**
     *
     */
    E type() const
    {
        return _type;
    }

    void type(E t)
    {
        _type = t;
    }

    const std::string& type_str() const
    {
        return _type_str._to_str(_type);
    }

    const std::string& payload() const
    {
        return _payload;
    }

    void payload(const std::string& p)
    {
        _payload = p;
    }

private:
    /**
     *  Message fields
     */
    E _type;

    std::string _payload;

    static const EString<E> _type_str;
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* Message Template Implementation                                             */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

template<typename E>
int Message<E>::parse_from(const std::string& input)
{
    std::istringstream is(input);
    std::string buffer, payloaz;

    if (!is.good())
    {
        goto error;
    }

    is >> buffer >> std::ws;

    _type = _type_str._from_str(buffer.c_str());

    if ( !is.good() || _type == E::UNDEFINED )
    {
        goto error;
    }

    buffer.clear();

    is >> buffer >> std::ws;

    base64_decode(buffer, payloaz);

    if ( zlib_decompress(payloaz, _payload) == -1 )
    {
        goto error;
    }

    return 0;

error:
    _type    = E::UNDEFINED;
    _payload = input;
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

template<typename E>
int Message<E>::write_to(std::string& out) const
{
    out.clear();

    std::string payloaz;
    std::string payloaz64;

    if (zlib_compress(_payload, payloaz) == -1)
    {
        return -1;
    }

    if ( base64_encode(payloaz, payloaz64) == -1)
    {
        return -1;
    }

    out = _type_str._to_str(_type);
    out += ' ';
    out += payloaz64;
    out += '\n';

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

template<typename E>
int Message<E>::write_to(int fd) const
{
    std::string out;

    if ( write_to(out) == -1)
    {
        return -1;
    }

    ::write(fd, (const void *) out.c_str(), out.size());

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

template<typename E>
int Message<E>::write_to(std::ostream& oss) const
{
    std::string out;

    if ( write_to(out) == -1)
    {
        return -1;
    }

    oss << out;

    return 0;
}

#endif /*MONITOR_MESSAGE_H_*/