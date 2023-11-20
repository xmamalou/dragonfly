#include "pch.h"
#include "CppUnitTest.h"

#include "Specialization.hpp"

import Dragonfly;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTesting
{
	TEST_CLASS(WindowTesting)
	{
	public:
		TEST_METHOD(WindowCreationTest)
		{
			Dfl::Observer::WindowInfo info;
			info.Resolution = { 1000, 1000 };

			Dfl::Observer::Window window(info);
			Assert::AreEqual(Dfl::Observer::WindowError::Success, window.OpenWindow());
		}
	};

	TEST_CLASS(SessionTesting)
	{
	public:
		TEST_METHOD(SessionCreationTest)
		{
			Dfl::Hardware::SessionInfo sesInfo = {
				.AppName{ "My super app" },
				.AppVersion{ Dfl::MakeVersion(0, 0, 0) },
				.DoDebug{ true }
			};

			Dfl::Hardware::Session session(sesInfo);
			Assert::AreEqual(Dfl::Hardware::SessionError::Success, session.InitVulkan());
		}

		TEST_METHOD(SessionDeviceOutOfBounds)
		{
			Dfl::Hardware::SessionInfo sesInfo = {
				.AppName{ "My super app" },
				.AppVersion{ Dfl::MakeVersion(1000, 1000, 1000) },
				.DoDebug{ true }
			};

			Dfl::Hardware::Session session(sesInfo);

			session.InitVulkan();

			Dfl::Hardware::GPUInfo gpuInfo = {
				.DeviceIndex{ 1000}
			};

			Assert::AreEqual(Dfl::Hardware::SessionError::VkDeviceIndexOutOfRangeError, session.InitDevice(gpuInfo));
		}
	};
}
