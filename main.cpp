// stdout-emoji-panel.cpp : Defines the entry point for the application.
//
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <algorithm>
#include "clipboard.h"

#ifdef _WIN32
    #include <Windows.h>
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

#define UTF8IFY(a) (const char*)(u8##a)  // std::cout and std::string do not accept u8""

#include "font.h"
class EmojiPicker {
    public:
    struct Options {
        bool clearScreen = true;
        bool moveCursor = true;
        bool useStdIn = false;  // better use _getch etc.
        char32_t initialCodePoint = U'😀';
    } options;

    private:
    const CodePointName* points;
    const CodePointName* blocks;
    int currentBlockIndex = 0;
    int currentPageIndex = 0;
    int blocksCount = 0;
    int numberOfPreviousBlocks = 2;  // how many blocks to show in list before the current one
    std::vector<const CodePointName*> currentBlockPoints;
    int blockCount() const { return blocksCount; }

    void clearScreen() {
        if (options.clearScreen) {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
        } else {
            std::cout << std::string(30, '\n');
        }
    }
    char mygetch() {
        char ch = '\0';
        if (options.useStdIn) {
            std::cin >> ch;
        } else {
#ifdef _WIN32
            ch = char(_getch());
#else
            termios oldt, newt;
            tcgetattr(STDIN_FILENO, &oldt);  // save old settings
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);         // disable buffering
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // apply new settings
            read(STDIN_FILENO, &ch, 1);
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // restore old settings
#endif
        }
        return ch;
    }

    std::string keyNameForIndex(int i) {
        if (i < 0 || i > 9) { return "?"; }
        const char* all = "1234567890";
        char str[] = {all[i], '\0'};
        return str;
    }

    int indexForKeyName(char ch) {
        if (ch == '0') { return 9; }
        return ch - '1';
    }

    void setCurrentBlock(char32_t codeInBlock) {
        currentBlockIndex = blockCount() - 1;
        int c = -1;
        for (auto* b = blocks; b->code != 0; ++b) {
            if (b->code > codeInBlock + 1) {
                if (c >= 0) {
                    currentBlockIndex = c;
                }
                break;
            }
            ++c;
        }
        loadCurrentBlockPoints();
    }

    public:
    EmojiPicker() {
        points = Font::points();
        blocks = Font::blockBegins();
        blocksCount = 0;
        for (auto* b = blocks; b->code != 0; ++b) { ++blocksCount; }
    }

    void run() {
        setCurrentBlock(options.initialCodePoint);  // start at emoji page
        char input;
        do {
            printMenu();
            input = mygetch();
            // std::cin >> input;
            char32_t picked = handleInput(std::toupper(input));

            if (picked != 0) {
                Clipboard::copyToClipboard(toUTF8(picked));
                return;
            }

        } while (input != 'Q' && input != 'q');
    }

    private:
    void loadCurrentBlockPoints() {
        currentBlockIndex = (currentBlockIndex + blockCount()) % blockCount();

        currentBlockPoints.clear();
        char32_t blockStart = blocks[currentBlockIndex].code;
        char32_t blockEnd = (blocks[currentBlockIndex + 1].name != nullptr)
                                ? blocks[currentBlockIndex + 1].code
                                : 0x10FFFF;

        for (const CodePointName* p = points; p->name != nullptr; ++p) {
            if (p->code >= blockStart && p->code < blockEnd) {
                currentBlockPoints.push_back(p);
            }
        }
        currentPageIndex = 0;
    }

    void moveCursorToColumn(int x) {
        if (options.moveCursor) {
#ifdef _WIN32
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);
            COORD pos = {static_cast<SHORT>(x - 1), csbi.dwCursorPosition.Y};
            SetConsoleCursorPosition(hOut, pos);
#else
            std::cout << "\033[" << x << "G";
#endif
        }
    }

    // Levenshtein distance function
    static int levenshtein(const std::string& s1, const std::string& s2) {
        int m = s1.size();
        int n = s2.size();
        std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

        for (int i = 0; i <= m; ++i) dp[i][0] = i;
        for (int j = 0; j <= n; ++j) dp[0][j] = j;
        auto imin = [](int a, int b) { return a < b ? a : b; };
        for (int i = 1; i <= m; ++i) {
            for (int j = 1; j <= n; ++j) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                int dd = dp[i - 1][j] + 1;         // Deletion
                int di = dp[i][j - 1] + 1;         // Insertion
                int ds = dp[i - 1][j - 1] + cost;  // Substitution

                dp[i][j] = imin(dd, imin(di, ds));
            }
        }

        int difflen = int(s1.length()) - int(s2.length());
        if (difflen < 0) { difflen *= -1; }

        return dp[m][n] * 1000 + difflen;
    }

    char32_t searchEmoji() {
        std::string query;
        const auto* points = Font::points();

        while (true) {
            clearScreen();
            std::cout << UTF8IFY("🔍 Search: ") << query << "\n";

            int minError = 0x7fffffff;
            char32_t currentPick = 0;
            if (query.length() > 1) {
                std::vector<std::pair<std::string, char32_t>> words;
                std::string search = query;
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                for (int i = 0; points[i].code != 0; ++i) {
                    std::string name = points[i].name;
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    if (name.find(search) != std::string::npos) {
                        words.push_back({name, points[i].code});
                    }
                }

                // too slow
                // std::sort(words.begin(), words.end(), [&search](const std::string& a, const std::string& b) {
                //     return levenshtein(a, search) < levenshtein(b, search);
                // });

                // Cache distances along with word+number
                std::vector<std::tuple<std::string, int, int>> scored;
                for (const auto& [word, number] : words) {
                    int dist = levenshtein(word, search);
                    scored.emplace_back(word, number, dist);
                }
                // Sort by distance
                std::sort(scored.begin(), scored.end(), [](auto& a, auto& b) {
                    return std::get<2>(a) < std::get<2>(b);
                });

                // Output
                int i = 0;
                for (const auto& [word, ucode, dist] : scored) {
                    if (i == 10) { break; }
                    // std::cout << word << " (" << ucode << ") - distance: " << dist << "\n";
                    if (i == 0) {
                        currentPick = ucode;
                        std::cout << UTF8IFY("👉 ");
                    } else {
                        std::cout << UTF8IFY("  ");
                    }
                    moveCursorToColumn(3);
                    std::cout << toUTF8(ucode);
                    moveCursorToColumn(6);
                    std::cout << ": " << word << "\n";

                    ++i;
                }
            }

            // Get key
            char ch = mygetch();

            if (ch == 27) {  // ESC
                break;
            } else if (ch == 127 || ch == 8) {  // Backspace
                if (!query.empty()) query.pop_back();
            } else if (isprint(static_cast<unsigned char>(ch))) {
                query.push_back(ch);
            } else if (ch == '\n' || ch == '\r') {
                return currentPick;
            }
        }
        return 0;
    }

    void printMenu() {
        clearScreen();

#if _DEBUG
        std::cout << "012345678.MAX.40.COLUMNS.PLEASE.23456789\n";
#endif
        std::cout << UTF8IFY("====== ⚡👈  Emoji Picker ======\n");
        int istartBlock = (currentBlockIndex - numberOfPreviousBlocks + blockCount()) % blockCount();
        for (int i = 0; i < 10; ++i) {
            int iblk = (istartBlock + i) % blockCount();
            std::string key = keyNameForIndex(i);
            if (iblk == currentBlockIndex) {
                std::string arrow = (const char*)(u8" 🚩");
                std::cout << "*" << ": " << blocks[iblk].name << arrow << "\n";
            } else {
                std::cout << key << ": " << blocks[iblk].name << "\n";
            }
        }
        std::cout << "\n";

        // Print Emoji Grid
        const int rows = 3, cols = 8;
        int start = currentPageIndex * rows * cols;
        std::cout << "Page " << (currentPageIndex + 1) << "\n";
        std::cout << UTF8IFY("┌───┬───┬───┬───┬───┬───┬───┬───┐\n");
        int colWidth = 4;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                int index = start + row * cols + col;
                if (index < (int)currentBlockPoints.size()) {
                    char label = 'A' + row * cols + col;
                    char32_t ch = currentBlockPoints[index]->code;

                    // move, because every terminal displays some emojis in a different width
                    // if your unicode codepoint is always 1 character, you might ommit this.
                    moveCursorToColumn(1 + col * colWidth);
                    std::cout << UTF8IFY("│") << label << toUTF8(ch) << " ";
                }
            }
            moveCursorToColumn(1 + cols * colWidth);
            std::cout << UTF8IFY("│\n");
            std::flush(std::cout);
            if (row + 1 < rows) {
                std::cout << UTF8IFY("├───┼───┼───┼───┼───┼───┼───┼───┤\n");
            } else {
                std::cout << UTF8IFY("└───┴───┴───┴───┴───┴───┴───┴───┘\n");
                std::cout << "[Y] PREVIOUS  [Z] NEXT  [ESC] Quit\n";
            }
            std::flush(std::cout);
        }
        // Description
        // std::cout << "\n";
        // for (int row = 0; row < rows; ++row) {
        //     for (int col = 0; col < cols; ++col) {
        //         int index = start + row * cols + col;
        //         if (index < (int)currentBlockPoints.size()) {
        //             char label = 'A' + row * cols + col;
        //             const CodePointName* cp = currentBlockPoints[index];
        //             std::cout << label << ": U+"
        //                 << std::hex << std::uppercase << int(cp->code)
        //                 << " " << cp->name << "\n";
        //         }
        //     }
        // }
    }

    char32_t handleInput(char input) {
        if (input >= '0' && input <= '9') {
            int index = indexForKeyName(input);
            currentBlockIndex += index - numberOfPreviousBlocks;
            loadCurrentBlockPoints();
        } else if (input == 'Y') {  // Prev Page
            if (currentPageIndex > 0) currentPageIndex--;
        } else if (input == 'Z') {  // Next Page
            int maxPage = int(currentBlockPoints.size() + 23) / 24;
            if (currentPageIndex + 1 < maxPage) currentPageIndex++;
        } else if (input >= 'A' && input <= 'X') {  // Select Emoji
            int index = currentPageIndex * 24 + (input - 'A');
            if (index < (int)currentBlockPoints.size()) {
                const CodePointName* cp = currentBlockPoints[index];
                return cp->code;
            }
        } else if (input == '?') {
            return searchEmoji();
        } else if (input == '\x1b') {
            exit(0);
        }
        return 0;
    }

    // UTF-8 encoding helper
    std::string toUTF8(char32_t cp) {
        std::string result;
        if (cp <= 0x7F)
            result += static_cast<char>(cp);
        else if (cp <= 0x7FF) {
            result += static_cast<char>(0xC0 | (cp >> 6));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            result += static_cast<char>(0xE0 | (cp >> 12));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (cp >> 18));
            result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
        return result;
    }
};

