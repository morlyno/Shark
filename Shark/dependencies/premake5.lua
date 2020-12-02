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
		--"%{prj.name}/backends/imgui_impl_win32.h",
		--"%{prj.name}/backends/imgui_impl_win32.cpp",
		--"%{prj.name}/backends/imgui_impl_dx11.h",
		--"%{prj.name}/backends/imgui_impl_dx11.cpp"
	}
	
	filter "system:windows"
		systemversion "latest"
