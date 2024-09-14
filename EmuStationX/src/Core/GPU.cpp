#include "GPU.h"

#include <stdio.h>

#include "Timer.h"
#include "InterruptControl.h"
#include "R3000.h"

#include "optick.h"

#include "Scheduler.h"

namespace esx {

	GPU::GPU(const SharedPtr<IRenderer>& renderer)
		: BusDevice(ESX_TEXT("GPU")),
		mRenderer(renderer)
	{
		addRange(ESX_TEXT("Root"), 0x1F801810, BYTE(0x9), 0xFFFFFFFF);

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUStartHBlank, [&](const SchedulerEvent& ev) { startHBlank(ev.ClockTarget); });
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUEndHBlank, [&](const SchedulerEvent& ev) { endHBlank(ev.ClockTarget); });
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUStartVBlank, [&](const SchedulerEvent& ev) { startVBlank(ev.ClockTarget); });
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUEndVBlank, [&](const SchedulerEvent& ev) { endVBlank(ev.ClockTarget); });
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUScanlineStart, [&](const SchedulerEvent& ev) { newScanline(ev.ClockTarget); });
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUFrameStart, [&](const SchedulerEvent& ev) { newFrame(ev.ClockTarget); });
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
	}

	void GPU::load(const StringView& busName, U32 address, U32& output)
	{
		switch (address) {
			case 0x1F801810: {
				output = gpuRead();
				break;
			}
			case 0x1F801814: {
				output = getGPUStat();
				break;
			}
		}
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

			if (mCurrentCommand.Function == &GPU::gp0DrawLinePrimitiveCommand || mCurrentCommand.Function == &GPU::gp0DrawPolygonPrimitiveCommand) {
				mGPUStat.ReadyToReceiveDMABlock = ESX_FALSE;
			}

			mGPUStat.ReadyCmdWord = ESX_FALSE;
			
			mCommandBuffer.clear();
		}

		if (mCurrentCommand.RemainingParameters > 0) {
			mCurrentCommand.RemainingParameters--;
			if (mCurrentCommand.RemainingParameters == 0) {
				mGPUStat.ReadyToReceiveDMABlock = ESX_FALSE;
			}
		}

		switch (mMode) {
			case GP0Mode::Command: {
				mCommandBuffer.push(instruction);

				if (mCurrentCommand.IsComplete(instruction)) {
					(this->*(mCurrentCommand.Function))();
					if (mCurrentCommand.RemainingParameters == 0) {
						mCurrentCommand.Complete = ESX_TRUE;
					}
					mGPUStat.ReadyToReceiveDMABlock = ESX_TRUE;
				}
				break;
			}

			case GP0Mode::CPUtoVRAM: {
				U16 pixel1 = (instruction >> 16) & 0xFFFF;
				U16 pixel2 = (instruction >> 0) & 0xFFFF;
				mPixelsToTransfer.emplace_back(IRenderer::fromU16(pixel2));
				mPixelsToTransfer.emplace_back(IRenderer::fromU16(pixel1));

				if (mCurrentCommand.IsComplete(instruction)) {
					mRenderer->VRAMWrite(mMemoryTransferDestinationCoordsX, mMemoryTransferDestinationCoordsY, mMemoryTransferWidth, mMemoryTransferHeight, mPixelsToTransfer);

					mMemoryTransferX = mMemoryTransferY = 0;

					mCurrentCommand.Complete = ESX_TRUE;
					mMode = GP0Mode::Command;
				}
				break;
			}
		}
		
		if (mCurrentCommand.Complete) {
			mGPUStat.ReadyCmdWord = ESX_TRUE;
			mGPUStat.ReadyToReceiveDMABlock = ESX_TRUE;
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

	U32 GPU::gpuRead()
	{
		if (mGPUStat.ReadySendVRAMToCPU) {
			U32 numHalfWords = mMemoryTransferWidth * mMemoryTransferHeight;
			BIT isOdd = numHalfWords % 2;

			if (mPixelsToTransfer.size() == 0 || mMemoryTransferVRAMToCPU >= numHalfWords) {
				return 0;
			}

			U16 pixel2 = IRenderer::toU16(mPixelsToTransfer[mMemoryTransferVRAMToCPU]);
			mMemoryTransferVRAMToCPU++;

			U16 pixel1 = (mMemoryTransferVRAMToCPU == (numHalfWords - 1) && isOdd) ? 0 : IRenderer::toU16(mPixelsToTransfer[mMemoryTransferVRAMToCPU]);
			mMemoryTransferVRAMToCPU++;

			U32 packet = (pixel1 << 16) | pixel2;

			if (mMemoryTransferVRAMToCPU == numHalfWords) {
				mGPUStat.ReadySendVRAMToCPU = ESX_FALSE;
			}

			mGPURead = packet;
		}

		return mGPURead;
	}

	void GPU::startHBlank(U64 clocks)
	{
		mTimer->startHblank();
	}

	void GPU::endHBlank(U64 clocks)
	{
		mTimer->endHblank();
	}

	void GPU::startVBlank(U64 clocks)
	{
		mTimer->startVblank();
		mInterruptControl->requestInterrupt(InterruptType::VBlank, ESX_FALSE, ESX_TRUE);
	}

	void GPU::endVBlank(U64 clocks)
	{
		mTimer->endVblank();

		mRenderer->FlushVRAMWrites();
		mRenderer->Flush();
		mRenderer->Begin();

		mFrameAvailable = ESX_TRUE;
	}

	void GPU::newScanline(U64 clocks)
	{
		mCurrentScanLine++;

		if (mGPUStat.VerticalResolution == VerticalResolution::V240) {
			mGPUStat.DrawOddLines = (mCurrentScanLine & 0x1) ? ESX_TRUE : ESX_FALSE;
		}
	}

	void GPU::newFrame(U64 clocks)
	{
		mCurrentScanLine = 0;
		if (mGPUStat.VerticalInterlace == ESX_TRUE && mGPUStat.VerticalResolution == VerticalResolution::V480) {
			mGPUStat.DrawOddLines = !mGPUStat.DrawOddLines;
			mGPUStat.InterlaceField = !mGPUStat.DrawOddLines;
		}

		mFrames++;
	}

	void GPU::clock(U64 clocks)
	{
	}

	void GPU::reset()
	{
		mGPUStat = {};
		mTexturedRectangleXFlip = ESX_FALSE;
		mTexturedRectangleYFlip = ESX_FALSE;
		mTextureWindowMaskX = 0x00;
		mTextureWindowMaskY = 0x00;
		mTextureWindowOffsetX = 0x00;
		mTextureWindowOffsetY = 0x00;
		mDrawAreaTopLeftX = 0x0000;
		mDrawAreaTopLeftY = 0x0000;
		mDrawAreaBottomRightX = 0x0000;
		mDrawAreaBottomRightY = 0x0000;
		mDrawOffsetX = 0x0000;
		mDrawOffsetY = 0x0000;
		mVRAMStartX = 0x0000;
		mVRAMStartY = 0x0000;
		mHorizontalRangeStart = 0x0000;
		mHorizontalRangeEnd = 0x0000;
		mVerticalRangeStart = 0x0000;
		mVerticalRangeEnd = 0x0000;
		mAllow2MBVRAM = ESX_FALSE;
		mGPURead = 0;

		mCommandBuffer = {};
		mCurrentCommand = {};

		mMode = GP0Mode::Command;

		mMemoryTransferX = 0x0000;
		mMemoryTransferY = 0x0000;
		mMemoryTransferVRAMToCPU = 0x0000;
		mMemoryTransferSourceCoordsX = 0x0000;
		mMemoryTransferSourceCoordsY = 0x0000;
		mMemoryTransferDestinationCoordsX = 0x0000;
		mMemoryTransferDestinationCoordsY = 0x0000;
		mMemoryTransferWidth = 0x0000;
		mMemoryTransferHeight = 0x0000;
		mNumWordsToTransfer = 0x00000000;
		mCurrentWordNumber = 0x00000000;
		mPixelsToTransfer = {};

		mFrames = 0;
		mCurrentScanLine = 0;
		mFrameAvailable = ESX_FALSE;

		mScanlinesPerFrame = 0;
		mClocksPerScanline = 0;

		mCurrentCommand.Complete = ESX_TRUE;

		mScanlinesPerFrame = SCANLINES_PER_FRAME;
		mClocksPerScanline = CLOCKS_PER_SCANLINE;

		SchedulerEvent newScanline = {
			.Type = SchedulerEventType::GPUScanlineStart,
			.ClockStart = 0,
			.ClockTarget = FromGPUClock(mClocksPerScanline),
			.Reschedule = ESX_TRUE,
			.RescheduleClocks = FromGPUClock(mClocksPerScanline)
		};

		SchedulerEvent newFrame = {
			.Type = SchedulerEventType::GPUFrameStart,
			.ClockStart = 0,
			.ClockTarget = FromGPUClock(mScanlinesPerFrame * mClocksPerScanline),
			.Reschedule = ESX_TRUE,
			.RescheduleClocks = FromGPUClock(mScanlinesPerFrame * mClocksPerScanline)
		};

		Scheduler::UnScheduleAllEvents(SchedulerEventType::GPUScanlineStart);
		Scheduler::UnScheduleAllEvents(SchedulerEventType::GPUFrameStart);
		Scheduler::ScheduleEvent(newScanline);
		Scheduler::ScheduleEvent(newFrame);

		SchedulerEvent startVBlank = {
			.Type = SchedulerEventType::GPUStartVBlank,
			.ClockStart = mCPU->getClocks(),
			.ClockTarget = mCPU->getClocks() + 0,
			.Reschedule = ESX_TRUE,
			.RescheduleClocks = FromGPUClock(mScanlinesPerFrame * mClocksPerScanline)
		};

		SchedulerEvent endVBlank = {
			.Type = SchedulerEventType::GPUEndVBlank,
			.ClockStart = mCPU->getClocks(),
			.ClockTarget = mCPU->getClocks() + 0,
			.Reschedule = ESX_TRUE,
			.RescheduleClocks = FromGPUClock(mScanlinesPerFrame * mClocksPerScanline)
		};

		Scheduler::UnScheduleAllEvents(SchedulerEventType::GPUStartVBlank);
		Scheduler::UnScheduleAllEvents(SchedulerEventType::GPUEndVBlank);
		Scheduler::ScheduleEvent(startVBlank);
		Scheduler::ScheduleEvent(endVBlank);

		mRenderer->Reset();
	}

	void GPU::init()
	{
		mCPU = getBus("Root")->getDevice<R3000>("R3000");
		mTimer = getBus("Root")->getDevice<Timer>("Timer");
		mInterruptControl = getBus("Root")->getDevice<InterruptControl>("InterruptControl");
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
		ESX_CORE_LOG_ERROR("{} not implemented yet", __FUNCTION__);
	}

	void GPU::gp0Unknown()
	{
		ESX_CORE_LOG_ERROR("{}", __FUNCTION__);
	}

	void GPU::gp0QuickRectangleFillCommand()
	{
		U32 command = mCommandBuffer.Data[0];
		U32 topLeft = mCommandBuffer.Data[1];
		U32 size = mCommandBuffer.Data[2];

		U16 XPos = (topLeft >> 0) & 0xFFFF;
		U16 YPos = (topLeft >> 16) & 0xFFFF;

		U16 XSiz = (size >> 0) & 0xFFFF;
		U16 YSiz = (size >> 16) & 0xFFFF;

		if (XSiz == 0 || YSiz == 0) return;

		XPos &= 0x3F0;
		YPos &= 0x1FF;

		XSiz = ((XSiz & 0x3FF) + 0x0F) & ~0x0F;
		YSiz &= 0x1FF;

		Color color = unpackColor(command & 0xFFFFFF);

		mRenderer->Clear(XPos, YPos, XSiz, YSiz, color);
	}

	void GPU::gp0InterruptRequest()
	{
		mInterruptControl->requestInterrupt(InterruptType::GPU, mGPUStat.InterruptRequest, ESX_TRUE);
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

		I32 numVertices = quad ? 4 : 3;
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
			vertices[i].dither = mGPUStat.DitherEnabled && (gourad || !rawTexture);
			vertices[i].semiTransparency = (semiTransparent && !textured) ? (U8)mGPUStat.SemiTransparency : 255;
			vertices[i].rawTexture = rawTexture;
		}

		if (textured) {
			U16 tx = (page >> 0) & 0xF;
			U16 ty = (page >> 4) & 0x1;
			SemiTransparency semiTransparency = (SemiTransparency)((page >> 5) & 0x3);
			TexturePageColors texturePageColors = (TexturePageColors)((page >> 7) & 0x3);
			U16 ty2 = (page >> 11) & 0x1;

			U16 cy = (clut >> 6) & 0x1FF;
			U16 cx = (clut & 0x3F) * 16;

			mGPUStat.TexturePageX = tx;
			mGPUStat.TexturePageYBase1 = ty;
			mGPUStat.TexturePageYBase2 = mAllow2MBVRAM ? ty2 : 0;
			mGPUStat.SemiTransparency = semiTransparency;
			mGPUStat.TexturePageColors = texturePageColors;


			for (PolygonVertex& vertex : vertices) {
				switch (texturePageColors) {
					case TexturePageColors::T4Bit: vertex.bpp = 4; break;
					case TexturePageColors::T8Bit: vertex.bpp = 8; break;

					case TexturePageColors::Reserved:
					case TexturePageColors::T15Bit: vertex.bpp = 16; break;
				}

				vertex.semiTransparency = semiTransparent ? (U8)semiTransparency : 255;

				transformUV(vertex.uv, tx, ty, vertex.bpp);
				vertex.clutUV = UV(cx, cy);

			}
		}

		//ESX_CORE_LOG_TRACE("GPU::gp0DrawPolygonPrimitiveCommand");
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
		U32 command = mCommandBuffer.Data[0];

		BIT gourad = (command >> 28) & 0x1;
		BIT polyLine = (command >> 27) & 0x1;
		BIT semiTransparent = (command >> 25) & 0x1;
		Color flatColor = unpackColor(command & 0xFFFFFF);

		U32 dataStart = 1;
		U32 dataEnd = polyLine ? (mCommandBuffer.Length - 1) : mCommandBuffer.Length;
		U32 numVertices = 2;
		if (polyLine) {
			numVertices = mCommandBuffer.Length - 2;
			if (gourad) {
				numVertices = (numVertices + 1) / 2;
			}
		}

		if (polyLine) { 
			mCommandBuffer.pop(); //Pop terminator
		} 

		Vector<PolygonVertex> vertices(numVertices);
		for (I32 i = numVertices - 1; i >= 0; i--) {
			vertices[i].vertex = unpackVertex(mCommandBuffer.pop());
			vertices[i].color = (gourad && i > 0) ? unpackColor(mCommandBuffer.pop()) : flatColor;
			vertices[i].dither = mGPUStat.DitherEnabled;
			vertices[i].semiTransparency = semiTransparent ? (U8)mGPUStat.SemiTransparency : 255;
		}

		//ESX_CORE_LOG_TRACE("GPU::gp0DrawLinePrimitiveCommand");
		mRenderer->DrawLineStrip(vertices);
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
		BIT semiTransparent = (command >> 25) & 0x1;
		BIT rawTexture = (command >> 24) & 0x1;

		U32 rectSize = size == 0 ? mCommandBuffer.pop() : 0;
		U32 texture = textured ? mCommandBuffer.pop() : 0;
		U32 position = mCommandBuffer.pop();
		U16 clut = (texture >> 16) & 0xFFFF;
		U32 firstColor = command & 0xFFFFFF;

		Vertex vertex = unpackVertex(position);
		Color color = unpackColor(firstColor);
		UV uv = unpackUV(texture & 0xFFFF);
		UV clutUV = UV((clut & 0x3F) * 16, (clut >> 6) & 0x1FF);
		U16 page = mGPUStat.TexturePageX;
		U16 tx = mGPUStat.TexturePageX;
		U16 ty = mGPUStat.TexturePageYBase1;
		U8 bpp = 4 << (U8)mGPUStat.TexturePageColors;

		U16 width = 0;
		U16 height = 0;

		switch (size) {
			case 0b00:
				width = rectSize & 0xFFFF;
				height = (rectSize >> 16) & 0xFFFF;
			break;

			case 0b01:
				width = 1;
				height = 1;
			break;

			case 0b10:
				width = 8;
				height = 8;
			break;

			case 0b11:
				width = 16;
				height = 16;
			break;
		}

		Vector<PolygonVertex> vertices(4);
		for (PolygonVertex& vertex : vertices) {
			vertex.color = color;
			vertex.textured = textured;
			vertex.clutUV = clutUV;
			vertex.bpp = bpp;
			vertex.semiTransparency = semiTransparent ? (U8)mGPUStat.SemiTransparency : 255;
			vertex.rawTexture = rawTexture;
		}
		
		vertices[0].vertex = Vertex(vertex.x + width, vertex.y + height);
		vertices[1].vertex = Vertex(vertex.x, vertex.y + height);
		vertices[2].vertex = Vertex(vertex.x + width, vertex.y);
		vertices[3].vertex = Vertex(vertex.x, vertex.y);

		if (textured) {
			vertices[0].uv = UV(mTexturedRectangleXFlip ? uv.u : (uv.u + width), mTexturedRectangleYFlip ? uv.v : (uv.v + height));
			vertices[1].uv = UV(mTexturedRectangleXFlip ? (uv.u + width) : uv.u, mTexturedRectangleYFlip ? uv.v : (uv.v + height));
			vertices[2].uv = UV(mTexturedRectangleXFlip ? uv.u : (uv.u + width), mTexturedRectangleYFlip ? (uv.v + height) : uv.v);
			vertices[3].uv = UV(mTexturedRectangleXFlip ? (uv.u + width) : uv.u, mTexturedRectangleYFlip ? (uv.v + height) : uv.v);

			for (PolygonVertex& vertex : vertices) {
				transformUV(vertex.uv, tx, ty, bpp);
			}
		}

		//ESX_CORE_LOG_TRACE("GPU::gp0DrawRectanglePrimitiveCommand");
		mRenderer->DrawPolygon(vertices);
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
		U32 sourceCoords = mCommandBuffer.Data[1];
		U32 destinationCoords = mCommandBuffer.Data[2];
		U32 size = mCommandBuffer.Data[3];

		mMemoryTransferWidth = (size >> 0) & 0xFFFF;
		mMemoryTransferHeight = (size >> 16) & 0xFFFF;
		mMemoryTransferDestinationCoordsX = (destinationCoords >> 0) & 0xFFFF;
		mMemoryTransferDestinationCoordsY = (destinationCoords >> 16) & 0xFFFF;
		mMemoryTransferSourceCoordsX = (sourceCoords >> 0) & 0xFFFF;
		mMemoryTransferSourceCoordsY = (sourceCoords >> 16) & 0xFFFF;

		mMemoryTransferSourceCoordsX &= 0x3FF;
		mMemoryTransferSourceCoordsY &= 0x1FF;
		mMemoryTransferDestinationCoordsX &= 0x3FF;
		mMemoryTransferDestinationCoordsY &= 0x1FF;
		mMemoryTransferWidth = ((mMemoryTransferWidth - 1) & 0x3FF) + 1;
		mMemoryTransferHeight = ((mMemoryTransferHeight - 1) & 0x1FF) + 1;

		mMemoryTransferX = 0;
		mMemoryTransferY = 0;

		mPixelsToTransfer.reserve(mMemoryTransferWidth * mMemoryTransferHeight);
		mPixelsToTransfer.resize(0);
		mPixelsToTransfer.clear();

		mRenderer->VRAMRead(mMemoryTransferSourceCoordsX, mMemoryTransferSourceCoordsY, mMemoryTransferWidth, mMemoryTransferHeight, mPixelsToTransfer);
		mRenderer->VRAMWrite(mMemoryTransferDestinationCoordsX, mMemoryTransferDestinationCoordsY, mMemoryTransferWidth, mMemoryTransferHeight, mPixelsToTransfer);
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
		mMemoryTransferDestinationCoordsX = (destinationCoords >> 0) & 0xFFFF;
		mMemoryTransferDestinationCoordsY = (destinationCoords >> 16) & 0xFFFF;

		mMemoryTransferDestinationCoordsX &= 0x3FF;
		mMemoryTransferDestinationCoordsY &= 0x1FF;
		mMemoryTransferWidth = ((mMemoryTransferWidth - 1) & 0x3FF) + 1;
		mMemoryTransferHeight = ((mMemoryTransferHeight - 1) & 0x1FF) + 1;

		U32 numWords = 0;
		U32 numHalfWords = mMemoryTransferWidth * mMemoryTransferHeight;
		numHalfWords = (numHalfWords + 1) & ~1;
		numWords = numHalfWords >> 1;

		mCurrentCommand.RemainingParameters = numWords;
		mMemoryTransferX = 0;
		mMemoryTransferY = 0;

		mPixelsToTransfer.reserve(mMemoryTransferWidth * mMemoryTransferHeight);
		mPixelsToTransfer.resize(0);
		mPixelsToTransfer.clear();

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
		U32 sourceCoord = mCommandBuffer.Data[1];
		U32 size = mCommandBuffer.Data[2];

		mMemoryTransferWidth = (size >> 0) & 0xFFFF;
		mMemoryTransferHeight = (size >> 16) & 0xFFFF;
		mMemoryTransferSourceCoordsX = (sourceCoord >> 0) & 0xFFFF;
		mMemoryTransferSourceCoordsY = (sourceCoord >> 16) & 0xFFFF;

		mMemoryTransferSourceCoordsX &= 0x3FF;
		mMemoryTransferSourceCoordsY &= 0x1FF;
		mMemoryTransferWidth = ((mMemoryTransferWidth - 1) & 0x3FF) + 1;
		mMemoryTransferHeight = ((mMemoryTransferHeight - 1) & 0x1FF) + 1;

		mMemoryTransferX = 0;
		mMemoryTransferY = 0;
		mMemoryTransferVRAMToCPU = 0;

		mPixelsToTransfer.reserve(mMemoryTransferWidth * mMemoryTransferHeight);
		mPixelsToTransfer.resize(0);
		mPixelsToTransfer.clear();

		mRenderer->VRAMRead(mMemoryTransferSourceCoordsX, mMemoryTransferSourceCoordsY, mMemoryTransferWidth, mMemoryTransferHeight, mPixelsToTransfer);

		mGPUStat.ReadySendVRAMToCPU = ESX_TRUE;
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
		mGPUStat.TexturePageYBase2 = mAllow2MBVRAM ? (instruction >> 11) & 0x1 : 0;
		mTexturedRectangleXFlip = (instruction >> 12) & 0x1;
		mTexturedRectangleYFlip = (instruction >> 13) & 0x1;
		//ESX_CORE_LOG_TRACE("{} {}", mTexturedRectangleXFlip, mTexturedRectangleYFlip);
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
		mGPUStat.DrawPixels = (instruction >> 1) & 0x1;

		mRenderer->SetForceAlpha(mGPUStat.SetMaskWhenDrawingPixels);
		mRenderer->SetCheckMask(mGPUStat.DrawPixels);
	}

	void GPU::gp1Reset()
	{
		gp1ResetCommandBuffer();
		gp1AckInterrupt();
		gp1SetDisplayEnable(1);
		gp1SetDMADirection(0);
		gp1SetVRAMStart(0);
		gp1SetHorizontalRange(0x200 | ((0x200 + 256 * 10) << 12));
		gp1SetVerticalRange(0x010 | ((0x010 + 240) << 10));
		gp1SetDisplayMode(0);
		mCommandBuffer.push(0); gp0DrawSettingCommand();
		mCommandBuffer.push(0); gp0TextureWindowSettingCommand();
		mCommandBuffer.push(0); gp0SetDrawingAreaTopLeftCommand();
		mCommandBuffer.push(0); gp0SetDrawingAreaBottomRightCommand();
		mCommandBuffer.push(0); gp0SetDrawingOffsetCommand();
		mCommandBuffer.push(0); gp0MaskBitSettingCommand();
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

		U64 HorizontalRangeStartClock = FromGPUClock(mHorizontalRangeStart);
		U64 HorizontalRangeEndClock = FromGPUClock(mHorizontalRangeEnd);

		U64 ScheduledEndHBlankClock = 0, ScheduledStartHBlankClock = 0;
		if (GetCurrentScanlineClock() < mHorizontalRangeStart) {
			ScheduledEndHBlankClock = mCPU->getClocks() + FromGPUClock(mHorizontalRangeStart - GetCurrentScanlineClock());
			ScheduledStartHBlankClock = ScheduledEndHBlankClock + FromGPUClock(mHorizontalRangeEnd - mHorizontalRangeStart);
		} else {
			if (GetCurrentScanlineClock() < mHorizontalRangeEnd) {
				ScheduledStartHBlankClock = mCPU->getClocks() + FromGPUClock(mHorizontalRangeEnd - GetCurrentScanlineClock());
				ScheduledEndHBlankClock = ScheduledStartHBlankClock + FromGPUClock((mClocksPerScanline - mHorizontalRangeEnd) + mHorizontalRangeStart);
			} else {
				ScheduledEndHBlankClock = mCPU->getClocks() + FromGPUClock((mClocksPerScanline - GetCurrentScanlineClock()) + mHorizontalRangeStart);
				ScheduledStartHBlankClock = ScheduledEndHBlankClock + FromGPUClock(mHorizontalRangeEnd - mHorizontalRangeStart);
			}
		}

		SchedulerEvent startHBlank = {
			.Type = SchedulerEventType::GPUStartHBlank,
			.ClockStart = mCPU->getClocks(),
			.ClockTarget = ScheduledStartHBlankClock,
			.Reschedule = ESX_TRUE,
			.RescheduleClocks = FromGPUClock(mClocksPerScanline)
		};

		SchedulerEvent endHBlank = {
			.Type = SchedulerEventType::GPUEndHBlank,
			.ClockStart = mCPU->getClocks(),
			.ClockTarget = ScheduledEndHBlankClock,
			.Reschedule = ESX_TRUE,
			.RescheduleClocks = FromGPUClock(mClocksPerScanline)
		};

		Scheduler::UnScheduleAllEvents(SchedulerEventType::GPUStartHBlank);
		Scheduler::UnScheduleAllEvents(SchedulerEventType::GPUEndHBlank);
		Scheduler::ScheduleEvent(startHBlank);
		Scheduler::ScheduleEvent(endHBlank);
	}

	void GPU::gp1SetVerticalRange(U32 instruction)
	{
		mVerticalRangeStart = (instruction >> 0) & 0x3FF;
		mVerticalRangeEnd = (instruction >> 10) & 0x3FF;

		U64 ScheduledEndVBlankClock = 0, ScheduledStartVBlankClock = 0;
		if (mCurrentScanLine < mVerticalRangeStart) {
			ScheduledEndVBlankClock = mCPU->getClocks() + FromGPUClock((mVerticalRangeStart - mCurrentScanLine) * mClocksPerScanline - GetCurrentScanlineClock());
			ScheduledStartVBlankClock = ScheduledEndVBlankClock + FromGPUClock((mVerticalRangeEnd - mVerticalRangeStart) * mClocksPerScanline);
		}
		else {
			if (mCurrentScanLine < mVerticalRangeEnd) {
				ScheduledStartVBlankClock = mCPU->getClocks() + FromGPUClock((mVerticalRangeEnd - mCurrentScanLine) * mClocksPerScanline - GetCurrentScanlineClock());
				ScheduledEndVBlankClock = ScheduledStartVBlankClock + FromGPUClock(((mScanlinesPerFrame - mVerticalRangeEnd) + mVerticalRangeStart) * mClocksPerScanline);
			}
			else {
				ScheduledEndVBlankClock = mCPU->getClocks() + FromGPUClock(((mScanlinesPerFrame - mCurrentScanLine) + mVerticalRangeStart) * mClocksPerScanline - GetCurrentScanlineClock());
				ScheduledStartVBlankClock = ScheduledEndVBlankClock + FromGPUClock((mVerticalRangeEnd - mVerticalRangeStart) * mClocksPerScanline);
			}
		}

		SchedulerEvent startVBlank = {
			.Type = SchedulerEventType::GPUStartVBlank,
			.ClockStart = mCPU->getClocks(),
			.ClockTarget = ScheduledStartVBlankClock,
			.Reschedule = ESX_TRUE,
			.RescheduleClocks = FromGPUClock(mScanlinesPerFrame * mClocksPerScanline)
		};

		SchedulerEvent endVBlank = {
			.Type = SchedulerEventType::GPUEndVBlank,
			.ClockStart = mCPU->getClocks(),
			.ClockTarget = ScheduledEndVBlankClock,
			.Reschedule = ESX_TRUE,
			.RescheduleClocks = FromGPUClock(mScanlinesPerFrame * mClocksPerScanline)
		};

		Scheduler::UnScheduleAllEvents(SchedulerEventType::GPUStartVBlank);
		Scheduler::UnScheduleAllEvents(SchedulerEventType::GPUEndVBlank);
		Scheduler::ScheduleEvent(startVBlank);
		Scheduler::ScheduleEvent(endVBlank);
	}

	void GPU::gp1SetDisplayMode(U32 instruction)
	{
		mGPUStat.HorizontalResolution = fromFields(instruction & 0x3, (instruction >> 6) & 0x1);
		mGPUStat.VerticalResolution = (VerticalResolution)((instruction >> 2) & 0x1);
		mGPUStat.VideoMode = (VideoMode)((instruction >> 3) & 0x1);
		mGPUStat.ColorDepth = (ColorDepth)((instruction >> 4) & 0x1);
		mGPUStat.VerticalInterlace = (instruction >> 5) & 0x1;
		mGPUStat.ReverseFlag = (instruction >> 7) & 0x1;

		mRenderer->SetDisplayMode24(mGPUStat.ColorDepth == ColorDepth::C24Bit);
	}

	void GPU::gp1SetTextureDisable(U32 instruction)
	{
		mAllow2MBVRAM = instruction & 0x1;
	}

	void GPU::gp1Unknown(U32 instruction)
	{
	}

	void GPU::gp1GetGPUInfo(U32 instruction)
	{
		U32 registerIndex = instruction & 0xFFFFFF;
		switch (registerIndex & 0xF) {
			case 0x02:
				mGPURead = (mTextureWindowMaskX) | (mTextureWindowMaskY << 5) | (mTextureWindowOffsetX << 10) | (mTextureWindowOffsetY << 15);
				break;

			case 0x03:
				mGPURead = (mDrawAreaTopLeftX << 0) | (mDrawAreaTopLeftY << 10);
				break;

			case 0x04:
				mGPURead = (mDrawAreaBottomRightX << 0) | (mDrawAreaBottomRightY << 10);
				break;

			case 0x05:
				mGPURead = (mDrawOffsetX << 0) | (mDrawOffsetY << 11);
				break;

			case 0x07:
				mGPURead = 2;
				break;

			case 0x08:
				mGPURead = 0;
				break;

			default:
				break;
		}
	}

	void GPU::gp1SetTextureDisableSpecial(U32 instruction){
		U32 x = instruction & 0xFFFFFF;
		if (x == 0x501) {
			mAllow2MBVRAM = ESX_FALSE;
		} else if (x == 0x504) {
			mAllow2MBVRAM = ESX_TRUE;
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
		result |= ((U8)mGPUStat.VerticalResolution & 0x1) << 19;
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
		result |= (mGPUStat.DrawOddLines & 0x1) << 31;

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

	U64 GPU::GetCurrentFrameClock()
	{
		return ToGPUClock(mCPU->getClocks()) - (mFrames * mScanlinesPerFrame * mClocksPerScanline);
	}

	U64 GPU::GetCurrentScanlineClock()
	{
		return GetCurrentFrameClock() - (mCurrentScanLine * mClocksPerScanline);
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
		}

		return ESX_FALSE;
	}

}