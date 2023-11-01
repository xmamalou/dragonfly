#include <string>
#include <tuple>
#include <vector>

export module Dragonfly.Graphics:Image;

namespace Dfl
{
    namespace Graphics
    {
        class Image
        {
            std::string_view               path;
            std::tuple<uint64_t, uint64_t> size;

            std::vector<char>              data;

        public:
            Image(const std::string_view& file);

            void Load();

            std::vector<char> Data() const
            {
                return data;
            }
        };
    }
}