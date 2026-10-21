module;

#include <string>
#include <cstdint>
#include <cctype>

export module color;

// ====================================================================
// FARBE
// ====================================================================

export struct ColorState {
    double R = 1;
    double G = 1;
    double B = 1;
    double A = 1;
};

// ---- Function declarations needed by ColorInput inline ctors ----
export ColorState ParseColor(const std::string& hex);
export ColorState ParseColor(double gray);
export ColorState ParseColor(double r, double g, double b);
export ColorState ParseColor(double r, double g, double b, double a);
export ColorState ParseHex(const std::string& s);
export bool ColEq(const ColorState& a, const ColorState& b);

export struct ColorInput {
    bool Set = false;
    ColorState Color;

    ColorInput() = default;
    ColorInput(const char* s) : Set(true), Color(ParseHex(s)) {}
    ColorInput(const std::string& s) : Set(true), Color(ParseHex(s)) {}
    ColorInput(double gray) : Set(true), Color(ParseColor(gray)) {}
    ColorInput(double r, double g, double b) : Set(true), Color(ParseColor(r, g, b)) {}
    ColorInput(double r, double g, double b, double a)
        : Set(true), Color(ParseColor(r, g, b, a)) {}
};

// ---- Inline implementations ----

inline ColorState ParseHex(const std::string& s) {
    std::string h = s;
    if (!h.empty() && h[0] == '#') {
        h.erase(h.begin());
    }
    if (h.size() == 3) {
        h = std::string{h[0], h[0], h[1], h[1], h[2], h[2]};
    }
    if (h.size() != 6) {
        return ColorState{1, 1, 1, 1};
    }

    std::uint32_t n = 0;
    for (char c : h) {
        int v = -1;
        if (c >= '0' && c <= '9') {
            v = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            v = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            v = c - 'A' + 10;
        }
        if (v < 0) {
            return ColorState{1, 1, 1, 1};
        }
        n = (n << 4) | static_cast<std::uint32_t>(v);
    }

    return ColorState{
        static_cast<double>((n >> 16) & 0xff) / 255,
        static_cast<double>((n >> 8) & 0xff) / 255,
        static_cast<double>(n & 0xff) / 255,
        1,
    };
}

inline ColorState ParseColor(const std::string& hex) { return ParseHex(hex); }

inline ColorState ParseColor(double gray) {
    const double g = gray / 255;
    return ColorState{g, g, g, 1};
}

inline ColorState ParseColor(double r, double g, double b) {
    return ColorState{r / 255, g / 255, b / 255, 1};
}

inline ColorState ParseColor(double r, double g, double b, double a) {
    return ColorState{r / 255, g / 255, b / 255, a / 255};
}

inline bool ColEq(const ColorState& a, const ColorState& b) {
    return a.R == b.R && a.G == b.G && a.B == b.B && a.A == b.A;
}