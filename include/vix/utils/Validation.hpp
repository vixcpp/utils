#ifndef VIX_VALIDATION_HPP
#define VIX_VALIDATION_HPP

#include <string>
#include <unordered_map>
#include <regex>
#include <optional>
#include "String.hpp"
#include "Result.hpp"

namespace Vix::utils
{
    using FieldErrors = std::unordered_map<std::string, std::string>;

    struct Rule
    {
        bool required = false;
        std::optional<size_t> min_len{};
        std::optional<size_t> max_len{};
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
        for (auto &[key, rule] : schema)
        {
            auto it = data.find(key);
            auto present = (it != data.end() && !it->second.empty());
            auto label = rule.label.empty() ? key : rule.label;

            if (rule.required && !present)
            {
                errs[key] = label + " is required";
                continue;
            }
            if (!present)
                continue;

            const auto &v = it->second;

            if (rule.min_len && v.size() < *rule.min_len)
                errs[key] = label + " must be at least " + std::to_string(*rule.min_len) + " chars";
            if (rule.max_len && v.size() > *rule.max_len)
                errs[key] = label + " must be at most " + std::to_string(*rule.max_len) + " chars";

            if (rule.min || rule.max)
            {
                try
                {
                    long long n = std::stoll(v);
                    if (rule.min && n < *rule.min)
                        errs[key] = label + " must be >= " + std::to_string(*rule.min);
                    if (rule.max && n > *rule.max)
                        errs[key] = label + " must be <= " + std::to_string(*rule.max);
                }
                catch (...)
                {
                    errs[key] = label + " must be a number";
                }
            }

            if (rule.pattern && !std::regex_match(v, *rule.pattern))
                errs[key] = label + " has invalid format";
        }
        if (!errs.empty())
            return Result<void, FieldErrors>::Err(std::move(errs));
        return Result<void, FieldErrors>::Ok();
    }

    inline Rule required(std::string label = "")
    {
        Rule r;
        r.required = true;
        r.label = std::move(label);
        return r;
    }
    inline Rule len(size_t minL, size_t maxL, std::string lbl = "")
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

}

#endif