static char32_t parseNextUtf8(const char*& utf8) {
    char32_t codepoint = 0;
    if (static_cast<unsigned char>(static_cast<unsigned char>(*utf8)) < 0x80) {
        // 1-byte sequence (ASCII)
        codepoint = static_cast<unsigned char>(*utf8);
        ++utf8;
    } else if ((static_cast<unsigned char>(*utf8) & 0xE0) == 0xC0) {
        // 2-byte sequence
        codepoint = static_cast<unsigned char>(*utf8) & 0x1F;
        ++utf8;
        if ((static_cast<unsigned char>(*utf8) & 0xC0) != 0x80) { return 0; }  // Invalid UTF-8 sequence
        codepoint = (codepoint << 6) | (static_cast<unsigned char>(*utf8) & 0x3F);
        ++utf8;
    } else if ((static_cast<unsigned char>(*utf8) & 0xF0) == 0xE0) {
        // 3-byte sequence
        codepoint = static_cast<unsigned char>(*utf8) & 0x0F;
        ++utf8;
        for (int i = 0; i < 2; ++i) {
            if ((static_cast<unsigned char>(*utf8) & 0xC0) != 0x80) { return 0; }  // Invalid UTF-8 sequence
            codepoint = (codepoint << 6) | (static_cast<unsigned char>(*utf8) & 0x3F);
            ++utf8;
        }
    } else if ((static_cast<unsigned char>(*utf8) & 0xF8) == 0xF0) {
        // 4-byte sequence
        codepoint = static_cast<unsigned char>(*utf8) & 0x07;
        ++utf8;
        for (int i = 0; i < 3; ++i) {
            if ((static_cast<unsigned char>(*utf8) & 0xC0) != 0x80) { return 0; }  // Invalid UTF-8 sequence
            codepoint = (codepoint << 6) | (static_cast<unsigned char>(*utf8) & 0x3F);
            ++utf8;
        }
    } else {
        if (*utf8 != '\0') { ++utf8; }
        return U'?';  // Invalid UTF-8 sequence
    }

    return codepoint;
}

int showHelp() {
    std::cout << "options:\n";

    std::cout << "--stdin  : use stdin + enter instead of waiting for key press\n";
    std::cout << "--nocls  : don't use clear screen\n";
    std::cout << "--nomove : don't use code to move the cursor\n";
    std::cout << "--start c: start with the page that contains the next codepoint\n";
    std::cout << "--help   : this help\n";
    return 0;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // 65001 - give programs a chance to output utf-8, if they care
#endif

    EmojiPicker picker;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--stdin") == 0) { picker.options.useStdIn = true; }
        if (strcmp(argv[i], "--nocls") == 0) { picker.options.clearScreen = false; }
        if (strcmp(argv[i], "--nomove") == 0) { picker.options.moveCursor = false; }
        if (strcmp(argv[i], "--start") == 0) {
            const char* p = argv[i + 1];
            picker.options.initialCodePoint = parseNextUtf8(p);
        }
        if (strcmp(argv[i], "--help") == 0) { return showHelp(); }
    }

    picker.run();
    return 0;
}