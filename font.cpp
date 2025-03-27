#include <vector>
#include <array>
#include "font.h"

const CodePointName* Font::points() {
    static CodePointName pts[] = {
#include "unicode_full.inc"
        {0, nullptr}};
    return pts;
}

const CodePointName* Font::blockBegins() {
    static const CodePointName blockList[] = {
        //       "................................" please, not longer than this (max 40 chars wide output)
        {0x2070, "Superscripts and Subscripts"},  //
        {0x20A0, "Currency Symbols"},             //
        {0x20D0, "Combining Diacritical Marks for Symbols"},
        {0x2190, "Arrows"},                 //
        {0x2500, "Box Drawing"},            //
        {0x2580, "Block Elements"},         //
        {0x25A0, "Geometric Shapes"},       //
        {0x2600, "Miscellaneous Symbols"},  //
        {0x2701, "Dingbats"},               //
        {0x27C0, "Miscellaneous Math Symbols - A"},
        {0x27F0, "Supplemental Arrows - A"},  //
        {0x2800, "Braille Patterns"},         //
        {0x2900, "Supplemental Arrows - B"},  //
        {0x2980, "Miscellaneous Math Symbols - B"},
        {0x2A00, "Supplemental Math Operators"},       //
        {0x2B00, "Miscellaneous Symbols and Arrows"},  //

        {0x1F300, "Miscellaneous Symbols and Picts"},
        {0x1F600, "Emoticons"},
        {0x1F680, "Transport and Map Symbols"},
        {0x1F700, "Alchemical Symbols"},
        {0x1F780, "Geometric Shapes Extended"},
        {0x1F800, "Supplemental Arrows-C"},
        {0x1F900, "Supplemental Symbols and Picts"},
        {0x1FA00, "Chess Symbols"},
        {0x1FA70, "Symbols and Picts Extended-A"},
        {0x1FB00, "Symbols for Legacy Computing"},
        {0, nullptr}  // end marker
    };
    return blockList;
}
