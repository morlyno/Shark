#include "NodeGraphEditor.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/Widgets.h"

#include "NodeGraph/ProcessNode.h"
#include "NodeGraph/NodeContext.h"
#include "NodeGraph/NodeGraphContext.h"
#include "NodeGraph/Nodes/MathNodes.h"
#include "NodeGraph/Nodes/EntityNodes.h"
#include "NodeGraph/Nodes/TriggerNodes.h"
#include "NodeGraph/Nodes/DebugNodes.h"

#include "NodeGraph/Factory.h"
#include "NodeGraph/CoreTypes.h"
#include "NodeGraph/EditorNodes.h"

#include "NodeGraph/Utilities/builders.h"
#include "NodeGraph/Utilities/drawing.h"
#include "NodeGraph/Utilities/widgets.h"
#include <functional>

namespace Shark::NodeGraph::Editor {

#if 0
	static ImColor GetIconColor(Editor::PinType type)
	{
		switch (type)
		{
			default:
			case Editor::PinType::Flow:     return ImColor(255, 255, 255);
			case Editor::PinType::Bool:     return ImColor(220, 48, 48);
			case Editor::PinType::Int:      return ImColor(68, 201, 156);
			case Editor::PinType::Float:    return ImColor(147, 226, 74);
			//case Editor::PinType::String:   return ImColor(124, 21, 153);
			//case Editor::PinType::Object:   return ImColor(51, 150, 215);
			//case Editor::PinType::Function: return ImColor(218, 0, 183);
			//case Editor::PinType::Delegate: return ImColor(255, 48, 48);
		}
	};
#endif

	static void DrawPinIcon(const Editor::Pin& pin, bool connected, int alpha)
	{
		ax::Drawing::IconType iconType;
		ImColor color = pin.Color;
		color.Value.w = alpha / 255.0f;
		switch (pin.PinType)
		{
			case Editor::CoreTypes::EPinType::Flow:        iconType = ax::Drawing::IconType::Flow;   break;
			case Editor::CoreTypes::EPinType::Bool:        iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::Int:         iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::Float:       iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::Vec3:        iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::EntityID:	   iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::AssetHandle: iconType = ax::Drawing::IconType::Circle; break;
			//case Editor::PinType::String:   iconType = ax::Drawing::IconType::Circle; break;
			//case Editor::PinType::Object:   iconType = ax::Drawing::IconType::Circle; break;
			//case Editor::PinType::Function: iconType = ax::Drawing::IconType::Circle; break;
			//case Editor::PinType::Delegate: iconType = ax::Drawing::IconType::Square; break;
			default:
				return;
		}

		ax::Widgets::Icon(ImVec2(24.0f, 24.0f), iconType, connected, color, ImColor(32, 32, 32, alpha));
	};

	static const char* GetSimpleNodeIcon(const Node& node)
	{
		if (choc::text::contains(node.Name, "Add")) return "+";
		if (choc::text::contains(node.Name, "Multiply")) return "*";
		if (choc::text::contains(node.Name, "Get")) return "get";
		return node.Name.c_str();
	}

	static std::string_view TypeToString(const choc::value::Type& type)
	{
		if (type.isObjectWithClassName("EntityID")) return "EntityID";

		if (type.isBool())    return "Bool";
		if (type.isInt32())   return "Int";
		if (type.isFloat32()) return "Float";
		
		SK_CORE_ASSERT(false, "Unknown Type");
		return "<unknown>";
	}

	static choc::value::Type StringToType(std::string_view name)
	{
		if (name == "EntityID") return CreateTypeEntityID();

		if (name == "Bool")   return choc::value::Type::createBool();
		if (name == "Int")   return choc::value::Type::createInt32();
		if (name == "Float") return choc::value::Type::createFloat32();

		SK_CORE_ASSERT(false, "Unknown Type '{}'", name);
		return choc::value::Type::createVoid();
	}

	static bool ControlValue(std::string_view label, choc::value::ValueView value)
	{
		if (value.isBool())
			return UI::Control(label, *static_cast<bool*>(value.getRawData()));
		if (value.isInt32())
			return UI::Control(label, *static_cast<int*>(value.getRawData()));
		if (value.isFloat32())
			return UI::Control(label, *static_cast<float*>(value.getRawData()));

		UI::Control(label, "Unknown value type");
		return false;
	}

