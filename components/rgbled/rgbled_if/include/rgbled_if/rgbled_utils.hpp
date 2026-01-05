#pragma once

#include <vector>
#include "rgbled_if.hpp"

namespace RgbLed
{

enum class Color {
    Red = 0,
    Blue,
    Green,
    Black,
    White,
    Pink,
    Yellow,
    Purple,
    Cyan,
    Magenta,
    Olive,
    Navy,
    Gray,
    Orange,
    Brown,
    Teal,
    COLOR_COUNT
};

class RgbColorCreator{
    public:

        static constexpr RgbColor Create(Color color){
            switch (color)
            {
                case Color::Red: return {255, 0, 0};
                case Color::Blue: return {0, 0, 255};
                case Color::Green: return {0, 255, 0};
                case Color::Black: return {0, 0, 0};
                case Color::Yellow: return {255, 255, 0};
                case Color::White: return {255, 255, 255};
                case Color::Pink: return {255, 192, 203};
                case Color::Purple: return {153, 0, 153};
                case Color::Cyan: return {0, 255, 255};
                case Color::Magenta: return {255, 0, 255};
                case Color::Olive: return {128, 128, 0};
                case Color::Navy: return {0, 0, 128};
                case Color::Gray: return {128, 128, 128};
                case Color::Orange: return {255, 156, 0};
                case Color::Brown: return {165,42,42};
                case Color::Teal: return {0, 128, 128};

                default:
                    break;
            };

            return RgbColor();
        }

        template <size_t N>
        static constexpr void FillColorArray(std::array<RgbColor, N> & rgbColorArr, std::array<Color, N> & colorArr)
        {
            for (int i = 0; i < N; i++)
            {
                rgbColorArr[i] = Create(colorArr[i]);
            }
        }

};

}