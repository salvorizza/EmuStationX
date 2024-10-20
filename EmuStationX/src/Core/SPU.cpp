#include "SPU.h"

#include "R3000.h"
#include "CDROM.h"

#include "Core/Scheduler.h"

namespace esx {
	
	static U16 getVolume(const Volume& volume)
	{
		U16 value = 0;

		if (volume.VolumeMode == VolumeMode::Volume) {
			value |= (static_cast<U16>(volume.Level) >> 1);
		} else {
			value |= (volume.Envelope.Step << 0);
			value |= (volume.Envelope.Shift << 2);
			value |= ((U8)volume.SweepPhase << 12);
			value |= ((U8)volume.Envelope.Direction << 13);
			value |= ((U8)volume.Envelope.Mode << 14);
		}

		value |= ((U8)volume.VolumeMode << 15);

		return value;
	}

	static void setVolume(Volume& volume, U16 value)
	{
		volume.VolumeMode = (VolumeMode)((value >> 15) & 0x1);

		if (volume.VolumeMode == VolumeMode::Volume) {
			volume.Level = static_cast<I16>(value << 1);
		} else {
			volume.Envelope.Step = (value >> 0) & 0x3;
			volume.Envelope.Shift = (value >> 2) & 0x1F;
			volume.SweepPhase = (SweepPhase)((value >> 12) & 0x1);
			volume.Envelope.Direction = (EnvelopeDirection)((value >> 13) & 0x1);
			volume.Envelope.Mode = (EnvelopeMode)((value >> 14) & 0x1);
		}
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

	static void tickEnvelope(EnvelopePhase& phase, BIT PhaseNegative, I16& outVolume, U32& outTick, I16& outStep) {
		BIT Decreasing = phase.Direction == EnvelopeDirection::Decrease;
		BIT Increase = phase.Direction == EnvelopeDirection::Increase;
		BIT Exponential = phase.Mode == EnvelopeMode::Exponential;

		I16 StepValue = 7 - phase.Step;
		if (Decreasing ^ PhaseNegative) {
			StepValue = ~StepValue;
		}
		I16 AdsrStep = StepValue << std::max(0, 11 - phase.Shift);
		U16 CounterIncrement = 0x8000 >> std::max(0, phase.Shift - 11);

		if (CounterIncrement == 0) return;

		if (Exponential && Increase && outVolume > 0x6000) {
			if (phase.Shift < 10) {
				AdsrStep >>= 2;
			} else if (phase.Shift >= 11) {
				CounterIncrement >>= 2;
			} else {
				AdsrStep >>= 1;;
				CounterIncrement >>= 1;
			}
		} else if (Exponential && Decreasing) {
			AdsrStep = (I32(AdsrStep) * I32(outVolume)) >> 15;
		}

		if ((phase.Step | (phase.Step << 2)) != 0xFF) {
			CounterIncrement = std::max<U16>(CounterIncrement, 1);
		}

		outTick += CounterIncrement;
		if ((outTick & 0x8000) == 0) {
			return;
		}
		outTick = 0;

		I32 newVolume = outVolume + AdsrStep;
		if (Increase) {
			newVolume = std::clamp<I32>(newVolume, -0x8000, 0x7FFF);
		} else if (PhaseNegative) {
			newVolume = std::clamp<I32>(newVolume, -0x8000, 0);
		} else {
			newVolume = std::max<I32>(newVolume, 0);
		}
		outVolume = static_cast<I16>(newVolume);
		outStep = AdsrStep;

		/*I32 step = phase.Direction == EnvelopeDirection::Increase ? (7 - phase.Step) : (-8 + phase.Step);
		U32 cycles = 1 << std::max(0, phase.Shift - 11);
		I32 envelopeStep = step << std::max(0, 11 - phase.Shift);
		if (phase.Mode == EnvelopeMode::Exponential) {
			if (phase.Direction == EnvelopeDirection::Increase && outVolume > 0x6000) {
				cycles *= 4;
			} else if (phase.Direction == EnvelopeDirection::Decrease) {
				envelopeStep = (envelopeStep * outVolume) >> 15;
			}
		}
		outTick = cycles;
		outStep = envelopeStep;

		outVolume = std::clamp(outVolume + envelopeStep, 0x0000, 0x7FFF);*/
	}

	static I16 processVolume(Volume& volume) {
		if (volume.VolumeMode == VolumeMode::Volume) {
			return volume.Level;
		}
		else {
			ESX_CORE_LOG_TRACE("Sweep");
			tickEnvelope(volume.Envelope, volume.SweepPhase == SweepPhase::Negative, volume.Level, volume.Tick, volume.Step);
			return volume.Level;
		}
	}


	static Array<std::ofstream, 24> sStreams;

	SPU::SPU()
		: BusDevice(ESX_TEXT("SPU"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801C00, BYTE(640), 0xFFFFFFFF);

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::SPUSample, [&](const SchedulerEvent& ev) {
			sampleClock(ev.ClockTarget);
		});

		reset();
	}

	SPU::~SPU()
	{
		sStreams[0].close();
	}
	
