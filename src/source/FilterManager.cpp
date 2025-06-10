#include "FilterManager.hpp"
#include "Colors.hpp"
#include <algorithm>
#include "Structure.hpp"

FilterManager::FilterManager(int numColumns_) : _filters(numColumns_) {}
static int iDebug=1;
void FilterManager::DrawFilterUI(int columnIndex_, const std::string& columnName_) {
    ImGui::PushID(columnIndex_);
    // Calculate header cell size
    ImVec2 headerSize = ImGui::GetContentRegionAvail();
    float filterButtonWidth = 20.0f; // Width of the filter button
    
    // Draw column name
    ImGui::Text("%s", columnName_);
    
    // Draw filter button
    ImGui::SameLine(headerSize.x - filterButtonWidth);
    if (ImGui::Button("▼", ImVec2(filterButtonWidth, 0))) {
        _filters[columnIndex_]._active = ! _filters[columnIndex_]._active;
    }
    
    // Draw filter dropdown if open
    if (_filters[columnIndex_]._active) {
        DrawFilterDropdown(columnIndex_, columnName_);
    }
    
    ImGui::PopID();
}

void FilterManager::DrawFilterDropdown(int columnIndex_, const std::string& columnName_) {
    auto& filter = _filters[columnIndex_];
    
    // Position the dropdown below the header
    //ImVec2 pos = ImGui::GetCursorScreenPos();
    //ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + ImGui::GetFrameHeight()));
    
    ImGui::BeginChild(("##filter_dropdown_" + std::to_string(columnIndex_)).c_str(), 
                     ImVec2(200, 300), true);
    
    // Filter input
    char buffer[256] = {0};
    strncpy(buffer, filter._value.c_str(), sizeof(buffer) - 1);
    if (ImGui::InputText("##filter_input", buffer, sizeof(buffer))) {
        filter._value = buffer;
        filter._active = !filter._value.empty();
    }
    
    ImGui::Separator();
    
    // Suggestions list
    ImGui::BeginChild("##suggestions", ImVec2(-1, -1), true);
    // Draw suggestions

    for (const auto& suggestion : filter._suggestions) {
        if (ImGui::Selectable(suggestion.c_str())) {
            filter._value = suggestion;
            filter._active = true;
        }
        
    }
    
    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::Separator();
    
    if (ImGui::Button("Clear")) {
        ClearFilter(columnIndex_);
    }
}
void FilterManager::DrawFilterPopup(int columnIndex_) {
    if (ImGui::BeginPopup(("FilterPopup" + std::to_string(columnIndex_)).c_str())) {
        DrawFilterDropdown(columnIndex_, "");
        ImGui::EndPopup();
    }
}

void FilterManager::ClearAllSuggestions()
{
    for (int index = 0;  index < _filters.size(); index++) {
            _filters[index]._suggestions.clear();
    }
}
void FilterManager::UpdateSuggestions(int columnIndex_, const std::string& newSuggestion_) {
    if (columnIndex_ >= 0 && columnIndex_ < _filters.size()) {
        _filters[columnIndex_]._suggestions.insert(newSuggestion_);
    }
}

bool FilterManager::ApplyFilters(const std::function<std::string(int)>& getCellValue_) {
    for (size_t index = 0; index < _filters.size(); index++) {
        if (!_filters[index]._active) continue;
        
        std::string cell_value = getCellValue_(static_cast<int>(index));
        if (cell_value.find(_filters[index]._value) == std::string::npos) {
            return false;
        }
    }
    return true;
}

void FilterManager::ClearFilters() {
    for (auto& filter : _filters) {
        filter._active = false;
        filter._value.clear();
    }
}

void FilterManager::ClearFilter(int columnIndex_) {
    if (columnIndex_ >= 0 && columnIndex_ < _filters.size()) {
        _filters[columnIndex_]._active = false;
        _filters[columnIndex_]._value.clear();
    }
}

const std::string& FilterManager::GetFilterValue(int columnIndex_) const {
    static const std::string empty;
    if (columnIndex_ >= 0 && columnIndex_ < _filters.size()) {
        return _filters[columnIndex_]._value;
    }
    return empty;
}

bool FilterManager::HasActiveFilters() const {
    return std::any_of(_filters.begin(), _filters.end(), 
                      [](const FilterState& filter) { return filter._active; });
} 