	NodeGraphEditor::NodeGraphEditor()
	{
	}

	NodeGraphEditor::~NodeGraphEditor()
	{
	}

	bool NodeGraphEditor::OnShowPanel()
	{
		ax::NodeEditor::Config config;
		config.UserPointer = this;

		m_EditorContext = ax::NodeEditor::CreateEditor(&config);
		m_PropertiesWindowID = fmt::format("Properties##{}", m_PanelName);
		OnInitialize();
		return true;
	}

	bool NodeGraphEditor::OnHidePanel()
	{
		OnShutdown();
		m_NodeGraph = nullptr;
		m_Context = nullptr;
		ax::NodeEditor::DestroyEditor(m_EditorContext);
		m_EditorContext = nullptr;
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
				OnCompileGraph();
			}

			if (ImGui::Button("Run") && m_NodeGraph)
			{
				for (auto* node : m_NodeGraph->Nodes)
				{
					node->Process(TimeStep::FromMilliSeconds(5));
				}

				SK_CORE_DEBUG("NodeGraph outputs:");
				for (size_t i = 0; i < m_NodeGraph->OutputVariables.size(); i++)
				{
					auto& output = m_NodeGraph->OutputVariables[i];
					std::string_view name = "<none>";

					if (i < m_NodeGraph->DebugOutputNames.size())
						name = m_NodeGraph->DebugOutputNames[i];

					if (output.isBool())
						SK_CORE_DEBUG(" - {} {}", name, output.getBool());
					else if (output.isInt32())
						SK_CORE_DEBUG(" - {} {}", name, output.getInt32());
					else if (output.isFloat32())
						SK_CORE_DEBUG(" - {} {}", name, output.getFloat32());
					else
						SK_CORE_DEBUG(" - {} Unknown type", name);
				}
			}

			ImGui::EndHorizontal();

