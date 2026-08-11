#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"

#include <span>

namespace Shark {

	using AssetHandle = UUID;

	enum class AssetType
	{
		None = 0,
		Scene,
		Texture,
		ScriptFile,
		Font,
		MeshSource,
		Mesh,
		Material,
		Environment,
		Prefab,
		AudioFile,
		SoundConfig,
		Animation,
		AnimationGraph
	};

	inline const std::unordered_map<std::string, AssetType> AssetExtensionMap = {
		{ ".skscene", AssetType::Scene },
		{ ".sktex", AssetType::Texture },
		{ ".png", AssetType::Texture },
		{ ".jpg", AssetType::Texture },
		{ ".jpeg", AssetType::Texture },
		{ ".cs", AssetType::ScriptFile },
		{ ".ttf", AssetType::Font },
		{ ".obj", AssetType::MeshSource },
		{ ".fbx", AssetType::MeshSource },
		{ ".gltf", AssetType::MeshSource },
		{ ".glb", AssetType::MeshSource },
		{ ".skmesh", AssetType::Mesh },
		{ ".skmat", AssetType::Material },
		{ ".hdr", AssetType::Environment },
		{ ".sfab", AssetType::Prefab },
		{ ".wav", AssetType::AudioFile },
		{ ".sksc", AssetType::SoundConfig },
		{ ".sanim", AssetType::Animation },
		{ ".sagraph", AssetType::AnimationGraph }
	};

	namespace AssetExtensions {
		static constexpr std::array Scene          = { ".skscene"sv };
		static constexpr std::array Texture        = { ".sktex"sv, ".png"sv, ".jpg"sv, ".jpeg"sv };
		static constexpr std::array ScriptFile     = { ".cs"sv };
		static constexpr std::array Font           = { ".ttf"sv };
		static constexpr std::array MeshSource     = { ".obj"sv, ".fbx"sv, ".gltf"sv, ".glb"sv };
		static constexpr std::array Mesh           = { ".skmesh"sv };
		static constexpr std::array Material       = { ".skmat"sv };
		static constexpr std::array Environment    = { ".hdr"sv };
		static constexpr std::array Prefab         = { ".sfab"sv };
		static constexpr std::array AudioFile      = { ".wav"sv };
		static constexpr std::array SoundConfig    = { ".sksc"sv };
		static constexpr std::array Animation      = { ".sanim"sv };
		static constexpr std::array AnimationGraph = { ".sagraph"sv };
	}

	inline static const std::map<AssetType, std::span<const std::string_view>> AssetTypeExtensions =
	{
		{ AssetType::Scene,          AssetExtensions::Scene       },
		{ AssetType::Texture,        AssetExtensions::Texture     },
		{ AssetType::ScriptFile,     AssetExtensions::ScriptFile  },
		{ AssetType::Font,           AssetExtensions::Font        },
		{ AssetType::MeshSource,     AssetExtensions::MeshSource  },
		{ AssetType::Mesh,           AssetExtensions::Mesh        },
		{ AssetType::Material,       AssetExtensions::Material    },
		{ AssetType::Environment,    AssetExtensions::Environment },
		{ AssetType::Prefab,         AssetExtensions::Prefab      },
		{ AssetType::AudioFile,      AssetExtensions::AudioFile   },
		{ AssetType::SoundConfig,    AssetExtensions::SoundConfig },
		{ AssetType::Animation,      AssetExtensions::Animation   },
		{ AssetType::AnimationGraph, AssetExtensions::AnimationGraph },
	};

	static AssetType AssetTypeFromExtension(std::string_view extension)
	{
		for (const auto& [type, extensions] : AssetTypeExtensions)
			for (const auto& ext : extensions)
				if (ext == extension)
					return type;

		return AssetType::None;
	}

	static AssetType AssetTypeFromPath(const std::filesystem::path& assetPath)
	{
		std::string extension = assetPath.extension().string();
		return AssetTypeFromExtension(extension);
	}

}
