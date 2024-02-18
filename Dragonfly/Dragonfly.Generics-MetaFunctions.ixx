module;

export module Dragonfly.Generics:MetaFunctions;

namespace Dfl {
    namespace Generics {
        template< typename T, typename U >
        constexpr bool SameType = false;

        template< typename T >
        constexpr bool SameType< T, T > = true;
    }
}