#pragma once

class DXDevice;
class DXCommands;

namespace DXAccess
{
	DXDevice* GetDevice();
	DXCommands* GetCommands();
}