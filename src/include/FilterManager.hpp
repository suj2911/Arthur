#pragma once

class FilterManager
{
    public:
        struct FilterState {
            bool _showSuggestions = false;
            std::unordered_map<std::string, bool> _checkboxSuggestions;
            bool _selectAll = true;
            std::string _searchText;
            bool _active = false;
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

    void ClearAllSuggestions();

private:
    void DrawFilterDropdown(int columnIndex_, const std::string& columnName_);
    void UpdateCheckboxStates(int columnIndex_);
    bool IsItemVisible(const std::string& item_, const std::string& searchText_);
    
    std::vector<FilterState> _filters;
    
}; 
