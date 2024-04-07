#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include <Core/IRenderer.h>

namespace esx {

	class GPU;
	using CommandFunction = void(GPU::*)();

	constexpr U64 PAL_SCANLINES_PER_FRAME = 314;
	constexpr U64 PAL_CLOCKS_PER_SCANLINE = 3406;

	enum class SemiTransparency : U8 {
		B2PlusF2 = 0,
		BPlusF = 1,
		BMinusF = 2,
		BPlusF4 = 3
	};

	enum class TexturePageColors : U8 {
		T4Bit = 0,
		T8Bit = 1,
		T15Bit = 2,
		Reserved = 3
	};

	enum class HorizontalResolution : U8 {
		H256 = 0b0000,
		H368 = 0b0001,
		H320 = 0b0010,
		H512 = 0b0100,
		H640 = 0b0110
	};

	enum class VerticalResolution : U8 {
		V240 = 0,
		V480 = 1
	};

	enum class VideoMode : U8 {
		NTSC = 0,
		PAL = 1
	};

	enum class ColorDepth : U8 {
		C15Bit = 0,
		C24Bit = 1
	};

	enum class DMADirection : U8 {
		Off = 0,
		FIFO = 1,
		CPUToGP0 = 2,
		GPUREADtoCPU = 3
	};

	struct CommandBuffer {
		Array<U32, 16> Data;
		U64 Length;

		void push(U32 word);
		U32 pop();
		void clear();
	};


	enum class BaseCase {
		RemainingParameters,
		PolyLine
	};

	struct Command {
		CommandFunction Function = nullptr;
		BaseCase BaseCase = BaseCase::RemainingParameters;
		U64 RemainingParameters;
		BIT Complete = ESX_FALSE;

		BIT IsComplete(U32 instruction);
	};

	enum class GP0Mode {
		Command,
		VRAMtoVRAM,
		CPUtoVRAM,
		VRAMtoCPU
	};



	struct GPUStat {
		U8 TexturePageX = 0;
		U8 TexturePageYBase1 = 0;
		SemiTransparency SemiTransparency = SemiTransparency::B2PlusF2;
		TexturePageColors TexturePageColors = TexturePageColors::T4Bit;
		BIT DitherEnabled = ESX_FALSE;
		BIT DrawToDisplay = ESX_FALSE;
		BIT SetMaskWhenDrawingPixels = ESX_FALSE;
		BIT DrawPixels = ESX_FALSE;
		BIT InterlaceField = ESX_FALSE;
		BIT ReverseFlag = ESX_FALSE;
		U8 TexturePageYBase2 = 0;
		HorizontalResolution HorizontalResolution = HorizontalResolution::H256;
		VerticalResolution VerticalResolution = VerticalResolution::V240;
		VideoMode VideoMode = VideoMode::PAL;
		ColorDepth ColorDepth = ColorDepth::C15Bit;
		BIT VerticalInterlace = ESX_FALSE;
		BIT DisplayEnable = ESX_FALSE;
		BIT InterruptRequest = ESX_FALSE;
		BIT ReadyCmdWord = ESX_FALSE;
		BIT ReadySendVRAMToCPU = ESX_FALSE;
		BIT ReadyToReceiveDMABlock = ESX_FALSE;
		DMADirection DMADirection = DMADirection::Off;
		BIT DrawOddLines = ESX_FALSE;
	};

	class Timer;
	class InterruptControl;

	class GPU : public BusDevice {
	public:
		GPU(const SharedPtr<IRenderer>& renderer);
		~GPU() = default;

		void store(const StringView& busName, U32 address, U32 value) override;
		void load(const StringView& busName, U32 address, U32& output) override;

	
		void gp0(U32 instruction);
		void gp1(U32 instruction);
		U32 gpuRead();

		void clock();

	private:
		//Misc commands
		Command gp0MiscCommands(U32 instruction) const;
		void gp0Nop();
		void gp0ClearCacheCommand();
		void gp0Unknown();
		void gp0QuickRectangleFillCommand();
		void gp0InterruptRequest();

		//Polygon primitive
		Command gp0PolygonPrimitiveCommands(U32 instruction) const;
		void gp0DrawPolygonPrimitiveCommand();

		//Line primitive
		Command gp0LinePrimitiveCommands(U32 instruction) const;
		void gp0DrawLinePrimitiveCommand();

		//Rectangle primitive
		Command gp0RectanglePrimitiveCommands(U32 instruction) const;
		void gp0DrawRectanglePrimitiveCommand();

