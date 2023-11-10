#pragma once

#include <chrono>

namespace esx {

	class Timer {
	public:
		Timer()
			:	mDeltaTime()
		{

		}

		~Timer() = default;

		void init() {
			mLastLoopTime = now();
		}

		void update() {
			auto& currentTime = now();
			mDeltaTime = currentTime - mLastLoopTime; 
			mLastLoopTime = currentTime;
		}

		double getDeltaTimeInSeconds() { return mDeltaTime.count() / 1000.0f; }

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> now() { return std::chrono::high_resolution_clock::now(); }

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> mLastLoopTime;
		std::chrono::duration<double, std::milli> mDeltaTime;
	};

}