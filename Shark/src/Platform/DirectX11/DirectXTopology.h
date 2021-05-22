#pragma once

#include "Shark/Render/Topology.h"

#include <d3d11.h>

namespace Shark {

	class DirectXTopology : public Topology
	{
	public:
		DirectXTopology(TopologyMode topology);
		virtual ~DirectXTopology();

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void SetTopology(TopologyMode toplogy) override;
		virtual TopologyMode GetTopology() const override;

	private:
		D3D11_PRIMITIVE_TOPOLOGY m_D3DTopology;

	};

}
