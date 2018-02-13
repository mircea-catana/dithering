#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include "lodepng.h"

static char*   inputPath  = const_cast<char*>("input.png");
static char*   outputPath = const_cast<char*>("output.png");
static uint8_t bpc        = 2;

template <typename T>
struct Pixel
{
    static_assert(std::is_arithmetic<T>::value, "Arithmetic type required.");

    Pixel() : r(0), g(0), b(0), a(0) {}
    Pixel(T r, T g, T b, T a) : r(r), g(g), b(b), a(a) {}

    template <typename uint8_t>
    Pixel(const Pixel<uint8_t>& from) {
        r = from.r;
        g = from.g;
        b = from.b;
        a = from.a;
    }

    Pixel operator- (const Pixel& other) {
        Pixel p;
        p.r = r - other.r;
        p.g = g - other.g;
        p.b = b - other.b;
        p.a = a - other.a;
        return p;
    }

    Pixel operator+ (const Pixel& other) {
        Pixel p;
        p.r = r + other.r;
        p.g = g + other.g;
        p.b = b + other.b;
        p.a = a + other.a;
        return p;
    }

    Pixel operator* (const float& factor) {
        Pixel p;
        p.r = r * factor;
        p.g = g * factor;
        p.b = b * factor;
        p.a = a * factor;
        return p;
    }

    T r;
    T g;
    T b;
    T a;
};

typedef Pixel<uint8_t> Pixelu8;
typedef Pixel<int32_t> Pixeli32;

void parseArgs(const int& argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0) {
            assert(argv[i + 1] != nullptr);
            inputPath = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0) {
            assert(argv[i + 1] != nullptr);
            outputPath = argv[++i];
        } else if (strcmp(argv[i], "-b") == 0) {
            assert(argv[i + 1] != nullptr);
            bpc = static_cast<uint32_t>(atoi(argv[++i]));
            assert(bpc > 0 && bpc < 8);
        } else {
            std::cout << "Usage (.png files only):\n-i <input path>\n-o <output path>\n"
                         "-b <no bits/channel of output> range [1,7]\n";
            exit(0);
        }
    }
}

bool readImage(uint32_t& width, uint32_t& height, std::vector<Pixelu8>& pixels)
{
    std::vector<uint8_t> data;

    uint32_t error = lodepng::decode(data, width, height, inputPath);
    if (error) {
        std::cerr << "Error decoding file " << inputPath << ":\n" <<
                     lodepng_error_text(error) << std::endl;
        return false;
    }

    for (uint32_t i = 0; i < data.size(); i += 4) {
        Pixelu8 p;
        p.r = data[i];
        p.g = data[i + 1];
        p.b = data[i + 2];
        p.a = data[i + 3];
        pixels.push_back(p);
    }

    return true;
}

bool writeImage(const uint32_t& width, const uint32_t& height, std::vector<Pixelu8>& pixels)
{
    std::vector<uint8_t> data;

    for (uint32_t i = 0; i < pixels.size(); ++i) {
        const Pixelu8& p = pixels[i];
        data.push_back(p.r);
        data.push_back(p.g);
        data.push_back(p.b);
        data.push_back(p.a);
    }

    uint32_t error = lodepng::encode(outputPath, data, width, height);
    if (error) {
        std::cerr << "Error encoding file " << outputPath << ":\n" <<
                     lodepng_error_text(error) << std::endl;
        return false;
    }

    return true;
}

Pixelu8 quantize(const Pixelu8& pixel)
{
    const float step = 255.f / powf(2.f, bpc);

    Pixelu8 p;
    p.r = step * roundf(pixel.r / step);
    p.g = step * roundf(pixel.g / step);
    p.b = step * roundf(pixel.b / step);
    p.a = step * roundf(pixel.a / step);

    return p;
}

Pixelu8 clamp(const Pixeli32& pixel)
{
    Pixelu8 p;
    p.r = std::max(std::min(pixel.r, 255), 0);
    p.g = std::max(std::min(pixel.g, 255), 0);
    p.b = std::max(std::min(pixel.b, 255), 0);
    p.a = std::max(std::min(pixel.a, 255), 0);

    return p;
}

void floydSteinberg(const uint32_t& width, const uint32_t& height, std::vector<Pixelu8>& pixels)
{
    static const float qE  = 7.f / 16.f;
    static const float qSW = 3.f / 16.f;
    static const float qS  = 5.f / 16.f;
    static const float qSE = 1.f / 16.f;

    for (uint32_t y = 0; y < height; ++y) {
        const uint32_t yw  = y * width;
        const uint32_t ywo = (y + 1) * width;

        for (uint32_t x = 0; x < width; ++x) {
            const uint32_t C  = yw + x;
            const uint32_t E  = C + 1;
            const uint32_t S  = ywo + x;
            const uint32_t SW = S - 1;
            const uint32_t SE = S + 1;

            Pixelu8 currentPixel = pixels[C];
            Pixelu8 newPixel = quantize(currentPixel);
            pixels[C]  = newPixel;

            if (y == height - 1)
                continue;

            Pixeli32 error = static_cast<Pixeli32>(currentPixel) - static_cast<Pixeli32>(newPixel);

            pixels[S] = clamp(static_cast<Pixeli32>(pixels[S]) + error * qS);

            if (x != 0)
                pixels[SW] = clamp(static_cast<Pixeli32>(pixels[SW]) + error * qSW);

            if (x != width - 1) {
                pixels[E]  = clamp(static_cast<Pixeli32>(pixels[E])  + error * qE);
                pixels[SE] = clamp(static_cast<Pixeli32>(pixels[SE]) + error * qSE);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    parseArgs(argc, argv);

    uint32_t width, height;
    std::vector<Pixelu8> pixels;

    if (!readImage(width, height, pixels)) return 0;

    floydSteinberg(width, height, pixels);

    writeImage(width, height, pixels);

    return 0;
}
