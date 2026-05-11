#pragma once

#include "scanner.hpp"
#include <string>

class JsonBuilder final{
public:
    static std::string build(const MediaFiles& files);

private:
    static std::string buildArray(const std::vector<std::string>& items, int indent);
};
