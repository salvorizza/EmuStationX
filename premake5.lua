include "dependencies.lua"

workspace "EmuStationX"
	architecture "x86_64"
	startproject "EmuStationX"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
group ""

include "EmuStationX"