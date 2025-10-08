#ifndef VIX_UUID_HPP
#define VIX_UUID_HPP

#include <string>
#include <array>
#include <random>
#include <cstdint>
#include <chrono>

namespace Vix::utils
{
    // RNG thread-local : seedé une seule fois par thread
    inline std::mt19937_64 &uuid_rng() noexcept
    {
        thread_local std::mt19937_64 rng([]
                                         {
            // mélange rd + horloge pour des environnements où random_device est pauvre
            std::random_device rd;
            auto mix = (static_cast<std::uint64_t>(rd()) << 32)
                       ^ static_cast<std::uint64_t>(
                           std::chrono::high_resolution_clock::now().time_since_epoch().count());
            return std::mt19937_64{mix}; }());
        return rng;
    }

    inline std::string uuid4() noexcept
    {
        // 16 octets aléatoires
        std::array<std::uint8_t, 16> b{};
        auto &rng = uuid_rng();
        std::uniform_int_distribution<std::uint32_t> dist32(0, 0xFFFFFFFFu);

        // remplir par paquets de 4 octets
        for (int i = 0; i < 16; i += 4)
        {
            std::uint32_t r = dist32(rng);
            b[i + 0] = static_cast<std::uint8_t>((r >> 24) & 0xFF);
            b[i + 1] = static_cast<std::uint8_t>((r >> 16) & 0xFF);
            b[i + 2] = static_cast<std::uint8_t>((r >> 8) & 0xFF);
            b[i + 3] = static_cast<std::uint8_t>((r >> 0) & 0xFF);
        }

        // RFC 4122 — version & variant
        b[6] = static_cast<std::uint8_t>((b[6] & 0x0F) | 0x40); // version 4
        b[8] = static_cast<std::uint8_t>((b[8] & 0x3F) | 0x80); // variant 10xxxxxx

        // format "8-4-4-4-12" en hex lowercase
        static constexpr char hexdig[] = "0123456789abcdef";
        std::array<char, 36> out{};
        int oi = 0;
        for (int i = 0; i < 16; ++i)
        {
            // positions des tirets : 8, 13, 18, 23
            if (oi == 8 || oi == 13 || oi == 18 || oi == 23)
                out[oi++] = '-';

            out[oi++] = hexdig[(b[i] >> 4) & 0x0F];
            out[oi++] = hexdig[(b[i] >> 0) & 0x0F];
        }

        return std::string(out.data(), out.size());
    }
}

#endif // VIX_UUID_HPP
