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

#include "Dragonfly.hxx"

//#define DFL_XAML
//#undef DFL_POPUPS
//#include "DflWinRT.h"
#include <Windows.h>
#include <inspectable.h>

namespace DflUI = Dfl::UI;
//namespace WinUI = winrt::Windows::UI;
//namespace WinXaml = winrt::Windows::UI::Xaml;

DflUI::Dialog::Dialog(const Info& info) 
: Window(DflUI::Window::Info{
        .Resolution{ DflUI::Dialog::Width, DflUI::Dialog::Height },
        .View{ DflUI::Dialog::Width, DflUI::Dialog::Height },
        .DoFullscreen{ false },
        .DoVsync{ false },
        .Rate{ 60 },
        .Position{ 0, 0 },
        .WindowTitle{ info.Title },
        .HasTitleBar{ true },
        .Extends{ false },
        .hWindow{ nullptr }
    })
{
}


inline const DflUI::Dialog::Result DflUI::Dialog::GetResult() const noexcept {
    return DflUI::Dialog::Result::NotReady;
}