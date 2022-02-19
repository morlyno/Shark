#include "skpch.h"
#include "UI.h"

#include "Shark/Utility/Math.h"
#include "Shark/Utility/String.h"

#include "Shark/Core/Application.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <misc/cpp/imgui_stdlib.h>

namespace ImGui {

	bool TableNextColumn(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f)
	{
		ImGuiContext& g = *GImGui;
		ImGuiTable* table = g.CurrentTable;
		if (!table)
			return false;

		if (table->IsInsideRow && table->CurrentColumn + 1 < table->ColumnsCount)
		{
			if (table->CurrentColumn != -1)
				TableEndCell(table);
			TableBeginCell(table, table->CurrentColumn + 1);
		}
		else
		{
			TableNextRow(row_flags, min_row_height);
			TableBeginCell(table, 0);
		}

		// Return whether the column is visible. User may choose to skip submitting items based on this return value,
		// however they shouldn't skip submitting for columns that may have the tallest contribution to row height.
		int column_n = table->CurrentColumn;
		return (table->RequestOutputMaskByIndex & ((ImU64)1 << column_n)) != 0;
	}

}

namespace Shark::UI {

	//////////////////////////////////////////////////////////////////////////////
	/// Global Data //////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	static std::vector<Flags::Text> s_TextFlagStack = std::vector<Flags::Text>(1, Flags::Text_None);
	static std::string s_StringBuffer;

	void NewFrame()
	{
		SK_CORE_ASSERT(s_TextFlagStack.size() == 1, "Push/Pop Text Flag missmatch");
		s_TextFlagStack.clear();
		s_TextFlagStack.push_back(Flags::Text_None);
	}


	//////////////////////////////////////////////////////////////////////////////
	/// Helpers //////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	ScopedID::~ScopedID()
	{
		PopID();
	}

	ScopedID::ScopedID(const std::string_view& strID)
	{
		PushID(GetID(strID));
	}

	ScopedID::ScopedID(const std::string& strID)
	{
		PushID(GetID(strID));
	}

	ScopedID::ScopedID(const char* strID)
	{
		PushID(GetID(strID));
	}

	ScopedID::ScopedID(void* ptrID)
	{
		PushID(GetID(ptrID));
	}

	ScopedID::ScopedID(int intID)
	{
		PushID(GetID(intID));
	}

	ScopedID::ScopedID(ImGuiID id)
	{
		PushID(id);
	}

	void ScopedStyle::Push(ImGuiCol idx, const ImVec4& col)
	{
		ImGui::PushStyleColor(idx, col); StyleColorCount++;
	}

	void ScopedStyle::Push(ImGuiStyleVar idx, const ImVec2& val)
	{
		ImGui::PushStyleVar(idx, val); StyleVarCount++;
	}

	void ScopedStyle::Push(ImGuiStyleVar idx, float val)
	{
		ImGui::PushStyleVar(idx, val); StyleVarCount++;
	}

	ScopedStyle::~ScopedStyle()
	{
		ImGui::PopStyleVar(StyleVarCount), ImGui::PopStyleColor(StyleColorCount);
	}

	ScopedStyle::ScopedStyle(ImGuiCol idx, const ImVec4& col)
	{
		Push(idx, col);
	}

	ScopedStyle::ScopedStyle(ImGuiStyleVar idx, const ImVec2& val)
	{
		Push(idx, val);
	}

	ScopedStyle::ScopedStyle(ImGuiStyleVar idx, float val)
	{
		Push(idx, val);
	}

	ImGuiID GetID(int intID)
	{
		return ImGui::GetCurrentWindowRead()->GetID(intID);
	}

	ImGuiID GetID(void* ptrID)
	{
		return ImGui::GetCurrentWindowRead()->GetID(ptrID);
	}

	ImGuiID GetID(const char* strID)
	{
		return ImGui::GetCurrentWindowRead()->GetID(strID);
	}

	ImGuiID GetID(const std::string& strID)
	{
		return ImGui::GetCurrentWindowRead()->GetID(strID.c_str());
	}

	ImGuiID GetID(const std::string_view& strID)
	{
		return ImGui::GetCurrentWindowRead()->GetID(strID.data());
	}

