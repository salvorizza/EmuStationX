#include "SPU.h"

namespace esx {



	SPU::SPU()
		: BusDevice("SPU")
	{
		addRange("Root", 0x1F801C00, BYTE(641), 0xFFFFFFFF);
		
	}

	SPU::~SPU()
	{
	}

	void SPU::write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize)
	{
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
		}
		else {
			switch (address) {
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
				case REVERB_APF_OFFSET_1_ADDRESS:{
					mIORegisters.REVERB_APF_OFFSET_1 = value;
					break;
				}
				case REVERB_APF_OFFSET_2_ADDRESS:{
					mIORegisters.REVERB_APF_OFFSET_2 = value;
					break;
				}
				case REVERB_REFLECTION_VOLUME_1_ADDRESS:{
					mIORegisters.REVERB_REFLECTION_VOLUME_1 = value;
					break;
				}
				case REVERB_COMB_VOLUME_1_ADDRESS:{
					mIORegisters.REVERB_COMB_VOLUME_1 = value;
					break;
				}
				case REVERB_COMB_VOLUME_2_ADDRESS:{
					mIORegisters.REVERB_COMB_VOLUME_2 = value;
					break;
				}
				case REVERB_COMB_VOLUME_3_ADDRESS:{
					mIORegisters.REVERB_COMB_VOLUME_3 = value;
					break;
				}
				case REVERB_COMB_VOLUME_4_ADDRESS:{
					mIORegisters.REVERB_COMB_VOLUME_4 = value;
					break;
				}
				case REVERB_REFLECTION_VOLUME_2_ADDRESS:{
					mIORegisters.REVERB_REFLECTION_VOLUME_2 = value;
					break;
				}
				case REVERB_APF_VOLUME_1_ADDRESS:{
					mIORegisters.REVERB_APF_VOLUME_1 = value;
					break;
				}
				case REVERB_APF_VOLUME_2_ADDRESS:{
					mIORegisters.REVERB_APF_VOLUME_2 = value;
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
			}
		}
	}

	uint32_t SPU::read(const std::string& busName, uint32_t address, size_t outputSize)
	{
		uint32_t output = 0;

		
		return output;
	}

}