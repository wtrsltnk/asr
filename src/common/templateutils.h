#ifndef TEMPLATEUTILS_H
#define TEMPLATEUTILS_H

#include <map>
#include <string>

class TemplateUtils
{
public:
    static std::string ReplaceVariablesInString(
        const char *input,
        const std::map<std::string, std::string> &variables);
};

#endif // TEMPLATEUTILS_H
