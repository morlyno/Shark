#pragma once

#include "Shark/Core/Base.h"

#include <nvrhi/nvrhi.h>

namespace Shark {
	class Shader;
}

namespace Shark {

	class ComputePipeline : public RefCount
	{
	public:
		static Ref<ComputePipeline> Create(Ref<Shader> computeShader, const std::string& debugName = {}) { return Ref<ComputePipeline>::Create(computeShader, debugName); }

	public:
		nvrhi::ComputePipelineHandle GetHandle() const;
		RefArg<Shader> GetShader() const;
		const std::string& GetDebugName() const { return m_DebugName; }

	public:
		ComputePipeline(Ref<Shader> computeShader, const std::string& debugName);
		~ComputePipeline();

	private:
		Ref<Shader> m_Shader;
		nvrhi::ComputePipelineHandle m_PipelineHandle;

		std::string m_DebugName;
	};

}
