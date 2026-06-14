#pragma once

#include "NodeGraph/Node.h"

namespace Shark {

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

	}

	REFLECT_NODE(
		Nodes::Add<float>,
		REFLECT_INPUTS(&Nodes::Add<float>::Value1,
					   &Nodes::Add<float>::Value2),
		REFLECT_OUTPUTS(&Nodes::Add<float>::Result)
	);

	REFLECT_NODE(
		Nodes::Add<int>,
		REFLECT_INPUTS(&Nodes::Add<int>::Value1,
					   &Nodes::Add<int>::Value2),
		REFLECT_OUTPUTS(&Nodes::Add<int>::Result)
	);

	REFLECT_NODE(
		Nodes::Multiply<float>,
		REFLECT_INPUTS(&Nodes::Multiply<float>::Value1,
					   &Nodes::Multiply<float>::Value2),
		REFLECT_OUTPUTS(&Nodes::Multiply<float>::Result)
	);

	REFLECT_NODE(
		Nodes::Multiply<int>,
		REFLECT_INPUTS(&Nodes::Multiply<int>::Value1,
					   &Nodes::Multiply<int>::Value2),
		REFLECT_OUTPUTS(&Nodes::Multiply<int>::Result)
	);

}