	void SPU::sampleClock(U64 clocks)
	{
		mSPUStatus.CDAudioEnable = mSPUControl.CDAudioEnable;
		mSPUStatus.ExternalAudioEnable = mSPUControl.ExternalAudioEnable;
		mSPUStatus.CDAudioReverb = mSPUControl.CDAudioReverb;
		mSPUStatus.ExternalAudioReverb = mSPUControl.ExternalAudioReverb;
		mSPUStatus.TransferMode = mSPUControl.TransferMode;

		if (mSPUControl.Enable) {
			if (mSPUStatus.TransferMode == TransferMode::ManualWrite) {
				for (I32 i = 0; i < (768 / 16); i++) {
					if (mFIFO.empty()) break;
					U16 value = mFIFO.front();
					writeToRAM<U16>(value);
					mFIFO.pop();
					mSPUStatus.DataTransferBusyFlag = !mFIFO.empty();
				}
			}

			I32 leftSum = 0, rightSum = 0;
			I32 reverbLeftSum = 0, reverbRightSum = 0;

			for (Voice& voice : mVoices) {
				if (voice.KeyOff) {
					voice.ADSR.Phase = ADSRPhaseType::Release;
					voice.KeyOff = ESX_FALSE;
				}

				if (voice.KeyOn) {
					startVoice(voice);
					voice.KeyOn = ESX_FALSE;
				}

				if (voice.ADPCMCurrentAddress == mSoundRAMIRQAddress) {
					getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::SPU, ESX_FALSE, ESX_TRUE);
				}

				auto [left, right] = sampleVoice(voice);

				//Mixing
				leftSum += (left * processVolume(voice.VolumeLeft)) >> 15;
				rightSum += (right * processVolume(voice.VolumeRight)) >> 15;

				if (voice.ReverbMode == ReverbMode::ToMixerAndToReverb) {
					reverbLeftSum += (left * processVolume(voice.VolumeLeft)) >> 15;
					reverbRightSum += (right * processVolume(voice.VolumeRight)) >> 15;
				}
			}

			if (!mSPUControl.Unmute) {
				leftSum = 0;
				rightSum = 0;
			}

			{
				mNoiseTimer = mNoiseTimer - (4 + mSPUControl.NoiseFrequencyStep);
				U8 parityBit = ((mNoiseLevel >> 15) & 0x1) ^ ((mNoiseLevel >> 12) & 0x1) ^ ((mNoiseLevel >> 11) & 0x1) ^ ((mNoiseLevel >> 10) & 0x1) ^ 1;
				if (mNoiseTimer < 0) mNoiseLevel = mNoiseLevel * 2 + parityBit;
				if (mNoiseTimer < 0) mNoiseTimer += 0x20000 << mSPUControl.NoiseFrequencyShift;
				if (mNoiseTimer < 0) mNoiseTimer += 0x20000 << mSPUControl.NoiseFrequencyShift;
			}

			AudioFrame cdFrame = mCDROM->HasAudioFramesAvailable() ? mCDROM->GetAudioFrame() : AudioFrame();
			if (mSPUControl.CDAudioEnable && mCDROM->HasAudioFramesAvailable()) {
				I16 left = (I32(cdFrame.Left) * I32(mCDInputVolume.Left)) >> 15;
				I16 right = (I32(cdFrame.Right) * I32(mCDInputVolume.Right)) >> 15;

				leftSum += left;
				rightSum += right;

				if (mSPUControl.CDAudioReverb) {
					reverbLeftSum += left;
					reverbRightSum += right;
				}
			}

			writeCaptureBuffer(0, cdFrame.Left);
			writeCaptureBuffer(1, cdFrame.Right);
			writeCaptureBuffer(2, mVoices[1].Latest);
			writeCaptureBuffer(3, mVoices[3].Latest);
			advanceCaptureBufferPointer();

			if (clocks % 1536 == 0) {
				auto [leftReverb, rightReverb] = reverb(static_cast<I16>(SATURATE(reverbLeftSum)), static_cast<I16>(SATURATE(reverbRightSum)));
				leftSum += leftReverb;
				rightSum += rightReverb;
			}

			I16 left = static_cast<I16>((SATURATE(leftSum) * processVolume(mMainVolumeLeft)) >> 15);
			I16 right = static_cast<I16>((SATURATE(rightSum) * processVolume(mMainVolumeRight)) >> 15);


			/*sStreams[0].write((char*)&left, 2);
			sStreams[0].write((char*)&right, 2);*/

			AudioFrame& sample = mBackBuffer.Batch[mBackBuffer.CurrentFrame];
			sample.Left = left;
			sample.Right = right;
			mBackBuffer.CurrentFrame++;
			if (mBackBuffer.Complete()) {
				std::scoped_lock<std::mutex> lc(mSamplesMutex);
				std::swap(mFrontBuffer, mBackBuffer);
				mBackBuffer = {};
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
					setReverbConfigurationAreaRegister(vLOUT, value);
					break;
				}
				case 0x1F801D86: {
					setReverbConfigurationAreaRegister(vROUT, value);
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
					setReverbConfigurationAreaRegister(mBASE, value);
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
			ReverbRegister reverbRegister = (ReverbRegister)((address - 0x1F801DC0) / 2 + 3);
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
					output = getReverbConfigurationAreaRegister(vLOUT);
					break;
				}
				case 0x1F801D86: {
					output = getReverbConfigurationAreaRegister(vROUT);
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
					output = getReverbConfigurationAreaRegister(mBASE);
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
			ReverbRegister reverbRegister = (ReverbRegister)((address - 0x1F801DC0) / 2 + 3);
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

	void SPU::store(const StringView& busName, U32 address, U32 value)
	{
		store(busName, address + 0x0, static_cast<U16>((value >> 0) & 0xFFFF));
		store(busName, address + 0x2, static_cast<U16>((value >> 16) & 0xFFFF));
	}

	void SPU::load(const StringView& busName, U32 address, U32& output)
	{
		U16 lower, upper;
		
		load(busName, address + 0x0, lower);
		load(busName, address + 0x2, upper);

		output = (static_cast<U32>(upper) << 16) | lower;
	}

	void SPU::init()
	{
		mCPU = getBus("Root")->getDevice<R3000>("R3000");
		mCDROM = getBus("Root")->getDevice<CDROM>("CDROM");
	}

	void SPU::reset()
	{
		mVoices = {};
		mMainVolumeLeft = {};
		mMainVolumeRight = {};
		mCurrentMainVolume = {};
		mCDInputVolume = {};
		mExternalInputVolume = {};
		mSPUControl = {};
		mSPUStatus = {};
		mDataTransferAddress = 0x0000;
		mDataTransferControl = {};
		mUnknown1F801DA0 = 0x0000;
		mSoundRAMIRQAddress = 0x0000;

		mNoiseTimer = 0;
		mNoiseLevel = 0;

		mReverb = {};
		mCurrentBufferAddress = 0x00000000;

		mFIFO = {};

		mCurrentTransferAddress = 0;

		std::scoped_lock lc(mSamplesMutex);
		mBackBuffer = {};
		mFrontBuffer = {};

		mRAM.resize(KIBI(512));
		std::fill(mRAM.begin(),mRAM.end(),0x00);
		for (U32 i = 0; i < 24; i++) {
			mVoices[i].Number = i;
			if(i==0 && !sStreams[i].is_open()) sStreams[i].open(std::to_string(i) + ".bin", std::ios::binary);
		}
	}

	Pair<I16, I16> SPU::reverb(I16 LeftInput, I16 RightInput)
	{
		#define MULT(x,y) (((x) * (y)) >> 15)
		#define LOAD(x) loadReverb(mReverb[x])
		#define LOAD_DIFF(x,y) loadReverb(mReverb[x] - mReverb[y])
		#define LOAD_SUB(x,y) loadReverb(mReverb[x] - y)
		#define VOLUME(x) (I16)mReverb[x]

		LeftInput = reverbFirFilter(LeftInput);
		RightInput = reverbFirFilter(RightInput);

		I16 Lin = MULT(VOLUME(vLIN), LeftInput);
		I16 Rin = MULT(VOLUME(vRIN), RightInput);

		//____Same Side Reflection (left-to-left and right-to-right)___________________
		I16 mlSame = SATURATE(MULT(Lin + MULT(LOAD(dLSAME), VOLUME(vWALL)) - LOAD_SUB(mLSAME, 2), VOLUME(vIIR)) + LOAD_SUB(mLSAME, 2));
		I16 mrSame = SATURATE(MULT(Rin + MULT(LOAD(dRSAME), VOLUME(vWALL)) - LOAD_SUB(mRSAME, 2), VOLUME(vIIR)) + LOAD_SUB(mRSAME, 2));
		writeReverb(mLSAME, mlSame);
		writeReverb(mRSAME, mrSame);

		//___Different Side Reflection (left-to-right and right-to-left)_______________
		I16 mlDiff = SATURATE(MULT(Lin + MULT(LOAD(dRDIFF), VOLUME(vWALL)) - LOAD_SUB(mLDIFF, 2), VOLUME(vIIR)) + LOAD_SUB(mLDIFF, 2));
		I16 mrDiff = SATURATE(MULT(Rin + MULT(LOAD(dLDIFF), VOLUME(vWALL)) - LOAD_SUB(mRDIFF, 2), VOLUME(vIIR)) + LOAD_SUB(mRDIFF, 2));
		writeReverb(mLDIFF, mlDiff);
		writeReverb(mRDIFF, mrDiff);

		//Early echo
		I16 Lout = SATURATE(MULT(VOLUME(vCOMB1), LOAD(mLCOMB1)) + MULT(VOLUME(vCOMB2), LOAD(mLCOMB2)) + MULT(VOLUME(vCOMB3), LOAD(mLCOMB3)) + MULT(VOLUME(vCOMB4), LOAD(mLCOMB4)));
		I16 Rout = SATURATE(MULT(VOLUME(vCOMB1), LOAD(mRCOMB1)) + MULT(VOLUME(vCOMB2), LOAD(mRCOMB2)) + MULT(VOLUME(vCOMB3), LOAD(mRCOMB3)) + MULT(VOLUME(vCOMB4), LOAD(mRCOMB4)));

		// Late reverb APF1
		Lout = SATURATE(Lout - MULT(VOLUME(vAPF1), LOAD_DIFF(mLAPF1, dAPF1)));
		Rout = SATURATE(Rout - MULT(VOLUME(vAPF1), LOAD_DIFF(mRAPF1, dAPF1)));

		writeReverb(mReverb[mLAPF1], Lout); 
		writeReverb(mReverb[mRAPF1], Rout);

		Lout = SATURATE(MULT(Lout, VOLUME(vAPF1)) + LOAD_DIFF(mLAPF1, dAPF1));
		Rout = SATURATE(MULT(Rout, VOLUME(vAPF1)) + LOAD_DIFF(mRAPF1, dAPF1));

		// Late reverb APF2
		Lout = SATURATE(Lout - MULT(VOLUME(vAPF2), LOAD_DIFF(mLAPF2, dAPF2)));
		Rout = SATURATE(Rout - MULT(VOLUME(vAPF2), LOAD_DIFF(mRAPF2, dAPF2)));

		writeReverb(mReverb[mLAPF2], Lout);
		writeReverb(mReverb[mRAPF2], Rout);

		Lout = SATURATE(MULT(Lout, VOLUME(vAPF2)) + LOAD_DIFF(mLAPF2, dAPF2));
		Rout = SATURATE(MULT(Rout, VOLUME(vAPF2)) + LOAD_DIFF(mRAPF2, dAPF2));

		//___Output to Mixer (Output volume multiplied with input from APF2)___________
		I16 LeftOutput = SATURATE(MULT(Lout, VOLUME(vLOUT)));
		I16 RightOutput = SATURATE(MULT(Rout, VOLUME(vROUT)));

		LeftOutput = reverbFirFilter(LeftOutput);
		RightOutput = reverbFirFilter(RightOutput);

		mCurrentBufferAddress = std::max(((U32)mReverb[mBASE]) << 3, (mCurrentBufferAddress + 2) & 0x7FFFE);

		return std::make_pair(LeftOutput, RightOutput);
	}

	Pair<I32, I32> SPU::sampleVoice(Voice& voice)
	{
		static constexpr Array<I32, 0x200> gauss = {
			 -0x0001,-0x0001,-0x0001,-0x0001,-0x0001,-0x0001,-0x0001,-0x0001,
			 -0x0001,-0x0001,-0x0001,-0x0001,-0x0001,-0x0001,-0x0001,-0x0001,
			  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0001,
			  0x0001,0x0001,0x0001,0x0002,0x0002,0x0002,0x0003,0x0003,
			  0x0003,0x0004,0x0004,0x0005,0x0005,0x0006,0x0007,0x0007,
			  0x0008,0x0009,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,
			  0x000F,0x0010,0x0011,0x0012,0x0013,0x0015,0x0016,0x0018,
			  0x0019,0x001B,0x001C,0x001E,0x0020,0x0021,0x0023,0x0025,
			  0x0027,0x0029,0x002C,0x002E,0x0030,0x0033,0x0035,0x0038,
			  0x003A,0x003D,0x0040,0x0043,0x0046,0x0049,0x004D,0x0050,
			  0x0054,0x0057,0x005B,0x005F,0x0063,0x0067,0x006B,0x006F,
			  0x0074,0x0078,0x007D,0x0082,0x0087,0x008C,0x0091,0x0096,
			  0x009C,0x00A1,0x00A7,0x00AD,0x00B3,0x00BA,0x00C0,0x00C7,
			  0x00CD,0x00D4,0x00DB,0x00E3,0x00EA,0x00F2,0x00FA,0x0101,
			  0x010A,0x0112,0x011B,0x0123,0x012C,0x0135,0x013F,0x0148,
			  0x0152,0x015C,0x0166,0x0171,0x017B,0x0186,0x0191,0x019C,
			  0x01A8,0x01B4,0x01C0,0x01CC,0x01D9,0x01E5,0x01F2,0x0200,
			  0x020D,0x021B,0x0229,0x0237,0x0246,0x0255,0x0264,0x0273,
			  0x0283,0x0293,0x02A3,0x02B4,0x02C4,0x02D6,0x02E7,0x02F9,
			  0x030B,0x031D,0x0330,0x0343,0x0356,0x036A,0x037E,0x0392,
			  0x03A7,0x03BC,0x03D1,0x03E7,0x03FC,0x0413,0x042A,0x0441,
			  0x0458,0x0470,0x0488,0x04A0,0x04B9,0x04D2,0x04EC,0x0506,
			  0x0520,0x053B,0x0556,0x0572,0x058E,0x05AA,0x05C7,0x05E4,
			  0x0601,0x061F,0x063E,0x065C,0x067C,0x069B,0x06BB,0x06DC,
			  0x06FD,0x071E,0x0740,0x0762,0x0784,0x07A7,0x07CB,0x07EF,
			  0x0813,0x0838,0x085D,0x0883,0x08A9,0x08D0,0x08F7,0x091E,
			  0x0946,0x096F,0x0998,0x09C1,0x09EB,0x0A16,0x0A40,0x0A6C,
			  0x0A98,0x0AC4,0x0AF1,0x0B1E,0x0B4C,0x0B7A,0x0BA9,0x0BD8,
			  0x0C07,0x0C38,0x0C68,0x0C99,0x0CCB,0x0CFD,0x0D30,0x0D63,
			  0x0D97,0x0DCB,0x0E00,0x0E35,0x0E6B,0x0EA1,0x0ED7,0x0F0F,
			  0x0F46,0x0F7F,0x0FB7,0x0FF1,0x102A,0x1065,0x109F,0x10DB,
			  0x1116,0x1153,0x118F,0x11CD,0x120B,0x1249,0x1288,0x12C7,
			  0x1307,0x1347,0x1388,0x13C9,0x140B,0x144D,0x1490,0x14D4,
			  0x1517,0x155C,0x15A0,0x15E6,0x162C,0x1672,0x16B9,0x1700,
			  0x1747,0x1790,0x17D8,0x1821,0x186B,0x18B5,0x1900,0x194B,
			  0x1996,0x19E2,0x1A2E,0x1A7B,0x1AC8,0x1B16,0x1B64,0x1BB3,
			  0x1C02,0x1C51,0x1CA1,0x1CF1,0x1D42,0x1D93,0x1DE5,0x1E37,
			  0x1E89,0x1EDC,0x1F2F,0x1F82,0x1FD6,0x202A,0x207F,0x20D4,
			  0x2129,0x217F,0x21D5,0x222C,0x2282,0x22DA,0x2331,0x2389,
			  0x23E1,0x2439,0x2492,0x24EB,0x2545,0x259E,0x25F8,0x2653,
			  0x26AD,0x2708,0x2763,0x27BE,0x281A,0x2876,0x28D2,0x292E,
			  0x298B,0x29E7,0x2A44,0x2AA1,0x2AFF,0x2B5C,0x2BBA,0x2C18,
			  0x2C76,0x2CD4,0x2D33,0x2D91,0x2DF0,0x2E4F,0x2EAE,0x2F0D,
			  0x2F6C,0x2FCC,0x302B,0x308B,0x30EA,0x314A,0x31AA,0x3209,
			  0x3269,0x32C9,0x3329,0x3389,0x33E9,0x3449,0x34A9,0x3509,
			  0x3569,0x35C9,0x3629,0x3689,0x36E8,0x3748,0x37A8,0x3807,
			  0x3867,0x38C6,0x3926,0x3985,0x39E4,0x3A43,0x3AA2,0x3B00,
			  0x3B5F,0x3BBD,0x3C1B,0x3C79,0x3CD7,0x3D35,0x3D92,0x3DEF,
			  0x3E4C,0x3EA9,0x3F05,0x3F62,0x3FBD,0x4019,0x4074,0x40D0,
			  0x412A,0x4185,0x41DF,0x4239,0x4292,0x42EB,0x4344,0x439C,
			  0x43F4,0x444C,0x44A3,0x44FA,0x4550,0x45A6,0x45FC,0x4651,
			  0x46A6,0x46FA,0x474E,0x47A1,0x47F4,0x4846,0x4898,0x48E9,
			  0x493A,0x498A,0x49D9,0x4A29,0x4A77,0x4AC5,0x4B13,0x4B5F,
			  0x4BAC,0x4BF7,0x4C42,0x4C8D,0x4CD7,0x4D20,0x4D68,0x4DB0,
			  0x4DF7,0x4E3E,0x4E84,0x4EC9,0x4F0E,0x4F52,0x4F95,0x4FD7,
			  0x5019,0x505A,0x509A,0x50DA,0x5118,0x5156,0x5194,0x51D0,
			  0x520C,0x5247,0x5281,0x52BA,0x52F3,0x532A,0x5361,0x5397,
			  0x53CC,0x5401,0x5434,0x5467,0x5499,0x54CA,0x54FA,0x5529,
			  0x5558,0x5585,0x55B2,0x55DE,0x5609,0x5632,0x565B,0x5684,
			  0x56AB,0x56D1,0x56F6,0x571B,0x573E,0x5761,0x5782,0x57A3,
			  0x57C3,0x57E2,0x57FF,0x581C,0x5838,0x5853,0x586D,0x5886,
			  0x589E,0x58B5,0x58CB,0x58E0,0x58F4,0x5907,0x5919,0x592A,
			  0x593A,0x5949,0x5958,0x5965,0x5971,0x597C,0x5986,0x598F,
			  0x5997,0x599E,0x59A4,0x59A9,0x59AD,0x59B0,0x59B2,0x59B3
		};

		if (voice.ADSR.Phase == ADSRPhaseType::Off) {
			voice.Latest = 0;
			return {};
		}

		if (mSPUControl.IRQ9Enable && voice.ADPCMCurrentAddress == mSoundRAMIRQAddress) {
			getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::SPU, mSPUStatus.IRQ9Flag, ESX_TRUE);
			mSPUStatus.IRQ9Flag = ESX_TRUE;
		}

		if (!voice.HasSamples) {
			ADPCMBlock currentBlock = readADPCMBlock(voice.ADPCMCurrentAddress);
			decodeBlock(voice, currentBlock);
			voice.HasSamples = ESX_TRUE;

			if (voice.CurrentBlockFlags & 0b100) {
				voice.ADPCMRepeatAddress = voice.ADPCMCurrentAddress;
			}
		}

		I32 volume = 0;
		if (voice.ADSR.CurrentVolume != 0) {
			I32 sample = 0;

			if (voice.NoiseMode == NoiseMode::Noise) {
				sample = mNoiseLevel;
			} else {
				U8 interpolationIndex = voice.getInterpolationIndex();
				U8 sampleIndex = 3 + voice.getSampleIndex();

				sample =  gauss[0x0FF - interpolationIndex] * I32(voice.CurrentSamples[sampleIndex - 3]);
				sample += gauss[0x1FF - interpolationIndex] * I32(voice.CurrentSamples[sampleIndex - 2]);
				sample += gauss[0x100 + interpolationIndex] * I32(voice.CurrentSamples[sampleIndex - 1]);
				sample += gauss[0x000 + interpolationIndex] * I32(voice.CurrentSamples[sampleIndex - 0]);
				sample >>= 15;
			}

			volume = (sample * voice.ADSR.CurrentVolume) >> 15;
		}

		voice.Latest = volume;

		tickADSR(voice);

		U16 step = voice.ADPCMSampleRate;
		if (voice.PitchModulation && voice.Number > 0) {
			const I32 factor = std::clamp<I32>(mVoices[voice.Number - 1].Latest, -0x8000, 0x7FFF) + 0x8000;
			step = static_cast<U16>((static_cast<I32>(static_cast<I16>(step)) * factor) >> 15);
		}
		step = std::min<U16>(step, 0x3FFF);
		voice.PitchCounter += step;
		
		if (voice.getSampleIndex() >= 28) {
			voice.setSampleIndex(voice.getSampleIndex() - 28);
			voice.HasSamples = ESX_FALSE;
			voice.ADPCMCurrentAddress += 0x2;

			switch (voice.CurrentBlockFlags & 0x3) {
				case 1: {
					voice.ADPCMCurrentAddress = voice.ADPCMRepeatAddress & ~1;
					voice.ReachedLoopEnd = ESX_TRUE;

					if (voice.NoiseMode == NoiseMode::ADPCM) {
						voice.ADSR.Phase = ADSRPhaseType::Off;
						voice.ADSR.CurrentVolume = 0x0000;
					}
					break;
				}

				case 3: {
					voice.ADPCMCurrentAddress = voice.ADPCMRepeatAddress & ~1;
					voice.ReachedLoopEnd = ESX_TRUE;
					break;
				}
			}
		}

		I32 left = (volume * processVolume(voice.VolumeLeft)) >> 15;
		I32 right = (volume * processVolume(voice.VolumeRight)) >> 15;

		return std::make_pair(left, right);
	}

	I16 SPU::loadReverb(U16 address)
	{
		U32 relative = ((address << 3) + mCurrentBufferAddress - (mReverb[mBASE] << 3)) % (0x80000 - (mReverb[mBASE] << 3));
		U32 wrapped = ((mReverb[mBASE] << 3) + relative) & 0x7FFFE;

		if (mSPUControl.IRQ9Enable && (wrapped / 8) == mSoundRAMIRQAddress) {
			getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::SPU, mSPUStatus.IRQ9Flag, ESX_TRUE);
			mSPUStatus.IRQ9Flag = ESX_TRUE;
		}

		return *(I16*)(mRAM.data() + wrapped);
	}

