#include "SPU.h"

#include "InterruptControl.h"

namespace esx {
	static U16 getVolume(const Volume& volume)
	{
		U16 value = 0;

		value |= (volume.Level << 0);
		value |= ((U8)volume.VolumeMode << 15);

		return value;
	}

	static void setVolume(Volume& volume, U16 value)
	{
		#define SIGNEXT16(x) (((x) & 0x4000) ? ((x) | 0x8000) : (x))

		volume.Level = SIGNEXT16(value & 0x7FFF);
		volume.VolumeMode = (VolumeMode)((value >> 15) & 0x1);

		volume.SweepStep = (value >> 0) & 0x3;
		volume.SweepShift = (value >> 2) & 0x1F;
		volume.SweepPhase = (SweepPhase)((value >> 12) & 0x1);
		volume.SweepDirection = (SPUDirection)((value >> 13) & 0x1);
		volume.SweepMode = (Mode)((value >> 14) & 0x1);
	}

	static U32 getStereoVolume(const StereoVolume& stereoVolume)
	{
		U32 value = 0;

		value |= (stereoVolume.Left << 0);
		value |= (stereoVolume.Right << 16);

		return value;
	}

	static void setStereoVolume(StereoVolume& stereoVolume, U32 value)
	{
		stereoVolume.Left = (value >> 0) & 0xFFFF;
		stereoVolume.Right = (value >> 16) & 0xFFFF;
	}

	SPU::SPU()
		: BusDevice(ESX_TEXT("SPU"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801C00, BYTE(640), 0xFFFFFFFF);
		mRAM.resize(KIBI(512));

		for (U32 i = 0; i < 24; i++) {
			mVoices[i].Number = i;
		}
	}

	SPU::~SPU()
	{
	}

