#pragma once

namespace Shark {

	enum class TopologyMode
	{
		None = 0,
		Triangle, Line, Point
	};

	class Topology : public RefCount
	{
	public:
		virtual ~Topology() = default;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual void SetTopology(TopologyMode topology) = 0;
		virtual TopologyMode GetTopology() const = 0;

		static Ref<Topology> Create(TopologyMode topology);

	};

}
