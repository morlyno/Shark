#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"

namespace Shark {

	class ComputePipeline : public RefCount
	{
	public:
		static Ref<ComputePipeline> Create(Ref<Shader> computeShader, const std::string& debugName = {}) { return Ref<ComputePipeline>::Create(computeShader, debugName); }

	public:
		nvrhi::ComputePipelineHandle GetHandle() const { return m_PipelineHandle; }
		Ref<Shader> GetShader() const { return m_Shader; }
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
