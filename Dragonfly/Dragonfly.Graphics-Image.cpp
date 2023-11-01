module;

#include <string>

module Dragonfly.Graphics:Image;

namespace DflGr = Dfl::Graphics;

DflGr::Image::Image(const std::string_view& file)
{
    this->path = file;
}

void DflGr::Image::Load()
{

}