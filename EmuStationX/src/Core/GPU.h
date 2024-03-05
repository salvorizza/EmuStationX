#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include <Core/IRenderer.h>

namespace esx {

	class GPU;
	using CommandFunction = void(GPU::*)();

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
		CommandFunction Function;
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
		U8 TexturePageX;
		U8 TexturePageYBase1;
		SemiTransparency SemiTransparency;
		TexturePageColors TexturePageColors;
		BIT DitherEnabled;
		BIT DrawToDisplay;
		BIT SetMaskWhenDrawingPixels;
		BIT DrawPixels;
		BIT InterlaceField;
		BIT ReverseFlag;
		U8 TexturePageYBase2;
		HorizontalResolution HorizontalResolution;
		VerticalResolution VerticalResolution;
		VideoMode VideoMode;
		ColorDepth ColorDepth;
		BIT VerticalInterlace;
		BIT DisplayEnable;
		BIT InterruptRequest;
		BIT ReadyCmdWord;
		BIT ReadySendVRAMToCPU;
		BIT ReadyToReceiveDMABlock;
		DMADirection DMADirection;
		BIT DrawOddLines;
	};

	class GPU : public BusDevice {
	public:
		GPU(const SharedPtr<IRenderer>& renderer);
		~GPU() = default;

		void store(const StringView& busName, U32 address, U32 value) override;
		void load(const StringView& busName, U32 address, U32& output) override;

	
		void gp0(U32 instruction);
		void gp1(U32 instruction);

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
		GPUStat	mGPUStat;
		BIT mTexturedRectangleXFlip, mTexturedRectangleYFlip;
		U8 mTextureWindowMaskX, mTextureWindowMaskY, mTextureWindowOffsetX, mTextureWindowOffsetY;
		U16 mDrawAreaTopLeftX, mDrawAreaTopLeftY;
		U16 mDrawAreaBottomRightX, mDrawAreaBottomRightY;
		I16 mDrawOffsetX, mDrawOffsetY;
		U16 mVRAMStartX, mVRAMStartY;
		U16 mHorizontalRangeStart, mHorizontalRangeEnd;
		U16 mVerticalRangeStart, mVerticalRangeEnd;

		CommandBuffer mCommandBuffer;
		Command mCurrentCommand;

		GP0Mode mMode = GP0Mode::Command;

		SharedPtr<IRenderer> mRenderer;

		U16 mMemoryTransferX, mMemoryTransferY;
		U16 mMemoryTransferCoordsX, mMemoryTransferCoordsY;
		U16 mMemoryTransferWidth, mMemoryTransferHeight;
	};

}