	void SPU::clock(U64 clocks)
	{
		if (clocks % 768 == 0 && mSPUControl.Enable) {
			mSPUStatus.CDAudioEnable = mSPUControl.CDAudioEnable;
			mSPUStatus.ExternalAudioEnable = mSPUControl.ExternalAudioEnable;
			mSPUStatus.CDAudioReverb = mSPUControl.CDAudioReverb;
			mSPUStatus.ExternalAudioReverb = mSPUControl.ExternalAudioReverb;
			mSPUStatus.TransferMode = mSPUControl.TransferMode;

			if (mSPUStatus.TransferMode == TransferMode::ManualWrite && !mFIFO.empty()) {
				U16 value = mFIFO.front();
				mRAM[mCurrentTransferAddress++] = (U8)(value & 0xFF);
				mRAM[mCurrentTransferAddress++] = (U8)((value >> 8) & 0xFF);
				mFIFO.pop();
				mSPUStatus.DataTransferBusyFlag = !mFIFO.empty();
			}

			for (Voice& voice : mVoices) {
				if ((voice.ADPCMCurrentAddress >> 3) == mSoundRAMIRQAddress) {
					getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::SPU, ESX_FALSE, ESX_TRUE);
				}

				U8 currentBlockADPCMHeaderShiftFilter = mRAM[voice.ADPCMCurrentAddress & ~0xF];
				U8 currentBlockADPCMHeaderFlag = mRAM[(voice.ADPCMCurrentAddress & ~0xF) + 1];
				U8 currentBlockADPCMByte = mRAM[voice.ADPCMCurrentAddress++];

				if (currentBlockADPCMHeaderFlag & 0b100) {
					voice.ADPCMRepeatAddress = voice.ADPCMCurrentAddress;
				}

				if ((voice.ADPCMCurrentAddress & 0xF) > 0x1) {
					U8 firstSample = currentBlockADPCMByte & 0xF;
					U8 secondSample = (currentBlockADPCMByte >> 4) & 0xF;

				}

				if (voice.Number == 0) {
					ESX_CORE_LOG_TRACE("{:02X}h", currentBlockADPCMHeaderFlag);
				}

				if ((voice.ADPCMCurrentAddress & 0xF) == 0xF) {
					switch (currentBlockADPCMHeaderFlag & 0x3) {
						case 1: {
							voice.ADPCMCurrentAddress = voice.ADPCMRepeatAddress;
							voice.ReachedLoopEnd = ESX_TRUE;
							//Release
							//ADSR Envelope = 0x0000 Volume?
							break;
						}

						case 3: {
							voice.ADPCMCurrentAddress = voice.ADPCMRepeatAddress;
							voice.ReachedLoopEnd = ESX_TRUE;
							break;
						}
					}
				}
			}

		}
	}

	void SPU::store(const StringView& busName, U32 address, U16 value)
	{
		if (address < 0x1F801D80) {
			U32 voice = (address >> 4) & 0x3F;
			U32 registerAddress = address & ~0x3F0;
			switch (registerAddress) {
				case 0x1F801C00: {
					setVoiceVolumeLeft(voice, value);
					break;
				}
				case 0x1F801C02: {
					setVoiceVolumeRight(voice, value);
					break;
				}
				case 0x1F801C04: {
					setVoiceADPCMSampleRate(voice, value);
					break;
				}
				case 0x1F801C06: {
					setVoiceADPCMStartAddress(voice, value);
					break;
				}
				case 0x1F801C08: {
					setVoiceADSRLower(voice, value);
					break;
				}
				case 0x1F801C0A: {
					setVoiceADSRUpper(voice, value);
					break;
				}
				case 0x1F801C0C: {
					setVoiceCurrentADSRVolume(voice, value);
					break;
				}
				case 0x1F801C0E: {
					setVoiceADPCMRepeatAddress(voice, value);
					break;
				}
				default: {
					ESX_CORE_LOG_ERROR("SPU - Writing to address {:08x} not implemented yet", address);
				}
			}
		} else if (address < 0x1F801DC0) {
			switch (address) {
				case 0x1F801D80: {
					setMainVolumeLeft(value);
					break;
				}
				case 0x1F801D82: {
					setMainVolumeRight(value);
					break;
				}
				case 0x1F801D84: {
					setReverbVolumeLeft(value);
					break;
				}
				case 0x1F801D86: {
					setReverbVolumeRight(value);
					break;
				}
				case 0x1F801D88: {
					setVoicesOnLower(value);
					break;
				}
				case 0x1F801D8A: {
					setVoicesOnUpper(value);
					break;
				}
				case 0x1F801D8C: {
					setVoicesOffLower(value);
					break;
				}
				case 0x1F801D8E: {
					setVoicesOffUpper(value);
					break;
				}
				case 0x1F801D90: {
					setVoicesPitchModulationLower(value);
					break;
				}
				case 0x1F801D92: {
					setVoicesPitchModulationUpper(value);
					break;
				}
				case 0x1F801D94: {
					setNoiseModeLower(value);
					break;
				}
				case 0x1F801D96: {
					setNoiseModeUpper(value);
					break;
				}
				case 0x1F801D98: {
					setReverbModeLower(value);
					break;
				}
				case 0x1F801D9A: {
					setReverbModeUpper(value);
					break;
				}
				case 0x1F801DA0: {
					setUnknown1F801DA0(value);
					break;
				}
				case 0x1F801DA2: {
					setReverbWorkAreaStartAddress(value);
					break;
				}
				case 0x1F801DA4: {
					setSoundRAMIRQAddress(value);
					break;
				}
				case 0x1F801DA6: {
					setDataTransferAdress(value);
					break;
				}
				case 0x1F801DA8: {
					setDataTransferFifo(value);
					break;
				}
				case 0x1F801DAA: {
					setControlRegister(value);
					break;
				}
				case 0x1F801DAC: {
					setDataTransferControl(value);
					break;
				}
				case 0x1F801DB0: {
					setCDInputVolumeLeft(value);
					break;
				}
				case 0x1F801DB2: {
					setCDInputVolumeRight(value);
					break;
				}
				case 0x1F801DB4: {
					setExternalInputVolumeLeft(value);
					break;
				}
				case 0x1F801DB6: {
					setExternalInputVolumeRight(value);
					break;
				}
				case 0x1F801DB8: {
					setCurrentMainVolumeLeft(value);
					break;
				}
				case 0x1F801DBA: {
					setCurrentMainVolumeRight(value);
					break;
				}
				case 0x1F801DBC: {
					ESX_CORE_LOG_TRACE("SPU - Unknown write to address {:08x} not implemented yet", address);
					break;
				}
				case 0x1F801DBF: {
					ESX_CORE_LOG_TRACE("SPU - Unknown write to address {:08x} not implemented yet", address);
					break;
				}
				default: {
					ESX_CORE_LOG_ERROR("SPU - Writing to address {:08x} not implemented yet", address);
				}
			}
		} else if (address < 0x1F801E00) {
			ReverbRegister reverbRegister = (ReverbRegister)((address - 0x1F801DC0) / 2);
			setReverbConfigurationAreaRegister(reverbRegister, value);
		} else if (address < 0x1F801E60) {
			U8 voice = (address & 0xFF) >> 2;
			if (address & 0x1) {
				setVoiceCurrentVolumeRight(voice, value);
			} else {
				setVoiceCurrentVolumeLeft(voice, value);
			}
		} else {
			ESX_CORE_LOG_ERROR("SPU - Writing to address {:08x} not implemented yet", address);
		}
	}

	void SPU::load(const StringView& busName, U32 address, U16& output)
	{
		if (address < 0x1F801D80) {
			U32 voice = (address >> 4) & 0x3F;
			U32 registerAddress = address & ~0x3F0;
			switch (registerAddress) {
				case 0x1F801C00: {
					output = getVoiceVolumeLeft(voice);
					break;
				}
				case 0x1F801C02: {
					output = getVoiceVolumeRight(voice);
					break;
				}
				case 0x1F801C04: {
					output = getVoiceADPCMSampleRate(voice);
					break;
				}
				case 0x1F801C06: {
					output = getVoiceADPCMStartAddress(voice);
					break;
				}
				case 0x1F801C08: {
					output = getVoiceADSRLower(voice);
					break;
				}
				case 0x1F801C0A: {
					output = output = getVoiceADSRUpper(voice);
					break;
				}
				case 0x1F801C0C: {
					output = getVoiceCurrentADSRVolume(voice);
					break;
				}
				case 0x1F801C0E: {
					output = getVoiceADPCMRepeatAddress(voice);
					break;
				}
				default: {
					ESX_CORE_LOG_ERROR("SPU - Reading from address {:08x} not implemented yet", address);
				}
			}
		} else if (address < 0x1F801DC0) {
			switch (address) {
				case 0x1F801D80: {
					output = getMainVolumeLeft();
					break;
				}
				case 0x1F801D82: {
					output = getMainVolumeRight();
					break;
				}
				case 0x1F801D84: {
					output = getReverbVolumeLeft();
					break;
				}
				case 0x1F801D86: {
					output = getReverbVolumeRight();
					break;
				}
				case 0x1F801D88: {
					//output = getVoicesOnLower();
					break;
				}
				case 0x1F801D8A: {
					//output = getVoicesOnUpper();
					break;
				}
				case 0x1F801D8C: {
					//output = getVoicesOffLower();
					break;
				}
				case 0x1F801D8E: {
					//output = getVoicesOffUpper();
					break;
				}
				case 0x1F801D90: {
					//output = getVoicesPitchModulationLower();
					break;
				}
				case 0x1F801D92: {
					//output = getVoicesPitchModulationUpper();
					break;
				}
				case 0x1F801D94: {
					output = getNoiseModeLower();
					break;
				}
				case 0x1F801D96: {
					output = getNoiseModeUpper();
					break;
				}
				case 0x1F801D98: {
					output = getReverbModeLower();
					break;
				}
				case 0x1F801D9A: {
					output = getReverbModeUpper();
					break;
				}
				case 0x1F801DA0: {
					output = getUnknown1F801DA0();
					break;
				}
				case 0x1F801DA2: {
					output = getReverbWorkAreaStartAddress();
					break;
				}
				case 0x1F801DA4: {
					output = getSoundRAMIRQAddress();
					break;
				}
				case 0x1F801DA6: {
					output = getDataTransferAdress();
					break;
				}
				case 0x1F801DA8: {
					//output = getDataTransferFifo();
					break;
				}
				case 0x1F801DAA: {
					output = getControlRegister();
					break;
				}
				case 0x1F801DAC: {
					output = getDataTransferControl();
					break;
				}
				case 0x1F801DAE: {
					output = getStatusRegister();
					break;
				}
				case 0x1F801DB0: {
					output = getCDInputVolumeLeft();
					break;
				}
				case 0x1F801DB2: {
					output = getCDInputVolumeRight();
					break;
				}
				case 0x1F801DB4: {
					output = getExternalInputVolumeLeft();
					break;
				}
				case 0x1F801DB6: {
					output = getExternalInputVolumeRight();
					break;
				}
				case 0x1F801DB8: {
					output = getCurrentMainVolumeLeft();
					break;
				}
				case 0x1F801DBA: {
					output = getCurrentMainVolumeRight();
					break;
				}
				case 0x1F801DBC: {
					ESX_CORE_LOG_WARNING("SPU - Unknown Reading from address {:08x} not implemented yet", address);
					break;
				}
				case 0x1F801DBF: {
					ESX_CORE_LOG_WARNING("SPU - Unknown Reading from address {:08x} not implemented yet", address);
					break;
				}
				default: {
					ESX_CORE_LOG_WARNING("SPU - Unknown Reading from address {:08x} not implemented yet", address);
				}
			}
		} else if (address < 0x1F801E00) {
			ReverbRegister reverbRegister = (ReverbRegister)((address - 0x1F801DC0) / 2);
			output = getReverbConfigurationAreaRegister(reverbRegister);
		} else if (address < 0x1F801E60) {
			U8 voice = (address & 0xFF) >> 2;
			if (address & 0x1) {
				output = getVoiceCurrentVolumeRight(voice);
			} else {
				output = getVoiceCurrentVolumeLeft(voice);
			}
		} else {
			ESX_CORE_LOG_ERROR("SPU - Reading from address {:08x} not implemented yet", address);
		}
	}

	void SPU::startVoice(U8 voice)
	{
		mVoices[voice].ReachedLoopEnd = ESX_FALSE;
		mVoices[voice].ADSRCurrentVolume = 0x0000;
		mVoices[voice].ADPCMRepeatAddress = mVoices[voice].ADPCMStartAddress;
		mVoices[voice].ADPCMCurrentAddress = mVoices[voice].ADPCMStartAddress * 8;
		//Starts the ADSR Envelope, 
		//and automatically initializes ADSR Volume to zero, 
		//and copies Voice Start Address to Voice Repeat Address.
	}

	U16 SPU::getVoiceVolumeLeft(U8 voice)
	{
		return getVolume(mVoices[voice].VolumeLeft);
	}

	void SPU::setVoiceVolumeLeft(U8 voice, U16 value)
	{
		setVolume(mVoices[voice].VolumeLeft, value);
	}

	U16 SPU::getVoiceVolumeRight(U8 voice)
	{
		return getVolume(mVoices[voice].VolumeRight);
	}

	void SPU::setVoiceVolumeRight(U8 voice, U16 value)
	{
		setVolume(mVoices[voice].VolumeRight, value);
	}

	U16 SPU::getVoiceADPCMSampleRate(U8 voice)
	{
		return mVoices[voice].ADPCMSampleRate;
	}

	void SPU::setVoiceADPCMSampleRate(U8 voice, U16 value)
	{
		mVoices[voice].ADPCMSampleRate = value;
	}

	U16 SPU::getVoiceADPCMStartAddress(U8 voice)
	{
		return mVoices[voice].ADPCMStartAddress;
	}

	void SPU::setVoiceADPCMStartAddress(U8 voice, U16 value)
	{
		mVoices[voice].ADPCMStartAddress = value;
	}

	U32 SPU::getVoiceADSR(U8 voice)
	{
		U32 value = (getVoiceADSRUpper(voice) << 16) | getVoiceADSRLower(voice);
		return value;
	}

	void SPU::setVoiceADSR(U8 voice, U32 value)
	{
		setVoiceADSRLower(voice, value & 0xFFFF);
		setVoiceADSRUpper(voice, (value >> 16) & 0xFFFF);
	}

	U16 SPU::getVoiceADSRUpper(U8 voice)
	{
		U16 value = 0;

		value |= (mVoices[voice].ADSR.ReleaseShift << 0);
		value |= ((U8)mVoices[voice].ADSR.ReleaseMode << 5);
		value |= (mVoices[voice].ADSR.SustainStep << 6);
		value |= (mVoices[voice].ADSR.SustainShift << 8);
		value |= ((U8)mVoices[voice].ADSR.SustainDirection << 14);
		value |= ((U8)mVoices[voice].ADSR.SustainMode << 15);

		return value;
	}

	void SPU::setVoiceADSRUpper(U8 voice, U16 value)
	{
		mVoices[voice].ADSR.ReleaseStep = 0; // Fixed
		mVoices[voice].ADSR.ReleaseShift = (value >> 0) & 0xF;
		mVoices[voice].ADSR.ReleaseDirection = SPUDirection::Decrease; // Fixed
		mVoices[voice].ADSR.ReleaseMode = (Mode)((value >> 5) & 0x1);

		mVoices[voice].ADSR.SustainStep = (value >> 6) & 0x3;
		mVoices[voice].ADSR.SustainShift = (value >> 8) & 0xF;
		mVoices[voice].ADSR.SustainDirection = (SPUDirection)((value >> 14) & 0x1);
		mVoices[voice].ADSR.SustainMode = (Mode)((value >> 15) & 0x1);
	}

	U16 SPU::getVoiceADSRLower(U8 voice)
	{
		U16 value = 0;

		value |= (mVoices[voice].ADSR.SustainLevel << 0);
		value |= (mVoices[voice].ADSR.DecayShift << 4);
		value |= (mVoices[voice].ADSR.AttackStep << 8);
		value |= (mVoices[voice].ADSR.AttackShift << 10);
		value |= ((U8)mVoices[voice].ADSR.AttackMode << 15);

		return value;
	}

	void SPU::setVoiceADSRLower(U8 voice, U16 value)
	{
		mVoices[voice].ADSR.SustainLevel = (value >> 0) & 0xF;

		mVoices[voice].ADSR.DecayStep = 0; // Fixed
		mVoices[voice].ADSR.DecayShift = (value >> 4) & 0xF;
		mVoices[voice].ADSR.DecayDirection = SPUDirection::Decrease; // Fixed
		mVoices[voice].ADSR.DecayMode = Mode::Exponential; // Fixed

		mVoices[voice].ADSR.AttackStep = (value >> 8) & 0x3;
		mVoices[voice].ADSR.AttackShift = (value >> 10) & 0xF;
		mVoices[voice].ADSR.AttackDirection = SPUDirection::Increase; // Fixed
		mVoices[voice].ADSR.AttackMode = (Mode)((value >> 15) & 0x1);
	}

	U16 SPU::getVoiceCurrentADSRVolume(U8 voice)
	{
		return mVoices[voice].ADSRCurrentVolume;
	}

	void SPU::setVoiceCurrentADSRVolume(U8 voice, U16 value)
	{
		mVoices[voice].ADSRCurrentVolume = value;
	}

	U32 SPU::getVoiceCurrentVolume(U8 voice)
	{
		return getStereoVolume(mVoices[voice].CurrentVolume);
	}

	void SPU::setVoiceCurrentVolume(U8 voice, U32 value)
	{
		setStereoVolume(mVoices[voice].CurrentVolume, value);
	}

	U16 SPU::getVoiceCurrentVolumeLeft(U8 voice)
	{
		return mVoices[voice].CurrentVolume.Left;
	}

	void SPU::setVoiceCurrentVolumeLeft(U8 voice, U16 value)
	{
		mVoices[voice].CurrentVolume.Left = value;
	}

	U16 SPU::getVoiceCurrentVolumeRight(U8 voice)
	{
		return mVoices[voice].CurrentVolume.Right;
	}

	void SPU::setVoiceCurrentVolumeRight(U8 voice, U16 value)
	{
		mVoices[voice].CurrentVolume.Right = value;
	}

	U16 SPU::getVoiceADPCMRepeatAddress(U8 voice)
	{
		return mVoices[voice].ADPCMRepeatAddress;
	}

	void SPU::setVoiceADPCMRepeatAddress(U8 voice, U16 value)
	{
		mVoices[voice].ADPCMRepeatAddress = value;
	}

	void SPU::setVoicesOn(U32 value)
	{
		setVoicesOnLower(value & 0xFFFF);
		setVoicesOnUpper((value >> 16) & 0xFFFF);
	}

	void SPU::setVoicesOnLower(U16 value)
	{
		for (U32 i = 0; i < 16; i++) {
			if (value & (1 << i)) {
				startVoice(i);
			}
		}
	}

	void SPU::setVoicesOnUpper(U16 value)
	{
		for (U32 i = 0; i < 8; i++) {
			if (value & (1 << i)) {
				startVoice(16 + i);
			}
		}
	}

	void SPU::setVoicesOff(U32 value)
	{
		setVoicesOffLower(value & 0xFFFF);
		setVoicesOffUpper((value >> 16) & 0xFFFF);
	}

	void SPU::setVoicesOffLower(U16 value)
	{
		for (U32 i = 0; i < 16; i++) {
			if (value & (1 << i)) {
				//For a full ADSR pattern, 
				// OFF would be usually issued in the Sustain period, 
				// however, it can be issued at any time (eg. to abort Attack, 
				// skip the Decay and Sustain periods, 
				// and switch immediately to Release).
			}
		}
	}

	void SPU::setVoicesOffUpper(U16 value)
	{
		for (U32 i = 0; i < 8; i++) {
			if (value & (1 << i)) {
				//For a full ADSR pattern, 
				// OFF would be usually issued in the Sustain period, 
				// however, it can be issued at any time (eg. to abort Attack, 
				// skip the Decay and Sustain periods, 
				// and switch immediately to Release).
			}
		}
	}

	void SPU::setVoicesPitchModulation(U32 value)
	{
		setVoicesPitchModulationLower(value & 0xFFFF);
		setVoicesPitchModulationUpper((value >> 16) & 0xFFFF);
	}

	void SPU::setVoicesPitchModulationLower(U16 value)
	{
		for (U32 i = 1; i < 16; i++) {
			mVoices[i].PitchModulation = (value >> i) & 0x1;
		}
	}

	void SPU::setVoicesPitchModulationUpper(U16 value)
	{
		for (U32 i = 0; i < 8; i++) {
			mVoices[16 + i].PitchModulation = (value >> i) & 0x1;
		}
	}

	U32 SPU::getVoicesStatus()
	{
		U32 value = 0;

		for (U32 i = 0; i < 24; i++) {
			if (mVoices[i].ReachedLoopEnd) {
				value |= (1 << i);
			}
		}

		return value;
	}

	U32 SPU::getNoiseMode()
	{
		U32 value = (getNoiseModeUpper() << 16) | getNoiseModeLower();
		return value;
	}

	U16 SPU::getNoiseModeLower()
	{
		U16 value = 0;

		for (U32 i = 0; i < 16; i++) {
			value |= ((U8)mVoices[i].NoiseMode << i);
		}

		return value;
	}

	U16 SPU::getNoiseModeUpper()
	{
		U16 value = 0;

		for (U32 i = 0; i < 8; i++) {
			value |= ((U8)mVoices[16 + i].NoiseMode << i);
		}

		return value;
	}

	void SPU::setNoiseMode(U32 value)
	{
		setNoiseModeLower(value & 0xFFFF);
		setNoiseModeUpper((value >> 16) & 0xFFFF);
	}

	void SPU::setNoiseModeLower(U16 value)
	{
		for (U32 i = 0; i < 16; i++) {
			mVoices[i].NoiseMode = (NoiseMode)((value >> i) & 0x1);
		}
	}

	void SPU::setNoiseModeUpper(U16 value)
	{
		for (U32 i = 0; i < 8; i++) {
			mVoices[16 + i].NoiseMode = (NoiseMode)((value >> i) & 0x1);
		}
	}

	U32 SPU::getReverbMode()
	{
		U32 value = (getReverbModeUpper() << 16) | getReverbModeLower();
		return value;
	}

	U16 SPU::getReverbModeLower()
	{
		U16 value = 0;

		for (U32 i = 0; i < 16; i++) {
			value |= ((U8)mVoices[i].ReverbMode << i);
		}

		return value;
	}

	U16 SPU::getReverbModeUpper()
	{
		U16 value = 0;

		for (U32 i = 0; i < 8; i++) {
			value |= ((U8)mVoices[16 + i].ReverbMode << i);
		}

		return value;
	}

	void SPU::setReverbMode(U32 value)
	{
		setReverbModeLower(value & 0xFFFF);
		setReverbModeUpper((value >> 16) & 0xFFFF);
	}

	void SPU::setReverbModeLower(U16 value)
	{
		for (U32 i = 0; i < 16; i++) {
			mVoices[i].ReverbMode = (ReverbMode)((value >> i) & 0x1);
		}
	}

	void SPU::setReverbModeUpper(U16 value)
	{
		for (U32 i = 0; i < 8; i++) {
			mVoices[16 + i].ReverbMode = (ReverbMode)((value >> i) & 0x1);
		}
	}

	U16 SPU::getUnknown1F801DA0()
	{
		return mUnknown1F801DA0;
	}

	void SPU::setUnknown1F801DA0(U16 value)
	{
		mUnknown1F801DA0 = value;
	}

	U16 SPU::getControlRegister()
	{
		U16 value = 0;

		value |= (mSPUControl.CDAudioEnable << 0);
		value |= (mSPUControl.ExternalAudioEnable << 1);
		value |= (mSPUControl.CDAudioReverb << 2);
		value |= (mSPUControl.ExternalAudioReverb << 3);
		value |= ((U8)mSPUControl.TransferMode << 4);
		value |= (mSPUControl.IRQ9Enable << 6);
		value |= (mSPUControl.ReverbMasterEnable << 7);
		value |= (mSPUControl.NoiseFrequencyStep << 8);
		value |= (mSPUControl.NoiseFrequencyShift << 10);
		value |= (mSPUControl.Mute << 14);
		value |= (mSPUControl.Enable << 15);

		return value;
	}

	void SPU::setControlRegister(U16 value)
	{
		mSPUControl.CDAudioEnable = (value >> 0) & 0x1;
		mSPUControl.ExternalAudioEnable = (value >> 1) & 0x1;
		mSPUControl.CDAudioReverb = (value >> 2) & 0x1;
		mSPUControl.ExternalAudioReverb = (value >> 3) & 0x1;
		mSPUControl.TransferMode = (TransferMode)((value >> 4) & 0x3);
		mSPUControl.IRQ9Enable = (value >> 6) & 0x1;
		mSPUControl.ReverbMasterEnable = (value >> 7) & 0x1;
		mSPUControl.NoiseFrequencyStep = (value >> 8) & 0x3;
		mSPUControl.NoiseFrequencyShift = (value >> 10) & 0xF;
		mSPUControl.Mute = (value >> 14) & 0x1;
		mSPUControl.Enable = (value >> 15) & 0x1;
	}

	U16 SPU::getStatusRegister()
	{
		U16 value = 0;

		value |= (mSPUStatus.CDAudioEnable << 0);
		value |= (mSPUStatus.ExternalAudioEnable << 1);
		value |= (mSPUStatus.CDAudioReverb << 2);
		value |= (mSPUStatus.ExternalAudioReverb << 3);
		value |= ((U8)mSPUControl.TransferMode << 4);
		value |= (mSPUStatus.IRQ9Flag << 6);
		value |= (((U8)mSPUStatus.TransferMode >> 1) << 7);
		value |= (mSPUStatus.DataTransferDMAWriteRequest << 8);
		value |= (mSPUStatus.DataTransferDMAReadRequest << 9);
		value |= (mSPUStatus.DataTransferBusyFlag << 10);
		value |= (mSPUStatus.WriteToSecondHalf << 11);

		return value;
	}

	U16 SPU::getDataTransferAdress()
	{
		return mDataTransferAddress;
	}

	void SPU::setDataTransferAdress(U16 value)
	{
		mDataTransferAddress = value;
		mCurrentTransferAddress = value * 8;
	}

	U16 SPU::getDataTransferControl()
	{
		U16 value = 0;

		value |= (mDataTransferControl.TransferType << 1);

		return value;
	}

	void SPU::setDataTransferControl(U16 value)
	{
		mDataTransferControl.TransferType = (value >> 1) & 0x7;
	}

	void SPU::setDataTransferFifo(U16 value)
	{
		mFIFO.emplace(value);
	}

	U32 SPU::getCurrentMainVolume()
	{
		return getStereoVolume(mCurrentMainVolume);
	}

	void SPU::setCurrentMainVolume(U32 value)
	{
		setStereoVolume(mCurrentMainVolume, value);
	}

	U16 SPU::getCurrentMainVolumeLeft()
	{
		return mCurrentMainVolume.Left;
	}

	void SPU::setCurrentMainVolumeLeft(U16 value)
	{
		mCurrentMainVolume.Left = value;
	}

	U16 SPU::getCurrentMainVolumeRight()
	{
		return mCurrentMainVolume.Right;
	}

	void SPU::setCurrentMainVolumeRight(U16 value)
	{
		mCurrentMainVolume.Right = value;
	}

	U16 SPU::getMainVolumeLeft()
	{
		return getVolume(mMainVolumeLeft);
	}

	void SPU::setMainVolumeLeft(U16 value)
	{
		setVolume(mMainVolumeLeft, value);
	}

	U16 SPU::getMainVolumeRight()
	{
		return getVolume(mMainVolumeRight);
	}

	void SPU::setMainVolumeRight(U16 value)
	{
		setVolume(mMainVolumeRight, value);
	}

	U16 SPU::getReverbVolumeLeft()
	{
		return getVolume(mReverbVolumeLeft);
	}

	void SPU::setReverbVolumeLeft(U16 value)
	{
		setVolume(mReverbVolumeLeft,value);
	}

	U16 SPU::getReverbVolumeRight()
	{
		return getVolume(mReverbVolumeRight);
	}

	void SPU::setReverbVolumeRight(U16 value)
	{
		setVolume(mReverbVolumeRight, value);
	}

	U32 SPU::getCDInputVolume()
	{
		return getStereoVolume(mCDInputVolume);
	}

	void SPU::setCDInputVolume(U32 value)
	{
		setStereoVolume(mCDInputVolume, value);
	}

	U16 SPU::getCDInputVolumeLeft()
	{
		return mCDInputVolume.Left;
	}

	void SPU::setCDInputVolumeLeft(U16 value)
	{
		mCDInputVolume.Left = value;
	}

	U16 SPU::getCDInputVolumeRight()
	{
		return mCDInputVolume.Right;
	}

	void SPU::setCDInputVolumeRight(U16 value)
	{
		mCDInputVolume.Right = value;
	}

	U16 SPU::getReverbWorkAreaStartAddress()
	{
		return mReverbWorkAreaStartAddress;
	}

	void SPU::setReverbWorkAreaStartAddress(U16 value)
	{
		mReverbWorkAreaStartAddress = value;
	}

	U16 SPU::getSoundRAMIRQAddress()
	{
		return mSoundRAMIRQAddress;
	}

	void SPU::setSoundRAMIRQAddress(U16 value)
	{
		mSoundRAMIRQAddress = value;
	}

	U32 SPU::getExternalInputVolume()
	{
		return getStereoVolume(mExternalInputVolume);
	}

	void SPU::setExternalInputVolume(U32 value)
	{
		setStereoVolume(mExternalInputVolume, value);
	}

	U16 SPU::getExternalInputVolumeLeft()
	{
		return mExternalInputVolume.Left;
	}

	void SPU::setExternalInputVolumeLeft(U16 value)
	{
		mExternalInputVolume.Left = value;
	}

	U16 SPU::getExternalInputVolumeRight()
	{
		return mExternalInputVolume.Right;
	}

	void SPU::setExternalInputVolumeRight(U16 value)
	{
		mExternalInputVolume.Right = value;
	}

	U16 SPU::getReverbConfigurationAreaRegister(ReverbRegister reg)
	{
		return mReverbConfigurationArea[(U8)reg];
	}

	void SPU::setReverbConfigurationAreaRegister(ReverbRegister reg, U16 value)
	{
		mReverbConfigurationArea[(U8)reg] = value;
	}

}