#include "Bus.h"

#include <cassert>

namespace esx {

	

	Bus::Bus(const StringView& name)
		: mName(name)
	{
	}

	void Bus::writeLine(const StringView& lineName, BIT value)
	{
		for (const auto& [name, device] : mDevices) {
			device->writeLine(mName, lineName, value);
		}
	}

	void Bus::sortRanges()
	{
		mIntervalTree = buildIntervalTree(mRanges);

		for (const auto& [name, device] : mDevices) {
			device->init();
		}

		auto bios = getDevice("Bios");
		auto ram = getDevice("RAM");

		for (I32 pageIndex = 0; pageIndex < 128; pageIndex++) {
			U8* pointer = ram->getFastPointer((pageIndex * PageSize) & 0x1FFFFF);
			Span<U8> span = Span<U8>(pointer, pointer + PageSize);

			mPageTableR[pageIndex + 0x0000] = span;
			mPageTableR[pageIndex + 0x8000] = span;
			mPageTableR[pageIndex + 0xA000] = span;

			mPageTableW[pageIndex + 0x0000] = span;
			mPageTableW[pageIndex + 0x8000] = span;
			mPageTableW[pageIndex + 0xA000] = span;
		}

		for (I32 pageIndex = 0; pageIndex < 8; pageIndex++) {
			U8* pointer = bios->getFastPointer(pageIndex * PageSize);
			Span<U8> span = Span<U8>(pointer, pointer + PageSize);

			mPageTableR[pageIndex + 0x1FC0] = span;
			mPageTableR[pageIndex + 0x9FC0] = span;
			mPageTableR[pageIndex + 0xBFC0] = span;
		}
	}

	void Bus::connectDevice(const SharedPtr<BusDevice>& device) {
		mDevices[device->getName()] = device;
	}

	void Bus::addRange(const StringView& deviceName, BusRange range)
	{
		mRanges.emplace_back(range, mDevices.at(deviceName));
	}

	void BusDevice::connectToBus(const SharedPtr<Bus>& pBus)
	{
		for (const BusRange& range : mStoredRanges) {
			pBus->addRange(mName, range);
		}
		mBusses[pBus->getName()] = pBus;
	}

	SharedPtr<Bus>& BusDevice::getBus(const StringView& busName)
	{
		return mBusses[busName];
	}

	void BusDevice::addRange(const StringView& busName, U64 start, U64 sizeInBytes, U64 mask)
	{
		mStoredRanges.emplace_back(start, sizeInBytes, mask);
	}

	IntervalTreeNode* Bus::buildIntervalTree(const Vector<Interval>& intervals) {
		if (intervals.empty()) return nullptr;

		Vector<Interval> sortedIntervals = intervals;
		std::sort(sortedIntervals.begin(), sortedIntervals.end(), [](const Interval& a, const Interval& b) {
			return a.first.Start < b.first.Start;
		});

		IntervalTreeNode* root = new IntervalTreeNode(sortedIntervals[intervals.size() / 2]);

		Vector<Interval> leftIntervals(sortedIntervals.begin(), sortedIntervals.begin() + intervals.size() / 2);
		Vector<Interval> rightIntervals(sortedIntervals.begin() + intervals.size() / 2 + 1, sortedIntervals.end());
		root->left = buildIntervalTree(leftIntervals);
		root->right = buildIntervalTree(rightIntervals);

		if (root->left) root->maxEnd = std::max(root->maxEnd, root->left->maxEnd);
		if (root->right) root->maxEnd = std::max(root->maxEnd, root->right->maxEnd);

		return root;
	}

	IntervalTreeNode* Bus::findRangeInIntervalTree(IntervalTreeNode* root, uint32_t address) {
		if (root == nullptr) return nullptr;

		if (root->maxEnd < address) {
			return nullptr;
		}

		if (address >= root->interval.first.Start && address <= root->interval.first.End) {
			return root;
		}
		
		if (root->left != nullptr && root->left->maxEnd >= address) {
			return findRangeInIntervalTree(root->left, address);
		}

		return findRangeInIntervalTree(root->right, address);
	}

}