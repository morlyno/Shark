#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/EditorCamera.h"

namespace Shark {

	class SceneRenderer : public RefCount
	{
	public:
		SceneRenderer(Ref<Scene> scene);
		~SceneRenderer();

		void SetScene(Ref<Scene> scene) { m_Scene = scene; }

		void OnRender(EditorCamera& camera);
		void OnRender();

		void Resize(uint32_t width, uint32_t height);

		Ref<FrameBuffer> GetFrameBuffer() const { return m_FrameBuffer; }

	private:
		Ref<Scene> m_Scene;
		Ref<FrameBuffer> m_FrameBuffer;

	};

}
