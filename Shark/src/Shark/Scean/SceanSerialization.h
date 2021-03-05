#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scean/Scean.h"

namespace Shark {

	class SceanSerializer
	{
	public:
		SceanSerializer(const Ref<Scean>& scean);
		~SceanSerializer() = default;

		bool Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);
	private:
		Ref<Scean> m_Scean;
	};

}
