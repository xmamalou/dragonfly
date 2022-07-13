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

#include "../include/vectors.h"

#include <stdlib.h>

void dflVectorPushing(DflVector* vector, int element)
{
    int* temp_data = realloc(vector->data, vector->size + sizeof(long)); // add one pointer-big memory to a temporary pointer 
    vector->size++;  
    vector->data = temp_data;
    vector->data[vector->size - 1] = element;
}

void dflVectorPopping(DflVector* vector)
{
    int* temp_data = realloc(vector->data, vector->size - sizeof(long)); // remove one pointer-big memory to a temporary pointer
    vector->size--;
    vector->data = temp_data;
}

void dflVectorExtending(DflVector* vector, int size)
{
    int* temp_data = realloc(vector->data, vector->size + (sizeof(long) * size));
    vector->size += size;
    vector->data = temp_data;
}

void dflVectorClearing(DflVector* vector)
{
    free(vector->data);
}