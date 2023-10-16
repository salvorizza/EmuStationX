project "EmuStationX"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	defines
	{
	}
	
	includedirs
	{
		"src"
	}

	links
	{
	}
	
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "REVIVENES_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "REVIVENES_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "REVIVENES_DIST"
		runtime "Release"
		optimize "on"