
#include "../src/common/templateutils.h"
#include <catch2/catch.hpp>

TEST_CASE("ReplaceVariablesInString replace variables", "[templateutils]")
{
    std::map<std::string, std::string> variables = {
        {"tableName", "Table A"},
        {"tablePrimaryKey", "Id"},
    };

    const char *testData = "test {tableName} test and {tablePrimaryKey} test";
    auto result = TemplateUtils::ReplaceVariablesInString(testData, variables);

    REQUIRE(result == "test Table A test and Id test");
}

TEST_CASE("ReplaceVariablesInString should leave escaped {{}}", "[templateutils]")
{
    std::map<std::string, std::string> variables = {
        {"tableName", "Table A"},
        {"tablePrimaryKey", "Id"},
    };

    const char *testData = "test {tableName} {{tableName}} and {tablePrimaryKey} test";
    auto result = TemplateUtils::ReplaceVariablesInString(testData, variables);

    REQUIRE(result == "test Table A {Table A} and Id test");
}