	ImGuiID GetIDWithSeed(int intID, uint32_t seed)
	{
		ImGuiID id = ImHashData(&intID, sizeof(int), seed);
		ImGui::KeepAliveID(id);
		return id;
	}

	ImGuiID GetIDWithSeed(void* ptrID, uint32_t seed)
	{
		ImGuiID id = ImHashData(&ptrID, sizeof(void*), seed);
		ImGui::KeepAliveID(id);
		return id;
	}

	ImGuiID GetIDWithSeed(const char* strID, uint32_t seed)
	{
		ImGuiID id = ImHashStr(strID, 0, seed);
		ImGui::KeepAliveID(id);
		return id;
	}

	ImGuiID GetIDWithSeed(const std::string& strID, uint32_t seed)
	{
		ImGuiID id = ImHashStr(strID.c_str(), strID.size(), seed);
		ImGui::KeepAliveID(id);
		return id;
	}

	void PushID(ImGuiID id)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->IDStack.push_back(id);
	}

	ImGuiID PopID()
	{
		const ImGuiID id = ImGui::GetCurrentWindowRead()->IDStack.back();
		ImGui::PopID();
		return id;
	}

	ImGuiID GetCurrentID()
	{
		return ImGui::GetCurrentWindowRead()->IDStack.back();
	}

	void SetBlend(bool blend)
	{
		ImGuiLayer& ctx = Application::Get().GetImGuiLayer();
		ctx.SubmitBlendCallback(blend);
	}

	void MoveCursor(const ImVec2& xy)
	{
		ImGui::SetCursorPos(ImGui::GetCursorPos() + xy);
	}

	void MoveCursorX(float x)
	{
		MoveCursor({ x, 0.0f });
	}

	void MoveCursorY(float y)
	{
		MoveCursor({ 0.0f, y });
	}

	void SpanAvailWith()
	{
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	}

	const std::string& FormatToString(const std::wstring& str)
	{
		String::ToNarrow(str, s_StringBuffer);
		return s_StringBuffer;
	}

	const std::string& FormatToString(const std::filesystem::path& str)
	{
		String::ToNarrow(str, s_StringBuffer);
		return s_StringBuffer;
	}

	const char* FormatToCString(const std::wstring& str)
	{
		String::ToNarrow(str, s_StringBuffer);
		return s_StringBuffer.c_str();
	}

	const char* FormatToCString(const std::filesystem::path& str)
	{
		String::ToNarrow(str, s_StringBuffer);
		return s_StringBuffer.c_str();
	}


	//////////////////////////////////////////////////////////////////////////////
	/// Text /////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	void PushTextFlag(Flags::Text flags)
	{
		s_TextFlagStack.push_back(flags);
	}

	void PopTextFlag(uint32_t count)
	{
		while (count-- > 0)
		{
			SK_CORE_ASSERT(s_TextFlagStack.size() > 1);
			s_TextFlagStack.pop_back();
		}
	}

	void Text(std::string_view str, Flags::Text flags)
	{
		flags |= s_TextFlagStack.back();

		ScopedStyle style;
		ScopedID id(str);

		if (flags & Flags::Text_Disabled)
			style.Push(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

		if (flags & Flags::Text_Selectable)
		{
			if (!(flags & Flags::Text_Background))
				style.Push(ImGuiCol_FrameBg, { 0.0, 0.0f, 0.0f, 0.0f });

			ImGui::InputText("##TextSelectable", (char*)str.data(), str.size(), ImGuiInputTextFlags_ReadOnly);
			return;
		}

		if (flags & Flags::Text_Aligned)
			ImGui::AlignTextToFramePadding();

		if (flags & Flags::Text_Background)
		{
			const ImGuiID id = GetID(str);
			ImGui::TreeNodeBehavior(id, ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_Leaf, str.data(), str.data() + str.size());
		}
		else
		{
			ImGui::TextUnformatted(str.data(), str.data() + str.size());
		}
	}

	void Text(const char* str, Flags::Text flags)
	{
		Text(std::string_view(str), flags);
	}

	void Text(const std::string& str, Flags::Text flags)
	{
		Text(std::string_view(str), flags);
	}

	void Text(const std::filesystem::path& path, Flags::Text flags)
	{
		String::ToNarrow(path.native(), s_StringBuffer);
		Text(s_StringBuffer, flags);
	}

	void Text(const char* str)
	{
		ImGui::Text(str);
	}

	void Text(const std::string& str)
	{
		ImGui::Text(str.c_str());
	}

	void Text(const std::filesystem::path& path)
	{
		String::ToNarrow(path.native(), s_StringBuffer);
		Text(s_StringBuffer);
	}

	void TextAligned(const char* str)
	{
		ImGui::AlignTextToFramePadding();
		Text(str);
	}

	void TextAligned(const std::string& str)
	{
		ImGui::AlignTextToFramePadding();
		Text(str);
	}

	void TextAligned(const std::filesystem::path& str)
	{
		ImGui::AlignTextToFramePadding();
		Text(str);
	}

	void TextWithBackGround(const std::string& text)
	{
		TextWithBackGround(text, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	}

	void TextWithBackGround(const std::string& text, const ImVec4& bgColor)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, bgColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, bgColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, bgColor);

		const ImGuiID id = GetID(text);
		ImGui::TreeNodeBehavior(id, ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_Leaf, text.c_str());

		ImGui::PopStyleColor(3);
	}

	void TextWithBackGround(const std::filesystem::path& text)
	{
		TextWithBackGround(text, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	}

	void TextWithBackGround(const std::filesystem::path& text, const ImVec4& bgColor)
	{
		TextWithBackGround(text.string(), bgColor);
	}

	//////////////////////////////////////////////////////////////////////////////
	/// Controls /////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	namespace Utils {

		static bool BeginProperty(ImGuiID id)
		{
			// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
			if (!ImGui::GetCurrentTable())
				return false;

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			PushID(id);

			return true;
		}

		static void EndProperty()
		{
			// Note(moro): if UI/Style stuff gets added to this function PropertyCustom
			//             will probably no loger work how it should!

			PopID();
		}

	}

	bool BeginProperty(Flags::Grid flags)
	{
		return BeginProperty(GetID("ControlsTable"), flags);
	}

	bool BeginProperty(const std::string& strID, Flags::Grid flags)
	{
		return BeginProperty(GetID(strID), flags);
	}

	bool BeginPropertyGrid(Flags::Grid flags)
	{
		return BeginProperty(GetID("ControlsTable"), flags | Flags::Property_GridDefualt);
	}

	bool BeginPropertyGrid(const std::string& strID, Flags::Grid flags)
	{
		return BeginProperty(GetID(strID), flags | Flags::Property_GridDefualt);
	}

	bool BeginProperty(ImGuiID customID, Flags::Grid flags)
	{
		PushID(customID);

		ImGuiTableFlags tableflags = ImGuiTableFlags_None;
		if (!(flags & Flags::Property_FixedSize)) tableflags |= ImGuiTableFlags_Resizable;
		if (flags & Flags::Property_MinWidth) tableflags |= ImGuiTableFlags_SizingFixedFit;
		if (flags & Flags::Property_GridInnerV) tableflags |= ImGuiTableFlags_BordersInnerV;
		if (flags & Flags::Property_GridInnerH) tableflags |= ImGuiTableFlags_BordersInnerH;
		if (flags & Flags::Property_GridOuterV) tableflags |= ImGuiTableFlags_BordersOuterV;
		if (flags & Flags::Property_GridOuterH) tableflags |= ImGuiTableFlags_BordersOuterH;

		if (ImGui::BeginTable("ControlsTable", 2, tableflags))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			return true;
		}
		return false;
	}

	void EndProperty()
	{
		if (ImGui::GetCurrentTable())
		{
			ImGui::PopStyleVar();
			ImGui::EndTable();
		}

		PopID();
	}

	bool PropertyCustom(const std::string& tag)
	{
		// ImGui's Table API currently crashes when BeginTable return false but Talbe functions get called
		if (!ImGui::GetCurrentTable())
			return false;

		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		return true;
	}

	bool DragFloat(const std::string& tag, float& val, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##X", &val, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool DragFloat(const std::string& tag, DirectX::XMFLOAT2& val, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 2.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool DragFloat(const std::string& tag, DirectX::XMFLOAT3& val, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 3.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Z", &val.z, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool DragFloat(const std::string& tag, DirectX::XMFLOAT4& val, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 4.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##Z", &val.z, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("W", { buttonSize, buttonSize }))
		{
			val.w = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragFloat("##W", &val.w, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}


	bool SliderFloat(const std::string& tag, float& val, float resetVal, float min, float max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##X", &val, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT2& val, float resetVal, float min, float max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 2.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Y", &val.y, min, max, fmt, flags);

		Utils::EndProperty();
		return changed;
	}

	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT3& val, float resetVal, float min, float max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 3.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Y", &val.y, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Z", &val.z, min, max, fmt, flags);

		Utils::EndProperty();
		return changed;
	}

	bool SliderFloat(const std::string& tag, DirectX::XMFLOAT4& val, float resetVal, float min, float max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 4.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Y", &val.y, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##Z", &val.z, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("W", { buttonSize, buttonSize }))
		{
			val.w = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderFloat("##W", &val.w, min, max, fmt, flags);

		Utils::EndProperty();
		return changed;
	}

	bool DragInt(const std::string& tag, int& val, int resetVal, int min, int max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##X", &val, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool DragInt(const std::string& tag, DirectX::XMINT2& val, int resetVal, int min, int max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 2.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool DragInt(const std::string& tag, DirectX::XMINT3& val, int resetVal, int min, int max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 3.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Z", &val.z, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool DragInt(const std::string& tag, DirectX::XMINT4& val, int resetVal, int min, int max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 4.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##X", &val.x, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Y", &val.y, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##Z", &val.z, speed, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("W", { buttonSize, buttonSize }))
		{
			val.w = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragInt("##W", &val.w, speed, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}


	bool SliderInt(const std::string& tag, int& val, int resetVal, int min, int max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##X", &val, min, max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool SliderInt(const std::string& tag, DirectX::XMINT2& val, int resetVal, int min, int max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 2.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Y", &val.y, min, max, fmt, flags);

		Utils::EndProperty();
		return changed;
	}

	bool SliderInt(const std::string& tag, DirectX::XMINT3& val, int resetVal, int min, int max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 3.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Y", &val.y, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Z", &val.z, min, max, fmt, flags);

		Utils::EndProperty();
		return changed;
	}

	bool SliderInt(const std::string& tag, DirectX::XMINT4& val, int resetVal, int min, int max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		constexpr float components = 4.0f;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (components - 1)) / components - buttonSize;

		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val.x = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##X", &val.x, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Y", { buttonSize, buttonSize }))
		{
			val.y = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Y", &val.y, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("Z", { buttonSize, buttonSize }))
		{
			val.z = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##Z", &val.z, min, max, fmt, flags);

		ImGui::SameLine();

		if (ImGui::Button("W", { buttonSize, buttonSize }))
		{
			val.w = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderInt("##W", &val.w, min, max, fmt, flags);

		Utils::EndProperty();
		return changed;
	}

	bool DragInt(const std::string& tag, uint32_t& val, uint32_t resetVal, uint32_t min, uint32_t max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);

		changed |= ImGui::DragScalar("##X", ImGuiDataType_U32, &val, speed, &min, &max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool SliderInt(const std::string& tag, uint32_t& val, uint32_t resetVal, uint32_t min, uint32_t max, const char* fmt, ImGuiSliderFlags flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = widthAvail - buttonSize;
		ImGui::PushItemWidth(width);

		bool changed = false;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val = resetVal;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderScalar(tag.c_str(), ImGuiDataType_U32, &val, &min, &max, fmt, flags);

		ImGui::PopItemWidth();

		Utils::EndProperty();
		return changed;
	}

	bool DragAngle(const std::string& tag, float& radians, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		auto degrees = Math::ToDegrees(radians);
		if (DragFloat(tag, degrees, resetVal, speed, min, max, fmt, flags))
		{
			radians = Math::ToRadians(degrees);
			return true;
		}
		return false;
	}

	bool DragAngle(const std::string& tag, DirectX::XMFLOAT2& radians, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		auto degrees = Math::ToDegrees(radians);
		if (DragFloat(tag, degrees, resetVal, speed, min, max, fmt, flags))
		{
			radians = Math::ToRadians(degrees);
			return true;
		}
		return false;
	}

	bool DragAngle(const std::string& tag, DirectX::XMFLOAT3& radians, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		auto degrees = Math::ToDegrees(radians);
		if (DragFloat(tag, degrees, resetVal, speed, min, max, fmt, flags))
		{
			radians = Math::ToRadians(degrees);
			return true;
		}
		return false;
	}

	bool DragAngle(const std::string& tag, DirectX::XMFLOAT4& radians, float resetVal, float min, float max, float speed, const char* fmt, ImGuiSliderFlags flags)
	{
		auto degrees = Math::ToDegrees(radians);
		if (DragFloat(tag, degrees, resetVal, speed, min, max, fmt, flags))
		{
			radians = Math::ToRadians(degrees);
			return true;
		}
		return false;
	}


	bool Checkbox(const std::string& tag, bool& v)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		const bool changed = ImGui::Checkbox("##X", &v);

		Utils::EndProperty();
		return changed;
	}

	bool Checkbox(const std::string& tag, const bool& v)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		bool tempVal = v;
		const bool changed = ImGui::Checkbox("##X", &tempVal);

		Utils::EndProperty();
		return changed;
	}

	bool ColorEdit(const std::string& tag, DirectX::XMFLOAT4& color, ImGuiColorEditFlags flags)
	{
		static_assert(sizeof(color) == (sizeof(float[4])));

		if (!Utils::BeginProperty(GetID(tag)))
			return false;

		ImGui::TableSetColumnIndex(0);
		TextAligned(tag);
		ImGui::TableSetColumnIndex(1);

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		const bool changed = ImGui::ColorEdit4("##ColorEditor4", (float*)&color, flags);

		Utils::EndProperty();
		return changed;

	}

	bool BeginCustomControl(const std::string& strID)
	{
		return BeginCustomControl(GetID(strID));
	}

	bool BeginCustomControl(ImGuiID id)
	{
		if (!Utils::BeginProperty(id))
			return false;

		return true;
	}

	void EndCustomControl()
	{
		Utils::EndProperty();
	}

	void Property(const std::string& tag, const char* val, Flags::Text flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return;

		ImGui::TableSetColumnIndex(0);
		UI::Text(tag, flags & Flags::Text_TagMask);

		ImGui::TableSetColumnIndex(1);
		UI::Text(val, flags);

		Utils::EndProperty();
	}

	void Property(const std::string& tag, const std::string& val, Flags::Text flags)
	{
		Property(tag, val.c_str(), flags);
	}

	void Property(const std::string& tag, const std::filesystem::path& path, Flags::Text flags)
	{
		if (!Utils::BeginProperty(GetID(tag)))
			return;

		ImGui::TableSetColumnIndex(0);
		UI::Text(tag, flags & Flags::Text_TagMask);

		ImGui::TableSetColumnIndex(1);
		UI::Text(path, flags);

		Utils::EndProperty();
	}

	//////////////////////////////////////////////////////////////////////////////
	/// Widgets //////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	static bool CustomTreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end, ImTextureID textureID)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
		const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

		if (!label_end)
			label_end = ImGui::FindRenderedTextEnd(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, label_end, false);

		// We vertically grow up to current line height up the typical widget height.
		const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
		ImRect frame_bb;
		frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
		frame_bb.Min.y = window->DC.CursorPos.y;
		frame_bb.Max.x = window->WorkRect.Max.x;
		frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
		if (display_frame)
		{
			// Framed header expand a little outside the default padding, to the edge of InnerClipRect
			// (FIXME: May remove this at some point and make InnerClipRect align with WindowPadding.x instead of WindowPadding.x*0.5f)
			frame_bb.Min.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
			frame_bb.Max.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
		}

		// Image
		const float image_padding = style.FramePadding.y;
		const float image_Size = frame_height - (image_padding * 2.0f);
		const float image_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);
		const float image_offset_y = image_padding;
		ImVec2 image_pos = { window->DC.CursorPos.x + image_offset_x, window->DC.CursorPos.y + image_offset_y };

		const float text_offset_x = (image_offset_x + image_Size) + (display_frame ? padding.x * 2 : padding.x * 1);   // Collapser arrow width + Spacing + Image
		const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);                               // Latch before ItemSize changes it
		const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);             // Include collapser
		ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
		ImGui::ItemSize(ImVec2(text_width, frame_height), padding.y);

		// For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
		ImRect interact_bb = frame_bb;
		if (!display_frame && (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth)) == 0)
			interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;

		// Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
		// For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
		// This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
		const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
		bool is_open = ImGui::TreeNodeBehaviorIsOpen(id, flags);
		if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
			window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);

		bool item_add = ImGui::ItemAdd(interact_bb, id);
		window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
		window->DC.LastItemDisplayRect = frame_bb;

		if (!item_add)
		{
			if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
				ImGui::TreePushOverrideID(id);
			IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.LastItemStatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
			return is_open;
		}

		ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
		if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
			button_flags |= ImGuiButtonFlags_AllowItemOverlap;
		if (!is_leaf)
			button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

		// We allow clicking on the arrow section with keyboard modifiers held, in order to easily
		// allow browsing a tree while preserving selection with code implementing multi-selection patterns.
		// When clicking on the rest of the tree node we always disallow keyboard modifiers.
		const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
		const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
		const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
		if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
			button_flags |= ImGuiButtonFlags_NoKeyModifiers;

		// Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
		// Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
		// - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
		// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
		// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
		// - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
		// - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
		// It is rather standard that arrow click react on Down rather than Up.
		// We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
		if (is_mouse_x_over_arrow)
			button_flags |= ImGuiButtonFlags_PressedOnClick;
		else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
			button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
		else
			button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

		bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
		const bool was_selected = selected;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
		bool toggled = false;
		if (!is_leaf)
		{
			if (pressed && g.DragDropHoldJustPressedId != id)
			{
				if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id))
					toggled = true;
				if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
					toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
				if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseDoubleClicked[0])
					toggled = true;
			}
			else if (pressed && g.DragDropHoldJustPressedId == id)
			{
				IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
				if (!is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
					toggled = true;
			}

			if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open)
			{
				toggled = true;
				ImGui::NavMoveRequestCancel();
			}
			if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
			{
				toggled = true;
				ImGui::NavMoveRequestCancel();
			}

			if (toggled)
			{
				is_open = !is_open;
				window->DC.StateStorage->SetInt(id, is_open);
				window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
			}
		}
		if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
			ImGui::SetItemAllowOverlap();

		// In this branch, TreeNodeBehavior() cannot toggle the selection so this will never trigger.
		if (selected != was_selected) //-V547
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

		// Render
		const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
		ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
		if (display_frame)
		{
			// Framed type
			const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
			ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);
			if (flags & ImGuiTreeNodeFlags_Bullet)
				ImGui::RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), text_col);
			else if (!is_leaf)
				ImGui::RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
			else // Leaf without bullet, left-adjusted text
				text_pos.x -= text_offset_x;
			if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
				frame_bb.Max.x -= g.FontSize + style.FramePadding.x;

			if (g.LogEnabled)
				ImGui::LogSetNextTextDecoration("###", "###");

			// Render Image
			window->DrawList->AddImage(textureID, image_pos, image_pos + ImVec2(image_Size, image_Size));

			ImGui::RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
		}
		else
		{
			// Unframed typed for tree nodes
			if (hovered || selected)
			{
				const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
				ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
				ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);
			}
			if (flags & ImGuiTreeNodeFlags_Bullet)
				ImGui::RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), text_col);
			else if (!is_leaf)
				ImGui::RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 0.70f);
			if (g.LogEnabled)
				ImGui::LogSetNextTextDecoration(">", NULL);

			// Render Image
			window->DrawList->AddImage(textureID, image_pos, image_pos + ImVec2(image_Size, image_Size));

			ImGui::RenderText(text_pos, label, label_end, false);
		}

		if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
			ImGui::TreePushOverrideID(id);
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
		return is_open;
	}


	bool TreeNode(const std::string& tag, ImGuiTreeNodeFlags flags, ImTextureID textureID)
	{
		return CustomTreeNodeBehavior(GetID(tag), flags, tag.c_str(), tag.c_str() + tag.size(), textureID);
	}

}

