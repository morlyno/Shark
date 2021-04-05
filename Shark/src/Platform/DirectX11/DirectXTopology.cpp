#include "skpch.h"
#include "DirectXTopology.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

    static D3D11_PRIMITIVE_TOPOLOGY TopologyToD3D11(TopologyMode topology)
    {
        switch (topology)
        {
            case TopologyMode::None: SK_CORE_ASSERT(false, "No Topology Specified"); break;
            case TopologyMode::Triangle:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case TopologyMode::Line:      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            case TopologyMode::Point:     return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        }
        SK_CORE_ASSERT(false, "Unkown Topology");
        return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }

    static TopologyMode D3D11ToTopology(D3D11_PRIMITIVE_TOPOLOGY topo)
    {
        switch (topo)
        {
            case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:  return TopologyMode::Triangle;
            case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:      return TopologyMode::Line;
            case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST:     return TopologyMode::Point;
        }
        SK_CORE_ASSERT(false, "Unkown Topology");
        return TopologyMode::None;
    }

    DirectXTopology::DirectXTopology(TopologyMode topology)
        : m_D3DTopology(TopologyToD3D11(topology))
    {
        m_DXApi = RendererCommand::GetRendererAPI().CastTo<DirectXRendererAPI>().GetWeak();
    }

    DirectXTopology::~DirectXTopology()
    {
    }

    void DirectXTopology::Bind()
    {
        m_DXApi->GetContext()->IASetPrimitiveTopology(m_D3DTopology);
    }

    void DirectXTopology::UnBind()
    {
        m_DXApi->GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED);
    }

    void DirectXTopology::SetTopology(TopologyMode toplogy)
    {
        m_D3DTopology = TopologyToD3D11(toplogy);
    }

    TopologyMode DirectXTopology::GetTopology() const
    {
        return D3D11ToTopology(m_D3DTopology);
    }

}
