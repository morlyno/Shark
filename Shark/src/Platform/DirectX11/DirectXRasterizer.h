#pragma once

#include "Shark/Render/Rasterizer.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	class DirectXRasterizer : public Rasterizer
	{
	public:
		DirectXRasterizer(const RasterizerSpecification& specs);
		virtual ~DirectXRasterizer();

		virtual void Bind() override;
		virtual void UnBind() override;

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
		Weak<DirectXRendererAPI> m_DXApi;

		ID3D11RasterizerState* m_Rasterizer = nullptr;
		RasterizerSpecification m_Specification;
	};

}