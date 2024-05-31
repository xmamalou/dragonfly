#pragma once

#include "winrt/base.h"
#include "winrt/Windows.System.h"
#include "winrt/Windows.Foundation.Collections.h"

#ifdef DFL_POPUPS
#include "winrt/windows.ui.popups.h"
#include "winrt/windows.ui.notifications.h"
#endif

#ifdef DFL_XAML
#include "winrt/windows.ui.xaml.hosting.h"
#include "winrt/Windows.UI.Xaml.Controls.h"
#include "winrt/Windows.UI.Xaml.Input.h"
#include "winrt/Windows.UI.Xaml.Media.h"
#include "winrt/Windows.UI.Xaml.Navigation.h"
#endif
