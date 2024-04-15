#include "Controller.h"

#include "SIO.h"

namespace esx {



	Controller::Controller(ControllerType type)
		:	mType(type),
			mState(0xFFFF)
	{
		mPhase = CommunicationPhase::IDLo;
	}

	void Controller::mosi(U8 value)
	{
		mRX.Push(value);
		if (mRX.Size == 8) {

			switch (mPhase) {
				case CommunicationPhase::IDLo: 	mTX.Set((U16)mType); break;
				case CommunicationPhase::IDHi: 	mTX.Set(((U16)mType) >> 8); break;
				case CommunicationPhase::Data1: mTX.Set(mState); break;
				case CommunicationPhase::Data2: mTX.Set(mState >> 8); break;
			}

			mPhase = (CommunicationPhase)((mPhase + 1) % CommunicationPhase::Max);

			mMaster->dsr();
			mRX = {};
		}
	}

	U8 Controller::miso()
	{
		return mTX.Pop();
	}

}