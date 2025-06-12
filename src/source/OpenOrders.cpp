#include "OpenOrders.hpp"

#include "API/Common.hpp"
#include "API/ContractInfo.hpp"
#include "Colors.hpp"
#include "Configuration.hpp"
#include "Enums.hpp"
#include "OrderForm.hpp"
#include "OrderHistory.hpp"
#include "Structure.hpp"
#include "TableColumnInfo.hpp"
#include "Utils.hpp"

#include <iterator>

static constexpr char CancelAllOrderWindow[] = "Cancel All Order Window";
static constexpr char BeginOpenOrders[]      = "Open Orders";
static constexpr char BeginOpenOrdersTable[] = "Open Orders Table";
static constexpr char BeginCancelBookTable[] = "Cancel Book Table";

OpenOrders::OpenOrders(const OrderFormPtrT& manualOrder_, ExecutorStrandT& strand_, bool& show_, FunctionT function_)
    : _manualOrder{manualOrder_},
      _function{std::move(function_)},
      _strand{strand_},
      _show{show_},
      _filter(BooksColumnIndex_END) 
      {}

void OpenOrders::Paint() noexcept {
    _pendingOrderUpdate.consume_all([this](const auto& pair_) { Update(pair_.first, pair_.second); });
    if (_show) {
        DrawPendingBook(&_show);
    }
}

/*
uint32_t _portfolio;
    uint32_t _uniqueId;
    uint32_t _token;
    
    uint64_t _orderNumber;
    
    

    Lancelot::Side _side;

    OrderStatus _statusValue;
    
    std::string _time;
    std::string _client;
    std::string _message;
*/
/*
    BooksColumnIndex_CLIENT,
    BooksColumnIndex_STATUS,
    BooksColumnIndex_TIME,
    BooksColumnIndex_GATEWAY,
    BooksColumnIndex_ORDER_NUMBER,
    BooksColumnIndex_MESSAGE,
*/
std::string OpenOrders::GetCellValue(const OrderInfoPtrT& order_, int column_index_) const {
    switch (column_index_) {
        case BooksColumnIndex_PF:
            return std::to_string(order_->_portfolio);
        case BooksColumnIndex_CONTRACT:
            return order_->_contract;
        case BooksColumnIndex_PRICE:
            return std::to_string(order_->_price);
        case BooksColumnIndex_QUANTITY:
            return std::to_string(order_->_quantity);
        case BooksColumnIndex_FILL_PRICE:
            return std::to_string(order_->_fillPrice);
        case BooksColumnIndex_FILL_QUANTITY:
            return std::to_string(order_->_fillQuantity);
        case BooksColumnIndex_REMAINING_QTY:
            return std::to_string(order_->_remaining);
        case BooksColumnIndex_CLIENT:
            return order_->_client;
        case BooksColumnIndex_STATUS:
            return OrderStatusInfoName[order_->_statusValue];
        case BooksColumnIndex_TIME:
            return order_->_time;
        case BooksColumnIndex_ORDER_NUMBER:
            return std::to_string(order_->_orderNumber);
        case BooksColumnIndex_MESSAGE:
            return order_->_message;
        default:
            return "";
    }
}

void OpenOrders::UpdateFilterSuggestions() {
    //_filter.ClearAllSuggestions();
    for (const auto& [_, order] : _container) {
        for (int index = 0;  index < BooksColumnIndex_END; index++) {
            _filter.UpdateSuggestions(index, GetCellValue(order, index));
        }
    }
}

