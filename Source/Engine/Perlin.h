// AGO - rumore di Perlin 2D (gradient noise) con ottave e persistenza
#pragma once
#include <cmath>
#include <cstdint>

namespace ago {

class Perlin {
public:
    Perlin() {
        uint32_t s = 0x9E3779B9u;
        for (int i = 0; i < 256; ++i) p[i] = (uint8_t) i;
        for (int i = 255; i > 0; --i) {
            s = s * 1664525u + 1013904223u;
            int j = (int) (s % (uint32_t) (i + 1));
            uint8_t t = p[i]; p[i] = p[j]; p[j] = t;
        }
        for (int i = 0; i < 256; ++i) p[256 + i] = p[i];
    }

    double noise(double x, double y) const {
        int X = (int) std::floor(x) & 255, Y = (int) std::floor(y) & 255;
        x -= std::floor(x); y -= std::floor(y);
        double u = fade(x), v = fade(y);
        int aa = p[p[X] + Y], ab = p[p[X] + Y + 1], ba = p[p[X + 1] + Y], bb = p[p[X + 1] + Y + 1];
        double r = lerp(v, lerp(u, grad(aa, x, y), grad(ba, x - 1, y)),
                           lerp(u, grad(ab, x, y - 1), grad(bb, x - 1, y - 1)));
        return r * 1.41421356;  // circa [-1, 1]
    }

    // persistence 0..1: peso delle ottave alte
    double fractal(double x, double y, double persistence, int octaves = 4) const {
        double sum = 0, amp = 1, norm = 0, f = 1;
        for (int o = 0; o < octaves; ++o) {
            sum += amp * noise(x * f, y * f);
            norm += amp; amp *= persistence; f *= 2.0;
        }
        return norm > 0 ? sum / norm : 0;
    }

private:
    uint8_t p[512];
    static double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static double lerp(double t, double a, double b) { return a + t * (b - a); }
    static double grad(int h, double x, double y) {
        switch (h & 7) {
            case 0: return x + y;  case 1: return -x + y; case 2: return x - y; case 3: return -x - y;
            case 4: return x;      case 5: return -x;     case 6: return y;     default: return -y;
        }
    }
};

} // namespace ago
