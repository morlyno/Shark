#include "skpch.h"
#include "Controls.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/UI/Widgets.h"
#include "Shark/Utils/Utilities.h"

namespace Shark::UI {

#define IMPLEMENT_DRAG(_dataType)																			   \
	[&]()																									   \
	{																										   \
		const auto modified = ImGui::DragScalar(label, _dataType, &v, v_speed, &v_min, &v_max, format, flags); \
		Draw::ItemActivityOutline();																		   \
		return modified;																					   \
	}();

#define IMPLEMENT_SLIDER(_dataType)																		\
	[&]()																								\
	{																									\
		const auto modified = ImGui::SliderScalar(label, _dataType, &v, &v_min, &v_max, format, flags); \
		Draw::ItemActivityOutline();																	\
		return modified;																				\
	}();

	bool Drag(const char* label, float& v,    float v_speed, float v_min,    float v_max,    const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_Float);  }
	bool Drag(const char* label, double& v,   float v_speed, double v_min,   double v_max,   const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_Double); }
	bool Drag(const char* label, int8_t& v,   float v_speed, int8_t v_min,   int8_t v_max,   const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_S8);     }
	bool Drag(const char* label, int16_t& v,  float v_speed, int16_t v_min,  int16_t v_max,  const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_S16);    }
	bool Drag(const char* label, int32_t& v,  float v_speed, int32_t v_min,  int32_t v_max,  const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_S32);    }
	bool Drag(const char* label, int64_t& v,  float v_speed, int64_t v_min,  int64_t v_max,  const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_S64);    }
	bool Drag(const char* label, uint8_t& v,  float v_speed, uint8_t v_min,  uint8_t v_max,  const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_U8);     }
	bool Drag(const char* label, uint16_t& v, float v_speed, uint16_t v_min, uint16_t v_max, const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_U16);    }
	bool Drag(const char* label, uint32_t& v, float v_speed, uint32_t v_min, uint32_t v_max, const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_U32);    }
	bool Drag(const char* label, uint64_t& v, float v_speed, uint64_t v_min, uint64_t v_max, const char* format, ImGuiSliderFlags flags) { return IMPLEMENT_DRAG(ImGuiDataType_U64);    }

	bool Slider(const char* label, float& v,    float v_min,    float v_max,    const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_Float);  }
	bool Slider(const char* label, double& v,   double v_min,   double v_max,   const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_Double); }
	bool Slider(const char* label, int8_t& v,   int8_t v_min,   int8_t v_max,   const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_S8);     }
	bool Slider(const char* label, int16_t& v,  int16_t v_min,  int16_t v_max,  const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_S16);    }
	bool Slider(const char* label, int32_t& v,  int32_t v_min,  int32_t v_max,  const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_S32);    }
	bool Slider(const char* label, int64_t& v,  int64_t v_min,  int64_t v_max,  const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_S64);    }
	bool Slider(const char* label, uint8_t& v,  uint8_t v_min,  uint8_t v_max,  const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_U8);     }
	bool Slider(const char* label, uint16_t& v, uint16_t v_min, uint16_t v_max, const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_U16);    }
	bool Slider(const char* label, uint32_t& v, uint32_t v_min, uint32_t v_max, const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_U32);    }
	bool Slider(const char* label, uint64_t& v, uint64_t v_min, uint64_t v_max, const char* format, ImGuiSliderFlags flags)              { return IMPLEMENT_SLIDER(ImGuiDataType_U64);    }

