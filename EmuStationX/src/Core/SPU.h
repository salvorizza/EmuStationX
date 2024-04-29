#pragma once

#include <array>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	enum class VolumeMode : U8 {
		Volume,
		Sweep
	};

	enum class Mode : U8 {
		Linear,
		Exponential
	};

	enum class SPUDirection : U8 {
		Increase,
		Decrease
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

	enum class ReverbRegister {
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

	struct Volume {
		VolumeMode VolumeMode;
		I16 Volume;

		Mode SweepMode;
		SPUDirection SweepDirection;
		SweepPhase SweepPhase;
		U8 SweepShift;
		U8 SweepStep;
	};

	struct ADSR {
		Mode AttackMode;
		SPUDirection AttackDirection = SPUDirection::Increase; //Fixed
		U8 AttackShift;
		U8 AttackStep;
		Mode DecayMode = Mode::Exponential; //Fixed
		SPUDirection DecayDirection = SPUDirection::Decrease;//Fixed
		U8 DecayShift;
		U8 DecayStep = 0; //Fixed
		U8 SustainLevel;
		Mode SustainMode;
		SPUDirection SustainDirection;
		U8 SustainShift;
		U8 SustainStep;
		Mode ReleaseMode;
		SPUDirection ReleaseDirection = SPUDirection::Decrease;//Fixed
		U8 ReleaseShift;
		U8 ReleaseStep = 0;//Fixed
	};

	struct StereoVolume {
		I16 Left;
		I16 Right;
	};

	struct Voice {
		U32 Number;
		Volume VolumeLeft, VolumeRight;/*1F801C00*/
		U16 ADPCMSampleRate;/*1F801C04*/
		U16 ADPCMStartAddress;/*1F801C06*/
		ADSR ADSR;/*1F801C08*/
		I16 ADSRCurrentVolume;/*1F801C0C*/
		U16 ADPCMRepeatAddress;/*1F801C0E*/
		StereoVolume CurrentVolume;/*1F801E00*/
		BIT ReachedLoopEnd = ESX_TRUE;
		NoiseMode NoiseMode;
		BIT PitchModulation = ESX_FALSE;
		ReverbMode ReverbMode;
	};

	struct DataTransferControl {
		U8 TransferType;
	};

	struct SPUControl {
		BIT Enable;
		BIT Mute;
		U8 NoiseFrequencyShift;
		U8 NoiseFrequencyStep;
		BIT ReverbMasterEnable;
		BIT IRQ9Enable;
		TransferMode TransferMode;
		BIT ExternalAudioReverb;
		BIT CDAudioReverb;
		BIT ExternalAudioEnable;
		BIT CDAudioEnable;
	};

	struct SPUStatus {
		BIT WriteToSecondHalf;
		BIT DataTransferBusyFlag;
		BIT DataTransferDMAReadRequest;
		BIT DataTransferDMAWriteRequest;
		BIT IRQ9Flag;
	};
	

	class SPU : public BusDevice {
	public:
		SPU();
		~SPU();

		virtual void store(const StringView& busName, U32 address, U16 value) override;
		virtual void load(const StringView& busName, U32 address, U16& output) override;

	private:
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

		U16 getReverbVolumeLeft();
		void setReverbVolumeLeft(U16 value);

		U16 getReverbVolumeRight();
		void setReverbVolumeRight(U16 value);

		U32 getCDInputVolume();
		void setCDInputVolume(U32 value);

		U16 getCDInputVolumeLeft();
		void setCDInputVolumeLeft(U16 value);

		U16 getCDInputVolumeRight();
		void setCDInputVolumeRight(U16 value);

		U16 getReverbWorkAreaStartAddress();
		void setReverbWorkAreaStartAddress(U16 value);

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
	private:
		Array<Voice, 24> mVoices;
		Volume mMainVolumeLeft, mMainVolumeRight;
		Volume mReverbVolumeLeft, mReverbVolumeRight;
		StereoVolume mCurrentMainVolume;
		StereoVolume mCDInputVolume, mExternalInputVolume;
		SPUControl mSPUControl;
		SPUStatus mSPUStatus;
		U16 mDataTransferAddress;
		DataTransferControl mDataTransferControl;
		U16 mUnknown1F801DA0;
		U16 mReverbWorkAreaStartAddress;
		U16 mSoundRAMIRQAddress;

		//Reverb configuration Area
		Array<U16, 32> mReverbConfigurationArea;

		Vector<U8> mRAM;

		U16 mCurrentTransferAddress;
	};

}