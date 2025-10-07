#ifndef VIX_UUID_HPP
#define VIX_UUID_HPP

#include <random>
#include <string>

namespace Vix::utils
{
    inline std::string uuid4()
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dist;
        auto r1 = dist(gen), r2 = dist(gen);
        unsigned char b[16];
        for (int i = 0; i < 8; ++i)
            b[i] = (r1 >> (i * 8)) & 0xFF;
        for (int i = 0; i < 8; ++i)
            b[8 + i] = (r2 >> (i * 8)) & 0xFF;

        b[6] = (b[6] & 0x0F) | 0x40; // version 4
        b[8] = (b[8] & 0x3F) | 0x80; // variant
        char out[37];
        std::snprintf(out, sizeof(out),
                      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                      b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
        return std::string(out);
    }
}

#endif