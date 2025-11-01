#ifndef VIX_UUID_HPP
#define VIX_UUID_HPP

#include <string>
#include <array>
#include <random>
#include <cstdint>
#include <chrono>
#include <cstddef>

/**
 * @file VIX_UUID_HPP
 * @brief UUID (version 4) generation utilities — RFC 4122 compliant.
 *
 * Provides a lightweight, header-only UUID v4 generator using a thread-local
 * Mersenne Twister (`std::mt19937_64`) engine. The generator mixes hardware
 * entropy (`std::random_device`) with a high-resolution clock timestamp to
 * create unique seeds for each thread.
 *
 * The resulting UUID string follows the canonical 8-4-4-4-12 hexadecimal format:
 *
 * ```
 * 123e4567-e89b-12d3-a456-426614174000
 * ```
 *
 * ### Features
 * - Thread-safe (thread-local RNG)
 * - Deterministic within a thread, random across threads
 * - RFC 4122-compliant version (4) and variant (10xxxxxx)
 * - No dynamic allocations except string construction
 *
 * @note Uses lowercase hexadecimal letters (a–f).
 *
 * ### Example
 * @code
 * using namespace Vix::utils;
 *
 * std::string id1 = uuid4();
 * std::string id2 = uuid4();
 *
 * std::cout << "Generated UUIDs:\n" << id1 << "\n" << id2 << std::endl;
 * // Example output:
 * //   550e8400-e29b-41d4-a716-446655440000
 * //   e1a6a60e-3dc3-49f2-b3fa-23b8d46f9d5f
 * @endcode
 */

namespace vix::utils
{
    // ---------------------------------------------------------------------
    // RNG helper
    // ---------------------------------------------------------------------

    /**
     * @brief Thread-local random number generator for UUID generation.
     *
     * Seeds the RNG once per thread by combining:
     * - `std::random_device` entropy
     * - High-resolution clock timestamp
     *
     * This approach ensures unique sequences even on systems where
     * `std::random_device` is deterministic.
     *
     * @return Reference to a thread-local `std::mt19937_64` instance.
     */
    inline std::mt19937_64 &uuid_rng() noexcept
    {
        thread_local std::mt19937_64 rng([]
                                         {
            // Mix entropy and time to strengthen seed uniqueness
            std::random_device rd;
            auto mix = (static_cast<std::uint64_t>(rd()) << 32)
                       ^ static_cast<std::uint64_t>(
                           std::chrono::high_resolution_clock::now().time_since_epoch().count());
            return std::mt19937_64{mix}; }());
        return rng;
    }

    // ---------------------------------------------------------------------
    // UUID v4 generator
    // ---------------------------------------------------------------------

    /**
     * @brief Generate a random UUID version 4 (RFC 4122).
     *
     * Fills a 16-byte array with random data, sets the version and variant bits,
     * and returns a canonical string of the form `"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"`.
     *
     * ### Algorithm
     * 1. Generate 16 random bytes.
     * 2. Apply version (`0100` for v4) and variant (`10xxxxxx`).
     * 3. Format into lowercase hex with dashes at positions 8, 13, 18, 23.
     *
     * @return A 36-character lowercase UUID v4 string.
     *
     * @code
     * std::string id = Vix::utils::uuid4();
     * // Example: "550e8400-e29b-41d4-a716-446655440000"
     * @endcode
     */
    inline std::string uuid4() noexcept
    {
        // 16 random bytes
        std::array<std::uint8_t, 16> b{};
        auto &rng = uuid_rng();
        std::uniform_int_distribution<std::uint32_t> dist32(0, 0xFFFFFFFFu);

        // Fill 4 bytes per iteration
        for (std::size_t i = 0; i < 16; i += 4)
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

        // Format as "8-4-4-4-12" hex string (lowercase)
        static constexpr char hexdig[] = "0123456789abcdef";
        std::array<char, 36> out{};
        std::size_t oi = 0;
        for (std::size_t i = 0; i < 16; ++i)
        {
            // Insert dashes at UUID canonical positions
            if (oi == 8 || oi == 13 || oi == 18 || oi == 23)
                out[oi++] = '-';

            out[oi++] = hexdig[static_cast<std::size_t>((b[i] >> 4) & 0x0F)];
            out[oi++] = hexdig[static_cast<std::size_t>((b[i] >> 0) & 0x0F)];
        }

        return std::string(out.data(), out.size());
    }

} // namespace vix::utils

#endif // VIX_UUID_HPP
