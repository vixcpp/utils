#ifndef VIX_VALIDATION_HPP
#define VIX_VALIDATION_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <regex>
#include <optional>
#include <charconv> // from_chars
#include <limits>

#include "String.hpp"
#include "Result.hpp"

namespace Vix::utils
{
    using FieldErrors = std::unordered_map<std::string, std::string>;

    struct Rule
    {
        bool required = false;
        std::optional<std::size_t> min_len{};
        std::optional<std::size_t> max_len{};
        std::optional<long long> min{};
        std::optional<long long> max{};
        std::optional<std::regex> pattern{};
        std::string label;
    };

    using Schema = std::unordered_map<std::string, Rule>;

    inline Result<void, FieldErrors> validate_map(
        const std::unordered_map<std::string, std::string> &data,
        const Schema &schema)
    {
        FieldErrors errs;

        for (const auto &entry : schema)
        {
            const std::string &key = entry.first;
            const Rule &rule = entry.second;

            const auto it = data.find(key);
            const bool present = (it != data.end() && !it->second.empty());
            const std::string &label = rule.label.empty() ? key : rule.label;

            // required ?
            if (rule.required && !present)
            {
                errs.emplace(key, label + " is required");
                continue; // passe au champ suivant
            }
            if (!present)
                continue;

            const std::string &v = it->second;

            // min_len / max_len
            if (!v.empty())
            {
                if (rule.min_len && v.size() < *rule.min_len)
                {
                    errs.emplace(key, label + " must be at least " + std::to_string(*rule.min_len) + " chars");
                    continue;
                }
                if (rule.max_len && v.size() > *rule.max_len)
                {
                    errs.emplace(key, label + " must be at most " + std::to_string(*rule.max_len) + " chars");
                    continue;
                }
            }

            // min/max numériques (base 10, sans lancer d'exceptions)
            if (rule.min || rule.max)
            {
                long long n = 0;
                const char *begin = v.data();
                const char *end = v.data() + v.size();
                auto [ptr, ec] = std::from_chars(begin, end, n, 10);

                if (ec != std::errc{} || ptr != end)
                {
                    errs.emplace(key, label + " must be a number");
                    continue;
                }

                if (rule.min && n < *rule.min)
                {
                    errs.emplace(key, label + " must be >= " + std::to_string(*rule.min));
                    continue;
                }
                if (rule.max && n > *rule.max)
                {
                    errs.emplace(key, label + " must be <= " + std::to_string(*rule.max));
                    continue;
                }
            }

            // regex pattern
            if (rule.pattern && !std::regex_match(v, *rule.pattern))
            {
                errs.emplace(key, label + " has invalid format");
                continue;
            }
        }

        if (!errs.empty())
            return Result<void, FieldErrors>::Err(std::move(errs));
        return Result<void, FieldErrors>::Ok();
    }

    // --------- helpers de construction de règles ---------
    inline Rule required(std::string label = "")
    {
        Rule r;
        r.required = true;
        r.label = std::move(label);
        return r;
    }

    inline Rule len(std::size_t minL, std::size_t maxL, std::string lbl = "")
    {
        Rule r;
        r.min_len = minL;
        r.max_len = maxL;
        r.label = std::move(lbl);
        return r;
    }

    inline Rule num_range(long long minV, long long maxV, std::string lbl = "")
    {
        Rule r;
        r.min = minV;
        r.max = maxV;
        r.label = std::move(lbl);
        return r;
    }

    inline Rule match(std::string regex_str, std::string lbl = "")
    {
        Rule r;
        r.pattern = std::regex(regex_str);
        r.label = std::move(lbl);
        return r;
    }

} // namespace Vix::utils

#endif // VIX_VALIDATION_HPP
