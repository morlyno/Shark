#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Input.h"

#include "Shark/Event/Event.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/ApplicationEvent.h"

#include "Shark/Layer/Layer.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"

#include "Shark/Core/Window.h"

// --- Render ---------------------------
#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Viewport.h"
#include "Shark/Render/SwapChain.h"
#include "Shark/Render/Camera.h"
#include "Shark/Render/EditorCamera.h"
// --------------------------------------

// --- ECS ------------------------------
#include "Shark/Scean/Scean.h"
#include "Shark/Scean/Entity.h"
#include "Shark/Scean/Components/Components.h"
// --------------------------------------
