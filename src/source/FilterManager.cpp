#include "FilterManager.hpp"
#include "Colors.hpp"
#include <algorithm>

FilterManager::FilterManager(int numColumns_) : _filters(numColumns_) {}

void FilterManager::DrawFilterUI(int columnIndex_, const std::string& columnName_) {
    ImGui::PushID(columnIndex_);
    
    // Draw filter button in header
    std::string header_label = columnName_;
    if (_filters[columnIndex_]._active) {
        header_label += " *";
    }
    ImGui::TableHeader(header_label.c_str());
    
    // Add filter button
    ImGui::SameLine(ImGui::GetColumnWidth() - 20);
    if (ImGui::Button("▼", ImVec2(20, 0))) {
        _activeFilterColumn = columnIndex_;
        ImGui::OpenPopup(("FilterPopup" + std::to_string(columnIndex_)).c_str());
    }
    
    DrawFilterPopup(columnIndex_);
    ImGui::PopID();
}

void FilterManager::DrawFilterDropdown(int columnIndex_, const std::string& columnName_) {
    auto& filter = _filters[columnIndex_];
    
    // Input text for filter
    char buffer[256] = {0};
    strncpy(buffer, filter._value.c_str(), sizeof(buffer) - 1);
    
    if (ImGui::InputText("##FilterInput", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        filter._value = buffer;
        filter._active = !filter._value.empty();
    }
    
    // Show suggestions if we have any
    if (!filter._suggestions.empty()) {
        ImGui::Separator();
        ImGui::BeginChild("Suggestions", ImVec2(0, 100), true);
        
        for (const auto& suggestion : filter._suggestions) {
            if (ImGui::Selectable(suggestion.c_str())) {
                filter._value = suggestion;
                filter._active = true;
            }
        }
        
        ImGui::EndChild();
    }
    
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
        _filters[columnIndex_]._suggestions.push_back(newSuggestion_);
    }
}

bool FilterManager::ApplyFilters(const std::function<std::string(int)>& getCellValue_) {
    for (size_t index = 0; index < _filters.size(); index++) {
        if (!_filters[index]._active) continue;
        
        std::string cell_value = getCellValue_(index);
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