#undef IMPLEMENT_DRAG
#undef IMPLEMENT_SLIDER

	bool Drag(const char* label, glm::vec2& v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const auto modified = ImGui::DragFloat2(label, glm::value_ptr(v), v_speed, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool Drag(const char* label, glm::vec3& v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const auto modified = ImGui::DragFloat3(label, glm::value_ptr(v), v_speed, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool Drag(const char* label, glm::vec4& v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const auto modified = ImGui::DragFloat3(label, glm::value_ptr(v), v_speed, v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool Slider(const char* label, glm::vec2& v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const auto modified = ImGui::SliderFloat2(label, glm::value_ptr(v), v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool Slider(const char* label, glm::vec3& v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const auto modified = ImGui::SliderFloat3(label, glm::value_ptr(v), v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	bool Slider(const char* label, glm::vec4& v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		const auto modified = ImGui::SliderFloat3(label, glm::value_ptr(v), v_min, v_max, format, flags);
		Draw::ItemActivityOutline();
		return modified;
	}

	namespace details {

		void GridSeparator()
		{
			ImGuiContext& g = *GImGui;
			ImGuiWindow* window = g.CurrentWindow;

			ImGuiTable* table = ImGui::GetCurrentTable();
			ImGuiTableColumn& collumn = table->Columns[table->CurrentColumn];
			float x1 = collumn.MinX;
			float x2 = collumn.MaxX;

			// FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
			if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
				x1 += window->DC.Indent.x;

			// We don't provide our width to the layout so that it doesn't get feed back into AutoFit
			float thickness_draw = 1.0f;
			float thickness_layout = 0.0f;

			const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
			window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), ImGui::GetColorU32(ImGuiCol_Separator));
		}

		bool BeginControl(ImGuiID id)
		{
			if (!ImGui::GetCurrentTable())
				return false;

			ImGui::PushID(id);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			if (ImGui::TableGetRowIndex() > 0)
			{
				GridSeparator();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
			}

			ImGui::AlignTextToFramePadding();

			return true;
		}

		void EndControl()
		{
			SK_CORE_ASSERT(ImGui::GetCurrentTable());
			ImGui::PopID();
		}

		static std::pair<std::string, bool> GetDisplayName(AssetHandle handle, const AssetControlArgs& args)
		{
			if (!handle)
				return { "", true };

			if (!AssetManager::IsValidAssetHandle(handle))
			{
				return { "Invalid", false };
			}

			auto assetManager = Project::GetEditorAssetManager();

			const bool isMemoryAsset = assetManager->IsMemoryAsset(handle);
			const bool valid = isMemoryAsset || assetManager->HasExistingFilePath(handle);

			if (!args.DisplayName.empty())
				return { std::string(args.DisplayName), valid };

			if (isMemoryAsset)
				return { fmt::format("{}", handle), valid };

			const auto& metadata = assetManager->GetMetadata(handle);
			return { metadata.FilePath.string(), valid };
		}

		void Label(std::string_view label)
		{
			auto cropped = label.substr(0, label.find("##"));
			ImGui::TextEx(label.data(), label.data() + label.length());
		}

		template<typename TSelected, typename TString>
		bool StringComboControl(std::string_view label, TSelected& selectedString, std::span<const TString> strings)
		{
			if (!details::BeginControl(ImGui::GetID(label)))
				return false;

			ImGui::Text(label);
			ImGui::TableNextColumn();

			std::string_view preview = selectedString;
			bool modified = false;

			ImGui::SetNextItemWidth(-1.0f);
			if (BeginCombo(GenerateID(), preview.data()))
			{
				for (size_t i = 0; i < strings.size(); i++)
				{
					const std::string_view name = strings[i];
					const bool isSelected = name == preview;

					if (ImGui::Selectable(name.data(), isSelected))
					{
						selectedString = strings[i];
						modified = true;
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				EndCombo();
			}
			details::EndControl();
			return modified;
		}

	}

	bool Control(std::string_view label, glm::vec3& value, as_color_t)
	{
		return Control(label, [&value]()
		{
			ImGui::SetNextItemWidth(-1.0f);
			return ColorEdit3(GenerateID(), glm::value_ptr(value));
		});
	}

	bool Control(std::string_view label, glm::vec4& value, as_color_t)
	{
		return Control(label, [&value]()
		{
			ImGui::SetNextItemWidth(-1.0f);
			return ColorEdit4(GenerateID(), glm::value_ptr(value));
		});
	}

	bool Control(std::string_view label, bool& value)
	{
		return Control(label, [&value]()
		{
			return Checkbox(GenerateID(), &value);
		});
	}

	bool Control(std::string_view label, const bool& value)
	{
		ScopedItemFlag readOnly(ImGuiItemFlags_ReadOnly);

		bool temp = value;
		return Control(label, temp);
	}

	bool Control(std::string_view label, bool& value, const char* vTrue, const char* vFalse)
	{
		return Control(label, [&]()
		{
			bool modified = false;
			const char* preview = value ? vTrue : vFalse;
			const bool isMixedValue = GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue;

			ImGui::SetNextItemWidth(-1.0f);
			if (UI::BeginCombo("##combo", isMixedValue ? nullptr : preview))
			{
				if (ImGui::Selectable(vFalse, value == false))
				{
					value = false;
					modified = true;
				}

				if (ImGui::Selectable(vTrue, value == true))
				{
					value = true;
					modified = true;
				}
				UI::EndCombo();
			}

			if (isMixedValue)
				UI::DrawTextAligned("--", ImVec2(0.5f, 0.5f), UI::GetItemRect());

			return modified;
		});
	}

	bool Control(std::string_view label, char* buffer, size_t bufferSize)
	{
		return Control(label, [&]()
		{
			ImGui::SetNextItemWidth(-1.0f);
			return InputText(GenerateID(), buffer, bufferSize);
		});
	}

	bool Control(std::string_view label, std::string& value)
	{
		return Control(label, [&value]()
		{
			ImGui::SetNextItemWidth(-1.0f);
			return InputText(GenerateID(), &value);
		});
	}

	bool Control(std::string_view label, std::string_view value)
	{
		Control(label, [&value]()
		{
			ImGui::SetNextItemWidth(-1.0f);
			// #Investigate should this be an input text
			InputText(GenerateID(), const_cast<char*>(value.data()), value.size(), ImGuiInputTextFlags_ReadOnly);
			return false;
		});
		return false;
	}

	bool Control(std::string_view label, std::string& selectedString, std::span<const std::string> strings)
	{
		return details::StringComboControl(label, selectedString, strings);
	}

	bool Control(std::string_view label, std::string& selectedString, std::span<const std::string_view> strings)
	{
		return details::StringComboControl(label, selectedString, strings);
	}

	bool Control(std::string_view label, std::string_view& selectedString, std::span<const std::string> strings)
	{
		return details::StringComboControl(label, selectedString, strings);
	}

	bool Control(std::string_view label, std::string_view& selectedString, std::span<const std::string_view> strings)
	{
		return details::StringComboControl(label, selectedString, strings);
	}

	bool ControlEntity(std::string_view label, Ref<Scene> scene, UUID& entityID, const EntityControlArgs& args)
	{
		return Control(label, [&scene, &entityID, &args]()
		{
			return Widgets::SelectEntity(scene, entityID, args);
		});
	}

	bool ControlAsset(std::string_view label, AssetType assetType, AssetHandle& assetHandle, const AssetControlArgs& args)
	{
		return ControlAsset(label, { { assetType } }, assetHandle, args);
	}

	bool ControlAsset(std::string_view label, std::span<const AssetType> assetTypes, AssetHandle& assetHandle, const AssetControlArgs& args)
	{
		return Control(label, [&assetTypes, &assetHandle, &args]()
		{
			return Widgets::SelectAsset(assetTypes, assetHandle, args);
		});
	}

	bool ControlScript(std::string_view label, uint64_t& scriptID, const ScriptControlArgs& args)
	{
		return Control(label, [&scriptID, &args]()
		{
			return Widgets::SelectScript(scriptID, args);
		});
	}

	bool ControlEntity(std::string_view label, Ref<Scene> scene, const UUID& entityID, const EntityControlArgs& args)
	{
		ScopedItemFlag readOnly(ImGuiItemFlags_ReadOnly);

		UUID id = entityID;
		return ControlEntity(label, scene, id, args);
	}

	bool ControlAsset(std::string_view label, AssetType assetType, const AssetHandle& assetHandle, const AssetControlArgs& args)
	{
		ScopedItemFlag readOnly(ImGuiItemFlags_ReadOnly);

		AssetHandle handle = assetHandle;
		return ControlAsset(label, assetType, handle, args);
	}

}
