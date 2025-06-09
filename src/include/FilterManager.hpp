#pragma once

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <imgui.h>

class FilterManager
{
    public:
        struct FilterState {
            bool _active = false;
            std::string _value;
            std::vector<std::string> _suggestions;
            bool _showSuggestions = false;
        };
        FilterManager(int numColumns_);
         ~FilterManager() = default;
    
    // Draw the filter UI for a specific column
    void DrawFilterUI(int columnIndex_, const std::string& columnName_);
    
    // Update suggestions for a column
    void UpdateSuggestions(int columnIndex_, const std::string& newSuggestion_);
    
    // Check if a row passes all active filters
    bool ApplyFilters(const std::function<std::string(int)>& getCellValue);
    
    // Clear all filters
    void ClearFilters();
    
    // Clear filter for specific column
    void ClearFilter(int columnIndex_);
    
    // Get filter value for a column
    const std::string& GetFilterValue(int columnIndex_) const;
    
    // Check if any filter is active
    bool HasActiveFilters() const;

    void ClearAllSuggestions();

private:
    void DrawFilterDropdown(int columnIndex_, const std::string& columnName_);
    void DrawFilterPopup(int columnIndex_);
    
    std::vector<FilterState> _filters;
    int _activeFilterColumn = -1;
}; 
