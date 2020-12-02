project "ImGui"
    location "ImGui"
    kind "StaticLib"
	language "c++"
	cppdialect "c++17"
	staticruntime "on"
	
	targetdir ("%{prj.name}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{prj.name}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
		"%{prj.name}/imconfig.h",
		"%{prj.name}/imgui.h",
		"%{prj.name}/imgui.cpp",
		"%{prj.name}/imgui_draw.cpp",
		"%{prj.name}/imgui_internal.h",
		"%{prj.name}/imgui_widgets.cpp",
		"%{prj.name}/imstb_rectpack.h",
		"%{prj.name}/imstb_textedit.h",
		"%{prj.name}/imstb_truetype.h",
		"%{prj.name}/imgui_demo.cpp"
	}
	
	filter "system:windows"
		systemversion "latest"
