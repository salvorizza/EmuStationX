#include "CDROM.h"

namespace esx {

	CDROM::CDROM()
		: BusDevice("CDROM")
	{
		addRange(ESX_TEXT("Root"), 0x1F801800, BYTE(0x4), 0xFFFFFFFF);

	}

	CDROM::~CDROM()
	{
	}

	void CDROM::store(const StringView& busName, U32 address, U8 value)
	{
		ESX_CORE_LOG_ERROR("CDROM - Writing to address 0x{:08X} value 0x{:02X}", address, value);

		switch (address) {
			case 0x1F801800: {
				setIndexStatusRegister(value);
				break;
			}

			case 0x1F801801: {
				switch (CDROM_REG0.Index) {
					case 0x0: {
						command((CommandType)value);
						break;
					}
					default: {
						ESX_CORE_LOG_ERROR("CDROM - Writing to address 0x{:08X} with Index 0x{:02X} not implemented yet", address, CDROM_REG0.Index);
					}
				}
				break;
			}

			case 0x1F801802: {
				switch (CDROM_REG0.Index) {
					case 0x0: {
						pushParameter(value);
						break;
					}
					case 0x1: {
						setInterruptEnableRegister(CDROM_REG2,value);
						break;
					}
					default: {
						ESX_CORE_LOG_ERROR("CDROM - Writing to address 0x{:08X} with Index 0x{:02X} not implemented yet", address, CDROM_REG0.Index);
					}
				}
				break;
			}

			case 0x1F801803: {
				switch (CDROM_REG0.Index) {
					case 0x1: {
						setInterruptFlagRegister(CDROM_REG3,value);
						break;
					}
					default: {
						ESX_CORE_LOG_ERROR("CDROM - Writing to address 0x{:08X} with Index 0x{:02X} not implemented yet", address, CDROM_REG0.Index);
					}
				}
				break;
			}
		}

	}

	void CDROM::load(const StringView& busName, U32 address, U8& output)
	{
		ESX_CORE_LOG_ERROR("CDROM - Reading from address 0x{:08X}", address);

		switch (address) {
			case 0x1F801800: {
				output = getIndexStatusRegister();
				break;
			}

			case 0x1F801801: {
				output = popResponse();
				break;
			}

			case 0x1F801802: {
				output = popData();
				break;
			}

			case 0x1F801803: {
				switch (CDROM_REG0.Index) {
					case 0x1:
					case 0x3: {
						output = getInterruptFlagRegister(CDROM_REG3);
						break;
					}

					case 0x0:
					case 0x2: {
						output = getInterruptEnableRegister(CDROM_REG3);
						break;
					}
				}
				break;
			}
		}

	}

	void CDROM::command(CommandType command)
	{
		ESX_CORE_LOG_ERROR("CDROM - Command {}", (U8)command);
		switch (command) {
			case CommandType::Test: {
				U8 parameter = popParameter();

				switch (parameter) {
					case 0x20: {
						//PSX (PU-7)
						pushResponse(0x94);
						pushResponse(0x09);
						pushResponse(0x19);
						pushResponse(0xC0);
						CDROM_REG3 = 0x3; //INT3
						break;
					}
				}

				break;
			}
		}

		CDROM_REG0.ParameterFifoEmpty = ESX_TRUE;
		CDROM_REG0.ParameterFifoFull = ESX_FALSE;
	}

	void CDROM::setIndexStatusRegister(U8 value)
	{
		CDROM_REG0.Index = (value >> 0) & 0x3;
	}

	U8 CDROM::getIndexStatusRegister()
	{
		U8 result = 0;

		result |= CDROM_REG0.Index;
		result |= (CDROM_REG0.ADPCMFifoEmpty == ESX_TRUE ? 0 : 1) << 2;
		result |= (CDROM_REG0.ParameterFifoEmpty == ESX_TRUE ? 1 : 0) << 3;
		result |= (CDROM_REG0.ParameterFifoFull == ESX_TRUE ? 0 : 1) << 4;
		result |= (CDROM_REG0.ResponseFifoEmpty == ESX_TRUE ? 0 : 1) << 5;
		result |= (CDROM_REG0.DataFifoEmpty == ESX_TRUE ? 0 : 1) << 6;
		result |= (CDROM_REG0.CommandTransmissionBusy == ESX_TRUE ? 1 : 0) << 7;

		return result;
	}

	void CDROM::setInterruptEnableRegister(U8& REG, U8 value)
	{
		REG |= value & 0x1F;
	}

	U8 CDROM::getInterruptEnableRegister(U8 REG)
	{
		U8 result = 0;

		result |= (REG & 0x1F) << 0;

		return result;
	}

	void CDROM::setInterruptFlagRegister(U8& REG, U8 value)
	{
		REG &= ~value;
	}

	U8 CDROM::getInterruptFlagRegister(U8 REG)
	{
		return REG;
	}

	void CDROM::pushParameter(U8 value)
	{
		mParameters.push(value);
		CDROM_REG0.ParameterFifoEmpty = ESX_FALSE;
		CDROM_REG0.ParameterFifoFull = mParameters.size() == 16 ? ESX_TRUE : ESX_FALSE;
	}

	U8 CDROM::popParameter()
	{
		U8 value = mParameters.front();
		mParameters.pop();
		return value;
	}

	void CDROM::pushResponse(U8 value)
	{
		mResponse.push(value);
		CDROM_REG0.ResponseFifoEmpty = ESX_FALSE;
	}

	U8 CDROM::popResponse()
	{
		U8 value = 0;

		if (!mResponse.empty()) {
			value = mResponse.front();
			mResponse.pop();
		}

		if (mResponse.empty()) {
			CDROM_REG0.ResponseFifoEmpty = ESX_TRUE;
		}

		return value;
	}

	void CDROM::pushData(U8 value)
	{
		mData.push(value);
		CDROM_REG0.DataFifoEmpty = ESX_FALSE;
	}

	U8 CDROM::popData()
	{
		U8 value = 0;

		if (!mData.empty()) {
			value = mData.front();
			mData.pop();
		}

		if (mData.empty()) {
			CDROM_REG0.DataFifoEmpty = ESX_TRUE;
		}

		return value;
	}

}