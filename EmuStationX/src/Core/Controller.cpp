#include "Controller.h"

#include "SIO.h"

namespace esx {



	Controller::Controller(ControllerType type)
		:	mType(type),
			mState(0xFFFF)
	{
	}

	void Controller::mosi(U8 value)
	{

	}

	U8 Controller::miso()
	{
		
	}

}