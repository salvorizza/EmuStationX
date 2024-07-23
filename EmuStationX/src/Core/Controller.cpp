#include "Controller.h"

#include "SIO.h"

namespace esx {



	Controller::Controller(ControllerType type)
		:	SerialDevice(SerialDeviceType::Controller),
			mType(type),
			mState(0xFFFF)
	{
		mPhase = CommunicationPhase::Addressing;
	}

	U8 Controller::receive(U8 value)
	{
		U8 tx = 0xFF;

		switch (mPhase) {
			case CommunicationPhase::Addressing: {
				if (value == 0x01) {
					mPhase = CommunicationPhase::Command;
					mSelected = ESX_TRUE;
					tx = (U8)mType;
				}
				else {
					mPhase = CommunicationPhase::NotSelected;
				}
				break;
			}

			case CommunicationPhase::Command: {
				mCommandResponse.emplace((U8)(((U16)mType) >> 8));
				mCommandResponse.emplace((U8)mState);
				mCommandResponse.emplace((U8)(mState >> 8));
				mPhase = CommunicationPhase::Data;
			}

			default: {
				if (!mCommandResponse.empty()) {
					tx = mCommandResponse.front();
					mCommandResponse.pop();
				}
			}
		}

		if (mSelected) {
			mMaster->dsr();
		}

		return tx;
	}

	void Controller::cs()
	{
		mPhase = CommunicationPhase::Addressing;
		mTX.Set(0xFF);
		mRX = {};
		mCommandResponse = {};
		mSelected = ESX_FALSE;
	}

}