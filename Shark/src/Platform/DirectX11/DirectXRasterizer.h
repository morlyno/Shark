#pragma once

#include "Shark/Render/Rasterizer.h"
#include <d3d11.h>

namespace Shark {

	class DirectXRasterizer : public Rasterizer
	{
	public:
		DirectXRasterizer(const RasterizerSpecification& specs);
		virtual ~DirectXRasterizer();

		virtual void Bind(Ref<RenderCommandBuffer> commandBuffer) override;
		virtual void UnBind(Ref<RenderCommandBuffer> commandBuffer) override;

		void Bind(ID3D11DeviceContext* ctx);
		void UnBind(ID3D11DeviceContext* ctx);

		virtual void SetFillMode(FillMode fill) override;
		virtual void SetCullMode(CullMode cull) override;

		virtual void SetMultisample(bool enabled) override;
		virtual void SetAntialising(bool enabled) override;

		virtual FillMode GetFillMode() const override { return m_Specification.Fill; }
		virtual CullMode GetCullMode() const override { return m_Specification.Cull; }

		virtual bool IsMultisample() const override { return m_Specification.Multisample; }
		virtual bool IsAntialising() const override { return m_Specification.Antialising; }

		virtual void SetSpecification(const RasterizerSpecification& specs) override;
		virtual const RasterizerSpecification& GetSpecification() const override { return m_Specification; }

	private:
		ID3D11RasterizerState* m_Rasterizer = nullptr;
		RasterizerSpecification m_Specification;

	};

}