void OpenOrders::DrawPendingBook(bool* show_) {
    if (ImGui::Begin(BeginOpenOrders, show_)) {
        const float frameHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        
        UpdateFilterSuggestions();
        if (ImGui::BeginTable(BeginOpenOrdersTable, BooksColumnIndex_END, TableFlags, ImVec2(-FLT_MIN, -frameHeight))) {
            ImGui::TableHeadersRow();
            // Setup columns with filter buttons
            for (int index = 0; index < BooksColumnIndex_END; index++) {
                //ImGui::TableSetupColumn(BookTableColumnName[index], TableColumnFlags);
                ImGui::TableNextColumn();
                _filter.DrawFilterUI(index, BookTableColumnName[index]);
            }
            
            _clipper.Begin(static_cast<int>(_container.size()));

            while (_clipper.Step()) {
                auto begin = _container.rbegin();
                std::ranges::advance(begin, _clipper.DisplayStart);

                auto end = begin;
                std::ranges::advance(end, _clipper.DisplayEnd - _clipper.DisplayStart, _container.rend());

                for (auto& iterator = begin; iterator != end; ++iterator) {
                    //ImGui::TableNextRow();
                    const OrderInfoPtrT& tradeInfo_ = iterator->second;
                    // Apply filters
                    if (!_filter.ApplyFilters([this, &tradeInfo_](int col) { 
                        return GetCellValue(tradeInfo_, col); 
                    })) {
                        continue;
                    }
                    
                    ImGui::TableNextRow();
                    ImGui::PushID(tradeInfo_->_uniqueId);
                    Utils::DrawTradeRow(tradeInfo_, _selectedRow, tradeInfo_->_uniqueId);

                    if (_selectedRow == tradeInfo_->_uniqueId and ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                        if (tradeInfo_->_portfolio == 0 and ImGui::IsKeyPressed(ImGuiKey_F2)) {
                            OrderFormInfoT info{
                                ._uniqueId    = tradeInfo_->_uniqueId,
                                ._price       = tradeInfo_->_price,
                                ._quantity    = (int)tradeInfo_->_quantity,
                                ._lotSize     = (int)Lancelot::ContractInfo::GetLotSize(tradeInfo_->_token),
                                ._orderNumber = tradeInfo_->_orderNumber,
                                ._type        = OrderType_LIMIT,
                                ._side        = tradeInfo_->_side,
                                ._status      = OrderStatus_REPLACED,
                                ._contract    = Lancelot::ContractInfo::GetDescription(tradeInfo_->_token),
                                ._client      = "PRO",
                                ._marketWatch = ContractInfo::GetLiveDataRef(tradeInfo_->_token),
                            };
                            _manualOrder->Update(info);
                            ImGui::OpenPopup(MODIFY_ORDER_WINDOW);
                        }
                        _manualOrder->Paint(MODIFY_ORDER_WINDOW);

                        if (ImGui::IsKeyPressed(ImGuiKey_F4)) {
                            OrderHistory::Instance().LoadOrderHistory(tradeInfo_->_orderNumber);
                            ImGui::OpenPopup(ORDER_HISTORY_POPUP_WINDOW);
                        }
                        OrderHistory::Instance().Paint(nullptr);

                        if (tradeInfo_->_portfolio == 0 and ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                            _strand.post([&]() {
                                _function(tradeInfo_);
                            });
                        }
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
            ImGui::Separator();
            if (ImGui::Button("Cancel All")) {
                _cancelOrder.clear();
                for (const auto& value : _container) {
                    if (value.second->_portfolio == 0) {
                        _cancelOrder.push_back(value.second);
                    }
                }
                _closeCancelPopup = true;
                ImGui::OpenPopup(CancelAllOrderWindow);
            }
            DrawManualOrderRequestedForCancel();
            ImGui::SameLine();
            ImGui::Text("| Total : [%zu] |", _container.size());
            ImGui::SameLine();
            ImGui::TextColored(BuySellColor(Lancelot::Side_BUY), "| Buy : [%d] |", _buyCount);
            ImGui::SameLine();
            ImGui::TextColored(BuySellColor(Lancelot::Side_SELL), "| Sell : [%d] |", _sellCount);
        }
    }
    ImGui::End();
}

/*
// Add buffers for column filters
char orderNumberFilter[128] = {0};
char sideFilter[128] = {0};
char portfolioFilter[128] = {0};
char priceFilter[128] = {0};


// Modify the `DrawPendingBook` method to include inline filters under each column header
void OpenOrders::DrawPendingBook(bool* show_) {
    if (ImGui::Begin(BeginOpenOrders, show_)) {
        const float frameHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

        if (ImGui::BeginTable(BeginOpenOrdersTable, BooksColumnIndex_END, TableFlags, ImVec2(-FLT_MIN, -frameHeight))) {
            for (const auto& name : BookTableColumnName) {
                ImGui::TableSetupColumn(name, TableColumnFlags);
            }

            // Render table headers with inline filters
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(BooksColumnIndex_ORDER_NUMBER);
            ImGui::InputText("##OrderNumberFilter", orderNumberFilter, sizeof(orderNumberFilter), ImGuiInputTextFlags_EnterReturnsTrue);

            
          
            _clipper.Begin(static_cast<int>(_container.size()));

            while (_clipper.Step()) {
                auto begin = _container.rbegin();
                std::ranges::advance(begin, _clipper.DisplayStart);

                auto end = begin;
                std::ranges::advance(end, _clipper.DisplayEnd - _clipper.DisplayStart, _container.rend());

                for (auto& iterator = begin; iterator != end; ++iterator) {
                    const OrderInfoPtrT& tradeInfo_ = iterator->second;

                    // Convert numeric values to strings for filtering
                    std::string orderNumberStr = std::to_string(tradeInfo_->_orderNumber);
                    std::string sideStr = Utils::SideToString(tradeInfo_->_side);
                    std::string portfolioStr = std::to_string(tradeInfo_->_portfolio);
                    std::string priceStr = std::to_string(tradeInfo_->_price);

                    // Apply filters: Skip rows that don't match the filter text
                    if (!std::string(orderNumberFilter).empty() && orderNumberStr.find(orderNumberFilter) == std::string::npos) {
                        continue;
                    }
                   

                    ImGui::TableNextRow();
                    ImGui::PushID(tradeInfo_->_uniqueId);
                    Utils::DrawTradeRow(tradeInfo_, _selectedRow, tradeInfo_->_uniqueId);

                    if (_selectedRow == tradeInfo_->_uniqueId and ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                        if (tradeInfo_->_portfolio == 0 and ImGui::IsKeyPressed(ImGuiKey_F2)) {
                            OrderFormInfoT info{
                                ._uniqueId    = tradeInfo_->_uniqueId,
                                ._price       = tradeInfo_->_price,
                                ._quantity    = (int)tradeInfo_->_quantity,
                                ._lotSize     = (int)Lancelot::ContractInfo::GetLotSize(tradeInfo_->_token),
                                ._orderNumber = tradeInfo_->_orderNumber,
                                ._type        = OrderType_LIMIT,
                                ._side        = tradeInfo_->_side,
                                ._status      = OrderStatus_REPLACED,
                                ._contract    = Lancelot::ContractInfo::GetDescription(tradeInfo_->_token),
                                ._client      = "PRO",
                                ._marketWatch = ContractInfo::GetLiveDataRef(tradeInfo_->_token),
                            };
                            _manualOrder->Update(info);
                            ImGui::OpenPopup(MODIFY_ORDER_WINDOW);
                        }
                        _manualOrder->Paint(MODIFY_ORDER_WINDOW);

                        if (ImGui::IsKeyPressed(ImGuiKey_F4)) {
                            OrderHistory::Instance().LoadOrderHistory(tradeInfo_->_orderNumber);
                            ImGui::OpenPopup(ORDER_HISTORY_POPUP_WINDOW);
                        }
                        OrderHistory::Instance().Paint(nullptr);

                        if (tradeInfo_->_portfolio == 0 and ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                            _strand.post([&]() {
                                _function(tradeInfo_);
                            });
                        }
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
            ImGui::Separator();
            if (ImGui::Button("Cancel All")) {
                _cancelOrder.clear();
                for (const auto& value : _container) {
                    if (value.second->_portfolio == 0) {
                        _cancelOrder.push_back(value.second);
                    }
                }
                _closeCancelPopup = true;
                ImGui::OpenPopup(CancelAllOrderWindow);
            }
            DrawManualOrderRequestedForCancel();
            ImGui::SameLine();
            ImGui::Text("| Total : [%zu] |", _container.size());
            ImGui::SameLine();
            ImGui::TextColored(BuySellColor(Lancelot::Side_BUY), "| Buy : [%d] |", _buyCount);
            ImGui::SameLine();
            ImGui::TextColored(BuySellColor(Lancelot::Side_SELL), "| Sell : [%d] |", _sellCount);
        }
    }
    ImGui::End();
}
   */ 
void OpenOrders::DrawManualOrderRequestedForCancel() {
    if (ImGui::BeginPopupModal(CancelAllOrderWindow, &_closeCancelPopup)) {
        const float frameHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginTable(BeginCancelBookTable, BooksColumnIndex_END, TableFlags, ImVec2(-FLT_MIN, -frameHeight))) {
            for (const auto& name : BookTableColumnName) {
                ImGui::TableSetupColumn(name, TableColumnFlags | ImGuiTableColumnFlags_WidthStretch);
            }
            ImGui::TableHeadersRow();

            _clipper.Begin(static_cast<int>(_cancelOrder.size()));
            while (_clipper.Step()) {
                auto begin = _cancelOrder.begin() + _clipper.DisplayStart;
                auto end   = begin + (_clipper.DisplayEnd - _clipper.DisplayStart);
                int  i     = _clipper.DisplayStart;
                for (auto iterator = begin; iterator < end; ++iterator, ++i) {
                    ImGui::PushID(i);
                    ImGui::TableNextRow();
                    Utils::DrawTradeRow(*iterator, _selectedRow, -2);
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::Separator();
        if (ImGui::Button(ICON_MD_DONE " Process")) {
            auto _ = std::async(std::launch::async, [&]() {
                for (const auto& tradeInfo : _cancelOrder) {
                    _strand.post([&]() { _function(tradeInfo); });
                }
            });
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_MD_CANCEL " Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        ImGui::Text("These all %zu orders will sent for cancellation", _cancelOrder.size());
        ImGui::EndPopup();
    }
}

void OpenOrders::Update(const OrderInfoPtrT& tradeInfo_, bool insert_) {
    {
        auto iterator = _hashing.find(tradeInfo_->_uniqueId);
        if (iterator != _hashing.end()) {
            if (const auto position = _container.find(iterator->second); position != _container.end()) {
                auto nextLocation = _container.erase(position);
                if (nextLocation != _container.end()) {
                    _selectedRow = static_cast<int>(nextLocation->second->_uniqueId);
                } else if (not _container.empty()) {
                    _selectedRow = static_cast<int>(_container.rbegin()->second->_uniqueId);
                }
                _buyCount -= static_cast<int>(tradeInfo_->_side == Lancelot::Side_BUY);
                _sellCount -= static_cast<int>(tradeInfo_->_side == Lancelot::Side_SELL);
            }
        }
        _hashing[tradeInfo_->_uniqueId] = tradeInfo_->_time;
    }

    if (insert_) {
        _container.emplace(tradeInfo_->_time, tradeInfo_);
        _buyCount += static_cast<int>(tradeInfo_->_side == Lancelot::Side_BUY);
        _sellCount += static_cast<int>(tradeInfo_->_side == Lancelot::Side_SELL);
    }
}
void OpenOrders::Insert(const OrderInfoPtrT& tradeInfo_, bool insert_) {
    _pendingOrderUpdate.push(std::make_pair(tradeInfo_, insert_));
}
