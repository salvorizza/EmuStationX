#pragma once

#include <array>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	enum class VolumeMode : U8 {
		Volume,
		Sweep
	};


	enum class SweepPhase : U8 {
		Positive,
		Negative
	};

	enum class NoiseMode : U8 {
		ADPCM,
		Noise
	};

	enum class ReverbMode : U8 {
		ToMixer,
		ToMixerAndToReverb
	};

	enum class TransferMode : U8 {
		Stop,
		ManualWrite,
		DMAwrite,
		DMAread
	};

	enum ReverbRegister {
		vLOUT,
		vROUT,
		mBASE,
		dAPF1,
		dAPF2,
		vIIR,
		vCOMB1,
		vCOMB2,
		vCOMB3,
		vCOMB4,
		vWALL,
		vAPF1,
		vAPF2,
		mLSAME,
		mRSAME,
		mLCOMB1,
		mRCOMB1,
		mLCOMB2,
		mRCOMB2,
		dLSAME,
		dRSAME,
		mLDIFF,
		mRDIFF,
		mLCOMB3,
		mRCOMB3,
		mLCOMB4,
		mRCOMB4,
		dLDIFF,
		dRDIFF,
		mLAPF1,
		mRAPF1,
		mLAPF2,
		mRAPF2,
		vLIN,
		vRIN
	};

	enum class EnvelopeMode : U8 {
		Linear,
		Exponential
	};

	enum class EnvelopeDirection : U8 {
		Increase,
		Decrease
	};


	struct EnvelopePhase {
		EnvelopeMode Mode = EnvelopeMode::Linear;
		EnvelopeDirection Direction = EnvelopeDirection::Increase;
		U8 Shift = 0x00;
		U8 Step = 0x00;
		I16 Target = 0x0000;

		EnvelopePhase()
		{}

		EnvelopePhase(EnvelopeMode mode, EnvelopeDirection direction, U8 shift, U8 step, I16 target)
			:	Mode(mode),
				Direction(direction),
				Shift(shift),
				Step(step),
				Target(target)
		{}
	};

	struct Volume {
		VolumeMode VolumeMode = VolumeMode::Volume;
		I16 Level = 0x0000;

		EnvelopePhase Envelope;
		SweepPhase SweepPhase = SweepPhase::Positive;

		U32 Tick = 0;
		I16 Step = 0;
	};

	enum class ADSRPhaseType : U8 {
		Attack,
		Decay,
		Sustain,
		Release,
		Off
	};

	struct ADPCMBlock {
		U8 ShiftFilter = 0;
		U8 Flags = 0;
		Array<U8, 14> Data = {};

		U8 Shift() const {
			const U8 shift =  (ShiftFilter & 0xF);
			return (shift > 12) ? 9 : shift;
		}

		U8 Filter() const {
			return (ShiftFilter >> 4) & 0x7;
		}

		U8 GetNibble(U8 nibble) const {
			const U8 byteIndex = nibble / 2;
			const U8 nibbleIndex = nibble % 2;

			return (Data[byteIndex] >> (nibbleIndex * 4)) & 0xF;
		}
	};

	struct ADSR {
		Array<EnvelopePhase, 4> Phases = {
			EnvelopePhase(EnvelopeMode::Linear,EnvelopeDirection::Increase,0x00,0x00,0x7FFF),//Attack
			EnvelopePhase(EnvelopeMode::Exponential,EnvelopeDirection::Decrease,0x00,0x00,0x00),//Decay
			EnvelopePhase(EnvelopeMode::Linear,EnvelopeDirection::Increase,0x00,0x00,0x00),//Sustain
			EnvelopePhase(EnvelopeMode::Linear,EnvelopeDirection::Decrease,0x00,0x00,0x00),//Release
		};

		U8 SustainLevel = 0x00;

		I16 CurrentVolume = 0;
		I16 Step = 0;

		ADSRPhaseType Phase = ADSRPhaseType::Off;
		U32 Tick = 0;
	};

	struct StereoVolume {
		I16 Left = 0x0000;
		I16 Right = 0x0000;
	};

	struct Voice {
		U32 Number = 0;
		Volume VolumeLeft = {};
		Volume VolumeRight = {};/*1F801C00*/
		U16 ADPCMSampleRate = 0x0000;/*1F801C04*/
		U16 ADPCMStartAddress = 0x0000;/*1F801C06*/
		ADSR ADSR = {};/*1F801C08*/
		U16 ADPCMRepeatAddress = 0x0000;/*1F801C0E*/
		StereoVolume CurrentVolume = {};/*1F801E00*/
		BIT ReachedLoopEnd = ESX_TRUE;
		NoiseMode NoiseMode = NoiseMode::ADPCM;
		BIT PitchModulation = ESX_FALSE;
		ReverbMode ReverbMode = ReverbMode::ToMixer;
		U16 ADPCMCurrentAddress = 0x0000;

		I16 Latest = 0;
		U32 PitchCounter = 0;

		BIT HasSamples = ESX_FALSE;
		Array<I16, 31> CurrentSamples = {}; //28 + 3
		Array<I16, 2> LastSamples = {};
		U8 CurrentBlockFlags = 0;

		BIT KeyOn = ESX_FALSE;
		BIT KeyOff = ESX_FALSE;

		inline U8 getSampleIndex() const {
			return (PitchCounter >> 12);
		}

		inline U8 getInterpolationIndex() const {
			return (PitchCounter >> 4) & 0xFF;
		}

		inline void setSampleIndex(U8 value) {
			PitchCounter &= 0xFFF;
			PitchCounter |= (value << 12);
		}
	};

	struct DataTransferControl {
		U8 TransferType = 0x00;
	};

	struct SPUControl {
		BIT Enable = ESX_FALSE;
		BIT Unmute = ESX_FALSE;
		U8 NoiseFrequencyShift = 0x00;
		U8 NoiseFrequencyStep = 0x00;
		BIT ReverbMasterEnable = ESX_FALSE;
		BIT IRQ9Enable = ESX_FALSE;
		TransferMode TransferMode = TransferMode::Stop;
		BIT ExternalAudioReverb = ESX_FALSE;
		BIT CDAudioReverb = ESX_FALSE;
		BIT ExternalAudioEnable = ESX_FALSE;
		BIT CDAudioEnable = ESX_FALSE;
	};

	struct SPUStatus {
		BIT CDAudioEnable = ESX_FALSE;
		BIT ExternalAudioEnable = ESX_FALSE;
		BIT CDAudioReverb = ESX_FALSE;
		BIT ExternalAudioReverb = ESX_FALSE;
		BIT WriteToSecondHalf = ESX_FALSE;
		BIT DataTransferBusyFlag = ESX_FALSE;
		BIT DataTransferDMAReadRequest = ESX_FALSE;
		BIT DataTransferDMAWriteRequest = ESX_FALSE;
		BIT IRQ9Flag = ESX_FALSE;
		TransferMode TransferMode = TransferMode::Stop;
	};

	#define SATURATE(x) std::clamp((x), -0x8000, 0x7FFF)
	

	class SPU : public BusDevice {
		friend class SPUStatusPanel;
	public:
		SPU();
		~SPU();

		virtual void clock(U64 clocks) override;

		virtual void store(const StringView& busName, U32 address, U16 value) override;
		virtual void load(const StringView& busName, U32 address, U16& output) override;


	private:
		ADPCMBlock readADPCMBlock(U16 address);
		void decodeBlock(Voice& voice, const ADPCMBlock& block);

		Pair<I32, I32> sampleVoice(Voice& voice);
		void tickADSR(Voice& voice);

		Pair<I16, I16> reverb(I16 LeftInput, I16 RightInput);
		I16 loadReverb(U16 addr);
		void writeReverb(U16 addr,I16 value);

		void startVoice(Voice& voice);
		void stopVoice(Voice& voice);

		U16 getVoiceVolumeLeft(U8 voice);
		void setVoiceVolumeLeft(U8 voice, U16 value);
		
		U16 getVoiceVolumeRight(U8 voice);
		void setVoiceVolumeRight(U8 voice, U16 value);

		U16 getVoiceADPCMSampleRate(U8 voice);
		void setVoiceADPCMSampleRate(U8 voice, U16 value);

		U16 getVoiceADPCMStartAddress(U8 voice);
		void setVoiceADPCMStartAddress(U8 voice, U16 value);

		U32 getVoiceADSR(U8 voice);
		void setVoiceADSR(U8 voice, U32 value);

		U16 getVoiceADSRUpper(U8 voice);
		void setVoiceADSRUpper(U8 voice, U16 value);

		U16 getVoiceADSRLower(U8 voice);
		void setVoiceADSRLower(U8 voice, U16 value);

		U16 getVoiceCurrentADSRVolume(U8 voice);
		void setVoiceCurrentADSRVolume(U8 voice, U16 value);

		U32 getVoiceCurrentVolume(U8 voice);
		void setVoiceCurrentVolume(U8 voice, U32 value);

		U16 getVoiceCurrentVolumeLeft(U8 voice);
		void setVoiceCurrentVolumeLeft(U8 voice, U16 value);

		U16 getVoiceCurrentVolumeRight(U8 voice);
		void setVoiceCurrentVolumeRight(U8 voice, U16 value);

		U16 getVoiceADPCMRepeatAddress(U8 voice);
		void setVoiceADPCMRepeatAddress(U8 voice, U16 value);

		void setVoicesOn(U32 value);
		void setVoicesOnLower(U16 value);
		void setVoicesOnUpper(U16 value);

		void setVoicesOff(U32 value);
		void setVoicesOffLower(U16 value);
		void setVoicesOffUpper(U16 value);

		void setVoicesPitchModulation(U32 value);
		void setVoicesPitchModulationLower(U16 value);
		void setVoicesPitchModulationUpper(U16 value);

		U32 getVoicesStatus();

		U32 getNoiseMode();
		U16 getNoiseModeLower();
		U16 getNoiseModeUpper();

		void setNoiseMode(U32 value);
		void setNoiseModeLower(U16 value);
		void setNoiseModeUpper(U16 value);

		U32 getReverbMode();
		U16 getReverbModeLower();
		U16 getReverbModeUpper();

		void setReverbMode(U32 value);
		void setReverbModeLower(U16 value);
		void setReverbModeUpper(U16 value);

		U16 getUnknown1F801DA0();
		void setUnknown1F801DA0(U16 value);

		U16 getControlRegister();
		void setControlRegister(U16 value);

		U16 getStatusRegister();

		U16 getDataTransferAdress();
		void setDataTransferAdress(U16 value);

		void setDataTransferFifo(U16 value);

		U16 getDataTransferControl();
		void setDataTransferControl(U16 value);

		U32 getCurrentMainVolume();
		void setCurrentMainVolume(U32 value);

		U16 getCurrentMainVolumeLeft();
		void setCurrentMainVolumeLeft(U16 value);

		U16 getCurrentMainVolumeRight();
		void setCurrentMainVolumeRight(U16 value);

		U16 getMainVolumeLeft();
		void setMainVolumeLeft(U16 value);

		U16 getMainVolumeRight();
		void setMainVolumeRight(U16 value);

		U32 getCDInputVolume();
		void setCDInputVolume(U32 value);

		U16 getCDInputVolumeLeft();
		void setCDInputVolumeLeft(U16 value);

		U16 getCDInputVolumeRight();
		void setCDInputVolumeRight(U16 value);

		U16 getSoundRAMIRQAddress();
		void setSoundRAMIRQAddress(U16 value);

		U32 getExternalInputVolume();
		void setExternalInputVolume(U32 value);

		U16 getExternalInputVolumeLeft();
		void setExternalInputVolumeLeft(U16 value);

		U16 getExternalInputVolumeRight();
		void setExternalInputVolumeRight(U16 value);

		U16 getReverbConfigurationAreaRegister(ReverbRegister reg);
		void setReverbConfigurationAreaRegister(ReverbRegister reg, U16 value);

		static I16 reverbFirFilter(I16 sample);
	private:
		Array<Voice, 24> mVoices = {};
		Volume mMainVolumeLeft = {};
		Volume mMainVolumeRight = {};
		StereoVolume mCurrentMainVolume = {};
		StereoVolume mCDInputVolume = {};
		StereoVolume mExternalInputVolume = {};
		SPUControl mSPUControl = {};
		SPUStatus mSPUStatus = {};
		U16 mDataTransferAddress = 0x0000;
		DataTransferControl mDataTransferControl = {};
		U16 mUnknown1F801DA0 = 0x0000;
		U16 mSoundRAMIRQAddress = 0x0000;

		I32 mNoiseTimer = 0;
		I16 mNoiseLevel = 0;

		//Reverb configuration Area
		Array<U16, 35> mReverb = {};
		U32 mCurrentBufferAddress = 0x00000000;

		Queue<U16> mFIFO = {};
		Vector<U8> mRAM = {};

		U32 mCurrentTransferAddress = 0;
	public:
		Array<I16, 441 * 2> mSamples = {}; U32 mWriteSample = 0; U32 mReadSample = 0; U32 mFrameCount = 0;
	};

}