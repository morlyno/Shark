#include "NodeGraphEditor.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/Widgets.h"

#include "NodeGraph/ProcessNode.h"
#include "NodeGraph/NodeContext.h"
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
			case Editor::CoreTypes::EPinType::Flow:     iconType = ax::Drawing::IconType::Flow;   break;
			case Editor::CoreTypes::EPinType::Bool:     iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::Int:      iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::Float:    iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::Vec3:     iconType = ax::Drawing::IconType::Circle; break;
			case Editor::CoreTypes::EPinType::EntityID:	iconType = ax::Drawing::IconType::Circle; break;
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
		if (name == "EntityID") return AsType<Types::EntityID>();

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

	template<typename TProcNode>
	static std::pair<std::string, AbstractFactory::Entry> CreateRegistryEntry(const NodeSettings& settings = {})
	{
		return {
			choc::text::replace(NodeType<TProcNode>::Inputs::Class, "<", " (", ">", ")"),
			{
				std::bind_front(CoreTypes::SpawnNode<TProcNode>, settings),
				[](UUID id) -> ProcessNode* { return new TProcNode(id); }
			}
		};
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

		m_Context = ax::NodeEditor::CreateEditor(&config);
		m_PropertiesWindowID = fmt::format("Properties##{}", m_PanelName);

#if 1
#define FACTORY_NODE(_procNode, ...) CreateRegistryEntry<_procNode>(__VA_ARGS__)

		const NodeSettings Simple       = { .IsSimple = true };
		const NodeSettings SimpleNoEdit = { .IsSimple = true, .CanEditPins = false };
		const NodeSettings NoEdit       = { .CanEditPins = false };

		m_Factory = Scope<Editor::CoreFactory>::Create(
			Editor::AbstractFactory::Registry{
				{
					"Math",
					{
						FACTORY_NODE(Nodes::Add<int>, Simple),
						FACTORY_NODE(Nodes::Add<float>, Simple),
						FACTORY_NODE(Nodes::Multiply<int>, Simple),
						FACTORY_NODE(Nodes::Multiply<float>, Simple),
						FACTORY_NODE(Nodes::Get, SimpleNoEdit),
						FACTORY_NODE(Nodes::Random<int>),
						FACTORY_NODE(Nodes::Random<float>),
					}
				},
				{
					"Trigger",
					{
						FACTORY_NODE(Nodes::BoolTrigger),
					}
				},
				{
					"Scene",
					{
						FACTORY_NODE(Nodes::EntityTransform)
					}
				},
				{
					"Debug",
					{
						FACTORY_NODE(Nodes::Test)
					}
				}
			}
		);

#undef FACTORY_NODE
#endif

		m_InputTypeNames = { "Bool", "Int", "Float", "EntityID" };

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
				if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				builder.Input(input.ID);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
				ImGui::Spring(0);

				if (!node.Settings.IsSimple && !input.Name.empty())
				{
					ImGui::TextUnformatted(input.Name.c_str());
					ImGui::Spring(0);
				}

				if (node.Settings.DrawPinEdit(input.Identifier) && !IsPinLinked(input.ID))
				{
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
						else if (endPin->PinType != startPin->PinType)
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
									m_Links.emplace_back(Editor::Link(GetNextID(), startPinId, endPinId));
									m_Links.back().Color = startPin->Color;
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

		if (m_EntityPinPopup.Open)
		{
			ImGui::OpenPopupEx(m_EntityPinPopup.PopupID);
			m_EntityPinPopup.Open = false;
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

			Editor::Node* node = nullptr;

			const auto& registry = m_Factory->GetRegistry();
			for (const auto& [categoryName, category] : registry)
			{
				if (!ImGui::BeginMenu(categoryName.c_str()))
					continue;

				for (const auto& [type, _] : category)
				{
					if (ImGui::MenuItem(type.c_str()))
					{
						node = &m_Nodes.emplace_back();
						m_Factory->SpawnNode(categoryName, type, *node);
					}
				}

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

							m_Links.emplace_back(Editor::Link(GetNextID(), startPin->ID, endPin->ID));
							m_Links.back().Color = startPin->Color;
							break;
						}
					}
				}
			}

			ImGui::EndPopup();
		}
		else
			createNewNode = false;

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
					pin->Value = choc::value::Value(AsType<Types::EntityID>());

				pin->Value["ID"].set(static_cast<int64_t>(value.Value()));
			}

			if (!ImGui::IsPopupOpen(m_EntityPinPopup.PopupID, 0))
				m_EntityPinPopup.Set(nullptr);

		}

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
				m_InputVariables.emplace_back("New Input", choc::value::createFloat32(0.0f));
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

				auto typeName = TypeToString(input.Value.getType());
				if (UI::Control("Type", typeName, m_InputTypeNames))
				{
					ChangeInputType(input, StringToType(typeName));
				}

				const auto drawControl = []<typename T>(choc::value::Value& value)
				{
					T v = value.get<T>();
					if (UI::Control("Value", v))
						value.getViewReference().set(v);
				};

				if (input.Value.isPrimitive())
				{
					ControlValue("Value", input.Value);
				}
				else if (input.Value.isObjectWithClassName("EntityID"))
				{
					UUID v = UUID::Make(input.Value["ID"].getInt64());

					if (UI::ControlEntity("Value", m_Scene, v))
						input.Value["ID"].set(static_cast<int64_t>(v.Value()));
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

	Scope<Graph> NodeGraphEditor::CompileGraph()
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

		std::unordered_map<Editor::Pin*, choc::value::ValueView> inputVariablePins;

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

			ProcessNode* process = m_Factory->AllocateProcess(editorNode.Category, editorNode.Name, editorNode.GetID());
			SK_CORE_VERIFY(process);

			const size_t nodeIndex = index++;
			//nodeGraph->Nodes.push_back(new Nodes::Add(editorNode.GetID()));
			nodeGraph->Nodes.push_back(process);

			for (auto& input : editorNode.Inputs)
				if (!IsPinLinked(input.ID))
					loosePins.push_back({ nodeIndex, &input, input.Identifier });
			for (auto& output : editorNode.Outputs)
				if (!IsPinLinked(output.ID))
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

		NodeContextSpecification spec;
		spec.ActiveScene = m_Scene;
		NodeContext context(spec);

		for (auto& node : nodeGraph->Nodes)
			node->Initialize(&context);

		for (auto& editorNode : m_Nodes)
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

	Node* NodeGraphEditor::FindNode(ax::NodeEditor::NodeId id)
	{
		for (auto& node : m_Nodes)
			if (node.ID == id)
				return &node;

		return nullptr;
	}

	Link* NodeGraphEditor::FindLink(ax::NodeEditor::LinkId id)
	{
		for (auto& link : m_Links)
			if (link.ID == id)
				return &link;

		return nullptr;
	}

	Link* NodeGraphEditor::FindLink(ax::NodeEditor::PinId id)
	{
		for (auto& link : m_Links)
			if (link.StartPinID == id || link.EndPinID == id)
				return &link;

		return nullptr;
	}

	Pin* NodeGraphEditor::FindPin(ax::NodeEditor::PinId id)
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

	bool NodeGraphEditor::CanCreateLink(Editor::Pin* pinA, Editor::Pin* pinB)
	{
		if (!pinA || !pinB || pinA == pinB || pinA->Kind == pinB->Kind || pinA->PinType != pinB->PinType || pinA->NodeID == pinB->NodeID)
			return false;

		return true;
	}

	bool NodeGraphEditor::WouldCreateLoop(Editor::Pin* startPin, Editor::Pin* endPin)
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

	Editor::Node* NodeGraphEditor::SpawnInputNode(std::string_view inputName)
	{
		const auto input = std::ranges::find(m_InputVariables, inputName, &Input::Name);
		if (input == m_InputVariables.end())
			return nullptr;

		auto& node = m_Nodes.emplace_back();
		node.ID = GetNextID();
		node.Name = "Input";

		auto& pin = node.Outputs.emplace_back();
		m_Factory->InitializePin(pin, input->Value.getType());
		pin.ID = GetNextID();
		pin.NodeID = node.ID;
		pin.Kind = ax::NodeEditor::PinKind::Output;
		pin.Name = std::string(inputName);

		return &node;
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
					pin->Value = AsValue(value);
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
			default:
				SK_CORE_ASSERT(false, "Unknown pin type");
				break;
		}

		return modified;
	}

	void NodeGraphEditor::ChangeInputType(Input& input, const choc::value::Type& newType)
	{
		auto& currentType = input.Value.getType();
		if (currentType == newType)
			return;

		for (auto& node : m_Nodes)
		{
			if (node.Name != "Input" || node.Outputs[0].Name != input.Name)
				continue;

			for (auto& output : node.Outputs)
			{
				RemoveLinks(output.ID);

				m_Factory->InitializePin(output, newType);
				output.Value = choc::value::Value(newType);
			}

		}

		input.Value = choc::value::Value(newType);
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

	void NodeGraphEditor::SetContext(Ref<Scene> context)
	{
		m_Scene = context;
	}

}
