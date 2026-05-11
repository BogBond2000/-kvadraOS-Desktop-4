#include "json_builder.hpp"
#include <sstream>
#include <cstdio>


static std::string escapeJson(const std::string& s) {
    std::ostringstream oss;
    for (unsigned char c : s) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    oss << buf;
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

std::string JsonBuilder::buildArray(const std::vector<std::string>& items, int indent) {
    std::string pad(indent, ' ');
    std::string innerPad(indent + 4, ' ');

    if (items.empty()) {
        return "[]";
    }

    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < items.size(); ++i) {
        oss << innerPad << "\"" << escapeJson(items[i]) << "\"";
        if (i + 1 < items.size()) oss << ",";
        oss << "\n";
    }
    oss << pad << "]";
    return oss.str();
}

std::string JsonBuilder::build(const MediaFiles& files) {
    std::ostringstream oss;
    oss << "{\n";
    oss << "    \"audio\": "  << buildArray(files.audio,  4) << ",\n";
    oss << "    \"video\": "  << buildArray(files.video,  4) << ",\n";
    oss << "    \"images\": " << buildArray(files.images, 4) << "\n";
    oss << "}\n";
    return oss.str();
}
