module;

export module Dragonfly.Generics:MetaFunctions;

namespace Dfl {
    template< typename T, typename U >
    constexpr bool SameType = false;

    template< typename T >
    constexpr bool SameType< T, T > = true;
}