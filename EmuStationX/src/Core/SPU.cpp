#include "SPU.h"

namespace esx {



	SPU::SPU()
		: BusDevice("SPU")
	{
		addRange("Root", 0x1F801C00, BYTE(640), 0xFFFFFFFF);
		
	}

	SPU::~SPU()
	{
	}

	void SPU::write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize)
	{
		ESX_CORE_LOG_WARNING("Writing to SPU Address 0x{:8X} not handled yet", address);
		if (address >= VOICE_VOLUME_ADDRESS && address < MAIN_VOLUME_ADDRESS) {
			if ((address & VOICE_VOLUME_ADDRESS) == VOICE_VOLUME_ADDRESS) {
				uint32_t key = (address & ~VOICE_VOLUME_ADDRESS) / 0x10;
				mIORegisters.VOICE_VOLUME[key] = value;
			} else if ((address & VOICE_ADPCM_SAMPLE_RATE_ADDRESS) == VOICE_ADPCM_SAMPLE_RATE_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADPCM_SAMPLE_RATE_ADDRESS) / 0x10;
				mIORegisters.VOICE_ADPCM_SAMPLE_RATE[key] = value;
			} else if ((address & VOICE_ADPCM_START_ADRESS_ADDRESS) == VOICE_ADPCM_START_ADRESS_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADPCM_START_ADRESS_ADDRESS) / 0x10;
				mIORegisters.VOICE_ADPCM_START_ADRESS[key] = value;
			} else if ((address & VOICE_ADSR_ADDRESS) == VOICE_ADSR_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADSR_ADDRESS) / 0x10;
				mIORegisters.VOICE_ADSR[key] = value;
			} else if ((address & VOICE_ADSR_CURRENT_VOLUME_ADDRESS) == VOICE_ADSR_CURRENT_VOLUME_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADSR_CURRENT_VOLUME_ADDRESS) / 0x10;
				mIORegisters.VOICE_ADSR_CURRENT_VOLUME[key] = value;
			} else if ((address & VOICE_ADPCM_REPEAT_ADRESS_ADDRESS) == VOICE_ADPCM_REPEAT_ADRESS_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADPCM_REPEAT_ADRESS_ADDRESS) / 0x10;
				mIORegisters.VOICE_ADPCM_REPEAT_ADRESS[key] = value;
			}
		} else if (address >= VOICE_CURRENT_VOLUME_ADDRESS && address < UNKNOWN_3_ADDRESS) {
			uint32_t key = (address & 0xFF) / 0x04;
			mIORegisters.VOICE_CURRENT_VOLUME[key] = value;
		} else if (address >= UNKNOWN_3_ADDRESS && address < UNKNOWN_4_ADDRESS) {
			memcpy(&mIORegisters.UNKNOWN_3[address - UNKNOWN_3_ADDRESS], &value, valueSize);
		} else if (address >= UNKNOWN_4_ADDRESS) {
			memcpy(&mIORegisters.UNKNOWN_4[address - UNKNOWN_4_ADDRESS], &value, valueSize);
		} else {
			switch (address & (~0x3)) {
				case MAIN_VOLUME_ADDRESS: {
					mIORegisters.MAIN_VOLUME = value;
					break;
				}
				case REVERB_OUTPUT_VOLUME_ADDRESS: {
					mIORegisters.REVERB_OUTPUT_VOLUME = value;
					break;
				}
				case VOICE_KEY_ON_ADDRESS: {
					mIORegisters.VOICE_KEY_ON = value;
					break;
				}
				case VOICE_KEY_OFF_ADDRESS: {
					mIORegisters.VOICE_KEY_OFF = value;
					break;
				}
				case VOICE_CHANNEL_FM_ADDRESS: {
					mIORegisters.VOICE_CHANNEL_FM = value;
					break;
				}
				case VOICE_CHANNEL_NOISE_MODE_ADDRESS: {
					mIORegisters.VOICE_CHANNEL_NOISE_MODE = value;
					break;
				}
				case VOICE_CHANNEL_REVERB_MODE_ADDRESS: {
					mIORegisters.VOICE_CHANNEL_REVERB_MODE = value;
					break;
				}
				case VOICE_CHANNEL_STATUS_ADDRESS: {
					mIORegisters.VOICE_CHANNEL_STATUS = value;
					break;
				}
				case CD_VOLUME_ADDRESS: {
					mIORegisters.CD_VOLUME = value;
					break;
				}
				case EXTERN_VOLUME_ADDRESS: {
					mIORegisters.EXTERN_VOLUME = value;
					break;
				}
				case CURRENT_MAIN_VOLUME_ADDRESS: {
					mIORegisters.CURRENT_MAIN_VOLUME = value;
					break;
				}
				case UNKNOWN_2_ADDRESS: {
					mIORegisters.UNKNOWN_2 = value;
					break;
				}
				case REVERB_SAME_SIDE_REFLECTION_ADDRESS_1_ADDRESS:{
					mIORegisters.REVERB_SAME_SIDE_REFLECTION_ADDRESS_1 = value;
					break;
				}
				case REVERB_COMB_ADDRESS_1_ADDRESS:{
					mIORegisters.REVERB_COMB_ADDRESS_1 = value;
					break;
				}
				case REVERB_COMB_ADDRESS_2_ADDRESS:{
					mIORegisters.REVERB_COMB_ADDRESS_2 = value;
					break;
				}
				case REVERB_SAME_SIDE_REFLECTION_ADDRESS_2_ADDRESS:{
					mIORegisters.REVERB_SAME_SIDE_REFLECTION_ADDRESS_2 = value;
					break;
				}
				case REVERB_DIFFERENT_SIDE_REFLECTION_ADDRESS_1_ADDRESS:{
					mIORegisters.REVERB_DIFFERENT_SIDE_REFLECTION_ADDRESS_1 = value;
					break;
				}
				case REVERB_COMB_ADDRESS_3_ADDRESS:{
					mIORegisters.REVERB_COMB_ADDRESS_3 = value;
					break;
				}
				case REVERB_COMB_ADDRESS_4_ADDRESS:{
					mIORegisters.REVERB_COMB_ADDRESS_4 = value;
					break;
				}
				case REVERB_DIFFERENT_SIDE_REFLECTION_ADDRESS2_ADDRESS:{
					mIORegisters.REVERB_DIFFERENT_SIDE_REFLECTION_ADDRESS2 = value;
					break;
				}
				case REVERB_APF_ADDRESS_1_ADDRESS:{
					mIORegisters.REVERB_APF_ADDRESS_1 = value;
					break;
				}
				case REVERB_APF_ADDRESS_2_ADDRESS:{
					mIORegisters.REVERB_APF_ADDRESS_2 = value;
					break;
				}
				case REVERB_INPUT_VOLUME_ADDRESS: {
					mIORegisters.REVERB_INPUT_VOLUME = value;
					break;
				}
				default: {
					switch (address & (~0x1)) {
						case UNKNOWN_1_ADDRESS: {
							mIORegisters.UNKNOWN_1 = value;
							break;
						}
						case SOUND_RAM_REVERB_START_ADDRESS_ADDRESS: {
							mIORegisters.SOUND_RAM_REVERB_START_ADDRESS = value;
							break;
						}
						case SOUND_RAM_IRQ_ADDRESS_ADDRESS: {
							mIORegisters.SOUND_RAM_IRQ_ADDRESS = value;
							break;
						}
						case SOUND_RAM_DATA_TRANSFER_ADDRESS_ADDRESS: {
							mIORegisters.SOUND_RAM_DATA_TRANSFER_ADDRESS = value;
							break;
						}
						case SOUND_RAM_DATA_TRANSFER_FIFO_ADDRESS: {
							mIORegisters.SOUND_RAM_DATA_TRANSFER_FIFO = value;
							break;
						}
						case SPU_CONTROL_REGISTER_ADDRESS: {
							mIORegisters.SPU_CONTROL_REGISTER = value;
							break;
						}
						case SOUND_RAM_DATA_TRANSFER_CONTROL_ADDRESS: {
							mIORegisters.SOUND_RAM_DATA_TRANSFER_CONTROL = value;
							break;
						}
						case SPU_STATUS_REGISTER_ADDRESS: {
							mIORegisters.SPU_STATUS_REGISTER = value;
							break;
						}
						case REVERB_APF_OFFSET_1_ADDRESS: {
							mIORegisters.REVERB_APF_OFFSET_1 = value;
							break;
						}
						case REVERB_APF_OFFSET_2_ADDRESS: {
							mIORegisters.REVERB_APF_OFFSET_2 = value;
							break;
						}
						case REVERB_REFLECTION_VOLUME_1_ADDRESS: {
							mIORegisters.REVERB_REFLECTION_VOLUME_1 = value;
							break;
						}
						case REVERB_COMB_VOLUME_1_ADDRESS: {
							mIORegisters.REVERB_COMB_VOLUME_1 = value;
							break;
						}
						case REVERB_COMB_VOLUME_2_ADDRESS: {
							mIORegisters.REVERB_COMB_VOLUME_2 = value;
							break;
						}
						case REVERB_COMB_VOLUME_3_ADDRESS: {
							mIORegisters.REVERB_COMB_VOLUME_3 = value;
							break;
						}
						case REVERB_COMB_VOLUME_4_ADDRESS: {
							mIORegisters.REVERB_COMB_VOLUME_4 = value;
							break;
						}
						case REVERB_REFLECTION_VOLUME_2_ADDRESS: {
							mIORegisters.REVERB_REFLECTION_VOLUME_2 = value;
							break;
						}
						case REVERB_APF_VOLUME_1_ADDRESS: {
							mIORegisters.REVERB_APF_VOLUME_1 = value;
							break;
						}
						case REVERB_APF_VOLUME_2_ADDRESS: {
							mIORegisters.REVERB_APF_VOLUME_2 = value;
							break;
						}
					}
					break;
				}
			}
		}
	}

	uint32_t SPU::read(const std::string& busName, uint32_t address, size_t outputSize)
	{
		ESX_CORE_LOG_WARNING("Reading to SPU Address 0x{:8X} not handled yet", address);

		uint32_t output = 0;

		if (address >= VOICE_VOLUME_ADDRESS && address < MAIN_VOLUME_ADDRESS) {
			if ((address & VOICE_VOLUME_ADDRESS) == VOICE_VOLUME_ADDRESS) {
				uint32_t key = (address & ~VOICE_VOLUME_ADDRESS) / 0x10;
				output = mIORegisters.VOICE_VOLUME[key];
			}
			else if ((address & VOICE_ADPCM_SAMPLE_RATE_ADDRESS) == VOICE_ADPCM_SAMPLE_RATE_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADPCM_SAMPLE_RATE_ADDRESS) / 0x10;
				output = mIORegisters.VOICE_ADPCM_SAMPLE_RATE[key];
			}
			else if ((address & VOICE_ADPCM_START_ADRESS_ADDRESS) == VOICE_ADPCM_START_ADRESS_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADPCM_START_ADRESS_ADDRESS) / 0x10;
				output = mIORegisters.VOICE_ADPCM_START_ADRESS[key];
			}
			else if ((address & VOICE_ADSR_ADDRESS) == VOICE_ADSR_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADSR_ADDRESS) / 0x10;
				output = mIORegisters.VOICE_ADSR[key];
			}
			else if ((address & VOICE_ADSR_CURRENT_VOLUME_ADDRESS) == VOICE_ADSR_CURRENT_VOLUME_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADSR_CURRENT_VOLUME_ADDRESS) / 0x10;
				output = mIORegisters.VOICE_ADSR_CURRENT_VOLUME[key];
			}
			else if ((address & VOICE_ADPCM_REPEAT_ADRESS_ADDRESS) == VOICE_ADPCM_REPEAT_ADRESS_ADDRESS) {
				uint32_t key = (address & ~VOICE_ADPCM_REPEAT_ADRESS_ADDRESS) / 0x10;
				output = mIORegisters.VOICE_ADPCM_REPEAT_ADRESS[key];
			}
		}
		else if (address >= VOICE_CURRENT_VOLUME_ADDRESS && address < UNKNOWN_3_ADDRESS) {
			uint32_t key = (address & 0xFF) / 0x04;
			output = mIORegisters.VOICE_CURRENT_VOLUME[key];
		}
		else if (address >= UNKNOWN_3_ADDRESS && address < UNKNOWN_4_ADDRESS) {
			memcpy(&output, &mIORegisters.UNKNOWN_3[address - UNKNOWN_3_ADDRESS], outputSize);
		}
		else if (address >= UNKNOWN_4_ADDRESS) {
			memcpy(&output, &mIORegisters.UNKNOWN_4[address - UNKNOWN_4_ADDRESS], outputSize);
		}
		else {
			switch (address & (~0x3)) {
				case MAIN_VOLUME_ADDRESS: {
					output = mIORegisters.MAIN_VOLUME;
					break;
				}
				case REVERB_OUTPUT_VOLUME_ADDRESS: {
					output = mIORegisters.REVERB_OUTPUT_VOLUME;
					break;
				}
				case VOICE_KEY_ON_ADDRESS: {
					output = mIORegisters.VOICE_KEY_ON;
					break;
				}
				case VOICE_KEY_OFF_ADDRESS: {
					output = mIORegisters.VOICE_KEY_OFF;
					break;
				}
				case VOICE_CHANNEL_FM_ADDRESS: {
					output = mIORegisters.VOICE_CHANNEL_FM;
					break;
				}
				case VOICE_CHANNEL_NOISE_MODE_ADDRESS: {
					output = mIORegisters.VOICE_CHANNEL_NOISE_MODE;
					break;
				}
				case VOICE_CHANNEL_REVERB_MODE_ADDRESS: {
					output = mIORegisters.VOICE_CHANNEL_REVERB_MODE;
					break;
				}
				case VOICE_CHANNEL_STATUS_ADDRESS: {
					output = mIORegisters.VOICE_CHANNEL_STATUS;
					break;
				}
				case CD_VOLUME_ADDRESS: {
					output = mIORegisters.CD_VOLUME;
					break;
				}
				case EXTERN_VOLUME_ADDRESS: {
					output = mIORegisters.EXTERN_VOLUME;
					break;
				}
				case CURRENT_MAIN_VOLUME_ADDRESS: {
					output = mIORegisters.CURRENT_MAIN_VOLUME;
					break;
				}
				case UNKNOWN_2_ADDRESS: {
					output = mIORegisters.UNKNOWN_2;
					break;
				}
				case REVERB_SAME_SIDE_REFLECTION_ADDRESS_1_ADDRESS: {
					output = mIORegisters.REVERB_SAME_SIDE_REFLECTION_ADDRESS_1;
					break;
				}
				case REVERB_COMB_ADDRESS_1_ADDRESS: {
					output = mIORegisters.REVERB_COMB_ADDRESS_1;
					break;
				}
				case REVERB_COMB_ADDRESS_2_ADDRESS: {
					output = mIORegisters.REVERB_COMB_ADDRESS_2;
					break;
				}
				case REVERB_SAME_SIDE_REFLECTION_ADDRESS_2_ADDRESS: {
					output = mIORegisters.REVERB_SAME_SIDE_REFLECTION_ADDRESS_2;
					break;
				}
				case REVERB_DIFFERENT_SIDE_REFLECTION_ADDRESS_1_ADDRESS: {
					output = mIORegisters.REVERB_DIFFERENT_SIDE_REFLECTION_ADDRESS_1;
					break;
				}
				case REVERB_COMB_ADDRESS_3_ADDRESS: {
					output = mIORegisters.REVERB_COMB_ADDRESS_3;
					break;
				}
				case REVERB_COMB_ADDRESS_4_ADDRESS: {
					output = mIORegisters.REVERB_COMB_ADDRESS_4;
					break;
				}
				case REVERB_DIFFERENT_SIDE_REFLECTION_ADDRESS2_ADDRESS: {
					output = mIORegisters.REVERB_DIFFERENT_SIDE_REFLECTION_ADDRESS2;
					break;
				}
				case REVERB_APF_ADDRESS_1_ADDRESS: {
					output = mIORegisters.REVERB_APF_ADDRESS_1;
					break;
				}
				case REVERB_APF_ADDRESS_2_ADDRESS: {
					output = mIORegisters.REVERB_APF_ADDRESS_2;
					break;
				}
				case REVERB_INPUT_VOLUME_ADDRESS: {
					output = mIORegisters.REVERB_INPUT_VOLUME;
					break;
				}
				default: {
					switch (address & (~0x1)) {
						case UNKNOWN_1_ADDRESS: {
							output = mIORegisters.UNKNOWN_1;
							break;
						}
						case SOUND_RAM_REVERB_START_ADDRESS_ADDRESS: {
							output = mIORegisters.SOUND_RAM_REVERB_START_ADDRESS;
							break;
						}
						case SOUND_RAM_IRQ_ADDRESS_ADDRESS: {
							output = mIORegisters.SOUND_RAM_IRQ_ADDRESS;
							break;
						}
						case SOUND_RAM_DATA_TRANSFER_ADDRESS_ADDRESS: {
							output = mIORegisters.SOUND_RAM_DATA_TRANSFER_ADDRESS;
							break;
						}
						case SOUND_RAM_DATA_TRANSFER_FIFO_ADDRESS: {
							output = mIORegisters.SOUND_RAM_DATA_TRANSFER_FIFO;
							break;
						}
						case SPU_CONTROL_REGISTER_ADDRESS: {
							output = mIORegisters.SPU_CONTROL_REGISTER;
							break;
						}
						case SOUND_RAM_DATA_TRANSFER_CONTROL_ADDRESS: {
							output = mIORegisters.SOUND_RAM_DATA_TRANSFER_CONTROL;
							break;
						}
						case SPU_STATUS_REGISTER_ADDRESS: {
							output = mIORegisters.SPU_STATUS_REGISTER;
							break;
						}
						case REVERB_APF_OFFSET_1_ADDRESS: {
							output = mIORegisters.REVERB_APF_OFFSET_1;
							break;
						}
						case REVERB_APF_OFFSET_2_ADDRESS: {
							output = mIORegisters.REVERB_APF_OFFSET_2;
							break;
						}
						case REVERB_REFLECTION_VOLUME_1_ADDRESS: {
							output = mIORegisters.REVERB_REFLECTION_VOLUME_1;
							break;
						}
						case REVERB_COMB_VOLUME_1_ADDRESS: {
							output = mIORegisters.REVERB_COMB_VOLUME_1;
							break;
						}
						case REVERB_COMB_VOLUME_2_ADDRESS: {
							output = mIORegisters.REVERB_COMB_VOLUME_2;
							break;
						}
						case REVERB_COMB_VOLUME_3_ADDRESS: {
							output = mIORegisters.REVERB_COMB_VOLUME_3;
							break;
						}
						case REVERB_COMB_VOLUME_4_ADDRESS: {
							output = mIORegisters.REVERB_COMB_VOLUME_4;
							break;
						}
						case REVERB_REFLECTION_VOLUME_2_ADDRESS: {
							output = mIORegisters.REVERB_REFLECTION_VOLUME_2;
							break;
						}
						case REVERB_APF_VOLUME_1_ADDRESS: {
							output = mIORegisters.REVERB_APF_VOLUME_1;
							break;
						}
						case REVERB_APF_VOLUME_2_ADDRESS: {
							output = mIORegisters.REVERB_APF_VOLUME_2;
							break;
						}
					}
					break;
				}
			}
		}
		
		return output;
	}

}