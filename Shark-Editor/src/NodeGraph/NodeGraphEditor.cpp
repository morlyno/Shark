#include "NodeGraphEditor.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"

#include "NodeGraph/Utilities/builders.h"
#include "NodeGraph/Utilities/drawing.h"
#include "NodeGraph/Utilities/widgets.h"

namespace Shark {

	static ImColor GetIconColor(GraphEditor::PinType type)
	{
		switch (type)
		{
			default:
			case GraphEditor::PinType::Flow:     return ImColor(255, 255, 255);
			case GraphEditor::PinType::Bool:     return ImColor(220, 48, 48);
			case GraphEditor::PinType::Int:      return ImColor(68, 201, 156);
			case GraphEditor::PinType::Float:    return ImColor(147, 226, 74);
			//case GraphEditor::PinType::String:   return ImColor(124, 21, 153);
			//case GraphEditor::PinType::Object:   return ImColor(51, 150, 215);
			//case GraphEditor::PinType::Function: return ImColor(218, 0, 183);
			//case GraphEditor::PinType::Delegate: return ImColor(255, 48, 48);
		}
	};

	static void DrawPinIcon(const GraphEditor::Pin& pin, bool connected, int alpha)
	{
		ax::Drawing::IconType iconType;
		ImColor  color = GetIconColor(pin.Type);
		color.Value.w = alpha / 255.0f;
		switch (pin.Type)
		{
			case GraphEditor::PinType::Flow:     iconType = ax::Drawing::IconType::Flow;   break;
			case GraphEditor::PinType::Bool:     iconType = ax::Drawing::IconType::Circle; break;
			case GraphEditor::PinType::Int:      iconType = ax::Drawing::IconType::Circle; break;
			case GraphEditor::PinType::Float:    iconType = ax::Drawing::IconType::Circle; break;
			//case GraphEditor::PinType::String:   iconType = ax::Drawing::IconType::Circle; break;
			//case GraphEditor::PinType::Object:   iconType = ax::Drawing::IconType::Circle; break;
			//case GraphEditor::PinType::Function: iconType = ax::Drawing::IconType::Circle; break;
			//case GraphEditor::PinType::Delegate: iconType = ax::Drawing::IconType::Square; break;
			default:
				return;
		}

		ax::Widgets::Icon(ImVec2(24.0f, 24.0f), iconType, connected, color, ImColor(32, 32, 32, alpha));
	};

	bool NodeGraphEditor::OnShowPanel()
	{
		ax::NodeEditor::Config config;
		config.UserPointer = this;

		m_Context = ax::NodeEditor::CreateEditor(&config);

		m_PropertiesWindowID = fmt::format("Properties##{}", m_PanelName);
		return true;
	}

	bool NodeGraphEditor::OnHidePanel()
	{
		m_NodeGraph = nullptr;
		m_Nodes.clear();
		m_Links.clear();
		ax::NodeEditor::DestroyEditor(m_Context);
		m_Context = nullptr;
		return true;
	}

