workspace "Delegates"
	filename "Delegates"
	basedir "../"
	configurations { "Debug", "Release" }
    platforms {"x86", "x64"}
    warnings "Extra"
    rtti "Off"
	characterset ("MBCS")
	defines { "_CONSOLE" }
	flags {"FatalWarnings"}
	language "C++"

    filter { "platforms:x64" }
		architecture "x64"
		defines {"x64", "PLATFORM_WINDOWS"}

	filter { "platforms:x86" }
		architecture "x32"
		defines {"x86", "PLATFORM_WINDOWS"}	

	filter { "configurations:Debug" }
			runtime "Debug"
		 	defines { "_DEBUG" }
		 	flags {  }
		 	symbols "On"
		 	optimize "Off"

	filter { "configurations:Release" }
		 	runtime "Release"
			defines { "NDEBUG" }
		 	flags {  }
		 	symbols "Off"
		 	optimize "Full"

	project "Delegates"
		filename "Delegates"
		location ".."
		targetdir "../Build/$(ProjectName)_$(Platform)_$(Configuration)"
		objdir "!../Build/Intermediate/$(ProjectName)_$(Platform)_$(Configuration)"

		kind "ConsoleApp"

		files
		{ 
			"../**.h",
			"../**.hpp",
			"../**.cpp",
			"../**.inl",
			"../**.natvis",
		}

newaction {
		trigger     = "clean",
		description = "Remove all binaries and generated files",

		execute = function()
			os.rmdir("../Build")
			os.rmdir("../ipch")
			os.rmdir("../.vs")
			os.remove("../Delegates.sln")
			os.remove("../Delegates.vcxproj")
		end
	}