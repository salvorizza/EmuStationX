#pragma once

#include "Base/Base.h"

namespace esx {

	enum class SchedulerEventType {
		None,
		CDROMCommand,
		DMAChannelDone,
		GPUScanlineStart,
		GPUFrameStart,
		GPUStartHBlank,
		GPUEndHBlank,
		GPUStartVBlank,
		GPUEndVBlank,
		SPUSample,
		Timer0ReachTarget,
		Timer1ReachTarget,
		Timer2ReachTarget,
		Timer0ReachMax,
		Timer1ReachMax,
		Timer2ReachMax
	};

	struct SchedulerEvent {
		SchedulerEventType Type = SchedulerEventType::None;
		U64 ClockStart = 0;
		U64 ClockTarget = 0;
		BIT Reschedule = ESX_FALSE;
		U64 RescheduleClocks = 0;
		Vector<U8> UserData = {};

		template<typename T>
		void Write(const T& Data) {
			const U8* dataBytes = reinterpret_cast<const U8*>(&Data);
			for (I32 i = 0; i < sizeof(T); i++) {
				UserData.emplace_back(*(dataBytes + i));
			}
		}

		template<typename T>
		const T& Read() const {
			return *reinterpret_cast<const T*>(UserData.data());
		}
	};

	struct SchedulerCompare {
		bool operator()(const SchedulerEvent& l, const SchedulerEvent& r) const { return l.ClockTarget > r.ClockTarget || (l.ClockTarget == r.ClockTarget && static_cast<U8>(l.Type) > static_cast<U8>(r.Type)); }
	};

	using SchedulerEventHandler = Function<void(const SchedulerEvent&)>;
	using SchedulerEventContainer = Deque<SchedulerEvent>;
	using SchedulerEventHandlerContainer = UnorderedMap<SchedulerEventType, Vector<SchedulerEventHandler>>;

	class Scheduler {
	public:
		Scheduler() = delete;
		~Scheduler() = default;

		static void ScheduleEvent(const SchedulerEvent& schedulerEvent);
		static void UnScheduleAllEvents(SchedulerEventType type);
		static Optional<SchedulerEvent> NextEventOfType(SchedulerEventType type);
		static const SchedulerEvent& NextEvent();
		static BIT CurrentEventHasBeenStopped();
		static void ExecuteEvent();
		static void Progress();
		static void AddSchedulerEventHandler(SchedulerEventType type, const SchedulerEventHandler& handler);
	private:
		static U64 sProgressClock;
		static SchedulerEventContainer sEvents;
		static SchedulerEventHandlerContainer sEventHandlers;
		static BIT sStopCurrentEvent;

	};

}