		//VRAM to VRAM blitting
		Command gp0VRAMtoVRAMBlitCommands(U32 instruction) const;
		void gp0VRAMtoVRAMBlitCommand();

		//CPU to VRAM blitting
		Command gp0CPUtoVRAMBlitCommands(U32 instruction) const;
		void gp0CPUtoVRAMBlitCommand();

		//VRAM to CPU blitting
		Command gp0VRAMtoCPUBlitCommands(U32 instruction) const;
		void gp0VRAMtoCPUBlitCommand();

		//Environment commands
		Command gp0EnvironmentCommands(U32 instruction) const;
		void gp0DrawSettingCommand();
		void gp0TextureWindowSettingCommand();
		void gp0SetDrawingAreaTopLeftCommand();
		void gp0SetDrawingAreaBottomRightCommand();
		void gp0SetDrawingOffsetCommand();
		void gp0MaskBitSettingCommand();


		void gp1Reset();
		void gp1ResetCommandBuffer();
		void gp1AckInterrupt();
		void gp1SetDisplayEnable(U32 instruction);
		void gp1SetDMADirection(U32 instruction);
		void gp1SetVRAMStart(U32 instruction);
		void gp1SetHorizontalRange(U32 instruction);
		void gp1SetVerticalRange(U32 instruction);
		void gp1SetDisplayMode(U32 instruction);
		void gp1SetTextureDisable(U32 instruction);
		void gp1Unknown(U32 instruction);
		void gp1GetGPUInfo(U32 instruction);
		void gp1SetTextureDisableSpecial(U32 instruction);

		U32 getGPUStat();

		static HorizontalResolution fromFields(U8 hr1, U8 hr2);
		static Vertex unpackVertex(U32 value);
		static UV unpackUV(U32 value);
		static Color unpackColor(U32 value);
		static Color unpackColor(U16 value);
		static U32 fromTexPageToVRAMAddress(U16 texPage);
		static U32 fromClutToVRAMAddress(U16 clut);
		static U32 fromCoordsToVRAMAddress(U32 coords);
		static void transformUV(UV& uv, U16 tx, U16 ty, U8 bpp);

	private:
		GPUStat	mGPUStat = {};
		BIT mTexturedRectangleXFlip = ESX_FALSE;
		BIT mTexturedRectangleYFlip = ESX_FALSE;
		U8 mTextureWindowMaskX = 0x00;
		U8 mTextureWindowMaskY = 0x00;
		U8 mTextureWindowOffsetX = 0x00;
		U8 mTextureWindowOffsetY = 0x00;
		U16 mDrawAreaTopLeftX = 0x0000;
		U16 mDrawAreaTopLeftY = 0x0000;
		U16 mDrawAreaBottomRightX = 0x0000;
		U16 mDrawAreaBottomRightY = 0x0000;
		I16 mDrawOffsetX = 0x0000;
		I16 mDrawOffsetY = 0x0000;
		U16 mVRAMStartX = 0x0000;
		U16 mVRAMStartY = 0x0000;
		U16 mHorizontalRangeStart = 0x0000;
		U16 mHorizontalRangeEnd = 0x0000;
		U16 mVerticalRangeStart = 0x0000;
		U16 mVerticalRangeEnd = 0x0000;

		CommandBuffer mCommandBuffer = {};
		Command mCurrentCommand = {};

		GP0Mode mMode = GP0Mode::Command;

		U16 mMemoryTransferX = 0x0000;
		U16 mMemoryTransferY = 0x0000;
		U16 mMemoryTransferSourceCoordsX = 0x0000;
		U16 mMemoryTransferSourceCoordsY = 0x0000;
		U16 mMemoryTransferDestinationCoordsX = 0x0000;
		U16 mMemoryTransferDestinationCoordsY = 0x0000;
		U16 mMemoryTransferWidth = 0x0000;
		U16 mMemoryTransferHeight = 0x0000;
		U32 mNumWordsToTransfer = 0x00000000;
		U32 mCurrentWordNumber = 0x00000000;

		float mClocks = 0;
		float mDotClocks = 0;
		U64 mCurrentScanLine = 0;
		BIT mVBlank = ESX_FALSE;

		SharedPtr<IRenderer> mRenderer = {};
		SharedPtr<Timer> mTimer = {};
		SharedPtr<InterruptControl> mInterruptControl = {};

	};

}