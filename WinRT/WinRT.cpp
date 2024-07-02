// WinRT.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"

#include "DflWinRT.h"

#include "Dragonfly.WinRT.hxx"

// TODO: This is an example of a library function
void Dfl::Initialize()
{
    winrt::init_apartment();
}
