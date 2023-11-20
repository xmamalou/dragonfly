#include "pch.h"
#include "CppUnitTest.h"

import Dragonfly;

namespace Microsoft
{
    namespace VisualStudio
    {
        namespace CppUnitTestFramework
        {
            // Window specializations
            template<> static std::wstring ToString < Dfl::Observer::Window > (const class Dfl::Observer::Window& t) { return L"Window"; }
            template<> static std::wstring ToString < Dfl::Observer::WindowError >(const enum class Dfl::Observer::WindowError& t) { return L"Window Error"; }

            // Session specializations
            template<> static std::wstring ToString < Dfl::Hardware::Session > (const class Dfl::Hardware::Session& t) { return L"Session"; }
            template<> static std::wstring ToString < Dfl::Hardware::SessionError >(const enum class Dfl::Hardware::SessionError& t) { return L"Session Error"; }
        }
    }
}