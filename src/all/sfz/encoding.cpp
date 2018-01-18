// Copyright (c) 2009 Chris Pickel <sfiera@gmail.com>
//
// This file is part of libsfz, a free software project.  You can redistribute
// it and/or modify it under the terms of the MIT License.

#include <sfz/encoding.hpp>

#include <algorithm>
#include <pn/data>
#include <pn/string>
#include <sfz/range.hpp>

namespace sfz {

namespace {

// Identifies surrogate code points.
//
// UTF-16 represents code points outside the basic multilingual plane (plane 0) with a pair of
// plane 0 code points, the first of which is in the range U+D800 to U+DBFF, and the second of
// which
// is in the range U+DC00 to U+DFFF.  These are not valid code points within a String, so we need
// to identify them as invalid.
//
// @param [in] code     A code point to test.
// @returns             true iff `code` is a surrogate code.
inline bool is_surrogate(uint32_t rune) { return (rune & 0xfffff800) == 0x00d800; }

}  // namespace

const pn::rune kUnknownCodePoint{0x00fffd};       // REPLACEMENT CHARACTER.
const pn::rune kAsciiUnknownCodePoint{0x00003f};  // QUESTION MARK.

bool is_valid_code_point(uint32_t rune) { return (rune <= 0x10ffff) && (!is_surrogate(rune)); }

namespace ascii {

pn::data encode(pn::string_view string) {
    pn::data out;
    for (pn::rune r : string) {
        uint8_t byte = kAsciiUnknownCodePoint.value();
        if (r.value() < 0x80) {
            byte = r.value();
        }
        out += pn::data_view{&byte, 1};
    }
    return out;
}

pn::string decode(pn::data_view data) {
    pn::string out;
    for (uint8_t byte : data) {
        if (byte < 0x80) {
            out += pn::rune{byte};
        } else {
            out += pn::rune{kUnknownCodePoint};
        }
    }
    return out;
}

}  // namespace ascii

namespace latin1 {

pn::data encode(pn::string_view string) {
    pn::data out;
    for (pn::rune r : string) {
        uint8_t byte = '?';
        if (r.value() < 0x100) {
            byte = r.value();
        }
        out += pn::data_view{&byte, 1};
    }
    return out;
}

pn::string decode(pn::data_view data) {
    pn::string out;
    for (uint8_t byte : data) {
        out += pn::rune{byte};
    }
    return out;
}

}  // namespace latin1

namespace macroman {

namespace {

uint16_t kMacRomanSupplement[0x80] = {
        0x00C4,  // LATIN CAPITAL LETTER A WITH DIAERESIS
        0x00C5,  // LATIN CAPITAL LETTER A WITH RING ABOVE
        0x00C7,  // LATIN CAPITAL LETTER C WITH CEDILLA
        0x00C9,  // LATIN CAPITAL LETTER E WITH ACUTE
        0x00D1,  // LATIN CAPITAL LETTER N WITH TILDE
        0x00D6,  // LATIN CAPITAL LETTER O WITH DIAERESIS
        0x00DC,  // LATIN CAPITAL LETTER U WITH DIAERESIS
        0x00E1,  // LATIN SMALL LETTER A WITH ACUTE
        0x00E0,  // LATIN SMALL LETTER A WITH GRAVE
        0x00E2,  // LATIN SMALL LETTER A WITH CIRCUMFLEX
        0x00E4,  // LATIN SMALL LETTER A WITH DIAERESIS
        0x00E3,  // LATIN SMALL LETTER A WITH TILDE
        0x00E5,  // LATIN SMALL LETTER A WITH RING ABOVE
        0x00E7,  // LATIN SMALL LETTER C WITH CEDILLA
        0x00E9,  // LATIN SMALL LETTER E WITH ACUTE
        0x00E8,  // LATIN SMALL LETTER E WITH GRAVE
        0x00EA,  // LATIN SMALL LETTER E WITH CIRCUMFLEX
        0x00EB,  // LATIN SMALL LETTER E WITH DIAERESIS
        0x00ED,  // LATIN SMALL LETTER I WITH ACUTE
        0x00EC,  // LATIN SMALL LETTER I WITH GRAVE
        0x00EE,  // LATIN SMALL LETTER I WITH CIRCUMFLEX
        0x00EF,  // LATIN SMALL LETTER I WITH DIAERESIS
        0x00F1,  // LATIN SMALL LETTER N WITH TILDE
        0x00F3,  // LATIN SMALL LETTER O WITH ACUTE
        0x00F2,  // LATIN SMALL LETTER O WITH GRAVE
        0x00F4,  // LATIN SMALL LETTER O WITH CIRCUMFLEX
        0x00F6,  // LATIN SMALL LETTER O WITH DIAERESIS
        0x00F5,  // LATIN SMALL LETTER O WITH TILDE
        0x00FA,  // LATIN SMALL LETTER U WITH ACUTE
        0x00F9,  // LATIN SMALL LETTER U WITH GRAVE
        0x00FB,  // LATIN SMALL LETTER U WITH CIRCUMFLEX
        0x00FC,  // LATIN SMALL LETTER U WITH DIAERESIS
        0x2020,  // DAGGER
        0x00B0,  // DEGREE SIGN
        0x00A2,  // CENT SIGN
        0x00A3,  // POUND SIGN
        0x00A7,  // SECTION SIGN
        0x2022,  // BULLET
        0x00B6,  // PILCROW SIGN
        0x00DF,  // LATIN SMALL LETTER SHARP S
        0x00AE,  // REGISTERED SIGN
        0x00A9,  // COPYRIGHT SIGN
        0x2122,  // TRADE MARK SIGN
        0x00B4,  // ACUTE ACCENT
        0x00A8,  // DIAERESIS
        0x2260,  // NOT EQUAL TO
        0x00C6,  // LATIN CAPITAL LETTER AE
        0x00D8,  // LATIN CAPITAL LETTER O WITH STROKE
        0x221E,  // INFINITY
        0x00B1,  // PLUS-MINUS SIGN
        0x2264,  // LESS-THAN OR EQUAL TO
        0x2265,  // GREATER-THAN OR EQUAL TO
        0x00A5,  // YEN SIGN
        0x00B5,  // MICRO SIGN
        0x2202,  // PARTIAL DIFFERENTIAL
        0x2211,  // N-ARY SUMMATION
        0x220F,  // N-ARY PRODUCT
        0x03C0,  // GREEK SMALL LETTER PI
        0x222B,  // INTEGRAL
        0x00AA,  // FEMININE ORDINAL INDICATOR
        0x00BA,  // MASCULINE ORDINAL INDICATOR
        0x03A9,  // GREEK CAPITAL LETTER OMEGA
        0x00E6,  // LATIN SMALL LETTER AE
        0x00F8,  // LATIN SMALL LETTER O WITH STROKE
        0x00BF,  // INVERTED QUESTION MARK
        0x00A1,  // INVERTED EXCLAMATION MARK
        0x00AC,  // NOT SIGN
        0x221A,  // SQUARE ROOT
        0x0192,  // LATIN SMALL LETTER F WITH HOOK
        0x2248,  // ALMOST EQUAL TO
        0x2206,  // INCREMENT
        0x00AB,  // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
        0x00BB,  // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
        0x2026,  // HORIZONTAL ELLIPSIS
        0x00A0,  // NO-BREAK SPACE
        0x00C0,  // LATIN CAPITAL LETTER A WITH GRAVE
        0x00C3,  // LATIN CAPITAL LETTER A WITH TILDE
        0x00D5,  // LATIN CAPITAL LETTER O WITH TILDE
        0x0152,  // LATIN CAPITAL LIGATURE OE
        0x0153,  // LATIN SMALL LIGATURE OE
        0x2013,  // EN DASH
        0x2014,  // EM DASH
        0x201C,  // LEFT DOUBLE QUOTATION MARK
        0x201D,  // RIGHT DOUBLE QUOTATION MARK
        0x2018,  // LEFT SINGLE QUOTATION MARK
        0x2019,  // RIGHT SINGLE QUOTATION MARK
        0x00F7,  // DIVISION SIGN
        0x25CA,  // LOZENGE
        0x00FF,  // LATIN SMALL LETTER Y WITH DIAERESIS
        0x0178,  // LATIN CAPITAL LETTER Y WITH DIAERESIS
        0x2044,  // FRACTION SLASH
        0x20AC,  // EURO SIGN
        0x2039,  // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
        0x203A,  // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
        0xFB01,  // LATIN SMALL LIGATURE FI
        0xFB02,  // LATIN SMALL LIGATURE FL
        0x2021,  // DOUBLE DAGGER
        0x00B7,  // MIDDLE DOT
        0x201A,  // SINGLE LOW-9 QUOTATION MARK
        0x201E,  // DOUBLE LOW-9 QUOTATION MARK
        0x2030,  // PER MILLE SIGN
        0x00C2,  // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
        0x00CA,  // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
        0x00C1,  // LATIN CAPITAL LETTER A WITH ACUTE
        0x00CB,  // LATIN CAPITAL LETTER E WITH DIAERESIS
        0x00C8,  // LATIN CAPITAL LETTER E WITH GRAVE
        0x00CD,  // LATIN CAPITAL LETTER I WITH ACUTE
        0x00CE,  // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
        0x00CF,  // LATIN CAPITAL LETTER I WITH DIAERESIS
        0x00CC,  // LATIN CAPITAL LETTER I WITH GRAVE
        0x00D3,  // LATIN CAPITAL LETTER O WITH ACUTE
        0x00D4,  // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
        0xF8FF,  // Apple logo
        0x00D2,  // LATIN CAPITAL LETTER O WITH GRAVE
        0x00DA,  // LATIN CAPITAL LETTER U WITH ACUTE
        0x00DB,  // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
        0x00D9,  // LATIN CAPITAL LETTER U WITH GRAVE
        0x0131,  // LATIN SMALL LETTER DOTLESS I
        0x02C6,  // MODIFIER LETTER CIRCUMFLEX ACCENT
        0x02DC,  // SMALL TILDE
        0x00AF,  // MACRON
        0x02D8,  // BREVE
        0x02D9,  // DOT ABOVE
        0x02DA,  // RING ABOVE
        0x00B8,  // CEDILLA
        0x02DD,  // DOUBLE ACUTE ACCENT
        0x02DB,  // OGONEK
        0x02C7,  // CARON
};

}  // namespace

pn::data encode(pn::string_view string) {
    pn::data out;
    for (pn::rune r : string) {
        uint8_t byte = '?';
        if (r.value() < 0x80) {
            byte = r.value();
        } else {
            for (int i : range(0x80)) {
                if (r.value() == kMacRomanSupplement[i]) {
                    byte = 0x80 + i;
                    break;
                }
            }
        }
        out += pn::data_view{&byte, 1};
    }
    return out;
}

pn::string decode(pn::data_view data) {
    pn::string out;
    for (uint8_t byte : data) {
        if (byte < 0x80) {
            out += pn::rune{byte};
        } else {
            out += pn::rune{kMacRomanSupplement[byte - 0x80]};
        }
    }
    return out;
}

}  // namespace macroman

}  // namespace sfz
