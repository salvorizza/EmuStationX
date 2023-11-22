project "EmuStationX"
	kind "WindowedApp"
	characterset ("MBCS")
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	icon "%{wks.location}/%{prj.name}/commons/icons/Logo.ico"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	defines
	{
		"GLFW_INCLUDE_NONE"
	}
	
	includedirs
	{
		"src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.ImGui}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.ImGuiConsole}"
	}

	links
	{
		"Glad",
		"GLFW",
		"ImGui",
		"ImGuiConsole",
		"STB",
		"opengl32.lib"
	}
	
	filter "files:vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"
		files { 'resources.rc', '**.ico' }
		vpaths { ['Resources/*'] = { '*.rc', '**.ico' } }

	filter "configurations:Debug"
		defines "ESX_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ESX_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ESX_DIST"
		runtime "Release"
		optimize "on"