#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Memory/Allocator.h"
#include "Shark/Memory/MemoryManager.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Project.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/Random.h"
#include "Shark/Core/Timer.h"
#include "Shark/Core/Counter.h"

// --- Utility --------------------------
#include "Shark/Utility/PlatformUtils.h"
#include "Shark/Utility/Utility.h"
#include "Shark/Utility/ImGuiUtils.h"
#include "Shark/Utility/FileSystem.h"
#include "Shark/Utility/Math.h"
#include "Shark/Utility/FileWatcher.h"
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
#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Rasterizer.h"
#include "Shark/Render/Camera.h"
#include "Shark/Render/EditorCamera.h"
// --------------------------------------

// --- Scene ----------------------------
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/SceneCamera.h"
#include "Shark/Scene/NativeScript.h"
#include "Shark/Scene/Components.h"
#include "Shark/Scene/SceneSerialization.h"
#include "Shark/Scene/NativeScriptFactory.h"
#include "Shark/Scene/SceneManager.h"
// --------------------------------------
