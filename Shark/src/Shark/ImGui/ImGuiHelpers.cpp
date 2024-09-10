#include "skpch.h"
#include "ImGuiHelpers.h"

#include <imgui_internal.h>

void ImGui::Text(std::string_view str)
{
	ImGui::TextEx(str.data(), str.data() + str.size());
}

void ImGui::Text(const std::string& string)
{
	return Text(std::string_view(string));
}

void ImGui::Text(const std::filesystem::path& path)
{
	std::string temp = path.string();
	return Text(temp);
}

bool ImGui::BeginDrapDropTargetWindow(const char* payload_type)
{
	ImRect inner_rect = GetCurrentWindow()->InnerRect;
	if (BeginDragDropTargetCustom(inner_rect, GetID("##WindowBgArea")) && payload_type)
	{
		if (const ImGuiPayload* payload = AcceptDragDropPayload(payload_type, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
		{
			if (payload->IsPreview())
			{
				ImDrawList* draw_list = GetForegroundDrawList();
				draw_list->AddRectFilled(inner_rect.Min, inner_rect.Max, GetColorU32(ImGuiCol_DragDropTarget, 0.05f));
				draw_list->AddRect(inner_rect.Min, inner_rect.Max, GetColorU32(ImGuiCol_DragDropTarget), 0.0f, 0, 2.0f);
			}
			if (payload->IsDelivery())
				return true;
			EndDragDropTarget();
		}
	}
	return false;
}

bool ImGui::BeginPopupModal(ImGuiID id, const char* name, bool* p_open, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (!IsPopupOpen(id, ImGuiPopupFlags_None))
	{
		g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
		if (p_open && *p_open)
			*p_open = false;
		return false;
	}

	// Center modal windows by default for increased visibility
	// (this won't really last as settings will kick in, and is mostly for backward compatibility. user may do the same themselves)
	// FIXME: Should test for (PosCond & window->SetWindowPosAllowFlags) with the upcoming window.
	if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) == 0)
	{
		const ImGuiViewport* viewport = window->WasActive ? window->Viewport : GetMainViewport(); // FIXME-VIEWPORT: What may be our reference viewport?
		SetNextWindowPos(viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	}

	flags |= ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
	const bool is_open = BeginEx(name, id, p_open, flags);
	if (!is_open || (p_open && !*p_open)) // NB: is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
	{
		EndPopup();
		if (is_open)
			ClosePopupToLevel(g.BeginPopupStack.Size, true);
		return false;
	}
	return is_open;
}
