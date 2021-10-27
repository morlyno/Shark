#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	enum class FillMode
	{
		Solid, Framed
	};

	enum class CullMode
	{
		None = 0,
		Front, Back
	};

	struct RasterizerSpecification
	{
		FillMode Fill = FillMode::Solid;
		CullMode Cull = CullMode::Back;
		bool Multisample = false;
		bool Antialising = false;

		RasterizerSpecification() = default;
		RasterizerSpecification(FillMode fill, CullMode cull, bool multisample, bool antialising)
			: Fill(fill), Cull(cull), Multisample(multisample), Antialising(antialising) {}
	};

	class Rasterizer : public RefCount
	{
	public:
		virtual ~Rasterizer() = default;

		virtual void Bind(Ref<RenderCommandBuffer> commandBuffer) = 0;
		virtual void UnBind(Ref<RenderCommandBuffer> commandBuffer) = 0;

		virtual void SetFillMode(FillMode fill) = 0;
		virtual void SetCullMode(CullMode cull) = 0;

		virtual void SetMultisample(bool enabled) = 0;
		virtual void SetAntialising(bool enabled) = 0;
		
		virtual FillMode GetFillMode() const = 0;
		virtual CullMode GetCullMode() const = 0;

		virtual bool IsMultisample() const = 0;
		virtual bool IsAntialising() const = 0;

		virtual void SetSpecification(const RasterizerSpecification& specs) = 0;
		virtual const RasterizerSpecification& GetSpecification() const = 0;

		static Ref<Rasterizer> Create(const RasterizerSpecification& specs);
		
	};

}
