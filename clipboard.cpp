#include "clipboard.h"

#ifdef _WIN32
    #include <Windows.h>
// Convert UTF-8 string to UTF-16 (wide string)
std::wstring utf8ToUtf16(const std::string& utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wideStr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wideStr[0], len);
    return wideStr;
}

void Clipboard::copyToClipboard(const std::string& utf8Text) {
    // Convert UTF-8 string to UTF-16
    std::wstring wideText = utf8ToUtf16(utf8Text);

    // Open clipboard
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();  // Clear previous clipboard contents

        // Allocate memory for the clipboard content
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (wideText.size() + 1) * sizeof(wchar_t));
        if (hGlobal) {
            // Lock and copy the wide string to memory
            wchar_t* pGlobal = static_cast<wchar_t*>(GlobalLock(hGlobal));
            if (pGlobal) {
                memcpy(pGlobal, wideText.c_str(), sizeof(wchar_t) * (wideText.size() + 1));
                GlobalUnlock(hGlobal);

                // Set the clipboard data (CF_UNICODETEXT)
                SetClipboardData(CF_UNICODETEXT, hGlobal);
            }
        }
        CloseClipboard();  // Close the clipboard
    }
}

#elif __APPLE__

    #include <cstdlib>
void Clipboard::copyToClipboard(const std::string& utf8Text) {
    std::string cmd = "echo \"" + utf8Text + "\" | pbcopy";
    system(cmd.c_str());
}

#else  // Linux / Unix

    #include <cstdlib>
void Clipboard::copyToClipboard(const std::string& utf8Text) {
    // try xsel
    std::string cmd = "echo \"" + utf8Text + "\" | xsel --clipboard";
    int result = system(cmd.c_str());
    if (result != 0) {
        // fallback xclip
        cmd = "echo \"" + utf8Text + "\" | xclip -selection clipboard";
        result = system(cmd.c_str());
    }
}
#endif
