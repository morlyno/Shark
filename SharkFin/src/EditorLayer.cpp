#include "skfpch.h"
#include "EditorLayer.h"

#include <Shark/Scene/Components/Components.h>

#include <Shark/Scene/SceneSerialization.h>
#include <Shark/Utility/PlatformUtils.h>
#include <Shark/Utility/ImGuiUtils.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <Shark/Render/TestRenderer.h>

#include <Shark/Debug/Instrumentor.h>

namespace Shark {

	static bool showDemoWindow = false;


	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
		SK_PROFILE_FUNCTION();
	}

	EditorLayer::~EditorLayer()
	{
		SK_PROFILE_FUNCTION();
	}

	void EditorLayer::OnAttach()
	{
		SK_PROFILE_FUNCTION();

		m_EditorCamera.SetProjection(1.0f, 45, 0.01f, 1000.0f);

		auto& app = Application::Get();
		auto& window = app.GetWindow();
		auto& proj = app.GetProject();

		m_WorkScene = Ref<Scene>::Create();
		m_SceneHirachyPanel.SetContext(m_WorkScene);
		if (proj.HasStartupScene())
		{
			m_WorkScene->SetFilePath(proj.GetStartupScene());
			LoadScene();
		}

		FrameBufferSpecification geofbspecs;
		geofbspecs.Width = window.GetWidth();
		geofbspecs.Height = window.GetHeight();
		geofbspecs.Atachments = { ImageFormat::RGBA8, ImageFormat::R32_SINT, ImageFormat::Depth };
		geofbspecs.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		geofbspecs.Atachments[0].Blend = true;
		m_GemometryFrameBuffer = FrameBuffer::Create(geofbspecs);

		FrameBufferSpecification effectfbspecs;
		effectfbspecs.Width = window.GetWidth();
		effectfbspecs.Height = window.GetHeight();
		effectfbspecs.Atachments = { ImageFormat::RGBA8 };


		effectfbspecs.ClearColor = { 0.4f, 0.8f, 0.4f, 1.0f };
		m_NegativeFrameBuffer = FrameBuffer::Create(effectfbspecs);

		effectfbspecs.ClearColor = { 0.8f, 0.4f, 0.4f, 1.0f };
		m_BlurFrameBuffer = FrameBuffer::Create(effectfbspecs);

		FrameBufferSpecification compositfbspecs;
		compositfbspecs.Width = window.GetWidth();
		compositfbspecs.Height = window.GetHeight();
		compositfbspecs.Atachments = { ImageFormat::RGBA8 };
		compositfbspecs.ClearColor = { 0.4f, 0.4f, 0.8f, 1.0f };
		m_CompositFrameBuffer = FrameBuffer::Create(compositfbspecs);

		RasterizerSpecification rrspecs;
		rrspecs.Fill = FillMode::Solid;
		rrspecs.Cull = CullMode::None;
		rrspecs.Multisample = false;
		rrspecs.Antialising = false;
		m_Rasterizer = Rasterizer::Create(rrspecs);

		rrspecs.Fill = FillMode::Framed;
		rrspecs.Cull = CullMode::None;
		m_HilightRasterizer = Rasterizer::Create(rrspecs);

	}

	void EditorLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		m_TimeStep = ts;

		m_Rasterizer->Bind();
		m_GemometryFrameBuffer->Bind();
		m_GemometryFrameBuffer->ClearAtachment(0);
		m_GemometryFrameBuffer->ClearAtachment(1, { -1.0f, -1.0f, -1.0f, -1.0f });
		m_GemometryFrameBuffer->ClearDepth();
		m_CompositFrameBuffer->Clear();
		m_NegativeFrameBuffer->Clear();

		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered && !m_ViewportFocused);

		if (m_ViewportSizeChanged)
		{
			SK_PROFILE_SCOPE("Viewport Size Changed");

			m_GemometryFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_NegativeFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_BlurFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_CompositFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);

			m_EditorCamera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
			m_WorkScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
			m_GemometryFrameBuffer->Bind();
		}

		if (m_WorkScene)
		{
			if (m_PlayScene)
			{
				SK_PROFILE_SCOPE("Update Scene Runtime");

				SceneManager::GetActiveScene()->OnUpdateRuntime(ts);
			}
			else
			{
				SK_PROFILE_SCOPE("Update Scene Editor");

				if (m_ViewportHovered)
					m_EditorCamera.OnUpdate(ts);
				m_WorkScene->OnUpdateEditor(ts, m_EditorCamera);
			}

			{
				SK_PROFILE_SCOPE("Composite Geometry FrameBuffer");

				m_CompositFrameBuffer->Bind();
				m_GemometryFrameBuffer->BindAsTexture(0, 0);
				Renderer::GetShaderLib().Get("FullScreen")->Bind();
				Renderer::SubmitFullScreenQuad();
				m_GemometryFrameBuffer->UnBindAsTexture(0, 0);
			}

			if (m_BlurEffect)
			{
				SK_PROFILE_SCOPE("Blur Effect");

				m_BlurFrameBuffer->Bind();
				m_CompositFrameBuffer->BindAsTexture(0, 0);
				Renderer::GetShaderLib().Get("BlurEffect")->Bind();
				Renderer::SubmitFullScreenQuad();
				m_CompositFrameBuffer->UnBindAsTexture(0, 0);

				m_CompositFrameBuffer->Bind();
				m_BlurFrameBuffer->BindAsTexture(0, 0);
				Renderer::GetShaderLib().Get("FullScreen")->Bind();
				Renderer::SubmitFullScreenQuad();
				m_BlurFrameBuffer->UnBindAsTexture(0, 0);
			}

			if (m_NegativeEffect)
			{
				SK_PROFILE_SCOPE("Negative Effect");

				m_NegativeFrameBuffer->Bind();
				m_CompositFrameBuffer->BindAsTexture(0, 0);
				Renderer::GetShaderLib().Get("NegativeEffect")->Bind();
				Renderer::SubmitFullScreenQuad();
				m_CompositFrameBuffer->UnBindAsTexture(0, 0);

				m_CompositFrameBuffer->Bind();
				m_NegativeFrameBuffer->BindAsTexture(0, 0);
				Renderer::GetShaderLib().Get("FullScreen")->Bind();
				Renderer::SubmitFullScreenQuad();
				m_NegativeFrameBuffer->UnBindAsTexture(0, 0);
			}


		}

		RendererCommand::BindMainFrameBuffer();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowResize));
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispacher.DispachEvent<SelectionChangedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnSelectionChanged));


		m_SceneHirachyPanel.OnEvent(event);

		if (m_PlayScene)
		{
			SceneManager::GetActiveScene()->OnEventRuntime(event);
		}
		else
		{
			m_EditorCamera.OnEvent(event);
			m_WorkScene->OnEventEditor(event);
		}
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
			return false;

		RendererCommand::ResizeSwapChain(event.GetWidth(), event.GetHeight());

		return false;
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		const bool control = Input::KeyPressed(Key::Control);
		const bool shift = Input::KeyPressed(Key::LeftShift);

		switch (event.GetKeyCode())
		{
			case Key::N: // New Scene
			{
				if (control)
				{
					NewScene();
					return true;
				}
				break;
			}
			/*
			case Key::O: // Open Scene
			{
				if (control)
				{
					OpenScene();
					return true;
				}
				break;
			}
			*/
			case Key::S: // Save Scene
			{
				if (control)
				{
					/*if (shift)
					{
						SaveScene();
						return true;
					}*/
					SaveScene();
					return true;
				}
				break;
			}
			case Key::P: // Play Scene
			{
				if (control)
				{
					if (m_PlayScene)
						OnStopScene();
					else
						OnPlayScene();
					return true;
				}
				break;
			}

			case Key::D: // Copy Entity
			{
				if (control)
				{
					Entity e = m_WorkScene->CopyEntity(m_SceneHirachyPanel.GetSelectedEntity());
					e.GetComponent<TagComponent>().Tag += " (Copy)";
					Event::Distribute(SelectionChangedEvent(e));
					return true;
				}
				break;
			}

			case Key::V: // VSync
			{
				auto& window = Application::Get().GetWindow();
				window.SetVSync(!window.IsVSync());
				return true;
			}

			case Key::Q:
			{
				m_CurrentOperation = 0;
				return true;
			}
			case Key::W:
			{
				m_CurrentOperation = ImGuizmo::TRANSLATE;
				return true;
			}
			case Key::E:
			{
				m_CurrentOperation = ImGuizmo::ROTATE;
				return true;
			}
			case Key::R:
			{
				m_CurrentOperation = ImGuizmo::SCALE;
				return true;
			}

			case Key::F:
			{
				if (m_SelectetEntity)
				{
					const auto& tf = m_SelectetEntity.GetComponent<TransformComponent>();
					m_EditorCamera.SetFocusPoint(tf.Position);
					return true;
				}
				break;
			}
		}

		return false;
	}

	bool EditorLayer::OnSelectionChanged(SelectionChangedEvent& event)
	{
		m_SelectetEntity = event.GetSelectedEntity();
		return false;
	}

	void EditorLayer::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
										          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
										          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Shark Editor DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

		ImGuiID dockspace_id = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (m_PlayScene)
				{
					if (ImGui::MenuItem("Stop Scene", "ctrl+P"))
						OnStopScene();
				}
				else
				{
					if (ImGui::MenuItem("Play Scene", "ctrl+P"))
						OnPlayScene();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("New", "ctrl+N"))
					NewScene();
				if (ImGui::MenuItem("Save", "ctrl+S"))
					SaveScene();
				/*
				if (ImGui::MenuItem("Save As..", "ctrl+shift+S"))
					SaveScene();
				if (ImGui::MenuItem("Open..", "ctrl+O"))
					OpenScene();
				*/

				ImGui::Separator();
				bool show = m_SceneHirachyPanel.PropertiesShown();
				if (ImGui::MenuItem("Properties", nullptr, &show))
					m_SceneHirachyPanel.ShwoProerties(show);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Entity"))
			{
				Entity se = m_SceneHirachyPanel.GetSelectedEntity();
				if (ImGui::MenuItem("Add"))
				{
					auto e = m_WorkScene->CreateEntity("New Entity");
					Event::Distribute(SelectionChangedEvent(e));
				}
				if (ImGui::MenuItem("Destroy", "delete", nullptr, se))
				{
					m_WorkScene->DestroyEntity(se);
					Event::Distribute(SelectionChangedEvent({}));
				}
				if (ImGui::MenuItem("Copy", "ctrl+D", nullptr, se))
				{
					se = m_WorkScene->CopyEntity(se);
					se.GetComponent<TagComponent>().Tag += " (Copy)";
					Event::Distribute(SelectionChangedEvent(se));
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Add Component", se))
				{
					if (ImGui::MenuItem("Transform", nullptr, nullptr, !se.HasComponent<TransformComponent>()))
						se.AddComponent<TransformComponent>();
					if (ImGui::MenuItem("Sprite Renderer", nullptr, nullptr, !se.HasComponent<SpriteRendererComponent>()))
						se.AddComponent<SpriteRendererComponent>();
					if (ImGui::MenuItem("Scene Camera", nullptr, nullptr, !se.HasComponent<CameraComponent>()))
						se.AddComponent<CameraComponent>();
					if (ImGui::MenuItem("Rigid Body", nullptr, nullptr, !se.HasComponent<RigidBodyComponent>()))
						se.AddComponent<RigidBodyComponent>();
					if (ImGui::MenuItem("Box Collider", nullptr, nullptr, !se.HasComponent<BoxColliderComponent>()))
						se.AddComponent<BoxColliderComponent>();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Remove Component", se))
				{
					if (ImGui::MenuItem("Transform", nullptr, nullptr, se.HasComponent<TransformComponent>()))
						se.RemoveComponent<TransformComponent>();
					if (ImGui::MenuItem("Sprite Renderer", nullptr, nullptr, se.HasComponent<SpriteRendererComponent>()))
						se.RemoveComponent<SpriteRendererComponent>();
					if (ImGui::MenuItem("Scene Camera", nullptr, nullptr, se.HasComponent<CameraComponent>()))
						se.RemoveComponent<CameraComponent>();
					if (ImGui::MenuItem("Rigid Body", nullptr, nullptr, se.HasComponent<RigidBodyComponent>()))
						se.RemoveComponent<RigidBodyComponent>();
					if (ImGui::MenuItem("Box Collider", nullptr, nullptr, se.HasComponent<BoxColliderComponent>()))
						se.RemoveComponent<BoxColliderComponent>();

					ImGui::EndMenu();
				}


				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Panels"))
			{
				bool show = m_SceneHirachyPanel.IsShowen();
				if (ImGui::MenuItem("Scene Hirachy", nullptr, &show))
					m_SceneHirachyPanel.ShowPanel(show);

				show = m_AssetsPanel.IsShowen();
				if (ImGui::MenuItem("Assets", nullptr, &show))
					m_AssetsPanel.ShowPanel(show);

				ImGui::MenuItem("Project", nullptr, &m_ShowProject);

				ImGui::MenuItem("Editor Camera", nullptr, &m_ShowEditorCameraControlls);
				ImGui::MenuItem("Info", nullptr, &m_ShowInfo);
				ImGui::MenuItem("ImGui Demo Window", nullptr, &showDemoWindow);

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					Application::Get().CloseApplication();

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
        ImGui::End();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImGui::PopStyleVar(3);

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		ImVec2 size = ImGui::GetContentRegionAvail();

		m_ViewportSizeChanged = false;
		if (m_ViewportWidth != size.x || m_ViewportHeight != size.y)
		{
			m_ViewportWidth = (uint32_t)size.x;
			m_ViewportHeight = (uint32_t)size.y;
			m_ViewportSizeChanged = true;
		}

		auto fbtex = m_CompositFrameBuffer->GetFramBufferContent(0);
		UI::NoAlpaImage(fbtex->GetRenderID(), size);

		ImGuiWindow* window = ImGui::GetCurrentWindow();

		// ImGuizmo
		if (m_CurrentOperation != 0 && m_SelectetEntity)
		{
			SK_CORE_ASSERT(m_SelectetEntity.HasComponent<TransformComponent>(), "Every entity is requiert to have a Transform Component");

			ImVec2 windowPos = window->WorkRect.Min;
			ImVec2 windowSize = window->WorkRect.GetSize();

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);


			DirectX::XMFLOAT4X4 view;
			DirectX::XMFLOAT4X4 projection;
			DirectX::XMStoreFloat4x4(&view, m_EditorCamera.GetView());
			DirectX::XMStoreFloat4x4(&projection, m_EditorCamera.GetProjection());

			auto& tf = m_SelectetEntity.GetComponent<TransformComponent>();
			DirectX::XMFLOAT4X4 transform;
			DirectX::XMStoreFloat4x4(&transform, tf.GetTranform());

			DirectX::XMFLOAT4X4 delta;
			ImGuizmo::Manipulate(&view.m[0][0], &projection.m[0][0], (ImGuizmo::OPERATION)m_CurrentOperation, ImGuizmo::MODE::LOCAL, &transform.m[0][0], &delta.m[0][0]);

			if (ImGuizmo::IsUsing())
			{
				DirectX::XMVECTOR position;
				DirectX::XMVECTOR rotQuat;
				DirectX::XMVECTOR scale;
				DirectX::XMMatrixDecompose(&scale, &rotQuat, &position, DirectX::XMLoadFloat4x4(&transform));
				if (m_CurrentOperation == ImGuizmo::OPERATION::TRANSLATE)
				{
					DirectX::XMMatrixDecompose(&scale, &rotQuat, &position, DirectX::XMLoadFloat4x4(&transform));
					DirectX::XMStoreFloat3(&tf.Position, position);
				}
				else if (m_CurrentOperation == ImGuizmo::OPERATION::SCALE)
				{
					DirectX::XMMatrixDecompose(&scale, &rotQuat, &position, DirectX::XMLoadFloat4x4(&transform));
					DirectX::XMStoreFloat3(&tf.Scaling, scale);
				}
				else if (m_CurrentOperation == ImGuizmo::OPERATION::ROTATE)
				{
					tf.Rotation = Math::GetRotation(transform);
				}
			}

		}

		// Mouse Picking
		int x = -1;
		int y = -1;
		if (!ImGuizmo::IsUsing())
		{
			SK_PROFILE_SCOPE("Mouse Picking");

			auto [mx, my] = ImGui::GetMousePos();
			auto [wx, wy] = window->WorkRect.Min;
			x = (int)(mx - wx);
			y = (int)(my - wy);
			m_HoveredEntityID = -1;

			auto&& [width, height] = m_GemometryFrameBuffer->GetSize();
			if (x >= 0 && x < (int)width && y >= 0 && y < (int)height)
			{
				{
					SK_PROFILE_SCOPE("Read Pixel");

					m_HoveredEntityID = m_GemometryFrameBuffer->ReadPixel(1, x, y);
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !Input::KeyPressed(Key::Alt) && m_ViewportHovered)
				{
					if (m_HoveredEntityID != -1)
					{
						Entity entity{ (entt::entity)(uint32_t)m_HoveredEntityID, m_WorkScene };
						SK_CORE_ASSERT(m_WorkScene->IsValidEntity(entity));
						if (m_WorkScene->IsValidEntity(entity))
							Event::Distribute(SelectionChangedEvent(entity));
					}
					else
					{
						Event::Distribute(SelectionChangedEvent({}));
					}
				}
			}
		}

		// DragDrop
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(UI::ContentPayload::ID);
			if (payload)
			{
				UI::ContentPayload* content = (UI::ContentPayload*)payload->Data;
				if (content->Type == UI::ContentType::Scene)
				{
					m_PlayScene = false;
					LoadNewScene(content->Path);
				}
				else if (content->Type == UI::ContentType::Texture)
				{
					if (m_HoveredEntityID != -1)
					{
						Entity entity{ (entt::entity)m_HoveredEntityID, m_WorkScene };
						SK_CORE_ASSERT(entity.IsValid());
						if (entity.HasComponent<SpriteRendererComponent>())
						{
							auto& sr = entity.GetComponent<SpriteRendererComponent>();
							sr.Texture = Texture2D::Create(content->Path);
						}
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::End();


		if (m_ShowInfo)
		{
			ImGui::Begin("Info", &m_ShowInfo);

			ImGui::Text("FPS: %.1f", 1.0f / m_TimeStep);
			ImGui::Text("FrameTime: %f", m_TimeStep.GetMilliSeconts());

			ImGui::NewLine();

			ImGui::Text("Renderer: Renderer2D");

			auto s = Renderer2D::GetStatistics();
			ImGui::Text("Draw Calls: %d", s.DrawCalls);
			ImGui::Text("Element Count: %d", s.ElementCount);
			ImGui::Text("Vertex Count: %d", s.VertexCount);
			ImGui::Text("Index Count: %d", s.IndexCount);
			ImGui::Text("Textur Count: %d", s.TextureCount);

			ImGui::NewLine();
			if (x >= 0 && x < (int)m_ViewportWidth && y >= 0 && y < (int)m_ViewportHeight && m_HoveredEntityID >= 0)
			{
				Entity e{ (entt::entity)m_HoveredEntityID, m_WorkScene };
				if (e.IsValid())
				{
					const auto& tag = e.GetComponent<TagComponent>().Tag;
					ImGui::Text("Hoverted Entity: ID: %d, Tag: %s", m_HoveredEntityID, tag.c_str());
				}
				else
				{
					ImGui::Text("Hovered Entity: UnValid Entity");
				}
			}
			else
			{
				if (m_HoveredEntityID == -1)
					ImGui::Text("Hovered Entity: No Entity");
				else
					ImGui::Text("Hovered Entity: No Entity, %d", m_HoveredEntityID);
			}
			ImGui::Text("Selected Entity ID: %d", (uint32_t)m_SceneHirachyPanel.GetSelectedEntity());

			ImGui::NewLine();
			static MemoryMetrics s_LastMemory;
			const auto& m = MemoryManager::GetMetrics();
			ImGui::Text("Memory Usage: %llu", m.MemoryUsage());
			ImGui::Text("Memory Allocated: %llu", m.MemoryAllocated);
			ImGui::Text("Memory Freed: %llu", m.MemoryFreed);
			ImGui::Text("Total Count: %llu", m.TotalAllocated - m.TotalFreed);
			ImGui::Text("Total Allocated: %llu", m.TotalAllocated);
			ImGui::Text("Total Freed: %llu", m.TotalFreed);

			ImGui::NewLine();

			ImGui::Text("Memory Usage Delta: %llu", (m.MemoryAllocated - s_LastMemory.MemoryAllocated) - (m.MemoryFreed - s_LastMemory.MemoryFreed));
			ImGui::Text("Memory Allocated Delta: %llu", m.MemoryAllocated - s_LastMemory.MemoryAllocated);
			ImGui::Text("Memory Freed Delta: %llu", m.MemoryFreed - s_LastMemory.MemoryFreed);
			ImGui::Text("Total Count Delta: %llu", (m.TotalAllocated - s_LastMemory.TotalAllocated) - (m.TotalFreed - s_LastMemory.TotalFreed));
			ImGui::Text("Total Allocated Delta: %llu", m.TotalAllocated - s_LastMemory.TotalAllocated);
			ImGui::Text("Total Freed Delta: %llu", m.TotalFreed - s_LastMemory.TotalFreed);
			s_LastMemory = m;

			ImGui::Checkbox("Negative Effect", &m_NegativeEffect);
			ImGui::Checkbox("Blur Effect", &m_BlurEffect);

			ImGui::End();


		}
		
		if (ImGui::Begin("Shaders"))
		{
			for (auto&& [key, shader] : Renderer::GetShaderLib())
			{
				if (ImGui::TreeNodeEx(key.c_str()))
				{
					ImGui::Text("Path: %s", shader->GetFilePath().c_str());
					if (ImGui::Button("ReCompile"))
						shader->ReCompile();
					// IDEA: Print Shader Detailes (From Reflection)
					ImGui::TreePop();
				}
			}
		}
		ImGui::End();



		if (m_ShowEditorCameraControlls)
		{
			if (ImGui::Begin("Editor Camera", &m_ShowEditorCameraControlls))
			{

				UI::DrawVec3Show("Position", m_EditorCamera.GetPosition());

				auto focuspoint = m_EditorCamera.GetFocusPoint();
				if (UI::DrawVec3Control("FocusPoint", focuspoint))
					m_EditorCamera.SetFocusPoint(focuspoint);

				DirectX::XMFLOAT2 py = { m_EditorCamera.GetPitch(), m_EditorCamera.GetYaw() };
				if (UI::DrawVec2Control("Orientation", py))
				{
					m_EditorCamera.SetPicht(py.x);
					m_EditorCamera.SetYaw(py.y);
				}

				float distance = m_EditorCamera.GetDistance();
				if (UI::DrawFloatControl("Distance", distance, 10))
					if (distance >= 0.25f)
						m_EditorCamera.SetDistance(distance);

				if (ImGui::Button("Reset"))
				{
					m_EditorCamera.SetFocusPoint({ 0.0f, 0.0f, 0.0f });
					m_EditorCamera.SetPicht(0.0f);
					m_EditorCamera.SetYaw(0.0f);
					m_EditorCamera.SetDistance(10.0f);
				}
			}
			ImGui::End();
		}

		OnImGuiRender_Project();
		m_SceneHirachyPanel.OnImGuiRender();
		m_AssetsPanel.OnImGuiRender();

		if (showDemoWindow)
			ImGui::ShowDemoWindow(&showDemoWindow);
	}

	void EditorLayer::OnImGuiRender_Project()
	{
		if (!m_ShowProject)
			return;

		if (ImGui::Begin("Project", &m_ShowProject))
		{
			Project& proj = Application::Get().GetProject();
			ImGui::BeginTable("Table", 2, ImGuiTableFlags_BordersInnerV);
			ImGui::TableSetupColumn("LabelCollumn", ImGuiTableColumnFlags_WidthFixed, 100);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Name");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##ProjectName", proj.GetProjectName());
			UI::GetContentPayload(proj.GetProjectName(), UI::ContentType::Directory);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Assets");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##AssetsPath", proj.GetAssetsPath());
			UI::GetContentPayload(proj.GetAssetsPath(), UI::ContentType::Directory);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Textures");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##TexturesPath", proj.GetTexturesPath());
			UI::GetContentPayload(proj.GetTexturesPath(), UI::ContentType::Directory);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Scens");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##ScenesPath", proj.GetScenesPath());
			UI::GetContentPayload(proj.GetScenesPath(), UI::ContentType::Directory);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Startup Scene");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##StartupScene", proj.GetStartupScene());
			UI::GetContentPayload(proj.GetStartupScene(), UI::ContentType::Scene);

			ImGui::EndTable();

			ImGui::Separator();

			ImGuiStyle& style = ImGui::GetStyle();

			if (ImGui::Button("Add Current Scene"))
			{
				const auto& path = m_WorkScene->GetFilePath();
				if (!Utility::Contains(proj.GetScenes(), path))
					proj.AddScene(path);
			}
			const float itemheight = ImGui::GetFontSize() + UI::GetFramePadding().y * 2.0f;
			const float height = std::max(ImGui::GetContentRegionAvail().y - (itemheight + UI::GetFramePadding().y) - 1, itemheight);
			if (ImGui::BeginChild("ProjectScenes", { 0, height }, true))
			{
				UI::MoveCurserPosY(-UI::GetFramePadding().y);
				for (uint32_t index = 0; index < proj.GetNumScenes();)
				{
					ImGui::PushID((int)index);
					bool incement = true;

					UI::Text(proj.GetSceneAt(index));

					float size = ImGui::GetFontSize();
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
					ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.8f, 0.8f, 0.8f, 0.1f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.8f, 0.8f, 0.2f });
					const bool pressed = UI::ButtonRightAligned("+", { size, size });
					ImGui::PopStyleColor(3);
					ImGui::PopStyleVar();

					const ImGuiID popupID = ImGui::GetID("EditScene_PopUp");
					if (pressed)
						ImGui::OpenPopupEx(popupID);
					if (UI::BeginPopup(popupID, ImGuiWindowFlags_None))
					{
						if (ImGui::Selectable("Move Up", false, index == 0 ? ImGuiSelectableFlags_Disabled : 0))
							std::swap(proj.GetSceneAt(index), proj.GetSceneAt(index - 1));

						if (ImGui::Selectable("Move Down", false, index == proj.GetNumScenes() - 1 ? ImGuiSelectableFlags_Disabled : 0))
							std::swap(proj.GetSceneAt(index), proj.GetSceneAt(index + 1));

						ImGui::Separator();

						if (ImGui::Selectable("Remove"))
						{
							proj.Remove(index);
							incement = false;
						}
						ImGui::EndPopup();
					}

					if (incement)
						index++;

					ImGui::PopID();
					ImGui::Separator();
				}
			}
			ImGui::EndChild();

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(UI::ContentPayload::ID);
				if (payload)
				{
					UI::ContentPayload* content = (UI::ContentPayload*)payload->Data;
					if (content->Type == UI::ContentType::Scene)
						if (!Utility::Contains(proj.GetScenes(), content->Path))
							proj.AddScene(content->Path);
				}
				ImGui::EndDragDropTarget();
			}
			if (ImGui::Button("Save Project"))
				proj.SaveProjectFile();
			ImGui::SameLine();
			if (ImGui::Button("Load Project"))
				proj.LoadProject();

		}
		ImGui::End();
	}

	void EditorLayer::NewScene()
	{
		SetActiveScene(Ref<Scene>::Create());
	}

	/*
	bool EditorLayer::SaveSceneWithDialogBox()
	{
		auto filepath = FileDialogs::SaveFile("Shark Scene (*.shark)\0*.shark\0");
		if (!filepath.empty())
		{
			SceneSerializer serializer(m_WorkScene);
			if (serializer.Deserialize(filepath))
				return true;
		}
		return false;
	}
	
	bool EditorLayer::OpenSceneWithDialogBox()
	{
		auto filepath = FileDialogs::OpenFile("Shark Scene (*.shark)\0*.shark\0");
		if (!filepath.empty())
		{
			m_PlayScene = false;
			SceneSerializer serializer(m_WorkScene);
			if (serializer.Serialize(filepath))
				return true;
		}
		return false;
	}
	*/

	void EditorLayer::SetActiveScene(Ref<Scene> scene)
	{
		m_WorkScene = scene;
		m_WorkScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		m_SceneHirachyPanel.SetContext(m_WorkScene);
	}

	bool EditorLayer::LoadNewScene(const std::filesystem::path& filepath)
	{
		auto scene = Ref<Scene>::Create();
		SceneSerializer serializer(scene);
		if (serializer.Deserialize(filepath))
		{
			scene->SetFilePath(filepath);
			SetActiveScene(scene);
			return true;
		}
		return false;
	}

	bool EditorLayer::LoadScene()
	{
		SceneSerializer serializer(m_WorkScene);
		return serializer.Deserialize();
	}

	bool EditorLayer::SaveScene()
	{
		SceneSerializer serializer(m_WorkScene);
		return serializer.Serialize();
	}

	void EditorLayer::OnPlayScene()
	{
		SK_PROFILE_FUNCTION();

		m_PlayScene = true;
		m_SceneHirachyPanel.SetScenePlaying(true);
		SceneManager::SetActiveScene(m_WorkScene->GetCopy());
		SceneManager::GetActiveScene()->OnScenePlay();
	}

	void EditorLayer::OnStopScene()
	{
		SK_PROFILE_FUNCTION();

		SceneManager::GetActiveScene()->OnSceneStop();
		SceneManager::SetActiveScene(nullptr);
		m_SceneHirachyPanel.SetScenePlaying(false);
		m_PlayScene = false;

		if (!m_WorkScene->IsValidEntity(m_SceneHirachyPanel.GetSelectedEntity()))
			Event::Distribute(SelectionChangedEvent({}));
	}

}