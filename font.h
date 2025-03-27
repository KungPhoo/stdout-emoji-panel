#pragma once
struct CodePointName {
    char32_t code;
    const char* name;
};

class Font {
    public:
    static const CodePointName* points();
    static const CodePointName* blockBegins();
};