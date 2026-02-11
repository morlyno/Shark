#include "skpch.h"
#include "ComputePipeline.h"
#include "Shark/Render/Renderer.h"

namespace Shark {

	ComputePipeline::ComputePipeline(Ref<Shader> computeShader, const std::string& debugName)
		: m_Shader(computeShader), m_DebugName(debugName)
	{
		nvrhi::ComputePipelineDesc desc;
		desc.CS = computeShader->GetHandle(nvrhi::ShaderType::Compute);
		desc.bindingLayouts = computeShader->GetBindingLayouts();

		auto device = Renderer::GetGraphicsDevice();
		m_PipelineHandle = device->createComputePipeline(desc);
	}

	ComputePipeline::~ComputePipeline()
	{

	}

}
