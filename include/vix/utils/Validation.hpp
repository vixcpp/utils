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

/**
 * @file VIX_VALIDATION_HPP
 * @brief Lightweight map validation utilities with schema-based rules.
 *
 * Provides a tiny, dependency-free validation layer for string maps
 * (e.g., form submissions, JSON-into-string maps) using a declarative
 * `Schema` of rules (`required`, length bounds, numeric ranges, regex).
 *
 * - **No exceptions**: parsing uses `std::from_chars`.
 * - **Granular errors**: returns an aggregated map of field → error message.
 * - **Composability**: helper builders to create common rules quickly.
 *
 * ### Example
 * @code
 * using namespace Vix::utils;
 *
 * std::unordered_map<std::string, std::string> data = {
 *   {"name", "Ada"},
 *   {"age",  "21"},
 *   {"email","ada@example.com"}
 * };
 *
 * Schema schema = {
 *   {"name",  required("Name")},
 *   {"age",   num_range(18, 120, "Age")},
 *   {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Email")}
 * };
 *
 * auto res = validate_map(data, schema);
 * if (res.is_err()) {
 *   const auto& errs = res.error();  // FieldErrors
 *   // handle errs.at("field")…
 * }
 * @endcode
 */

namespace vix::utils
{
    /**
     * @brief Field-wise error messages, keyed by field name.
     *
     * Example entry: `"email" -> "Email has invalid format"`.
     */
    using FieldErrors = std::unordered_map<std::string, std::string>;

    /**
     * @brief Declarative validation rule for a single field.
     *
     * Use helper builders (`required`, `len`, `num_range`, `match`) to construct
     * a `Rule` with one or more constraints. The first failing constraint
     * produces an error message for that field.
     */
    struct Rule
    {
        /** @brief If true, the field must be present and non-empty. */
        bool required = false;

        /** @brief Minimum string length (inclusive). */
        std::optional<std::size_t> min_len{};

        /** @brief Maximum string length (inclusive). */
        std::optional<std::size_t> max_len{};

        /** @brief Minimum numeric value (base 10, inclusive). */
        std::optional<long long> min{};

        /** @brief Maximum numeric value (base 10, inclusive). */
        std::optional<long long> max{};

        /** @brief Regex pattern to match (full match). */
        std::optional<std::regex> pattern{};

        /**
         * @brief Human-friendly label used in error messages.
         * If empty, the field key is used.
         */
        std::string label;
    };

    /**
     * @brief A schema maps field names to their validation rules.
     */
    using Schema = std::unordered_map<std::string, Rule>;

    /**
     * @brief Validate a string map against a schema.
     *
     * Validates each field present in the `schema` by applying, in order:
     *  1) `required` (presence & non-empty)
     *  2) `min_len` / `max_len` (string length)
     *  3) `min` / `max` (numeric, base-10 via `std::from_chars`)
     *  4) `pattern` (full-match regex)
     *
     * For each field, the *first* failing check yields an error message and
     * validation proceeds to the next field. If any error occurs, returns
     * `Result<void, FieldErrors>::Err` containing all field messages.
     * Otherwise, returns `Ok()`.
     *
     * @param data   Input map (field → value as strings).
     * @param schema Field rules to apply.
     * @return `Ok()` if everything passes, otherwise `Err(FieldErrors)`.
     *
     * @note Numeric checks run only if `min` or `max` is set for that field.
     * @note Regex uses `std::regex_match` (entire value must match).
     */
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
                continue; // next field
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

            // numeric min/max (base 10, no exceptions)
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

    // ---------------------------------------------------------------------
    // Rule builder helpers
    // ---------------------------------------------------------------------

    /**
     * @brief Build a `required` rule.
     * @param label Friendly label for messages (fallback to key if empty).
     * @return Rule with `required=true`.
     */
    inline Rule required(std::string label = "")
    {
        Rule r;
        r.required = true;
        r.label = std::move(label);
        return r;
    }

    /**
     * @brief Build a length-bounded rule.
     * @param minL Minimum length (inclusive).
     * @param maxL Maximum length (inclusive).
     * @param lbl  Friendly label for messages.
     * @return Rule with `min_len` and `max_len`.
     */
    inline Rule len(std::size_t minL, std::size_t maxL, std::string lbl = "")
    {
        Rule r;
        r.min_len = minL;
        r.max_len = maxL;
        r.label = std::move(lbl);
        return r;
    }

    /**
     * @brief Build a numeric range rule (base-10, inclusive).
     * @param minV Minimum allowed value.
     * @param maxV Maximum allowed value.
     * @param lbl  Friendly label for messages.
     * @return Rule with `min` and `max`.
     */
    inline Rule num_range(long long minV, long long maxV, std::string lbl = "")
    {
        Rule r;
        r.min = minV;
        r.max = maxV;
        r.label = std::move(lbl);
        return r;
    }

    /**
     * @brief Build a regex-match rule (full match).
     * @param regex_str ECMAScript regex string.
     * @param lbl       Friendly label for messages.
     * @return Rule with `pattern`.
     *
     * @note Uses `std::regex` (default ECMAScript grammar).
     */
    inline Rule match(std::string regex_str, std::string lbl = "")
    {
        Rule r;
        r.pattern = std::regex(regex_str);
        r.label = std::move(lbl);
        return r;
    }

} // namespace vix::utils

#endif // VIX_VALIDATION_HPP
