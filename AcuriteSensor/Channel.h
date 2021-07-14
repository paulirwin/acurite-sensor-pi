#pragma once

enum Channel { UNKNOWN, A, B, C };

inline char channelToUpperChar(const Channel channel)
{
    switch (channel)
    {
    case A:
        return 'A';
    case B:
        return 'B';
    case C:
        return 'C';
    default:
        return '?';
    }
}

inline char channelToLowerChar(const Channel channel)
{
    switch (channel)
    {
    case A:
        return 'a';
    case B:
        return 'b';
    case C:
        return 'c';
    default:
        return '?';
    }
}