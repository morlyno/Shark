#include "skpch.h"
#include "DirectXRasterizer.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

    static D3D11_FILL_MODE FillModeToD3D11(FillMode fill)
    {
        switch (fill)
        {
            case Shark::FillMode::Solid:   return D3D11_FILL_SOLID;
            case Shark::FillMode::Framed:  return D3D11_FILL_WIREFRAME;
        }
        SK_CORE_ASSERT(false, "Unkown Fill Mode");
        return (D3D11_FILL_MODE)0;
    }

    static D3D11_CULL_MODE CullModeToD3D11(CullMode cull)
    {
        switch (cull)
        {
            case Shark::CullMode::None:  return D3D11_CULL_NONE;
            case Shark::CullMode::Front: return D3D11_CULL_FRONT;
            case Shark::CullMode::Back:  return D3D11_CULL_BACK;
        }
        SK_CORE_ASSERT(false, "Unkone Cull Mode");
        return (D3D11_CULL_MODE)0;
    }

    DirectXRasterizer::DirectXRasterizer(const RasterizerSpecification& specs)
        : m_Specification(specs)
    {
        m_DXApi = RendererCommand::GetRendererAPI().CastTo<DirectXRendererAPI>().GetWeak();

        SetSpecification(specs);
    }

    DirectXRasterizer::~DirectXRasterizer()
    {
        if (m_Rasterizer)
            m_Rasterizer->Release();
    }

    void DirectXRasterizer::Bind()
    {
        m_DXApi->GetContext()->RSSetState(m_Rasterizer);
    }

    void DirectXRasterizer::UnBind()
    {
        m_DXApi->GetContext()->RSSetState(nullptr);
    }

    void DirectXRasterizer::SetFillMode(FillMode fill)
    {
        m_Specification.Fill = fill;
        SetSpecification(m_Specification);
    }

    void DirectXRasterizer::SetCullMode(CullMode cull)
    {
        m_Specification.Cull = cull;
        SetSpecification(m_Specification);
    }

    void DirectXRasterizer::SetMultisample(bool enabled)
    {
        m_Specification.Multisample = enabled;
        SetSpecification(m_Specification);
    }

    void DirectXRasterizer::SetAntialising(bool enabled)
    {
        m_Specification.Antialising = enabled;
        SetSpecification(m_Specification);
    }

    void DirectXRasterizer::SetSpecification(const RasterizerSpecification& specs)
    {
        if (m_Rasterizer)
            m_Rasterizer->Release();

        D3D11_RASTERIZER_DESC rrd = CD3D11_RASTERIZER_DESC{};
        rrd.FillMode = FillModeToD3D11(specs.Fill);
        rrd.CullMode = CullModeToD3D11(specs.Cull);
        rrd.AntialiasedLineEnable = specs.Antialising;
        rrd.MultisampleEnable = specs.Multisample;

        m_DXApi->GetDevice()->CreateRasterizerState(&rrd, &m_Rasterizer);
    }

}
