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
#include <memory>
#include <optional>
#include <array>

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

        export template< typename T, uint32_t NodeNumber >
        class Tree {
                  T        NodeValue;

                  Tree*    Nodes[NodeNumber];
                  uint32_t CurrentEmptyNode{ 0 };
            const uint32_t CurrentDepth{ 0 };

                     Tree(T value, uint32_t depth) : NodeValue(value), CurrentDepth(depth) { }
        public:
                     Tree(T value) : NodeValue(value) { }
                     ~Tree() { for(uint32_t i = 0; i < this->CurrentEmptyNode; i++) { delete this->Nodes[i]; } }

                     operator T () { return this->NodeValue; }

            Tree&    operator[] (const uint32_t node) { if(node < NodeNumber ) { return this->Nodes[node] != nullptr ? *this->Nodes[node] : *this; } else { return *this; } }
            T&       operator= (const uint32_t value) { this->NodeValue = value; return this->NodeValue; }

            uint32_t GetDepth() const { return this->CurrentDepth; }
            T        GetNodeValue() const { return this->NodeValue; }
            bool     HasBranch(uint32_t position) const { return position < NodeNumber ? (position < this->CurrentEmptyNode ? true : false ) : false; }

            void     MakeBranch(const T& value) { if(this->CurrentEmptyNode < NodeNumber) { this->Nodes[this->CurrentEmptyNode] = new Tree(value, this->CurrentDepth + 1); this->CurrentEmptyNode++; } }
        };

        export template< typename T >
        using LinkedList = Tree<T, 1>;

        export template< typename T >
        using BinaryTree = Tree<T, 2>;
    }
}