#include "templateutils.h"

#include <fmt/format.h>
#include <regex>

std::string TemplateUtils::ReplaceVariablesInString(
    const char *input,
    const std::map<std::string, std::string> &variables)
{
    std::string result(input);

    for (auto &var : variables)
    {
        auto regexStr = fmt::format(R"(\{{{}\}})", var.first);
        auto rx = std::regex(regexStr);
        result = std::regex_replace(result, rx, var.second);
    }

    auto rx3 = std::regex(R"(\{\{)");
    result = std::regex_replace(result, rx3, "{");

    auto rx2 = std::regex(R"(\}\})");
    result = std::regex_replace(result, rx2, "}");

    return result;
}
