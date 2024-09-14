#include "Scheduler.h"

#include "Utils/LoggingSystem.h"

namespace esx {

	U64 Scheduler::sProgressClock = 0;
	SchedulerEventContainer Scheduler::sEvents = {};
	SchedulerEventHandlerContainer Scheduler::sEventHandlers = {};
	BIT Scheduler::sStopCurrentEvent = ESX_FALSE;

	void Scheduler::ScheduleEvent(const SchedulerEvent& schedulerEvent)
	{
		auto it = std::find_if(sEvents.begin(), sEvents.end(), [&](const SchedulerEvent& ev) { return schedulerEvent.ClockTarget < ev.ClockTarget; });
		if (it != sEvents.end()) {
			sEvents.insert(it, schedulerEvent);
		} else {
			sEvents.push_back(schedulerEvent);
		}
	}

	void Scheduler::UnScheduleAllEvents(SchedulerEventType type)
	{
		std::erase_if(sEvents, [&](const SchedulerEvent& ev) { return ev.Type == type; });
	}

	Optional<SchedulerEvent> Scheduler::NextEventOfType(SchedulerEventType type)
	{
		Optional<SchedulerEvent> result = {};
		auto it = std::find_if(sEvents.begin(), sEvents.end(), [&](const SchedulerEvent& ev) { return ev.Type == type; });
		if (it != sEvents.end()) {
			result.emplace(*it);
		}
		return result;
	}

	const SchedulerEvent& Scheduler::NextEvent()
	{
		return sEvents.front();
	}

	void Scheduler::ExecuteEvent()
	{
		const SchedulerEvent& currentFront = sEvents.front();

		for (const SchedulerEventHandler& evHandler : sEventHandlers[currentFront.Type]) {
			evHandler(currentFront);
		}
	}

	void Scheduler::Progress()
	{

		const SchedulerEvent& currentFront = sEvents.front();

		if (currentFront.Type != currentFront.Type) {
			ESX_CORE_LOG_ERROR("Errore Fatale");
		}

		if (currentFront.Reschedule) {
			SchedulerEvent rescheduleEvent = currentFront;
			rescheduleEvent.ClockStart = rescheduleEvent.ClockTarget;
			rescheduleEvent.ClockTarget = rescheduleEvent.ClockStart + rescheduleEvent.RescheduleClocks;
			ScheduleEvent(rescheduleEvent);
		}

		sProgressClock = currentFront.ClockTarget;

		sEvents.pop_front();
	}

	void Scheduler::AddSchedulerEventHandler(SchedulerEventType type, const SchedulerEventHandler& handler)
	{
		sEventHandlers[type].push_back(handler);
	}

}