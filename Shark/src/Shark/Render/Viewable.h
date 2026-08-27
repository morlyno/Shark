#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Hash.h"
#include "Shark/Render/RendererResource.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	struct ViewInfo
	{
		nvrhi::TextureHandle Handle;
		nvrhi::Format Format = nvrhi::Format::UNKNOWN;
		nvrhi::TextureDimension Dimension = nvrhi::TextureDimension::Unknown;
		nvrhi::TextureSubresourceSet SubresourceSet = nvrhi::AllSubresources;

		// Optional, only used by Texture and ImGui renderer
		nvrhi::SamplerHandle TextureSampler;

		bool operator==(const ViewInfo&) const = default;
	};

	// #TODO rename to IViewable
	class ViewableResource : public RendererResource
	{
	public:
		virtual const ViewInfo& GetViewInfo() const = 0;
		virtual bool HasSampler() const = 0;

	};

}

namespace std {

	template<>
	struct hash<Shark::ViewInfo>
	{
		static_assert(sizeof(Shark::ViewInfo) == 40);
		size_t operator()(const Shark::ViewInfo& viewInfo) const
		{
			uint64_t hash = Shark::Hash::FNVBase;
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.Handle));
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.Format));
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.Dimension));
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.SubresourceSet));
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.TextureSampler));
			return hash;
		}
	};

}
