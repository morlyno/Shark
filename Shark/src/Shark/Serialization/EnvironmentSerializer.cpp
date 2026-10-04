#include "skpch.h"
#include "EnvironmentSerializer.h"

#include "Shark/Render/DeviceManager.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Environment.h"

#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool EnvironmentSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool EnvironmentSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		const auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		ImageData imageData;
		imageData.Data = TextureImporter::ToBufferFromFile(filesystemPath, imageData.Format, imageData.Width, imageData.Height);

		auto commandBuffer = RenderCommandBuffer::Create(nvrhi::CommandQueue::Compute, fmt::format("CreateEnvironmentMap '{}'", metadata.FilePath));

		commandBuffer->RT_Begin();
		auto [radianceMap, irradianceMap] = Renderer::RT_CreateEnvironmentMap(commandBuffer, imageData, metadata.FilePath.generic_string());
		auto environment = Ref<Environment>::Create(radianceMap, irradianceMap);
		commandBuffer->RT_End();
		commandBuffer->RT_Execute();

		auto query = EventQuery::Create();
		query->RT_Set(nvrhi::CommandQueue::Compute);

		context->AddTask([query, environment](AssetLoadContext* context)
		{
			if (!query->RT_Poll())
				return false;

			Renderer::GetDeviceManager()->ExecuteCommand(nvrhi::CommandQueue::Graphics, [environment](nvrhi::ICommandList* cmd)
			{
				cmd->setPermanentTextureState(environment->GetRadianceMap()->GetHandle(), nvrhi::ResourceStates::ShaderResource);
				cmd->setPermanentTextureState(environment->GetIrradianceMap()->GetHandle(), nvrhi::ResourceStates::ShaderResource);
			});

			return true;
		});

		asset = environment;
		asset->Handle = metadata.Handle;
		return true;
	}

}
