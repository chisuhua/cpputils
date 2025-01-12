#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "String.h"

TEST_CASE("String Interning") {
    StringRef str1 = "hello"_hs;
    StringRef str2 = "hello";
    StringRef str3 = std::string("hello");

    CHECK(str1 == str2);
    CHECK(str2 == str3);

    CHECK(StringPool::getInstance()->isStringIntern("hello"));

    std::cout << *str1 << std::endl;  // Output: hello

    std::unordered_set<StringRef> stringSet;
    stringSet.insert(str1);
    stringSet.insert(str2);
    stringSet.insert(str3);

    CHECK(stringSet.size() == 1);
}

