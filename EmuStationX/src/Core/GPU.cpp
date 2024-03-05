#include "GPU.h"

#include <stdio.h>

namespace esx {

	GPU::GPU(const SharedPtr<IRenderer>& renderer)
		:	BusDevice(ESX_TEXT("GPU")),
			mRenderer(renderer)
	{
		addRange(ESX_TEXT("Root"), 0x1F801810, BYTE(0x9), 0xFFFFFFFF);
		mCurrentCommand.Complete = ESX_TRUE;
	}

	void GPU::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			case 0x1F801810: {
				gp0(value);
				break;
			}
			case 0x1F801814: {
				gp1(value);
				break;
			}
		}

		ESX_CORE_LOG_WARNING("GPU - Writing to address {:08x} value {:08x}", address, value);
	}

	void GPU::load(const StringView& busName, U32 address, U32& output)
	{
		switch (address) {
			case 0x1F801810: {
				output = 0;
				break;
			}
			case 0x1F801814: {
				output = getGPUStat();
				break;
			}
		}
		//ESX_CORE_LOG_WARNING("GPU - Reading address {:08x} not implemented yet", address);
	}

	void GPU::gp0(U32 instruction)
	{
		if (mCurrentCommand.Complete == ESX_TRUE) {
			U8 opcode = (instruction >> 29) & 0x7;
			switch (opcode) {
				case 0b000:
					mCurrentCommand = gp0MiscCommands(instruction);
					break;

				case 0b001:
					mCurrentCommand = gp0PolygonPrimitiveCommands(instruction);
					break;

				case 0b010:
					mCurrentCommand = gp0LinePrimitiveCommands(instruction);
					break;

				case 0b011:
					mCurrentCommand = gp0RectanglePrimitiveCommands(instruction);
					break;

				case 0b100:
					mCurrentCommand = gp0VRAMtoVRAMBlitCommands(instruction);
					break;

				case 0b101:
					mCurrentCommand = gp0CPUtoVRAMBlitCommands(instruction);
					break;

				case 0b110:
					mCurrentCommand = gp0VRAMtoCPUBlitCommands(instruction);
					break;

				case 0b111:
					mCurrentCommand = gp0EnvironmentCommands(instruction);
					break;
			}
			
			mCommandBuffer.clear();
		}

		if (mCurrentCommand.RemainingParameters > 0) {
			mCurrentCommand.RemainingParameters--;
		}

		switch (mMode) {
			case GP0Mode::Command: {
				mCommandBuffer.push(instruction);

				if (mCurrentCommand.IsComplete(instruction)) {
					(this->*(mCurrentCommand.Function))();
					if (mCurrentCommand.RemainingParameters == 0) {
						mCurrentCommand.Complete = ESX_TRUE;
					}
				}
				break;
			}

			case GP0Mode::VRAMtoVRAM: {
				//Copy VRAM
				ESX_CORE_LOG_ERROR("VRAMtoVRAM not implemented yet");


				if (mCurrentCommand.IsComplete(instruction)) {
					mCurrentCommand.Complete = ESX_TRUE;
					mMode = GP0Mode::Command;
				}
				break;
			}

			case GP0Mode::CPUtoVRAM: {
				//Copy from CPU
				U16 pixel1 = (instruction >> 16) & 0xFFFF;
				U16 pixel2 = (instruction >> 0) & 0xFFFF;

				mRenderer->VRAMWrite(mMemoryTransferCoordsX + mMemoryTransferX, mMemoryTransferCoordsY + mMemoryTransferY, pixel2);
				mMemoryTransferX++;
				if (mMemoryTransferX >= mMemoryTransferWidth) {
					mMemoryTransferX = 0;
					mMemoryTransferY++;
				}

				mRenderer->VRAMWrite(mMemoryTransferCoordsX + mMemoryTransferX, mMemoryTransferCoordsY + mMemoryTransferY, pixel1);
				mMemoryTransferX++;
				if (mMemoryTransferX >= mMemoryTransferWidth) {
					mMemoryTransferX = 0;
					mMemoryTransferY++;
				}

				if (mCurrentCommand.IsComplete(instruction)) {
					mMemoryTransferX = mMemoryTransferY = 0;

					mCurrentCommand.Complete = ESX_TRUE;
					mMode = GP0Mode::Command;
				}
				break;
			}

			case GP0Mode::VRAMtoCPU: {
				//Copy to CPU
				ESX_CORE_LOG_ERROR("VRAMtoCPU not implemented yet");

				if (mCurrentCommand.IsComplete(instruction)) {
					mCurrentCommand.Complete = ESX_TRUE;
					mMode = GP0Mode::Command;
				}
				break;
			}
		}
		
	}

	void GPU::gp1(U32 instruction)
	{
		U8 command = (instruction >> 24) & 0xFF;
		switch (command & 0x3F)
		{
			case 0x00:
				gp1Reset();
				break;

			case 0x01:
				gp1ResetCommandBuffer();
				break;

			case 0x02:
				gp1AckInterrupt();
				break;

			case 0x03:
				gp1SetDisplayEnable(instruction);
				break;

			case 0x04:
				gp1SetDMADirection(instruction);
				break;

			case 0x05:
				gp1SetVRAMStart(instruction);
				break;

			case 0x06:
				gp1SetHorizontalRange(instruction);
				break;

			case 0x07:
				gp1SetVerticalRange(instruction);
				break;

			case 0x08:
				gp1SetDisplayMode(instruction);
				break;

			case 0x09:
				gp1SetTextureDisable(instruction);
				break;

			case 0x0B:
				gp1Unknown(instruction);
				break;

			case 0x10:
			case 0x11:
			case 0x12:
			case 0x13:
			case 0x14:
			case 0x15:
			case 0x16:
			case 0x17:
			case 0x18:
			case 0x19:
			case 0x1A:
			case 0x1B:
			case 0x1C:
			case 0x1D:
			case 0x1E:
			case 0x1F:
				gp1GetGPUInfo(instruction);
				break;

			case 0x20:
				gp1SetTextureDisableSpecial(instruction);
				break;
		}
	}

	Command GPU::gp0MiscCommands(U32 instruction) const
	{
		U8 command = (instruction >> 24) & 0xFF;
		switch (command) {
			case 0x00:
				return Command{
					.Function = &GPU::gp0Nop,
					.RemainingParameters = 1
				};
			case 0x01:
				return Command{
					.Function = &GPU::gp0ClearCacheCommand,
					.RemainingParameters = 1
				};
			case 0x02:
				return Command{
					.Function = &GPU::gp0QuickRectangleFillCommand,
					.RemainingParameters = 3
				};
			case 0x03:
				return Command{
					.Function = &GPU::gp0Unknown,
					.RemainingParameters = 3
				};
			case 0x1F: 
				return Command{
					.Function = &GPU::gp0InterruptRequest,
					.RemainingParameters = 1
				};
			
			default:
				return Command{
					.Function = &GPU::gp0Nop,
					.RemainingParameters = 1
				};
		}
	}

	void GPU::gp0Nop()
	{
	}

	void GPU::gp0ClearCacheCommand()
	{
		//TODO: Cache clear
	}

	void GPU::gp0Unknown()
	{
	}

	void GPU::gp0QuickRectangleFillCommand()
	{
	}

	void GPU::gp0InterruptRequest()
	{
		mGPUStat.InterruptRequest = ESX_TRUE;
	}

	Command GPU::gp0PolygonPrimitiveCommands(U32 instruction) const
	{
		BIT gourad = (instruction >> 28) & 0x1;
		BIT quad = (instruction >> 27) & 0x1;
		BIT textured = (instruction >> 26) & 0x1;

		U64 numVertices = quad ? 4 : 3;
		U64 numAttributesPerVertex = 1 + (gourad ? 1 : 0) + (textured ? 1 : 0);
		U64 numWords = (numVertices * numAttributesPerVertex) + (gourad ? 0 : 1);

		return Command{
			.Function = &GPU::gp0DrawPolygonPrimitiveCommand,
			.RemainingParameters = numWords
		};
	}

	void GPU::gp0DrawPolygonPrimitiveCommand()
	{
		U32 command = mCommandBuffer.Data[0];

		BIT gourad = (command >> 28) & 0x1;
		BIT quad = (command >> 27) & 0x1;
		BIT textured = (command >> 26) & 0x1;
		BIT semiTransparent = (command >> 25) & 0x1;
		BIT rawTexture = (command >> 24) & 0x1;
		Color flatColor = unpackColor(command & 0xFFFFFF);
		U16 page = 0,clut = 0;

		Vector<PolygonVertex> vertices(quad ? 4 : 3);

		size_t numVertices = quad ? 4 : 3;
		for(I32 i = numVertices - 1; i >= 0;i--) {
			if (textured) {
				U32 uvWord = mCommandBuffer.pop();
				vertices[i].uv = unpackUV(uvWord & 0xFFFF);
				switch (i) {
					case 0:
						clut = (uvWord >> 16) & 0xFFFF;
						break;

					case 1:
						page = (uvWord >> 16) & 0xFFFF;
						break;
				}

			}
			vertices[i].vertex = unpackVertex(mCommandBuffer.pop());
			vertices[i].color = gourad ? unpackColor(mCommandBuffer.pop()) : flatColor;
			vertices[i].textured = textured;
		}

		if (textured) {
			U16 tx = (page >> 0) & 0xF;
			U16 ty = (page >> 4) & 0x1;
			SemiTransparency semiTransparency = (SemiTransparency)((page >> 5) & 0x3);
			TexturePageColors texturePageColors = (TexturePageColors)((page >> 7) & 0x3);

			U16 cy = (clut >> 6) & 0x1FF;
			U16 cx = (clut & 0x3F) * 16;


			for (PolygonVertex& vertex : vertices) {
				switch (texturePageColors) {
					case TexturePageColors::T4Bit: vertex.bpp = 4; break;
					case TexturePageColors::T8Bit: vertex.bpp = 8; break;
					case TexturePageColors::T15Bit: vertex.bpp = 16; break;
				}

				vertex.semiTransparency = (U8)semiTransparency;

				transformUV(vertex.uv, tx, ty, vertex.bpp);
				vertex.clutUV = UV(cx, cy);

			}

			ESX_CORE_LOG_TRACE("Page/Clut: {},{}", fromTexPageToVRAMAddress(page), fromClutToVRAMAddress(clut));
		}

		mRenderer->DrawPolygon(vertices);
	}

	Command GPU::gp0LinePrimitiveCommands(U32 instruction) const
	{
		BIT gourad = (instruction >> 28) & 0x1;
		BIT polyLine = (instruction >> 27) & 0x1;

		U64 numVertices = 2;
		U64 numAttributesPerVertex = 1 + (gourad ? 1 : 0);
		U64 numWords = (numVertices * numAttributesPerVertex) + (gourad ? 0 : 1);

		return Command{
			.Function = &GPU::gp0DrawLinePrimitiveCommand,
			.BaseCase = polyLine ? BaseCase::PolyLine : BaseCase::RemainingParameters,
			.RemainingParameters = numWords
		};
	}

	void GPU::gp0DrawLinePrimitiveCommand()
	{
	}

	Command GPU::gp0RectanglePrimitiveCommands(U32 instruction) const
	{
		U8 size = (instruction >> 27) & 0x3;
		BIT textured = (instruction >> 26) & 0x1;
		U64 numAttributes = 2 + (textured ? 1 : 0) + (size == 0 ? 1 : 0);

		return Command{
			.Function = &GPU::gp0DrawRectanglePrimitiveCommand,
			.RemainingParameters = numAttributes
		};
	}

	void GPU::gp0DrawRectanglePrimitiveCommand()
	{
		U32 command = mCommandBuffer.Data[0];
		U8 size = (command >> 27) & 0x3;
		BIT textured = (command >> 26) & 0x1;

		U32 rectSize = size == 0 ? mCommandBuffer.pop() : 0;
		U32 texture = textured ? mCommandBuffer.pop() : 0;
		Vertex vertex = unpackVertex(mCommandBuffer.pop());
		Color color = unpackColor(command & 0xFFFFFF);

		U16 width = 0;
		U16 height = 0;

		switch (size) {
			case 0b00:
				width = rectSize & 0xFFFF;
				height = (rectSize >> 16) & 0xFFFF;
			break;

			case 0b01:
				width = height = 1;
				break;

			case 0b10:
				width = height = 8;
				break;

			case 0b11:
				width = height = 16;
				break;
		}

		mRenderer->DrawRectangle(vertex, width, height, color);
	}

	Command GPU::gp0VRAMtoVRAMBlitCommands(U32 instruction) const
	{
		return Command{
			.Function = &GPU::gp0VRAMtoVRAMBlitCommand,
			.RemainingParameters = 4
		};
	}

	void GPU::gp0VRAMtoVRAMBlitCommand()
	{
		mMode = GP0Mode::VRAMtoVRAM;
	}

	Command GPU::gp0CPUtoVRAMBlitCommands(U32 instruction) const
	{
		return Command{
			.Function = &GPU::gp0CPUtoVRAMBlitCommand,
			.RemainingParameters = 3
		};
	}

	void GPU::gp0CPUtoVRAMBlitCommand()
	{
		U32 destinationCoords = mCommandBuffer.Data[1];
		U32 size = mCommandBuffer.Data[2];

		mMemoryTransferWidth = (size >> 0) & 0xFFFF;
		mMemoryTransferHeight = (size >> 16) & 0xFFFF;

		U32 numHalfWords = mMemoryTransferWidth * mMemoryTransferHeight;
		numHalfWords = (numHalfWords + 1) & ~1;
		U32 numWords = numHalfWords >> 1;

		mCurrentCommand.RemainingParameters = numWords;
		mMemoryTransferCoordsX = (destinationCoords >> 0) & 0xFFFF;
		mMemoryTransferCoordsY = (destinationCoords >> 16) & 0xFFFF;
		mMemoryTransferX = mMemoryTransferY = 0;

		mMode = GP0Mode::CPUtoVRAM;
	}

	Command GPU::gp0VRAMtoCPUBlitCommands(U32 instruction) const
	{
		return Command{
			.Function = &GPU::gp0VRAMtoCPUBlitCommand,
			.RemainingParameters = 3
		};
	}

	void GPU::gp0VRAMtoCPUBlitCommand()
	{
		mMode = GP0Mode::VRAMtoCPU;
	}

	Command GPU::gp0EnvironmentCommands(U32 instruction) const
	{
		U8 command = (instruction >> 24) & 0xFF;
		switch (command) {
			case 0xE1:
				return Command{
					.Function = &GPU::gp0DrawSettingCommand,
					.RemainingParameters = 1
				};

			case 0xE2:
				return Command{
					.Function = &GPU::gp0TextureWindowSettingCommand,
					.RemainingParameters = 1
				};

			case 0xE3:
				return Command{
					.Function = &GPU::gp0SetDrawingAreaTopLeftCommand,
					.RemainingParameters = 1
				};

			case 0xE4:
				return Command{
					.Function = &GPU::gp0SetDrawingAreaBottomRightCommand,
					.RemainingParameters = 1
				};
				break;

			case 0xE5:
				return Command{
					.Function = &GPU::gp0SetDrawingOffsetCommand,
					.RemainingParameters = 1
				};

			case 0xE6:
				return Command{
					.Function = &GPU::gp0MaskBitSettingCommand,
					.RemainingParameters = 1
				};

			default:
				return Command{
					.Function = &GPU::gp0Nop,
					.RemainingParameters = 1
				};
		}
	}

	void GPU::gp0DrawSettingCommand()
	{
		U32 instruction = mCommandBuffer.pop();
		mGPUStat.TexturePageX = (instruction >> 0) & 0xF;
		mGPUStat.TexturePageYBase1 = (instruction >> 4) & 0x1;
		mGPUStat.SemiTransparency = (SemiTransparency)((instruction >> 5) & 0x3);
		mGPUStat.TexturePageColors = (TexturePageColors)((instruction >> 7) & 0x3);
		mGPUStat.DitherEnabled = (instruction >> 9) & 0x1;
		mGPUStat.DrawToDisplay = (instruction >> 10) & 0x1;
		mGPUStat.TexturePageYBase2 = (instruction >> 11) & 0x1;
		mTexturedRectangleXFlip = (instruction >> 12) & 0x1;
		mTexturedRectangleYFlip = (instruction >> 13) & 0x1;
	}

	void GPU::gp0TextureWindowSettingCommand()
	{
		U32 instruction = mCommandBuffer.pop();
		mTextureWindowMaskX = (instruction >> 0) & 0xF;
		mTextureWindowMaskY = (instruction >> 5) & 0xF;
		mTextureWindowOffsetX = (instruction >> 10) & 0xF;
		mTextureWindowOffsetY = (instruction >> 15) & 0xF;
	}

	void GPU::gp0SetDrawingAreaTopLeftCommand()
	{
		U32 instruction = mCommandBuffer.pop();
		mDrawAreaTopLeftX = (instruction >> 0) & 0x3FF;
		mDrawAreaTopLeftY = (instruction >> 10) & 0x3FF;

		mRenderer->SetDrawTopLeft(mDrawAreaTopLeftX, mDrawAreaTopLeftY);
	}

	void GPU::gp0SetDrawingAreaBottomRightCommand()
	{
		U32 instruction = mCommandBuffer.pop();
		mDrawAreaBottomRightX = (instruction >> 0) & 0x3FF;
		mDrawAreaBottomRightY = (instruction >> 10) & 0x3FF;

		mRenderer->SetDrawBottomRight(mDrawAreaBottomRightX, mDrawAreaBottomRightY);
	}

	void GPU::gp0SetDrawingOffsetCommand()
	{
		mRenderer->Flush();
		mRenderer->Begin();

		U32 instruction = mCommandBuffer.pop();

		U16 drawOffsetX = (instruction >> 0) & 0x7FF;
		U16 drawOffsetY = (instruction >> 11) & 0x7FF;

		mDrawOffsetX = ((I16)(drawOffsetX << 5)) >> 5;
		mDrawOffsetY = ((I16)(drawOffsetY << 5)) >> 5;

		mRenderer->SetDrawOffset(mDrawOffsetX, mDrawOffsetY);
	}

	void GPU::gp0MaskBitSettingCommand()
	{
		U32 instruction = mCommandBuffer.pop();
		mGPUStat.SetMaskWhenDrawingPixels = (instruction >> 0) & 0x1;
		mGPUStat.DrawPixels = (instruction >> 0) & 0x1;
	}

	void GPU::gp1Reset()
	{
		mGPUStat.TexturePageX = 0;
		mGPUStat.TexturePageYBase1 = 0;
		mGPUStat.SemiTransparency = SemiTransparency::B2PlusF2;
		mGPUStat.TexturePageColors = TexturePageColors::T4Bit;
		mTextureWindowMaskX = mTextureWindowMaskY = 0;
		mTextureWindowOffsetX = mTextureWindowOffsetY = 0;
		mGPUStat.DitherEnabled = ESX_FALSE;
		mGPUStat.DrawToDisplay = ESX_FALSE;
		mGPUStat.TexturePageYBase2 = 0;
		mTexturedRectangleXFlip = mTexturedRectangleYFlip = 0;
		mDrawAreaTopLeftX = mDrawAreaTopLeftY = 0;
		mDrawAreaBottomRightX = mDrawAreaBottomRightY = 0;
		mDrawOffsetX = mDrawOffsetY = 0;
		mGPUStat.SetMaskWhenDrawingPixels = ESX_FALSE;
		mGPUStat.DrawPixels = ESX_FALSE;
		mGPUStat.DMADirection = DMADirection::Off;
		mGPUStat.DisplayEnable = ESX_TRUE;
		mVRAMStartX = mVRAMStartY = 0;
		mGPUStat.HorizontalResolution = HorizontalResolution::H256;
		mGPUStat.VerticalResolution = VerticalResolution::V240;
		mGPUStat.VideoMode = VideoMode::NTSC;
		mGPUStat.InterlaceField = ESX_TRUE;
		mHorizontalRangeStart = 0x200;
		mHorizontalRangeEnd = 0xC00;
		mVerticalRangeStart = 0x10;
		mVerticalRangeEnd = 0x100;
		mGPUStat.ColorDepth = ColorDepth::C15Bit;
	}

	void GPU::gp1ResetCommandBuffer()
	{
		mCommandBuffer.clear();
		mCurrentCommand.RemainingParameters = 0;
		mMode = GP0Mode::Command;
	}

	void GPU::gp1AckInterrupt()
	{
		mGPUStat.InterruptRequest = ESX_FALSE;
	}

	void GPU::gp1SetDisplayEnable(U32 instruction)
	{
		mGPUStat.DisplayEnable = (instruction & 0x1) == 0;
	}

	void GPU::gp1SetDMADirection(U32 instruction)
	{
		mGPUStat.DMADirection = (DMADirection)(instruction & 0x3);
	}

	void GPU::gp1SetVRAMStart(U32 instruction)
	{
		mVRAMStartX = instruction & 0x3FF;
		mVRAMStartY = (instruction >> 10) & 0x1FF;
	}

	void GPU::gp1SetHorizontalRange(U32 instruction)
	{
		mHorizontalRangeStart = (instruction >> 0) & 0xFFF;
		mHorizontalRangeEnd = (instruction >> 12) & 0xFFF;
	}

	void GPU::gp1SetVerticalRange(U32 instruction)
	{
		mVerticalRangeStart = (instruction >> 0) & 0x3FF;
		mVerticalRangeEnd = (instruction >> 10) & 0x3FF;
	}

	void GPU::gp1SetDisplayMode(U32 instruction)
	{
		mGPUStat.HorizontalResolution = fromFields(instruction & 0x3, (instruction >> 6) & 0x1);
		mGPUStat.VerticalResolution = (VerticalResolution)((instruction >> 2) & 0x1);
		mGPUStat.VideoMode = (VideoMode)((instruction >> 3) & 0x1);
		mGPUStat.ColorDepth = (ColorDepth)((instruction >> 4) & 0x1);
		mGPUStat.VerticalInterlace = (instruction >> 5) & 0x1;
		mGPUStat.ReverseFlag = (instruction >> 7) & 0x1;
	}

	void GPU::gp1SetTextureDisable(U32 instruction)
	{
		mGPUStat.TexturePageYBase2 = instruction & 0x1;
	}

	void GPU::gp1Unknown(U32 instruction)
	{
	}

	void GPU::gp1GetGPUInfo(U32 instruction)
	{
	}

	void GPU::gp1SetTextureDisableSpecial(U32 instruction){
		U32 x = instruction & 0xFFFFFF;
		if (x == 0x501) {
			mGPUStat.TexturePageYBase2 = 1;
		} else if (x == 0x504) {
			mGPUStat.TexturePageYBase2 = 0;
		}
	}

	U32 GPU::getGPUStat()
	{
		U32 result = 0;

		U8 dataRequest = 0;
		switch (mGPUStat.DMADirection)
		{
			case DMADirection::Off:
				dataRequest = 0;
				break;

			case DMADirection::FIFO:
				//TODO: FIFO State
				dataRequest = 1;
				break;

			case DMADirection::CPUToGP0:
				dataRequest = mGPUStat.ReadyToReceiveDMABlock;
				break;

			case DMADirection::GPUREADtoCPU:
				dataRequest = mGPUStat.ReadySendVRAMToCPU;
				break;
		}

		//TODO: Da togliere
		mGPUStat.ReadyCmdWord = ESX_TRUE;
		mGPUStat.ReadySendVRAMToCPU = ESX_TRUE;
		mGPUStat.ReadyToReceiveDMABlock = ESX_TRUE;

		result |= (mGPUStat.TexturePageX & 0x0F) << 0;
		result |= (mGPUStat.TexturePageYBase1 & 0x1) << 4;
		result |= ((U8)mGPUStat.SemiTransparency & 0x3) << 5;
		result |= ((U8)mGPUStat.TexturePageColors & 0x3) << 7;
		result |= (mGPUStat.DitherEnabled & 0x1) << 9;
		result |= (mGPUStat.DrawToDisplay & 0x1) << 10;
		result |= (mGPUStat.SetMaskWhenDrawingPixels & 0x1) << 11;
		result |= (mGPUStat.DrawPixels & 0x1) << 12;
		result |= (mGPUStat.InterlaceField & 0x1) << 13;
		result |= (mGPUStat.ReverseFlag & 0x1) << 14;
		result |= (mGPUStat.TexturePageYBase2 & 0x1) << 15;
		result |= ((U8)mGPUStat.HorizontalResolution & 0x1) << 16;
		result |= (((U8)mGPUStat.HorizontalResolution >> 1) & 0x3) << 17;
		//result |= ((U8)mGPUStat.VerticalResolution & 0x1) << 19; TODO: Emulate Bit 31 correctly endless loop
		result |= ((U8)mGPUStat.VideoMode & 0x1) << 20;
		result |= ((U8)mGPUStat.ColorDepth & 0x1) << 21;
		result |= (mGPUStat.VerticalInterlace & 0x1) << 22;
		result |= (mGPUStat.DisplayEnable & 0x1) << 23;
		result |= (mGPUStat.InterruptRequest & 0x1) << 24;
		result |= (dataRequest & 0x1) << 25;
		result |= (mGPUStat.ReadyCmdWord & 0x1) << 26;
		result |= (mGPUStat.ReadySendVRAMToCPU & 0x1) << 27;
		result |= (mGPUStat.ReadyToReceiveDMABlock & 0x1) << 28;
		result |= ((U8)mGPUStat.DMADirection & 0x3) << 29;
		result |= (mGPUStat.DrawOddLines & 0x3) << 31;

		return result;
	}

	HorizontalResolution GPU::fromFields(U8 hr1, U8 hr2)
	{
		return (HorizontalResolution)((hr1 << 1) | hr2);
	}

	Vertex GPU::unpackVertex(U32 value) {
		return Vertex((I16)((value >> 0) & 0xFFFF), (I16)((value >> 16) & 0xFFFF));
	}

	UV GPU::unpackUV(U32 value) {
		return UV((U8)((value >> 0) & 0xFF), (U8)((value >> 8) & 0xFF));
	}

	Color GPU::unpackColor(U32 value) {
		return Color((U8)((value >> 0) & 0xFF), (U8)((value >> 8) & 0xFF), (U8)((value >> 16) & 0xFF));
	}

	Color GPU::unpackColor(U16 value)
	{
		U8 r5 = (value >> 0) & 0x1F;
		U8 g5 = (value >> 5) & 0x1F;
		U8 b5 = (value >> 10) & 0x1F;

		return Color(r5 << 3, g5 << 3, b5 << 3);
	}

	U32 GPU::fromTexPageToVRAMAddress(U16 texPage)
	{
		U16 pageX = texPage & 0xF;
		U16 pageY = (texPage >> 11) & 0x1;
		return pageY * 2048 + pageX * 64;
	}

	U32 GPU::fromClutToVRAMAddress(U16 clut)
	{
		U16 clutY = (clut >> 6) & 0x1FF;
		U16 clutX = clut & 0x3F;
		return clutY * 2048 + clutX * 16;
	}

	U32 GPU::fromCoordsToVRAMAddress(U32 coords)
	{
		U16 y = (coords >> 16) & 0xFFFF;
		U16 x = (coords >> 0) & 0xFFFF;
		return y * 2048 + x;
	}

	void GPU::transformUV(UV& uv, U16 tx, U16 ty, U8 bpp)
	{
		U16 r = 16 / bpp;
		uv.u = tx * 64 * r + uv.u;
		uv.v = ty * 256 + uv.v;
	}

	void CommandBuffer::push(U32 word)
	{
		Data[Length++] = word;
	}

	U32 CommandBuffer::pop()
	{
		return Data[--Length];
	}

	void CommandBuffer::clear()
	{
		Length = 0;
	}

	BIT Command::IsComplete(U32 instruction)
	{
		if (RemainingParameters == 0) {
			switch (BaseCase)
			{
				case BaseCase::RemainingParameters:
					return ESX_TRUE;

				case BaseCase::PolyLine:
					return (instruction & 0xF000F000) == 0x50005000;
			}
		} else {
			return ESX_FALSE;
		}
	}

}