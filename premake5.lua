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
	include "EmuStationX/vendor/Glad"
	include "EmuStationX/vendor/glfw"
	include "EmuStationX/vendor/imgui"
	include "EmuStationX/vendor/stb"
	include "EmuStationX/vendor/imgui-console"
		include "EmuStationX/vendor/optick"
group ""

include "EmuStationX"