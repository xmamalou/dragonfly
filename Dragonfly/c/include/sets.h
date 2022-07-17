/*Copyright 2022 Christopher-Marios Mamaloukas

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or other materials 
provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

/*! \file sets.h
    \brief Dynamic arrays.

    sets.h (in dragonfly_engine/c/include) contains the definition of a data type similar
    to C++'s Vector type; a dynamic array type. Was formerly known as vector.h, but changed
    name so the name vector can be reserved for ordered sets of numbers (like actual 
    mathematical vectors). Sets here will store strings (character arrays)
*/

#pragma once
#ifndef SETS_H
#define SETS_H

/*
* Many functions return an error code in the form of a number.
* 0 = no error
* Any other code is unique to the error.
* The Haskell binding should store the error code in a variable
* and act accordingly.
*
* Void functions won't return anything.
*/

/*
* All functions in this library will start with 
* dfl (from dragonfly) and will have a noun and a gerund 
* that briefly descibe the funtion.
* 
* Functions and variables will follow the Haskell paradigm
* of having a small initial letter.
* CamelCase is used for both.
* Global variables will be prefixed with dfl
* Scope specific variables won't be prefixed with that.
* You will know whether a name refers to a variable or a function
* if it ends with a gerund (then it's a function) or a noun (then it's a variable).
* Boolean variables will be a participle and always of the type 'char'.
*
* Structures and Unions will follow the Haskell paradigm of data types
* having one initial capital letter.
* CamelCase will also be used for them.
*
* Macros will use snake_case and all capitals.
* Macros will mostly be used for error codes.
* Enums will also be written like this.
*/

// TYPE DEFINITION
/*!
* @brief DflSets (formerly DflVectors) are an implementation of dynamic string arrays 
* that are aware of their size. 
*
* @param data the data of the set. Can store character 
* arrays (strings).
* @param size the size of the set
*/
typedef struct DflSets {
    const char** data; // strings that are stored
    unsigned int size; // size of set (NOT the strings)
} DflSet;

// FUNCTIONS
/*! @brief Adds an element to the end of the set.
*
* @param Set the Set to add the element to.
* @param element the element to add to the Set.
*
* @since This function exists since Dragonfly 0.0.1.1.
*/
void dflSetPushing(DflSet* set, const char* element);
/*! @brief Removes an element from the end of a Set.
*
* @param Set the Set to remove the element from.
*
* @since This function exists since Dragonfly 0.0.1.1.
*/
void dflSetPopping(DflSet* set);
/*! @brief Extends (or shrinks, if size < 0) a Set by as many 
* spaces as specified by size.
*
* @param set the Set to extend (or shrink).
* @param times the amount of spaces to add to the Set.
*
* @since This function exists since Dragonfly 0.0.1.1.
*/
void dflSetExtending(DflSet* set, int times);
/*! @brief Clears a Set.
*
* @param set the set to clear.
*
* @since This function exists since Dragonfly 0.0.1.1.
*/
void dflSetClearing(DflSet* set);

#endif 