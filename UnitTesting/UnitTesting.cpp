#include "pch.h"
#include "CppUnitTest.h"

import Dragonfly;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTesting
{
	TEST_CLASS(UnitTesting)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			Dfl::Hardware::SessionInfo info;
			Dfl::Hardware::Session session(info);
			Assert::AreEqual(Dfl::Hardware::SessionError::Success, session.InitVulkan());
		}
	};
}
