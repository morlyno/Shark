
IncludeDir = {}
IncludeDir["spdlog"] = "%{wks.location}/Shark/dependencies/spdlog/include"
IncludeDir["ImGui"] = "%{wks.location}/Shark/dependencies/ImGui"
IncludeDir["stb_image"] = "%{wks.location}/Shark/dependencies/stb_image"
IncludeDir["EnTT"] = "%{wks.location}/Shark/dependencies/EnTT/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/Shark/dependencies/yaml-cpp/include"
IncludeDir["box2d"] = "%{wks.location}/Shark/dependencies/box2d/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/Shark/dependencies/ImGuizmo"
IncludeDir["fmt"] = "%{wks.location}/Shark/dependencies/fmt/include"
IncludeDir["Optick"] = "%{wks.location}/Shark/dependencies/Optick/src"
IncludeDir["glm"] = "%{wks.location}/Shark/dependencies/glm"
IncludeDir["Mono"] = "%{wks.location}/Shark/dependencies/mono/include"
IncludeDir["msdfgen"] = "%{wks.location}/Shark/dependencies/msdf-atlas-gen/msdfgen"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/Shark/dependencies/msdf-atlas-gen/msdf-atlas-gen"

LibraryDir = {}
LibraryDir["mono"] = "%{wks.location}/Shark/dependencies/mono/lib/%{cfg.buildcfg}"

Library = {}
Library["mono"] = "%{LibraryDir.mono}/libmono-static-sgen.lib"

Library["D3D11"] = "d3d11.lib"
Library["DXGI"] = "dxgi.lib"

Library["Winmm"] = "Winmm.lib"
Library["Version"] = "Version.lib"
Library["Bcrypt"] = "Bcrypt.lib"
