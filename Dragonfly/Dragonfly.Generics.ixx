/*
   Copyright 2023 Christopher-Marios Mamaloukas

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

module;

#include <type_traits>

export module Dragonfly.Generics;

export import :MetaFunctions;

namespace Dfl {
    namespace Generics {
        class BitFlag;

        export template< typename I >
        concept Bitable = std::is_enum_v< I > || std::is_integral_v< I > || SameType< BitFlag, I >;

        export class BitFlag {
            unsigned int flag{ 0 };
        public:
                                BitFlag ( ) {}
            template< Bitable N >
                                BitFlag ( N num ) { this->flag = static_cast<unsigned int>(num); }

            // assignment

            template< Bitable N >
                  BitFlag&      operator= ( const N num ) { this->flag = static_cast<unsigned int>(num); return *this; }
            template< Bitable N >
                  BitFlag&      operator|= ( const N num ) { this->flag |= static_cast<unsigned int>(num); return *this; }
            template< Bitable N >
                  BitFlag&      operator&= ( const N num ) { this->flag &= static_cast<unsigned int>(num); return *this; }

            // bitwise

            template< Bitable N >
                  BitFlag       operator| ( const N num ) const { return BitFlag(this->flag | static_cast<unsigned int>(num)); }
            template< Bitable N >
                  BitFlag       operator& ( const N num ) const { return BitFlag(this->flag & static_cast<unsigned int>(num)); }

            // conversions

                                operator       unsigned int () { return this->flag; }

             const unsigned int GetValue() const { return this->flag; }
        };
    }
}