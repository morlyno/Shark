#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Memory/Allocator.h"
#include "Shark/Memory/MemoryManager.h"

// --- Core -----------------------------
#include "Shark/Core/Application.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/Timer.h"
// --------------------------------------

// --- Utility --------------------------
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Math/Math.h"
#include "Shark/Utils/String.h"
// --------------------------------------

// --- UI -------------------------------
#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
// --------------------------------------

// --- File -----------------------------
#include "Shark/File/FileWatcher.h"
#include "Shark/File/FileSystem.h"
// --------------------------------------

// --- Event ----------------------------
#include "Shark/Event/Event.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/ApplicationEvent.h"
// --------------------------------------

// --- Layer ----------------------------
#include "Shark/Layer/Layer.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"
// --------------------------------------

#include "Shark/Core/Window.h"

// --- Render ---------------------------
#include "Shark/Render/RendererAPI.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Camera.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Render/SceneRenderer.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/Material.h"
// --------------------------------------

// --- Scene ----------------------------
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/SceneCamera.h"
#include "Shark/Scene/NativeScript.h"
#include "Shark/Scene/Components.h"
#include "Shark/Scene/NativeScriptFactory.h"
// --------------------------------------

// --- Asset ----------------------------
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetTypes.h"
#include "Shark/Asset/ResourceManager.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/Asset/SceneSerialization.h"
#include "Shark/Asset/TextureSerialization.h"
// --------------------------------------
