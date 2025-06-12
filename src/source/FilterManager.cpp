#include "FilterManager.hpp"
#include "Colors.hpp"
#include <algorithm>
#include "Structure.hpp"

FilterManager::FilterManager(int numColumns_) : _filters(numColumns_) {}

void FilterManager::DrawFilterUI(int columnIndex_, const std::string& columnName_) {
    ImGui::PushID(columnIndex_);
    // Calculate header cell size
    ImVec2 headerSize = ImGui::GetContentRegionAvail();
    float filterButtonWidth = 20.0f; // Width of the filter button
    
    // Draw column name
    ImGui::Text("%s", columnName_);
    
    // Draw filter button
    ImGui::SameLine(headerSize.x - filterButtonWidth);
    if (ImGui::Button(ICON_MD_ARROW_DROP_DOWN , ImVec2(filterButtonWidth, 0))) {
        _filters[columnIndex_]._active = ! _filters[columnIndex_]._active;
        if (_filters[columnIndex_]._active) {
            UpdateCheckboxStates(columnIndex_);
        }
    }
    
    // Draw filter dropdown if open
    if (_filters[columnIndex_]._active) {
        DrawFilterDropdown(columnIndex_, columnName_);
    }
    
    ImGui::PopID();
}

void FilterManager::DrawFilterDropdown(int columnIndex_, const std::string& columnName_) {
    auto& filter = _filters[columnIndex_];
    
    // Get the position of the filter button
    ImVec2 buttonPos = ImGui::GetItemRectMin();
    ImVec2 buttonSize = ImGui::GetItemRectSize();
   
   // Calculate window size
    ImVec2 windowSize(250, 300);
    // Get the viewport/work area
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 workAreaMin = viewport->WorkPos;
    ImVec2 workAreaMax(viewport->WorkPos.x + viewport->WorkSize.x, 
                   viewport->WorkPos.y + viewport->WorkSize.y);
    
    // Calculate initial window position (to the left of the button)
    ImVec2 windowPos(buttonPos.x - windowSize.x, buttonPos.y + buttonSize.y);
    
    // Adjust horizontal position if window would go off-screen
    if (windowPos.x < workAreaMin.x) {
        LOG_INFO("Less Than");
        // If window would go off left edge, position it to the right of the button
        windowPos.x = buttonPos.x + buttonSize.x;
    }
    
    // Ensure window doesn't go off right edge
    if (windowPos.x + windowSize.x > workAreaMax.x) {
        LOG_INFO("Greater Than");
        windowPos.x = workAreaMax.x - windowSize.x;
    }
    
    // Ensure window doesn't go off bottom edge
    if (windowPos.y + windowSize.y > workAreaMax.y) {
        windowPos.y = workAreaMax.y - windowSize.y;
    }
    // Set the window position and size
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);

    // Create the filter window
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | 
                                  ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin(("Filter: " + columnName_).c_str(), &filter._active, 
                 window_flags);
    
    // Filter input
    char searchBuffer[256] = {0};
    strncpy(searchBuffer, filter._searchText.c_str(), sizeof(searchBuffer) - 1);
    if (ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        filter._searchText = searchBuffer;
    }
    ImGui::Separator();
     // Select All checkbox
    if (ImGui::Checkbox("Select All", &filter._selectAll)) {
        for (auto& [key, value] : filter._checkboxSuggestions) {
            value = filter._selectAll;
        }
    }
    ImGui::Separator();
    
    // Suggestions list
    ImGui::BeginChild("##suggestions", ImVec2(-1, -1), true);
    // Draw suggestions

    for (auto& [key, value] : filter._checkboxSuggestions) {
        if (!IsItemVisible(key, filter._searchText)) continue;
        
        bool& checked = value;
        if (ImGui::Checkbox(key.c_str(), &checked)) {
            // Update select all state based on individual checkboxes
            filter._selectAll = std::all_of(filter._checkboxSuggestions.begin(), filter._checkboxSuggestions.end(),
                [](const auto& pair) { return pair.second; });
        }
    }
    
    ImGui::EndChild();
    ImGui::Separator();
    
    // Buttons at the bottom
    ImGui::Separator();
    if (ImGui::Button("OK")) {
        // Apply the filter based on checked items
        filter._active = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        ClearFilter(columnIndex_);
    }
    ImGui::End();
}
bool FilterManager::IsItemVisible(const std::string& item, const std::string& searchText) {
    if (searchText.empty()) return true;
    return item.find(searchText) != std::string::npos;
}
void FilterManager::UpdateCheckboxStates(int columnIndex_) {
    auto& filter = _filters[columnIndex_];
    filter._searchText.clear();
    for (auto& [key, value] : filter._checkboxSuggestions) {
        value = filter._selectAll;
    }
}
void FilterManager::ClearAllSuggestions()
{
    for (int index = 0;  index < _filters.size(); index++) {
        _filters[index]._checkboxSuggestions.clear();
    }
}
void FilterManager::UpdateSuggestions(int columnIndex_, const std::string& newSuggestion_) {
    if (columnIndex_ >= 0 && columnIndex_ < _filters.size()) {
        // Initialize checkbox state for new suggestion
        if (_filters[columnIndex_]._checkboxSuggestions.find(newSuggestion_) == _filters[columnIndex_]._checkboxSuggestions.end()) {
            _filters[columnIndex_]._checkboxSuggestions[newSuggestion_] = _filters[columnIndex_]._selectAll;
        }

    }
}

bool FilterManager::ApplyFilters(const std::function<std::string(int)>& getCellValue_) {
    for (size_t index = 0; index < _filters.size(); index++) {
        if (_filters[index]._checkboxSuggestions.empty()) continue;
        
        std::string cell_value = getCellValue_(static_cast<int>(index));
        auto itr = _filters[index]._checkboxSuggestions.find(cell_value);
        if (itr == _filters[index]._checkboxSuggestions.end() || !itr->second) {
            return false;
        }
    }
    return true;
}

void FilterManager::ClearFilters() {
    for (auto index=0; index<_filters.size();index++) {
        ClearFilter(index);
    }
}

void FilterManager::ClearFilter(int columnIndex_) {
    if (columnIndex_ >= 0 && columnIndex_ < _filters.size()) {
        auto& filter = _filters[columnIndex_];
        filter._searchText.clear();
        filter._selectAll = true;
        filter._active = false;
        for (auto& [key, value] : filter._checkboxSuggestions) {
            value = true;
        }
    }
}