	void SPU::writeReverb(U16 address, I16 value)
	{
		if (mSPUControl.ReverbMasterEnable == ESX_FALSE) return;

		U32 relative = ((address << 3) + mCurrentBufferAddress - (mReverb[mBASE] << 3)) % (0x80000 - (mReverb[mBASE] << 3));
		U32 wrapped = ((mReverb[mBASE] << 3) + relative) & 0x7FFFE;

		if (mSPUControl.IRQ9Enable && (wrapped / 8) == mSoundRAMIRQAddress) {
			getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::SPU, mSPUStatus.IRQ9Flag, ESX_TRUE);
			mSPUStatus.IRQ9Flag = ESX_TRUE;
		}

		*(I16*)(mRAM.data() + wrapped) = value;
	}

	void SPU::writeCaptureBuffer(I32 index, I16 value)
	{
		U32 writeAddress = (index * 0x400) + mCaptureBufferPointer;
		*reinterpret_cast<I16*>(&mRAM[writeAddress]) = value;
		if (mSPUControl.IRQ9Enable && (writeAddress / 8) == mSoundRAMIRQAddress) {
			getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::SPU, mSPUStatus.IRQ9Flag, ESX_TRUE);
			mSPUStatus.IRQ9Flag = ESX_TRUE;
		}
		mSPUStatus.WriteToSecondHalf = mCaptureBufferPointer >= 0x200;
	}

	void SPU::advanceCaptureBufferPointer()
	{
		mCaptureBufferPointer += sizeof(I16);
		mCaptureBufferPointer %= 0x400;
	}

	void SPU::startVoice(Voice& voice)
	{
		voice.ReachedLoopEnd = ESX_FALSE;
		voice.ADPCMCurrentAddress = voice.ADPCMStartAddress;
		voice.ADSR.Phase = ADSRPhaseType::Attack;
		voice.ADSR.CurrentVolume = 0;
		voice.ADSR.Tick = 0;
		voice.HasSamples = ESX_FALSE;
		voice.LastSamples.fill(0);

		voice.CurrentSamples[voice.CurrentSamples.size() - 3] = 0;
		voice.CurrentSamples[voice.CurrentSamples.size() - 2] = 0;
		voice.CurrentSamples[voice.CurrentSamples.size() - 1] = 0;
	}

	void SPU::stopVoice(Voice& voice)
	{
		if (voice.ADSR.Phase == ADSRPhaseType::Off || voice.ADSR.Phase == ADSRPhaseType::Release)
			return;

		voice.KeyOff = ESX_TRUE;
	}

	U16 SPU::getVoiceVolumeLeft(U8 voice)
	{
		return getVolume(mVoices[voice].VolumeLeft);
	}

	ADPCMBlock SPU::readADPCMBlock(U16 address)
	{
		ADPCMBlock block = {};

		U32 ramAddress = address * 8;

		if ((ramAddress + sizeof(ADPCMBlock)) <= mRAM.size()) {
			std::memcpy(&block, &mRAM[ramAddress], sizeof(ADPCMBlock));
		} else {
			for (U8 i = 0; i < sizeof(ADPCMBlock); i++) {
				reinterpret_cast<U8*>(&block)[i] = mRAM[ramAddress];
				ramAddress = (ramAddress + 1) & (mRAM.size() - 1);
			}
		}

		return block;
	}

	void SPU::decodeBlock(Voice& voice, const ADPCMBlock& block) {
		
		voice.CurrentSamples[2] = voice.CurrentSamples[voice.CurrentSamples.size() - 1];
		voice.CurrentSamples[1] = voice.CurrentSamples[voice.CurrentSamples.size() - 2];
		voice.CurrentSamples[0] = voice.CurrentSamples[voice.CurrentSamples.size() - 3];

		Array<I16, 2> lastSamples = voice.LastSamples;

		const I32 filter_pos = pos_xa_adpcm_table[block.Filter()];
		const I32 filter_neg = neg_xa_adpcm_table[block.Filter()];
		const U8 shift = block.Shift();

		for (I32 i = 0; i < voice.CurrentSamples.size() - 3; i++) {
			U8 nibble = block.GetNibble(i);
			I16 extendedNibble = static_cast<I16>(static_cast<U16>(nibble) << 12);

			I32 sample = I32(extendedNibble >> shift);
			sample += (lastSamples[0] * filter_pos + lastSamples[1] * filter_neg + 32) / 64;

			lastSamples[1] = lastSamples[0];
			voice.CurrentSamples[3 + i] = lastSamples[0] = static_cast<I16>(SATURATE(sample));
		}

		voice.LastSamples = lastSamples;
		voice.CurrentBlockFlags = block.Flags;
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

		value |= (mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Release].Shift << 0);
		value |= ((U8)mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Release].Mode << 5);
		value |= (mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Sustain].Step << 6);
		value |= (mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Sustain].Shift << 8);
		value |= ((U8)mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Sustain].Direction << 14);
		value |= ((U8)mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Sustain].Mode << 15);

		return value;
	}

	void SPU::setVoiceADSRUpper(U8 voice, U16 value)
	{
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Release].Step = 0; // Fixed
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Release].Shift = (value >> 0) & 0x1F;
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Release].Direction = EnvelopeDirection::Decrease; // Fixed
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Release].Mode = (EnvelopeMode)((value >> 5) & 0x1);

		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Sustain].Step = (value >> 6) & 0x3;
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Sustain].Shift = (value >> 8) & 0x1F;
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Sustain].Direction = (EnvelopeDirection)((value >> 14) & 0x1);
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Sustain].Mode = (EnvelopeMode)((value >> 15) & 0x1);
	}

	U16 SPU::getVoiceADSRLower(U8 voice)
	{
		U16 value = 0;

		value |= (mVoices[voice].ADSR.SustainLevel << 0);
		value |= (mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Decay].Shift << 4);
		value |= (mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Attack].Step << 8);
		value |= (mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Attack].Shift << 10);
		value |= ((U8)mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Attack].Mode << 15);

		return value;
	}

	void SPU::setVoiceADSRLower(U8 voice, U16 value)
	{
		mVoices[voice].ADSR.SustainLevel = (value >> 0) & 0xF;

		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Decay].Step = 0; // Fixed
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Decay].Shift = (value >> 4) & 0xF;
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Decay].Direction = EnvelopeDirection::Decrease; // Fixed
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Decay].Mode = EnvelopeMode::Exponential; // Fixed
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Decay].Target = std::min<I32>((mVoices[voice].ADSR.SustainLevel + 1) * 0x800, 0x7FFF);

		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Attack].Step = (value >> 8) & 0x3;
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Attack].Shift = (value >> 10) & 0x1F;
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Attack].Direction = EnvelopeDirection::Increase; // Fixed
		mVoices[voice].ADSR.Phases[(U8)ADSRPhaseType::Attack].Mode = (EnvelopeMode)((value >> 15) & 0x1);
	}

	U16 SPU::getVoiceCurrentADSRVolume(U8 voice)
	{
		return mVoices[voice].ADSR.CurrentVolume;
	}

	void SPU::setVoiceCurrentADSRVolume(U8 voice, U16 value)
	{
		mVoices[voice].ADSR.CurrentVolume = value;
	}

	U32 SPU::getVoiceCurrentVolume(U8 voice)
	{
		return getStereoVolume(mVoices[voice].CurrentVolume);
	}

	void SPU::tickADSR(Voice& voice)
	{
		ADSR& adsr = voice.ADSR;
		if (adsr.Phase != ADSRPhaseType::Off) {
			EnvelopePhase& currentPhase = adsr.Phases[(U8)adsr.Phase];
			tickEnvelope(currentPhase, ESX_FALSE, adsr.CurrentVolume, adsr.Tick, adsr.Step);
			BIT goToNextPhase = currentPhase.Direction == EnvelopeDirection::Decrease ? (adsr.CurrentVolume <= currentPhase.Target) : (adsr.CurrentVolume >= currentPhase.Target);
			if (goToNextPhase && adsr.Phase != ADSRPhaseType::Sustain) {
				adsr.CurrentVolume = currentPhase.Target;
				adsr.Phase = (ADSRPhaseType)((U8)adsr.Phase + 1);
			}
		}
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
				mVoices[i].KeyOn = ESX_TRUE;
			}
		}
	}

	void SPU::setVoicesOnUpper(U16 value)
	{
		for (U32 i = 0; i < 8; i++) {
			if (value & (1 << i)) {
				mVoices[16 + i].KeyOn = ESX_TRUE;
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
				stopVoice(mVoices[i]);
			}
		}
	}

	void SPU::setVoicesOffUpper(U16 value)
	{
		for (U32 i = 0; i < 8; i++) {
			if (value & (1 << i)) {
				stopVoice(mVoices[16 + i]);
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
		value |= (mSPUControl.Unmute << 14);
		value |= (mSPUControl.Enable << 15);

		return value;
	}

	void SPU::setControlRegister(U16 value)
	{
		BIT oldEnable = mSPUControl.Enable;

		mSPUControl.CDAudioEnable = (value >> 0) & 0x1;
		mSPUControl.ExternalAudioEnable = (value >> 1) & 0x1;
		mSPUControl.CDAudioReverb = (value >> 2) & 0x1;
		mSPUControl.ExternalAudioReverb = (value >> 3) & 0x1;
		mSPUControl.TransferMode = (TransferMode)((value >> 4) & 0x3);
		mSPUControl.IRQ9Enable = (value >> 6) & 0x1;
		mSPUControl.ReverbMasterEnable = (value >> 7) & 0x1;
		mSPUControl.NoiseFrequencyStep = (value >> 8) & 0x3;
		mSPUControl.NoiseFrequencyShift = (value >> 10) & 0xF;
		mSPUControl.Unmute = (value >> 14) & 0x1;
		mSPUControl.Enable = (value >> 15) & 0x1;

		if (mSPUControl.IRQ9Enable == ESX_FALSE && mSPUStatus.IRQ9Flag == ESX_TRUE) {
			mSPUStatus.IRQ9Flag = ESX_FALSE;
		}

		if (!oldEnable && mSPUControl.Enable) {
			SchedulerEvent spuSample = {
				.Type = SchedulerEventType::SPUSample,
				.ClockStart = mCPU->getClocks(),
				.ClockTarget = ((mCPU->getClocks() / 768) * 768) + 768,
				.Reschedule = ESX_TRUE,
				.RescheduleClocks = 768
			};
			Scheduler::UnScheduleAllEvents(SchedulerEventType::SPUSample);
			Scheduler::ScheduleEvent(spuSample);
		}
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

		value |= (mDataTransferControl.Unknown1 << 0);
		value |= (mDataTransferControl.TransferType << 1);
		value |= (mDataTransferControl.Unknown2 << 4);

		return value;
	}

	void SPU::setDataTransferControl(U16 value)
	{
		mDataTransferControl.Unknown1 = (value >> 0) & 0x1;
		mDataTransferControl.TransferType = (value >> 1) & 0x7;
		mDataTransferControl.Unknown2 = (value >> 4) & 0xFFF;

		if (mDataTransferControl.TransferType != 2) {
			ESX_CORE_LOG_ERROR("{} Data Transfer Control {} not supported yet", __FUNCTION__, mDataTransferControl.TransferType);
		}
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
		return mReverb[reg];
	}

	void SPU::setReverbConfigurationAreaRegister(ReverbRegister reg, U16 value)
	{
		if (reg == mBASE) {
			mCurrentBufferAddress = value * 8;
		}

		mReverb[reg] = value;
	}

	static Array<I16, 39> fir_filter_coefficients = {
		 -0x0001,  0x0000,  0x0002,  0x0000, -0x000A,  0x0000,  0x0023,  0x0000,
		 -0x0067,  0x0000,  0x010A,  0x0000, -0x0268,  0x0000,  0x0534,  0x0000,
		 -0x0B90,  0x0000,  0x2806,  0x4000,  0x2806,  0x0000, -0x0B90,  0x0000,
		  0x0534,  0x0000, -0x0268,  0x0000,  0x010A,  0x0000, -0x0067,  0x0000,
		  0x0023,  0x0000, -0x000A,  0x0000,  0x0002,  0x0000, -0x0001,
	};
	I16 SPU::reverbFirFilter(I16 sample)
	{
		I32 output = 0;

		for (I16 coefficient : fir_filter_coefficients) {
			output += sample * coefficient;
		}

		return output >> 15;
	}

}