			DrawCanvas();

		}
		ImGui::End();

		DrawGraphIO();
		DrawProperty();
	}

	void NodeGraphEditor::DrawCanvas()
	{
		namespace ne = ax::NodeEditor;

		ne::SetCurrentEditor(m_EditorContext);
		ne::Begin("Node Graph");

		ne::Utilities::BlueprintNodeBuilder builder;

		for (auto& node : m_Context->GetNodes())
		{
			builder.Begin(node.ID);

			if (!node.Settings.IsSimple)
			{
				builder.Header(node.Color);
				ImGui::Spring(0);
				ImGui::TextUnformatted(node.Name.c_str());
				ImGui::Spring(1);
				ImGui::Text("%d", node.EvaluationIndex);
				ImGui::Spring(0);
				ImGui::Dummy(ImVec2(0, 28));
				ImGui::Spring(0);
				builder.EndHeader();
			}

			for (auto& input : node.Inputs)
			{
				auto alpha = ImGui::GetStyle().Alpha;
				if (m_CurrentState.NewLinkPin && !m_Context->CanCreateLink(m_CurrentState.NewLinkPin, &input) && &input != m_CurrentState.NewLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				builder.Input(input.ID);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				DrawPinIcon(input, m_Context->IsPinLinked(&input), (int)(alpha * 255));
				ImGui::Spring(0);

				if (!node.Settings.IsSimple && !input.Name.empty())
				{
					ImGui::TextUnformatted(input.Name.c_str());
					ImGui::Spring(0);
				}

				if (node.Settings.DrawPinEdit(input.Identifier) && !m_Context->IsPinLinked(&input))
				{
					ImGui::Spring(1);
					DrawPinValueEdit(&input);
					ImGui::Spring(0);
				}

				ImGui::PopStyleVar();
				builder.EndInput();
			}

			if (node.Settings.IsSimple)
			{
				builder.Middle();

				ImGui::Spring(1, 0);
				ImGui::TextUnformatted(GetSimpleNodeIcon(node));
				ImGui::Spring(1, 0);
			}

			for (auto& output : node.Outputs)
			{
				auto alpha = ImGui::GetStyle().Alpha;
				if (m_CurrentState.NewLinkPin && !m_Context->CanCreateLink(m_CurrentState.NewLinkPin, &output) && &output != m_CurrentState.NewLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				builder.Output(output.ID);
				if (!output.Name.empty())
				{
					ImGui::Spring(0);
					ImGui::TextUnformatted(output.Name.c_str());
				}
				ImGui::Spring(1);
				DrawPinIcon(output, m_Context->IsPinLinked(&output), (int)(alpha * 255));
				ImGui::PopStyleVar();
				builder.EndOutput();
			}

			builder.End();
		}

		for (auto& link : m_Context->GetLinks())
			ne::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

		if (!m_CurrentState.CreateNewNode)
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
					auto startPin = m_Context->FindPin(startPinId);
					auto endPin = m_Context->FindPin(endPinId);

					m_CurrentState.NewLinkPin = startPin ? startPin : endPin;

					QueryLinkResult linkStats = m_Context->QueryCanLinkStatus(startPin, endPin);
					switch (linkStats)
					{
						case QueryLinkResult::Ok:
						{
							showLabel("+ Create Link", ImColor(32, 45, 32, 180));
							if (ne::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
							{
								m_Context->CreateLink(startPin, endPin);
							}
							break;
						}

						case QueryLinkResult::SamePin:
						case QueryLinkResult::SameNode:
						{
							showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
							ne::RejectNewItem(ImColor(255, 0, 0), 1.0f);
							break;
						}
						case QueryLinkResult::IncompadiblePinType:
						{
							showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
							ne::RejectNewItem(ImColor(255, 128, 128), 1.0f);
							break;
						}
						case QueryLinkResult::IncompadiblePinKind:
						{
							showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
							ne::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							break;
						}

						case QueryLinkResult::CausesLoop:
						{
							showLabel("x Connection causes loop", ImColor(46, 32, 32, 180));
							ne::RejectNewItem(ImColor(255, 0, 0), 1.0f);
							break;
						}

						case QueryLinkResult::InvalidEndPin:
							break;
					}
				}


				ne::PinId pinId = 0;
				if (ne::QueryNewNode(&pinId))
				{
					m_CurrentState.NewLinkPin = m_Context->FindPin(pinId);
					if (m_CurrentState.NewLinkPin)
						showLabel("+ Create Node", ImColor(32, 45, 32, 180));

					if (ne::AcceptNewItem())
					{
						m_CurrentState.CreateNewNode = true;
						m_CurrentState.NewNodeLinkPinID = pinId;
						m_CurrentState.NewLinkPin = nullptr;
						ne::Suspend();
						ImGui::OpenPopup("Create New Node");
						ne::Resume();
					}
				}
			}
			else
			{
				m_CurrentState.NewLinkPin = nullptr;
			}

			ne::EndCreate();


			if (ne::BeginDelete())
			{
				ne::NodeId nodeId = 0;
				while (ne::QueryDeletedNode(&nodeId))
					if (ne::AcceptDeletedItem())
						m_Context->RemoveNode(nodeId);

				ne::LinkId linkId = 0;
				while (ne::QueryDeletedLink(&linkId))
					if (ne::AcceptDeletedItem())
						m_Context->RemoveLink(linkId);
			}
			ne::EndDelete();
		}


		auto openPopupPosition = ImGui::GetMousePos();
		ne::Suspend();
		if (ne::ShowNodeContextMenu(&m_CurrentState.ContextNodeId))
			ImGui::OpenPopup("Node Context Menu");
		else if (ne::ShowPinContextMenu(&m_CurrentState.ContextPinId))
			ImGui::OpenPopup("Pin Context Menu");
		else if (ne::ShowLinkContextMenu(&m_CurrentState.ContextLinkId))
			ImGui::OpenPopup("Link Context Menu");
		else if (ne::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
			m_CurrentState.NewNodeLinkPinID = 0;
		}

		if (m_EntityPinPopup.OpenPopup)
		{
			ImGui::OpenPopupEx(m_EntityPinPopup.PopupID);
			m_EntityPinPopup.OpenPopup = false;
		}

		if (m_AssetPinPopup.OpenPopup)
		{
			ImGui::OpenPopupEx(m_AssetPinPopup.PopupID);
			m_AssetPinPopup.OpenPopup = false;
		}

		ne::Resume();

		ne::Suspend();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		if (ImGui::BeginPopup("Node Context Menu"))
		{
			auto node = m_Context->FindNode(m_CurrentState.ContextNodeId);

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
				ImGui::Text("Unknown node: %p", m_CurrentState.ContextLinkId.AsPointer());
			ImGui::Separator();
			if (ImGui::MenuItem("Delete"))
				ne::DeleteNode(m_CurrentState.ContextNodeId);
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Pin Context Menu"))
		{
			auto pin = m_Context->FindPin(m_CurrentState.ContextPinId);

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
				ImGui::Text("Unknown pin: %p", m_CurrentState.ContextPinId.AsPointer());

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Link Context Menu"))
		{
			auto link = m_Context->FindLink(m_CurrentState.ContextLinkId);

			ImGui::TextUnformatted("Link Context Menu");
			ImGui::Separator();
			if (link)
			{
				ImGui::Text("ID: %p", link->ID.AsPointer());
				ImGui::Text("From: %p", link->StartPinID.AsPointer());
				ImGui::Text("To: %p", link->EndPinID.AsPointer());
			}
			else
				ImGui::Text("Unknown link: %p", m_CurrentState.ContextLinkId.AsPointer());
			ImGui::Separator();
			if (ImGui::MenuItem("Delete"))
				ne::DeleteLink(m_CurrentState.ContextLinkId);
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Create New Node"))
		{
			auto newNodePostion = openPopupPosition;
			//ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

			//auto drawList = ImGui::GetWindowDrawList();
			//drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

			Editor::Node* node = nullptr;

			const auto& registry = m_Context->GetFactory()->GetRegistry();
			for (const auto& [categoryName, category] : registry)
			{
				if (!ImGui::BeginMenu(categoryName.c_str()))
					continue;

				for (const auto& [type, _] : category)
				{
					if (ImGui::MenuItem(type.c_str()))
						node = m_Context->CreateNode(categoryName, type);
				}

				ImGui::Dummy({ 100.0f, 0.0f });
				ImGui::EndMenu();
			}


			if (node)
			{
				m_CurrentState.CreateNewNode = false;
				ne::SetNodePosition(node->ID, newNodePostion);

				if (auto startPin = m_Context->FindPin(m_CurrentState.NewNodeLinkPinID))
				{
					auto& pins = startPin->Kind == ne::PinKind::Input ? node->Outputs : node->Inputs;

					for (auto& pin : pins)
					{
						if (!m_Context->CanCreateLink(startPin, &pin))
							continue;

						m_Context->CreateLink(startPin, &pin);
						break;
					}
				}
			}

			ImGui::EndPopup();
		}
		else
			m_CurrentState.CreateNewNode = false;

		if (m_EntityPinPopup.EntityPin)
		{
			Pin* pin = m_EntityPinPopup.EntityPin;

			UUID value;
			bool isEntityObject = false;
			if (isEntityObject = pin->Value.isObjectWithClassName("EntityID"))
				value = UUID::Make(pin->Value["ID"].getInt64());

			if (UI::Widgets::SearchEntityPopup(m_Scene, value, m_EntityPinPopup.PopupID))
			{
				if (!isEntityObject)
					pin->Value = CreateEntityID();

				pin->Value["ID"].set(static_cast<int64_t>(value.Value()));
			}

			if (!ImGui::IsPopupOpen(m_EntityPinPopup.PopupID, 0))
				m_EntityPinPopup.Set(nullptr);

		}

		if (m_AssetPinPopup.AssetPin)
		{
			Pin* pin = m_AssetPinPopup.AssetPin;

			AssetHandle value;
			bool isAssetObject = false;
			if (isAssetObject = pin->Value.isObjectWithClassName("AssetHandle"))
				value = AssetHandle::Make(pin->Value["Handle"].getInt64());

			AssetType assetType = m_Context->GetPinAssetType(pin);
			if (UI::Widgets::SearchAssetPopup(assetType, value, m_AssetPinPopup.PopupID))
			{
				if (!isAssetObject)
					pin->Value = CreateAssetHandle();

				pin->Value["Handle"].set(static_cast<int64_t>(value.Value()));
			}

			if (!ImGui::IsPopupOpen(m_AssetPinPopup.PopupID, 0))
				m_AssetPinPopup.Set(nullptr);
		}

		ImGui::PopStyleVar();
		ne::Resume();

		ne::End();

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("input_variable"))
			{
				auto* node = m_Context->CreateInputNode(std::string_view(static_cast<const char*>(payload->Data), payload->DataSize));
				ne::SetNodePosition(node->ID, ne::ScreenToCanvas(ImGui::GetMousePos()));
			}

			ImGui::EndDragDropTarget();
		}

	}

	void NodeGraphEditor::DrawGraphIO()
	{
		ImGui::SetNextWindowClass(&m_WindowClass);
		if (ImGui::Begin("Variables"))
		{
			OnDrawGraphIO();

			if (ImGui::Button("Add Input"))
			{
				m_Context->AddInput(choc::value::createFloat32(0.0f));
			}

			auto& inputProperties = m_Context->GetInputs();
			for (auto& name : inputProperties.GetNames())
			{
				if (ImGui::Selectable(name.c_str(), m_SelectedInput.Name == name))
					SelectInput(name);

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("input_variable", name.c_str(), name.size());
					ImGui::Text(name);
					ImGui::EndDragDropSource();
				}
			}
		}
		ImGui::End();
	}

	void NodeGraphEditor::DrawProperty()
	{
		ImGui::SetNextWindowClass(&m_WindowClass);
		if (ImGui::Begin(m_PropertiesWindowID.c_str()))
		{
			auto& inputProperties = m_Context->GetInputs();

			if (inputProperties.HasValue(m_SelectedInput.Name))
			{
				UI::BeginControlsGrid();
				UI::Control("Name", m_SelectedInput.RenameBuffer);

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					if (m_Context->RenameInput(m_SelectedInput.Name, m_SelectedInput.RenameBuffer))
						m_SelectedInput.Name = m_SelectedInput.RenameBuffer;
					else
						m_SelectedInput.RenameBuffer = m_SelectedInput.Name;
				}

				auto value = inputProperties.GetValue(m_SelectedInput.Name);

				auto typeName = TypeToString(value.getType());
				if (UI::Control("Type", typeName, m_Context->GetInputTypes()))
				{
					m_Context->ChangeInputType(m_SelectedInput.Name,
											   StringToType(typeName));

					value = inputProperties.GetValue(m_SelectedInput.Name);
				}

				if (value.isPrimitive())
				{
					ControlValue("Value", value);
				}
				else if (value.isObjectWithClassName("EntityID"))
				{
					UUID v = UUID::Make(value["ID"].getInt64());

					if (UI::ControlEntity("Value", m_Scene, v))
						value["ID"].set(static_cast<int64_t>(v.Value()));
				}
				else
				{
					UI::Control("Value", []
					{
						UI::ScopedColor text(ImGuiCol_Text, UI::Colors::Theme::TextError);
						ImGui::Text("Unknown type");
					});
				}

				UI::EndControlsGrid();
			}
		}
		ImGui::End();
	}

	Scope<Graph> NodeGraphEditor::CompileGraph(NodeContext* context)
	{
		auto nodeGraph = Scope<Graph>::Create();

		struct LoosePin
		{
			size_t NodeIndex;
			Editor::Pin* GraphPin;
			Identifier Pin;
		};

		std::vector<LoosePin> loosePins;
		std::vector<LoosePin> looseOutputPins;
		std::unordered_map<UUID, std::set<UUID>> dependencies;

		std::unordered_map<const Pin*, choc::value::ValueView> inputVariablePins;
		std::unordered_map<const Pin*, choc::value::ValueView> outputVariablePins;

		auto& inputProperties = m_Context->GetInputs();

		for (size_t index = 0;
			auto& editorNode : m_Context->GetNodes())
		{
			if (editorNode.Name == "Input")
			{
				auto propertyName = editorNode.Outputs[0].Name;
				if (inputProperties.HasValue(propertyName))
					inputVariablePins.emplace(&editorNode.Outputs[0], inputProperties.GetValue(propertyName));
				continue;
			}

			if (editorNode.Name == "Output")
			{
				outputVariablePins.emplace(&editorNode.Inputs[0], choc::value::ValueView{});
				continue;
			}

			ProcessNode* process = m_Context->GetFactory()->AllocateProcess(editorNode.Category, editorNode.Name, editorNode.GetID(), context);
			SK_CORE_VERIFY(process);

			const size_t nodeIndex = index++;
			//nodeGraph->Nodes.push_back(new Nodes::Add(editorNode.GetID()));
			nodeGraph->Nodes.push_back(process);

			for (auto& input : editorNode.Inputs)
				if (!m_Context->IsPinLinked(&input))
					loosePins.push_back({ nodeIndex, &input, input.Identifier });
			for (auto& output : editorNode.Outputs)
				if (!m_Context->IsPinLinked(&output))
					looseOutputPins.push_back({ nodeIndex, &output, output.Identifier });
		}

		const auto FindNodeByID = [&nodes = nodeGraph->Nodes](UUID id) -> ProcessNode*
		{
			for (auto* node : nodes)
				if (node->ID == id)
					return node;
			return nullptr;
		};


		// connect nodes
		for (auto& link : m_Context->GetLinks())
		{
			auto* startPin = m_Context->FindPin(link.StartPinID);
			auto* endPin = m_Context->FindPin(link.EndPinID);
			SK_CORE_ASSERT(startPin && endPin);

			auto startNode = FindNodeByID(startPin->GetNodeID());
			auto endNode = FindNodeByID(endPin->GetNodeID());

			if (!startNode && inputVariablePins.contains(startPin))
			{
				choc::value::ValueView& input = endNode->GetInput(endPin->Identifier);
				input = inputVariablePins.at(startPin);
				continue;
			}

			if (!endNode && outputVariablePins.contains(endPin))
			{
				choc::value::ValueView& output = startNode->GetOutput(startPin->Identifier);
				outputVariablePins.at(endPin) = output;
				continue;
			}

			SK_CORE_ASSERT(startNode && endNode);

			if (endNode->IsInputEvent(endPin->Identifier))
			{
				ProcessNode::OutputEvent& output = startNode->GetOutputEvent(startPin->Identifier);
				ProcessNode::InputEvent& input = endNode->GetInputEvent(endPin->Identifier);

				output.AddTarget(input);
			}
			else
			{
				choc::value::ValueView& output = startNode->GetOutput(startPin->Identifier);
				choc::value::ValueView& input = endNode->GetInput(endPin->Identifier);

				input = output;
			}

			auto& inputNodes = dependencies[endPin->GetNodeID()];
			inputNodes.emplace(startPin->GetNodeID());
		}

		nodeGraph->LocalVariables.reserve(loosePins.size());

		for (auto& loosePin : loosePins)
		{
			auto* node = nodeGraph->Nodes[loosePin.NodeIndex];
			if (node->IsInputEvent(loosePin.Pin))
				continue;

			choc::value::ValueView& input = node->GetInput(loosePin.Pin);
			if (input.getRawData())
				continue;

			if (loosePin.GraphPin->Value.isVoid())
				loosePin.GraphPin->Value = choc::value::Value(input.getType());

			input = nodeGraph->LocalVariables.emplace_back(loosePin.GraphPin->Value);
		}

#if 1
		for (auto& loosePin : looseOutputPins)
		{
			auto* node = nodeGraph->Nodes[loosePin.NodeIndex];
			if (node->IsOutputEvent(loosePin.Pin))
				continue;

			nodeGraph->OutputVariables.push_back(node->GetOutput(loosePin.Pin));
			nodeGraph->DebugOutputNames.push_back(loosePin.GraphPin->Name);
		}
#endif

		// sort nodes
		// this assumes there are no loops in the graph

		const auto graphIsLess = [&dependencies](const ProcessNode* lhs, const ProcessNode* rhs)
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
			node->Initialize(context);

		for (auto& editorNode : m_Context->GetNodes())
		{
			auto id = editorNode.GetID();

			for (int i = 0; i < nodeGraph->Nodes.size(); i++)
			{
				if (nodeGraph->Nodes[i]->ID == id)
				{
					editorNode.EvaluationIndex = i;
					break;
				}
			}
		}

		return nodeGraph;
	}

	void NodeGraphEditor::SetupNodeContext(NodeContextSpecification& specification) const
	{
		specification.ActiveScene = m_Scene;
	}

	bool NodeGraphEditor::DrawPinValueEdit(Editor::Pin* pin)
	{
		bool modified = false;

		switch (pin->PinType)
		{
			case CoreTypes::EPinType::Flow:
				break;

			case CoreTypes::EPinType::Bool:
			{
				bool value = pin->Value.getWithDefault<bool>(false);

				if (modified = UI::Checkbox("##bool", &value))
					pin->Value = choc::value::createBool(value);
				break;
			}
			case Editor::CoreTypes::EPinType::Int:
			{
				int value = pin->Value.isVoid() ? 0 : pin->Value.get<int>();

				char buffer[64];
				ImFormatString(buffer, std::size(buffer), "%d", value);
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(buffer).x + ImGui::GetStyle().FramePadding.x * 2.0f);

				if (modified = UI::DragInt32("##int", &value, 0.05f))
					pin->Value = choc::value::createInt32(value);
				break;
			}
			case Editor::CoreTypes::EPinType::Float:
			{
				float value = pin->Value.isVoid() ? 0.0f : pin->Value.get<float>();

				char buffer[64];
				ImFormatString(buffer, std::size(buffer), "%.3f", value);
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(buffer).x + ImGui::GetStyle().FramePadding.x * 2.0f);

				if (modified = UI::DragFloat("##float", &value, 0.05f))
					pin->Value = choc::value::createFloat32(value);

				break;
			}
			case Editor::CoreTypes::EPinType::Vec3:
			{
				glm::vec3 value;
				if (pin->Value.isArray() && pin->Value.size() == 3)
				{
					value.x = pin->Value[0].getFloat32();
					value.y = pin->Value[1].getFloat32();
					value.t = pin->Value[2].getFloat32();
				}

				if (modified = UI::DragFloat3("##float3", glm::value_ptr(value)))
					pin->Value = CreateVec3(value);
				break;
			}
			case Editor::CoreTypes::EPinType::EntityID:
			{
				UUID value;

				if (pin->Value.isObjectWithClassName("EntityID"))
					value = UUID::Make(pin->Value["ID"].getInt64());

				const ImVec2 size = ImGui::CalcTextSize("0123456789ABCDEF") + ImGui::GetStyle().FramePadding * 2.0f;

				if (UI::Widgets::EntityButton(m_Scene, value, { .Size = size }))
					m_EntityPinPopup.Set(pin);

				break;
			}
			case Editor::CoreTypes::EPinType::AssetHandle:
			{
				AssetHandle value;

				if (pin->Value.isObjectWithClassName("AssetHandle"))
					value = AssetHandle::Make(pin->Value["Handle"].getInt64());

				const ImVec2 size = ImGui::CalcTextSize("0123456789ABCDEF") + ImGui::GetStyle().FramePadding * 2.0f;
				if (UI::Widgets::AssetButton(value, { .Size = size }))
					m_AssetPinPopup.Set(pin);

				break;
			}
			default:
				SK_CORE_ASSERT(false, "Unknown pin type");
				break;
		}

		return modified;
	}

	void NodeGraphEditor::SetContext(Ref<Scene> context)
	{
		m_Scene = context;
	}

	void NodeGraphEditor::SelectInput(std::string_view name)
	{
		m_SelectedInput.Name = name;
		m_SelectedInput.RenameBuffer.assign(name);
	}

	void NodeGraphEditor::SetGraphContext(Scope<NodeGraphContext> graphContext)
	{
		m_Context = std::move(graphContext);
	}

	NodeGraphContext* NodeGraphEditor::GetGraphContext()
	{
		return m_Context.Raw();
	}

	void NodeGraphEditor::OnCompileGraph()
	{
		NodeContextSpecification specification;
		SetupNodeContext(specification);

		NodeContext context(specification);
		m_NodeGraph = CompileGraph(&context);
	}

}
