#pragma once
#include <string>

class Clipboard {
    public:
    static void copyToClipboard(const std::string& utf8Text);
};