	void NodeGraphEditor::OnImGuiRender(bool& isOpen)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 2.0f, 0.0f });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::Begin(m_PanelName, &isOpen);
		ImGui::PopStyleVar(2);

		m_WindowClass.ClassId = ImGui::GetID(m_PanelName);
		m_WindowClass.DockingAllowUnclassed = false;
		m_DockspaceID = ImGui::DockSpace(ImGui::GetID(m_PanelName), { 0, 0 }, ImGuiDockNodeFlags_AutoHideTabBar);
		Draw();

		ImGui::End();
	}

	void NodeGraphEditor::Draw()
	{
		ImGui::SetNextWindowDockID(m_DockspaceID, ImGuiCond_Always);
		ImGui::SetNextWindowClass(&m_WindowClass);
		if (ImGui::Begin("Canvas"))
		{
			ImGui::BeginHorizontal(UI::GenerateID());
			if (ImGui::Button("Compile"))
			{
				m_NodeGraph = CompileGraph();
			}

			if (ImGui::Button("Run"))
			{
				if (!m_NodeGraph)
					m_NodeGraph = CompileGraph();

				for (auto* node : m_NodeGraph->Nodes)
				{
					node->Process();
				}

				SK_CORE_DEBUG("NodeGraph outputs:");
				for (auto& output : m_NodeGraph->OutputVariables)
				{
					if (output.isBool())
						SK_CORE_DEBUG(" - {}", output.getBool());
					else if (output.isInt32())
						SK_CORE_DEBUG(" - {}", output.getInt32());
					else if (output.isFloat32())
						SK_CORE_DEBUG(" - {}", output.getFloat32());
					else
						SK_CORE_DEBUG(" - Unknown type");
				}
			}

			ImGui::EndHorizontal();

			DrawCanvas();

		}
		ImGui::End();

		DrawVariables();
	}

	void NodeGraphEditor::DrawCanvas()
	{
		namespace ne = ax::NodeEditor;

		ne::SetCurrentEditor(m_Context);
		ne::Begin("Node Graph");

		ne::Utilities::BlueprintNodeBuilder builder;

		for (auto& node : m_Nodes)
		{
			builder.Begin(node.ID);

			builder.Header(node.Color);
			ImGui::Spring(0);
			ImGui::TextUnformatted(node.Name.c_str());
			ImGui::Spring(1);
			ImGui::Dummy(ImVec2(0, 28));
			ImGui::Spring(0);
			builder.EndHeader();

			for (auto& input : node.Inputs)
			{
				auto alpha = ImGui::GetStyle().Alpha;
				if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				builder.Input(input.ID);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
				ImGui::Spring(0);
				if (!input.Name.empty())
				{
					ImGui::TextUnformatted(input.Name.c_str());
					ImGui::Spring(0);
				}

				if (!IsPinLinked(input.ID))
				{
					DrawPinValueEdit(&input);
					ImGui::Spring(0);
				}

				ImGui::PopStyleVar();
				builder.EndInput();
			}

			for (auto& output : node.Outputs)
			{
				auto alpha = ImGui::GetStyle().Alpha;
				if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				builder.Output(output.ID);
				if (!output.Name.empty())
				{
					ImGui::Spring(0);
					ImGui::TextUnformatted(output.Name.c_str());
				}
				ImGui::Spring(1);
				DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
				ImGui::PopStyleVar();
				builder.EndOutput();
			}

			builder.End();
		}

		for (auto& link : m_Links)
			ne::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

		if (!createNewNode)
		{
			if (ne::BeginCreate(ImColor(255, 255, 255), 2.0f))
			{
				auto showLabel = [](const char* label, ImColor color)
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
					auto size = ImGui::CalcTextSize(label);

					auto padding = ImGui::GetStyle().FramePadding;
					auto spacing = ImGui::GetStyle().ItemSpacing;

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

					auto rectMin = ImGui::GetCursorScreenPos() - padding;
					auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
					ImGui::TextUnformatted(label);
				};

				ne::PinId startPinId = 0, endPinId = 0;
				if (ne::QueryNewLink(&startPinId, &endPinId))
				{
					auto startPin = FindPin(startPinId);
					auto endPin = FindPin(endPinId);

					newLinkPin = startPin ? startPin : endPin;

					if (startPin->Kind == ne::PinKind::Input)
					{
						std::swap(startPin, endPin);
						std::swap(startPinId, endPinId);
					}

					if (startPin && endPin)
					{
						if (endPin == startPin || IsPinLinked(endPin->ID))
						{
							ne::RejectNewItem(ImColor(255, 0, 0), 2.0f);
						}
						else if (endPin->Kind == startPin->Kind)
						{
							showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
							ne::RejectNewItem(ImColor(255, 0, 0), 2.0f);
						}
						else if (endPin->NodeID == startPin->NodeID)
						{
							showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
							ne::RejectNewItem(ImColor(255, 0, 0), 1.0f);
						}
						else if (endPin->Type != startPin->Type)
						{
							showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
							ne::RejectNewItem(ImColor(255, 128, 128), 1.0f);
						}
						else
						{
							showLabel("+ Create Link", ImColor(32, 45, 32, 180));
							if (ne::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
							{
								if (WouldCreateLoop(startPin, endPin))
								{
									SK_CONSOLE_ERROR("[NodeGraph] Connection would have created a loop");
								}
								else
								{
									m_Links.emplace_back(GraphEditor::Link(GetNextID(), startPinId, endPinId));
									m_Links.back().Color = GetIconColor(startPin->Type);
								}
							}
						}
					}
				}


				ne::PinId pinId = 0;
				if (ne::QueryNewNode(&pinId))
				{
					newLinkPin = FindPin(pinId);
					if (newLinkPin)
						showLabel("+ Create Node", ImColor(32, 45, 32, 180));

					if (ne::AcceptNewItem())
					{
						createNewNode = true;
						newNodeLinkPin = FindPin(pinId);
						newLinkPin = nullptr;
						ne::Suspend();
						ImGui::OpenPopup("Create New Node");
						ne::Resume();
					}
				}
			}
			else
				newLinkPin = nullptr;

			ne::EndCreate();


			if (ne::BeginDelete())
			{
				ne::NodeId nodeId = 0;
				while (ne::QueryDeletedNode(&nodeId))
				{
					if (ne::AcceptDeletedItem())
					{
						auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
						if (id != m_Nodes.end())
							m_Nodes.erase(id);
					}
				}

				ne::LinkId linkId = 0;
				while (ne::QueryDeletedLink(&linkId))
				{
					if (ne::AcceptDeletedItem())
					{
						auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
						if (id != m_Links.end())
							m_Links.erase(id);
					}
				}
			}
			ne::EndDelete();
		}


		auto openPopupPosition = ImGui::GetMousePos();
		ne::Suspend();
		if (ne::ShowNodeContextMenu(&contextNodeId))
			ImGui::OpenPopup("Node Context Menu");
		else if (ne::ShowPinContextMenu(&contextPinId))
			ImGui::OpenPopup("Pin Context Menu");
		else if (ne::ShowLinkContextMenu(&contextLinkId))
			ImGui::OpenPopup("Link Context Menu");
		else if (ne::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
			newNodeLinkPin = nullptr;
		}
		ne::Resume();

		ne::Suspend();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		if (ImGui::BeginPopup("Node Context Menu"))
		{
			auto node = FindNode(contextNodeId);

			ImGui::TextUnformatted("Node Context Menu");
			ImGui::Separator();
			if (node)
			{
				ImGui::Text("ID: %p", node->ID.AsPointer());
				//ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
				ImGui::Text("Inputs: %d", (int)node->Inputs.size());
				ImGui::Text("Outputs: %d", (int)node->Outputs.size());
			}
			else
				ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
			ImGui::Separator();
			if (ImGui::MenuItem("Delete"))
				ne::DeleteNode(contextNodeId);
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Pin Context Menu"))
		{
			auto pin = FindPin(contextPinId);

			ImGui::TextUnformatted("Pin Context Menu");
			ImGui::Separator();
			if (pin)
			{
				ImGui::Text("ID: %p", pin->ID.AsPointer());
				if (pin->NodeID)
					ImGui::Text("Node: %p", pin->NodeID.AsPointer());
				else
					ImGui::Text("Node: %s", "<none>");
			}
			else
				ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Link Context Menu"))
		{
			auto link = FindLink(contextLinkId);

			ImGui::TextUnformatted("Link Context Menu");
			ImGui::Separator();
			if (link)
			{
				ImGui::Text("ID: %p", link->ID.AsPointer());
				ImGui::Text("From: %p", link->StartPinID.AsPointer());
				ImGui::Text("To: %p", link->EndPinID.AsPointer());
			}
			else
				ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
			ImGui::Separator();
			if (ImGui::MenuItem("Delete"))
				ne::DeleteLink(contextLinkId);
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Create New Node"))
		{
			auto newNodePostion = openPopupPosition;
			//ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

			//auto drawList = ImGui::GetWindowDrawList();
			//drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

			GraphEditor::Node* node = nullptr;
			if (ImGui::BeginMenu("Math"))
			{
				const auto menuItem = [](const char* label, const char* extra = nullptr)
				{
					const bool pressed = ImGui::MenuItem(label);
					if (extra)
						UI::DrawTextAligned(extra, { 1, 0 }, UI::GetItemRect());
					return pressed;
				};

				if (menuItem("Add##int", "int"))
					node = SpawnNode<Nodes::Add<int>>();
				if (menuItem("Add##float", "float"))
					node = SpawnNode<Nodes::Add<float>>();
				if (menuItem("Multiply##int", "int"))
					node = SpawnNode<Nodes::Multiply<int>>();
				if (menuItem("Multiply##float", "float"))
					node = SpawnNode<Nodes::Multiply<float>>();

				ImGui::Dummy({ 100.0f, 0.0f });
				ImGui::EndMenu();
			}


			if (node)
			{
				createNewNode = false;
				ne::SetNodePosition(node->ID, newNodePostion);

				if (auto startPin = newNodeLinkPin)
				{
					auto& pins = startPin->Kind == ne::PinKind::Input ? node->Outputs : node->Inputs;

					for (auto& pin : pins)
					{
						if (CanCreateLink(startPin, &pin))
						{
							auto endPin = &pin;
							if (startPin->Kind == ne::PinKind::Input)
								std::swap(startPin, endPin);

							m_Links.emplace_back(GraphEditor::Link(GetNextID(), startPin->ID, endPin->ID));
							m_Links.back().Color = GetIconColor(startPin->Type);
							break;
						}
					}
				}
			}

			ImGui::EndPopup();
		}
		else
			createNewNode = false;
		ImGui::PopStyleVar();
		ne::Resume();

		ne::End();

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("input_variable"))
			{
				auto* node = SpawnInputNode(std::string_view(static_cast<const char*>(payload->Data), payload->DataSize));
				ne::SetNodePosition(node->ID, ne::ScreenToCanvas(ImGui::GetMousePos()));
			}

			ImGui::EndDragDropTarget();
		}

	}

	void NodeGraphEditor::DrawVariables()
	{
		ImGui::SetNextWindowClass(&m_WindowClass);
		if (ImGui::Begin("Variables"))
		{
			if (ImGui::Button("Add Input"))
			{
				m_InputVariables.emplace_back("New Input", choc::value::createFloat32(0.0f), GraphEditor::PinType::Float);
			}

			for (size_t i = 0; i < m_InputVariables.size(); i++)
			{
				auto& input = m_InputVariables[i];

				const char* name = input.Name.empty() ? "##temp" : input.Name.c_str();
				if (ImGui::Selectable(name, m_SelectedInput == i))
					m_SelectedInput = i;

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("input_variable", input.Name.c_str(), input.Name.size());
					ImGui::Text(input.Name);
					ImGui::EndDragDropSource();
				}
			}

		}
		ImGui::End();

		ImGui::SetNextWindowClass(&m_WindowClass);
		if (ImGui::Begin(m_PropertiesWindowID.c_str()))
		{
			if (m_SelectedInput != ~0 && m_SelectedInput < m_InputVariables.size())
			{
				auto& input = m_InputVariables[m_SelectedInput];
				UI::BeginControlsGrid();

				m_RenameBuffer.assign(input.Name);
				UI::Control("Name", input.Name);
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					RenameInput(input, m_RenameBuffer);
				}

				auto inputType = input.Type;
				if (UI::Control("Type", inputType))
				{
					ChangeInputType(input, inputType);
				}

				const auto drawControl = []<typename T>(choc::value::Value& value)
				{
					T v = value.isVoid() ? T{} : value.get<T>();
					if (UI::Control("Value", v))
					{
						if (value.isVoid())
							value = choc::value::createPrimitive(v);
						else
							value.getViewReference().set(v);
					}
				};

				switch (input.Type)
				{
					case GraphEditor::PinType::Bool: drawControl.operator() < bool > (input.Value); break;
					case GraphEditor::PinType::Int: drawControl.operator() < int > (input.Value); break;
					case GraphEditor::PinType::Float: drawControl.operator() < float > (input.Value); break;
						//case GraphEditor::PinType::String: drawControl.operator()<std::string>(input.Value); break;
					default:
					{
						UI::ScopedStyle text(ImGuiCol_Text, UI::Colors::Theme::TextError);
						ImGui::Text("Unknown type");
						break;
					}
				}

				UI::EndControlsGrid();
			}
		}
		ImGui::End();
	}

	Scope<NodeGraph> NodeGraphEditor::CompileGraph()
	{
		auto nodeGraph = Scope<NodeGraph>::Create();

		struct LoosePin
		{
			size_t NodeIndex;
			GraphEditor::Pin* GraphPin;
			Identifier Pin;
		};

		std::vector<LoosePin> loosePins;
		std::vector<LoosePin> looseOutputPins;
		std::unordered_map<UUID, std::set<UUID>> dependencies;

		std::unordered_map<GraphEditor::Pin*, choc::value::ValueView> inputVariablePins;

		for (size_t index = 0;
			auto& editorNode : m_Nodes)
		{
			if (editorNode.Name == "Input")
			{
				const auto input = std::ranges::find(m_InputVariables, editorNode.Outputs[0].Name, [](auto& input) { return input.Name; });
				if (input != m_InputVariables.end())
					inputVariablePins.emplace(&editorNode.Outputs[0], input->Value);
				continue;
			}

			if (!m_NodeAllocators.contains(editorNode.Name))
			{
				SK_CORE_ASSERT(false);
				continue;
			}

			const size_t nodeIndex = index++;
			//nodeGraph->Nodes.push_back(new Nodes::Add(editorNode.GetID()));
			nodeGraph->Nodes.push_back(m_NodeAllocators.at(editorNode.Name)(editorNode.GetID()));

			for (auto& input : editorNode.Inputs)
				if (!IsPinLinked(input.ID))
					loosePins.push_back({ nodeIndex, &input, input.Identifier });
			for (auto& output : editorNode.Outputs)
				if (!IsPinLinked(output.ID))
					looseOutputPins.push_back({ nodeIndex, &output, output.Identifier });
		}

		const auto FindNodeByID = [&nodes = nodeGraph->Nodes](UUID id) -> Node*
		{
			for (auto* node : nodes)
				if (node->ID == id)
					return node;
			return nullptr;
		};


		// connect nodes
		for (auto& link : m_Links)
		{
			auto* startPin = FindPin(link.StartPinID);
			auto* endPin = FindPin(link.EndPinID);
			SK_CORE_ASSERT(startPin && endPin);

			auto startNode = FindNodeByID(startPin->GetNodeID());
			auto endNode = FindNodeByID(endPin->GetNodeID());

			if (!startNode && inputVariablePins.contains(startPin))
			{
				choc::value::ValueView& input = endNode->GetInput(endPin->Identifier);
				input = inputVariablePins.at(startPin);
				continue;
			}

			SK_CORE_ASSERT(startNode && endNode);

			choc::value::ValueView& output = startNode->GetOutput(startPin->Identifier);
			choc::value::ValueView& input = endNode->GetInput(endPin->Identifier);

			input = output;

			auto& inputNodes = dependencies[endPin->GetNodeID()];
			inputNodes.emplace(startPin->GetNodeID());
		}

		nodeGraph->LocalVariables.reserve(loosePins.size());

		for (auto& loosePin : loosePins)
		{
			auto* node = nodeGraph->Nodes[loosePin.NodeIndex];
			choc::value::ValueView& input = node->GetInput(loosePin.Pin);

			if (loosePin.GraphPin->Value.isVoid())
				loosePin.GraphPin->Value = choc::value::Value(input.getType());

			input = nodeGraph->LocalVariables.emplace_back(loosePin.GraphPin->Value);
		}

#if 1
		for (auto& loosePin : looseOutputPins)
		{
			auto* node = nodeGraph->Nodes[loosePin.NodeIndex];
			nodeGraph->OutputVariables.push_back(node->GetOutput(loosePin.Pin));
			nodeGraph->DebugOutputNames.push_back(loosePin.GraphPin->Name);
		}
#endif

		// sort nodes
		// this assumes there are no loops in the graph

		const auto graphIsLess = [&dependencies](const Node* lhs, const Node* rhs)
		{
			const bool lhsHasDeps = dependencies.contains(lhs->ID);
			const bool rhsHasDeps = dependencies.contains(rhs->ID);

			if (!lhsHasDeps && !rhsHasDeps)
				return lhs->ID < rhs->ID;

			if (lhsHasDeps != rhsHasDeps)
				return lhsHasDeps < rhsHasDeps;

			// check if lhs depends on rhs
			if (dependencies.at(lhs->ID).contains(rhs->ID))
			{
				// lhs depends on rhs

				// rhs must be processed first
				// => rhs is less
				return false;
			}

			if (dependencies.at(rhs->ID).contains(lhs->ID))
			{
				// rhs depends on lhs

				// lhs must be processed fist
				// => lhs is less
				return true;
			}

			// lhs and rhs don't directly depend on each other
			// the parent/children might though
			// do i need to traverse the hole tree here or does sort do that for me?
			// sort by ID

			//const bool lhsEmpty = dependencies.at(lhs->ID).empty();
			//const bool rhsEmpty = dependencies.at(rhs->ID).empty();
			//
			//if (lhsEmpty != rhsEmpty)
			//	return lhsEmpty;

			return lhs->ID < rhs->ID;
		};

		std::ranges::sort(nodeGraph->Nodes, graphIsLess);

		for (auto& node : nodeGraph->Nodes)
			node->Initialize();

		return nodeGraph;
	}

	Shark::GraphEditor::Node* NodeGraphEditor::FindNode(ax::NodeEditor::NodeId id)
	{
		for (auto& node : m_Nodes)
			if (node.ID == id)
				return &node;

		return nullptr;
	}

	Shark::GraphEditor::Link* NodeGraphEditor::FindLink(ax::NodeEditor::LinkId id)
	{
		for (auto& link : m_Links)
			if (link.ID == id)
				return &link;

		return nullptr;
	}

	Shark::GraphEditor::Link* NodeGraphEditor::FindLink(ax::NodeEditor::PinId id)
	{
		for (auto& link : m_Links)
			if (link.StartPinID == id || link.EndPinID == id)
				return &link;

		return nullptr;
	}

	Shark::GraphEditor::Pin* NodeGraphEditor::FindPin(ax::NodeEditor::PinId id)
	{
		if (!id)
			return nullptr;

		for (auto& node : m_Nodes)
		{
			for (auto& pin : node.Inputs)
				if (pin.ID == id)
					return &pin;

			for (auto& pin : node.Outputs)
				if (pin.ID == id)
					return &pin;
		}

		return nullptr;
	}

	void NodeGraphEditor::RemoveLinks(ax::NodeEditor::PinId id)
	{
		for (auto link = m_Links.begin(); link != m_Links.end();)
		{
			if (link->StartPinID != id && link->EndPinID != id)
			{
				++link;
				continue;
			}

			link = m_Links.erase(link);
		}

	}

	bool NodeGraphEditor::IsPinLinked(ax::NodeEditor::PinId id) const
	{
		if (!id)
			return false;

		for (auto& link : m_Links)
			if (link.StartPinID == id || link.EndPinID == id)
				return true;

		return false;
	}

	bool NodeGraphEditor::CanCreateLink(GraphEditor::Pin* pinA, GraphEditor::Pin* pinB)
	{
		if (!pinA || !pinB || pinA == pinB || pinA->Kind == pinB->Kind || pinA->Type != pinB->Type || pinA->NodeID == pinB->NodeID)
			return false;

		return true;
	}

	bool NodeGraphEditor::WouldCreateLoop(GraphEditor::Pin* startPin, GraphEditor::Pin* endPin)
	{
		// startPin is output pin
		// endPin is input pin

		// connected to self
		if (startPin->NodeID == endPin->NodeID)
			return true;

		// loop over inputs because one output could have multiple links
		auto* startNode = FindNode(startPin->NodeID);
		for (auto& inputPin : startNode->Inputs)
		{
			auto* link = FindLink(inputPin.ID);
			if (!link)
				continue;

			// StartPinID is output
			auto pin = FindPin(link->StartPinID);
			SK_CORE_ASSERT(pin, "Should never be null");
			if (WouldCreateLoop(pin, endPin))
				return true;
		}

		return false;
	}

	GraphEditor::Node* NodeGraphEditor::SpawnInputNode(std::string_view inputName)
	{
		const auto input = std::ranges::find(m_InputVariables, inputName, &Input::Name);
		if (input == m_InputVariables.end())
			return nullptr;

		auto& node = m_Nodes.emplace_back();
		node.ID = GetNextID();
		node.Name = "Input";

		node.Outputs.push_back({
			.ID = GetNextID(),
			.NodeID = node.ID,
			.Kind = ax::NodeEditor::PinKind::Output,
			.Name = std::string(inputName),
			.Type = input->Type
		});

		return &node;
	}

	bool NodeGraphEditor::DrawPinValueEdit(GraphEditor::Pin* pin)
	{
		bool modified = false;

		switch (pin->Type)
		{
			case GraphEditor::PinType::Bool:
			{
				bool value = pin->Value.isVoid() ? false : pin->Value.get<bool>();
				if (modified = UI::Checkbox("##bool", &value))
					pin->Value = choc::value::createBool(value);
				break;
			}
			case GraphEditor::PinType::Int:
			{
				int value = pin->Value.isVoid() ? 0 : pin->Value.get<int>();

				char buffer[64];
				ImFormatString(buffer, std::size(buffer), "%d", value);
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(buffer).x + ImGui::GetStyle().FramePadding.x * 2.0f);

				if (modified = UI::DragInt32("##int", &value, 0.05f))
					pin->Value = choc::value::createInt32(value);
				break;
			}
			case GraphEditor::PinType::Float:
			{
				float value = pin->Value.isVoid() ? 0.0f : pin->Value.get<float>();

				char buffer[64];
				ImFormatString(buffer, std::size(buffer), "%.3f", value);
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(buffer).x + ImGui::GetStyle().FramePadding.x * 2.0f);

				if (modified = UI::DragFloat("##float", &value, 0.05f))
					pin->Value = choc::value::createFloat32(value);
				break;
			}
			default:
				SK_CORE_ASSERT(false, "Unknown pin type");
				break;
		}

		return modified;
	}

	void NodeGraphEditor::ChangeInputType(Input& input, GraphEditor::PinType type)
	{
		if (input.Type == type)
			return;

		for (auto& node : m_Nodes)
		{
			if (node.Name != "Input" && node.Outputs[0].Name != input.Name)
				continue;

			for (auto& output : node.Outputs)
			{
				RemoveLinks(output.ID);

				output.Type = type;
				output.Value = choc::value::Value();
			}

		}

		input.Type = type;
		input.Value = choc::value::Value();
	}

	void NodeGraphEditor::RenameInput(Input& input, const std::string& newName)
	{
		for (auto& node : m_Nodes)
		{
			if (node.Name != "Input" && node.Outputs[0].Name != input.Name)
				continue;

			node.Outputs[0].Name = newName;
		}
	}

}
