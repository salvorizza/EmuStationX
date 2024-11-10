#include "Controller.h"

#include "SIO.h"

namespace esx {



	Controller::Controller(ControllerType type)
		:	SerialDevice(SerialDeviceType::Controller),
			mType(type),
			mState(0xFFFF)
	{
		mPhase = CommunicationPhase::Addressing;
		mMode = ControllerMode::Normal;
	}

	U8 Controller::receive(U8 value)
	{
		U8 tx = 0xFF;
		BIT acknowledge = ESX_TRUE;

		switch (mPhase) {
			case CommunicationPhase::Addressing: {
				if (value == 0x01) {
					mUpdateFunction(this);
					mPhase = CommunicationPhase::Command;
					mSelected = ESX_TRUE;
					tx = (mMode == ControllerMode::Normal) ? static_cast<U8>(mType) : static_cast<U8>(ControllerType::ConfigMode);
				}
				else {
					mPhase = CommunicationPhase::NotSelected;
					mSelected = ESX_FALSE;
				}
				break;
			}

			case CommunicationPhase::GetVariableResponseAii: {
				U8 cc = 0, dd = 0, ee = 0, ff = 0;
				switch (value) {
					case 0x00: {
						cc = 0x01;
						dd = 0x02;
						ee = 0x00;
						ff = 0x0A;
						break;
					}
					case 0x01: {
						cc = 0x01;
						dd = 0x01;
						ee = 0x01;
						ff = 0x14;
						break;
					}
					default: {
						cc = 0x00;
						dd = 0x00;
						ee = 0x00;
						ff = 0x00;
						break;
					}
				}
				tx = 0x00;
				mCommandResponse.emplace(0x00);
				mCommandResponse.emplace(cc);
				mCommandResponse.emplace(dd);
				mCommandResponse.emplace(ee);
				mCommandResponse.emplace(ff);

				mPhase = CommunicationPhase::Data;
				break;
			}

			case CommunicationPhase::GetVariableResponseBii: {
				U8 dd = 0;
				switch (value) {
					case 0x00: {
						dd = 0x04;
						break;
					}
					case 0x01: {
						dd = 0x07;
						break;
					}
					default: {
						dd = 0x00;
						break;
					}
				}
				tx = 0x00;
				mCommandResponse.emplace(0x00);
				mCommandResponse.emplace(0x00);
				mCommandResponse.emplace(dd);  //dd
				mCommandResponse.emplace(0x00);
				mCommandResponse.emplace(0x00);

				mPhase = CommunicationPhase::Data;
				break;
			}

			case CommunicationPhase::Command: {
				mCurrentCommand = static_cast<ControllerCommand>(value);

				tx = (mMode == ControllerMode::Normal) ? U8(U16(mType) >> 8) : U8(U16(ControllerType::ConfigMode) >> 8);
				mPhase = CommunicationPhase::Data;

				switch (mCurrentCommand) {
					case ControllerCommand::Read: {
						//ESX_CORE_LOG_INFO("ControllerCommand::Read");

						mCommandResponse.emplace(mState & 0xFF);
						mCommandResponse.emplace((mState >> 8) & 0xFF);

						if (mMode == ControllerMode::Configuration) {
							mCommandResponse.emplace(0x00);
							mCommandResponse.emplace(0x00);
							mCommandResponse.emplace(0x00);
							mCommandResponse.emplace(0x00);
						}
						break;
					}

					default: {
						ESX_CORE_LOG_TRACE("Controller - Unknwon command {:02x}h", value);
						tx = 0xFF;
						acknowledge = ESX_FALSE;
						break;
					}

					/*
					* TODO: Analog stuff
					case ControllerCommand::EnterExitConfigurationMode: {
						ESX_CORE_LOG_INFO("ControllerCommand::EnterExitConfigurationMode");

						if (mMode == ControllerMode::Normal) {
							mCommandResponse.emplace(mState & 0xFF);
							mCommandResponse.emplace((mState >> 8) & 0xFF);
						} else {
							mCommandResponse.emplace(0x00);
							mCommandResponse.emplace(0x00);
						}

						mMode = (mMode == ControllerMode::Normal) ? ControllerMode::Configuration : ControllerMode::Normal;
						break;
					}

					case ControllerCommand::GetLEDState: {
						ESX_CORE_LOG_INFO("ControllerCommand::GetLEDState");

						mCommandResponse.emplace(0x00); //Typ
						mCommandResponse.emplace(0x02);
						mCommandResponse.emplace(0x00); //Led
						mCommandResponse.emplace(0x02);
						mCommandResponse.emplace(0x01);
						mCommandResponse.emplace(0x00);
						break;
					}

					case ControllerCommand::GetWhateverValues: {
						ESX_CORE_LOG_INFO("ControllerCommand::GetWhateverValues");

						mCommandResponse.emplace(0x00);
						mCommandResponse.emplace(0x00);
						mCommandResponse.emplace(0x02);
						mCommandResponse.emplace(0x00);
						mCommandResponse.emplace(0x01);
						mCommandResponse.emplace(0x00);
						break;
					}

					case ControllerCommand::GetVariableResponseA: {
						ESX_CORE_LOG_INFO("ControllerCommand::GetVariableResponseA");

						mPhase = CommunicationPhase::GetVariableResponseAii;

						break;
					}

					case ControllerCommand::GetVariableResponseB: {
						ESX_CORE_LOG_INFO("ControllerCommand::GetVariableResponseB");

						mPhase = CommunicationPhase::GetVariableResponseBii;
						break;
					}

					default: {
						ESX_CORE_LOG_ERROR("Controller Command: {} not implememted yet", char(value));
						break;
					}
					*/
				}

				break;
			}

			default: {
				if (!mCommandResponse.empty()) {
					tx = mCommandResponse.front();
					mCommandResponse.pop();
				}
			}
		}

		if (mSelected && acknowledge) {
			mMaster->dsr();
		}

		return tx;
	}

	void Controller::cs()
	{
		mCurrentCommand = ControllerCommand::None;
		mPhase = CommunicationPhase::Addressing;
		mTX.Set(0xFF);
		mRX = {};
		mCommandResponse = {};
		mSelected = ESX_FALSE;
	}

}