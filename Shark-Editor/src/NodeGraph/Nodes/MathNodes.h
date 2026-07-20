#pragma once

#include "NodeGraph/ProcessNode.h"
#include <random>

namespace Shark::NodeGraph {

	namespace Nodes {

		template<typename T>
		struct Add : public Details::TypedNode<Add<T>>
		{
			T* Value1 = nullptr;
			T* Value2 = nullptr;

			T Result;

			using Details::TypedNode<Add<T>>::TypedNode;
			virtual void Process() override
			{
				Result = *Value1 + *Value2;
			}
		};

		template<typename T>
		struct Multiply : public Details::TypedNode<Multiply<T>>
		{
			T* Value1 = nullptr;
			T* Value2 = nullptr;

			T Result;

			using Details::TypedNode<Multiply<T>>::TypedNode;
			virtual void Process() override
			{
				Result = *Value1 * *Value2;
			}
		};

		struct Get : public Details::TypedNode<Get>
		{
			const glm::vec3* Vector = nullptr;

			float X;
			float Y;
			float Z;

			using Details::TypedNode<Get>::TypedNode;
			virtual void Process() override
			{
				X = Vector->x;
				Y = Vector->y;
				Z = Vector->z;
			}
		};

		template<typename T>
		struct Random : public ProcessNode
		{
			T* Minimum = nullptr;
			T* Maximum = nullptr;
			int32_t* Seed = 0;

			T Result;

		public:
			Random(UUID id)
				: ProcessNode(id)
			{
				Details::RegisterVariables(this);
			}

			virtual void Initialize(NodeContext* context) override
			{
				Details::InitializeInputs(this);

				if (*Seed == 0)
					*Seed = context->GetSeed();

				m_Engine = std::mt19937(*Seed);
				m_Distribution = Distribution<T>(*Minimum, *Maximum);
			}

			virtual void Process() override
			{
				Result = m_Distribution(m_Engine);
			}

		private:
			template<typename T>
			struct Distribution;

			template<std::integral T>
			struct Distribution<T> : std::uniform_int_distribution<T> { using std::uniform_int_distribution<T>::uniform_int_distribution; };

			template<std::floating_point T>
			struct Distribution<T> : std::uniform_real_distribution<T> { using std::uniform_real_distribution<T>::uniform_real_distribution; };

		private:
			std::mt19937 m_Engine;
			Distribution<T> m_Distribution;

		};

	}

}

REFLECT_NODE(
	Shark::NodeGraph::Nodes::Add<float>,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::Add<float>::Value1,
				   &Shark::NodeGraph::Nodes::Add<float>::Value2),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::Add<float>::Result)
);

REFLECT_NODE(
	Shark::NodeGraph::Nodes::Add<int>,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::Add<int>::Value1,
				   &Shark::NodeGraph::Nodes::Add<int>::Value2),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::Add<int>::Result)
);

REFLECT_NODE(
	Shark::NodeGraph::Nodes::Multiply<float>,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::Multiply<float>::Value1,
				   &Shark::NodeGraph::Nodes::Multiply<float>::Value2),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::Multiply<float>::Result)
);

REFLECT_NODE(
	Shark::NodeGraph::Nodes::Multiply<int>,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::Multiply<int>::Value1,
				   &Shark::NodeGraph::Nodes::Multiply<int>::Value2),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::Multiply<int>::Result)
);

REFLECT_NODE(
	Shark::NodeGraph::Nodes::Get,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::Get::Vector),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::Get::X,
					&Shark::NodeGraph::Nodes::Get::Y,
					&Shark::NodeGraph::Nodes::Get::Z)
);

REFLECT_NODE(
	Shark::NodeGraph::Nodes::Random<int>,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::Random<int>::Minimum,
				   &Shark::NodeGraph::Nodes::Random<int>::Maximum,
				   &Shark::NodeGraph::Nodes::Random<int>::Seed),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::Random<int>::Result)
);

REFLECT_NODE(
	Shark::NodeGraph::Nodes::Random<float>,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::Random<float>::Minimum,
				   &Shark::NodeGraph::Nodes::Random<float>::Maximum,
				   &Shark::NodeGraph::Nodes::Random<float>::Seed),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::Random<float>::Result)
);
