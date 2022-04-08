#include "ControlContext.hpp"
#include <deque>
#include <queue>
#include <stack>
#include <set>
#include <charconv>

namespace AppCUI::Controls
{
constexpr auto TreeSearchBarWidth      = 23U;
constexpr auto TreeScrollbarLeftOffset = 25U;
constexpr auto ItemSymbolOffset        = 2U;
constexpr auto MinColumnWidth          = 10U;
constexpr auto BorderOffset            = 1U;
constexpr auto InvalidIndex            = 0xFFFFFFFFU;

const static Utils::UnicodeStringBuilder cb{};

TreeView::TreeView(string_view layout, const TreeViewFlags flags, const uint32 noOfColumns)
    : Control(new TreeControlContext(), "", layout, true)
{
    const auto cc        = reinterpret_cast<TreeControlContext*>(Context);
    cc->Layout.MinHeight = 1;
    cc->Layout.MaxHeight = 200000;
    cc->Layout.MinWidth  = 20;

    cc->treeFlags = static_cast<uint32>(flags);

    cc->Flags = GATTR_ENABLE | GATTR_VISIBLE | GATTR_TABSTOP;

    if ((cc->treeFlags & TreeViewFlags::HideScrollBar) == TreeViewFlags::None)
    {
        cc->Flags |= GATTR_VSCROLL;
        cc->ScrollBars.LeftMargin     = TreeScrollbarLeftOffset; // search field
        cc->ScrollBars.TopMargin      = BorderOffset + 1;        // border + column header
        cc->ScrollBars.OutsideControl = false;
    }

    if ((cc->treeFlags & TreeViewFlags::HideSearchBar) == TreeViewFlags::None)
    {
        if ((cc->treeFlags & TreeViewFlags::FilterSearch) != TreeViewFlags::None)
        {
            cc->filter.mode = TreeControlContext::FilterMode::Filter;
        }
        else if ((cc->treeFlags & TreeViewFlags::Searchable) != TreeViewFlags::None)
        {
            cc->filter.mode = TreeControlContext::FilterMode::Search;
        }
        else
        {
            cc->treeFlags |= static_cast<uint32>(TreeViewFlags::HideSearchBar);
        }
    }

    cc->itemsToDrew.reserve(100);
    cc->orderedItems.reserve(100);

    if ((cc->treeFlags & TreeViewFlags::Sortable) != TreeViewFlags::None)
    {
        cc->columnIndexToSortBy = 0;
        cc->Sort();
    }

    AdjustItemsBoundsOnResize();

    const auto columnsCount = std::max<>(noOfColumns, 1U);
    const auto width        = std::max<>((static_cast<uint32>(cc->Layout.Width) / columnsCount), MinColumnWidth);
    for (auto i = 0U; i < columnsCount; i++)
    {
        TreeColumnData cd{ static_cast<uint32>(cc->columns.size() * width + BorderOffset),
                           width,
                           static_cast<uint32>(cc->Layout.Height - 2),
                           {},
                           TextAlignament::Center,
                           TextAlignament::Left };
        cc->columns.emplace_back(cd);
    }

    cc->separatorIndexSelected = InvalidIndex;

    SetColorForItems(cc->Cfg->Text.Normal);
}

bool TreeView::ItemsPainting(Graphics::Renderer& renderer, const ItemHandle /*ih*/) const
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    WriteTextParams wtp;
    wtp.Y = ((cc->treeFlags & TreeViewFlags::HideColumns) == TreeViewFlags::None) +
            ((cc->treeFlags & TreeViewFlags::HideBorder) == TreeViewFlags::None); // 0  is for border | 1 is for header

    for (auto i = cc->offsetTopToDraw; i < std::min<size_t>(cc->offsetBotToDraw, cc->itemsToDrew.size()); i++)
    {
        auto& item = cc->items[cc->itemsToDrew[i]];

        uint32 j = 0; // column index
        for (const auto& col : cc->columns)
        {
            wtp.Flags = WriteTextFlags::SingleLine | WriteTextFlags::ClipToWidth;
            wtp.Align = col.contentAlignment;
            if (j == 0)
            {
                wtp.X     = col.x + item.depth * ItemSymbolOffset - 1;
                wtp.Width = col.width - item.depth * ItemSymbolOffset -
                            ((cc->treeFlags & TreeViewFlags::HideColumnsSeparator) == TreeViewFlags::None);

                if (wtp.X < static_cast<int>(col.x + col.width))
                {
                    if (item.isExpandable)
                    {
                        if (item.expanded)
                        {
                            renderer.WriteSpecialCharacter(
                                  wtp.X, wtp.Y, SpecialChars::TriangleDown, cc->Cfg->Symbol.Arrows);
                        }
                        else
                        {
                            renderer.WriteSpecialCharacter(
                                  wtp.X, wtp.Y, SpecialChars::TriangleRight, cc->Cfg->Symbol.Arrows);
                        }
                    }
                    else
                    {
                        renderer.WriteSpecialCharacter(
                              wtp.X, wtp.Y, SpecialChars::CircleFilled, cc->Cfg->Symbol.Inactive);
                    }
                }

                wtp.X += ItemSymbolOffset;
            }
            else
            {
                wtp.X     = col.x;
                wtp.Width = col.width;
            }

            if (j < item.values.size())
            {
                if (item.handle == cc->currentSelectedItemHandle)
                {
                    wtp.Color = cc->Cfg->Text.Focused;
                    item.values[j].SetColor(wtp.Color);
                }
                else if (item.markedAsFound == false)
                {
                    wtp.Color = cc->Cfg->Text.Normal;
                    item.values[j].SetColor(wtp.Color);
                }

                if (wtp.X < static_cast<int>(col.x + col.width))
                {
                    renderer.WriteText(item.values[j], wtp);
                }
            }

            j++;
        }

        wtp.Y++;
    }

    return true;
}

bool TreeView::PaintColumnHeaders(Graphics::Renderer& renderer)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);
    CHECK(cc->columns.size() > 0, true, "");

    const auto controlWidth = GetWidth() - 2 * ((cc->treeFlags & TreeViewFlags::HideBorder) == TreeViewFlags::None);

    renderer.FillHorizontalLineSize(GetX() - 1, 1, controlWidth, ' ', cc->Cfg->Header.Text.Focused);

    const auto enabled = (cc->Flags & GATTR_ENABLE) != 0;

    WriteTextParams wtp{ WriteTextFlags::SingleLine | WriteTextFlags::ClipToWidth };
    wtp.Y     = 1; // 0  is for border
    wtp.Color = cc->Cfg->Text.Normal;

    uint32 i = 0;
    for (const auto& col : cc->columns)
    {
        wtp.Align = col.headerAlignment;
        wtp.X     = col.x;
        wtp.Width = col.width - ((cc->treeFlags & TreeViewFlags::HideColumnsSeparator) == TreeViewFlags::None);

        if (wtp.X >= controlWidth)
        {
            continue;
        }

        if (wtp.X + static_cast<int32>(wtp.Width) >= controlWidth)
        {
            wtp.Width = controlWidth - wtp.X;
        }

        wtp.Color = enabled ? ((i == cc->columnIndexToSortBy) ? cc->Cfg->Header.Text.PressedOrSelected
                                                              : cc->Cfg->Header.Text.Focused)
                            : cc->Cfg->Header.Text.Inactive;

        renderer.FillHorizontalLineSize(col.x, 1, wtp.Width, ' ', wtp.Color);

        renderer.WriteText(*const_cast<CharacterBuffer*>(&col.headerValue), wtp);

        if (i == cc->columnIndexToSortBy)
        {
            renderer.WriteSpecialCharacter(
                  static_cast<int32>(wtp.X + wtp.Width),
                  1,
                  cc->sortAscendent ? SpecialChars::TriangleUp : SpecialChars::TriangleDown,
                  cc->Cfg->Header.HotKey.PressedOrSelected);
        }

        i++;
    }

    return true;
}

bool TreeView::PaintColumnSeparators(Graphics::Renderer& renderer)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);
    CHECK(cc->columns.size() > 0, true, "");

    if (cc->separatorIndexSelected != InvalidIndex)
    {
        CHECK(cc->separatorIndexSelected <= cc->columns.size(), // # columns + 1 separators
              false,
              "%u %u",
              cc->separatorIndexSelected,
              cc->columns.size());
    }

    for (auto i = 0U; i < cc->columns.size(); i++)
    {
        const auto& col = cc->columns[i];
        if (static_cast<int32>(col.x + col.width) <=
            GetWidth() - 2 * ((cc->treeFlags & TreeViewFlags::HideBorder) == TreeViewFlags::None))
        {
            const auto& color = (cc->separatorIndexSelected == i || cc->mouseOverColumnSeparatorIndex == i)
                                      ? cc->Cfg->Lines.Hovered
                                      : cc->Cfg->Lines.Normal;
            renderer.DrawVerticalLine(
                  col.x + col.width,
                  1,
                  cc->Layout.Height - 2 * ((cc->treeFlags & TreeViewFlags::HideBorder) == TreeViewFlags::None),
                  color);
        }
    }

    return true;
}

bool TreeView::MoveUp()
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    if (cc->itemsToDrew.size() > 0)
    {
        const auto it       = find(cc->itemsToDrew.begin(), cc->itemsToDrew.end(), cc->currentSelectedItemHandle);
        const auto index    = static_cast<uint32>(it - cc->itemsToDrew.begin());
        const auto newIndex = std::min<uint32>(index - 1, static_cast<uint32>(cc->itemsToDrew.size() - 1U));
        cc->currentSelectedItemHandle = cc->itemsToDrew[newIndex];

        if (newIndex == cc->itemsToDrew.size() - 1)
        {
            if (cc->itemsToDrew.size() > cc->maxItemsToDraw)
            {
                cc->offsetBotToDraw = static_cast<uint32>(cc->itemsToDrew.size());
            }

            if (cc->offsetBotToDraw >= cc->maxItemsToDraw)
            {
                cc->offsetTopToDraw = cc->offsetBotToDraw - cc->maxItemsToDraw;
            }
        }
        else if (newIndex < cc->offsetTopToDraw && cc->offsetTopToDraw > 0)
        {
            cc->offsetBotToDraw--;
            cc->offsetTopToDraw--;
        }

        return true;
    }

    return false;
}

bool TreeView::MoveDown()
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    if (cc->itemsToDrew.size() > 0)
    {
        const auto it       = find(cc->itemsToDrew.begin(), cc->itemsToDrew.end(), cc->currentSelectedItemHandle);
        const auto index    = static_cast<uint32>(it - cc->itemsToDrew.begin());
        const auto newIndex = std::min<uint32>(index + 1, (index + 1 > cc->itemsToDrew.size() - 1 ? 0 : index + 1));
        cc->currentSelectedItemHandle = cc->itemsToDrew[newIndex];

        if (newIndex == 0)
        {
            cc->offsetBotToDraw = cc->maxItemsToDraw;
            cc->offsetTopToDraw = 0;
        }
        else if (newIndex >= cc->offsetBotToDraw)
        {
            cc->offsetBotToDraw++;
            cc->offsetTopToDraw++;
        }

        return true;
    }

    return false;
}

bool TreeView::ProcessItemsToBeDrawn(const ItemHandle handle, bool clear)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    if (clear)
    {
        cc->itemsToDrew.clear();
        cc->itemsToDrew.reserve(cc->items.size());
    }

    CHECK(cc->items.size() > 0, true, "");

    if (handle == InvalidItemHandle)
    {
        for (const auto& handle : cc->roots)
        {
            const auto& item = cc->items[handle];
            if (cc->filter.mode == TreeControlContext::FilterMode::Filter && cc->filter.searchText.Len() > 0)
            {
                if (item.hasAChildThatIsMarkedAsFound == false && item.markedAsFound == false)
                {
                    continue;
                }
            }

            cc->itemsToDrew.emplace_back(handle);

            if (item.isExpandable == false || item.expanded == false)
            {
                continue;
            }

            for (auto& it : item.children)
            {
                const auto& child = cc->items[it];

                if (cc->filter.mode == TreeControlContext::FilterMode::Filter && cc->filter.searchText.Len() > 0)
                {
                    if (child.hasAChildThatIsMarkedAsFound == false && child.markedAsFound == false)
                    {
                        continue;
                    }
                }

                if (child.isExpandable)
                {
                    if (child.expanded)
                    {
                        ProcessItemsToBeDrawn(it, false);
                    }
                    else
                    {
                        cc->itemsToDrew.emplace_back(child.handle);
                    }
                }
                else
                {
                    cc->itemsToDrew.emplace_back(child.handle);
                }
            }
        }
    }
    else
    {
        const auto& item = cc->items[handle];

        if (cc->filter.mode == TreeControlContext::FilterMode::Filter && cc->filter.searchText.Len() > 0)
        {
            if (item.hasAChildThatIsMarkedAsFound == false && item.markedAsFound == false)
            {
                return true;
            }
        }

        cc->itemsToDrew.emplace_back(item.handle);
        CHECK(item.isExpandable, true, "");

        for (auto& it : item.children)
        {
            const auto& child = cc->items[it];

            if (cc->filter.mode == TreeControlContext::FilterMode::Filter && cc->filter.searchText.Len() > 0)
            {
                if (child.hasAChildThatIsMarkedAsFound == false && child.markedAsFound == false)
                {
                    continue;
                }
            }

            if (child.isExpandable)
            {
                if (child.expanded)
                {
                    ProcessItemsToBeDrawn(it, false);
                }
                else
                {
                    cc->itemsToDrew.emplace_back(child.handle);
                }
            }
            else
            {
                cc->itemsToDrew.emplace_back(child.handle);
            }
        }
    }

    return true;
}

bool TreeView::IsAncestorOfChild(const ItemHandle ancestor, const ItemHandle child) const
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    std::queue<ItemHandle> ancestorRelated;
    ancestorRelated.push(ancestor);

    while (ancestorRelated.empty() == false)
    {
        ItemHandle current = ancestorRelated.front();
        ancestorRelated.pop();

        for (const auto& handle : cc->items[current].children)
        {
            if (handle == child)
            {
                return true;
            }
            else
            {
                ancestorRelated.push(handle);
            }
        }
    }

    return false;
}

bool TreeView::ToggleExpandRecursive(const ItemHandle handle)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    std::queue<ItemHandle> ancestorRelated;
    ancestorRelated.push(handle);

    while (ancestorRelated.empty() == false)
    {
        ItemHandle current = ancestorRelated.front();
        ancestorRelated.pop();

        ToggleItem(current);

        const auto& item = cc->items[current];
        for (const auto& handle : item.children)
        {
            ancestorRelated.push(handle);
        }
    }

    if ((cc->treeFlags & TreeViewFlags::Sortable) != TreeViewFlags::None)
    {
        cc->Sort();
    }

    cc->notProcessed = true;

    return true;
}

void TreeView::Paint(Graphics::Renderer& renderer)
{
    CHECKRET(Context != nullptr, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    PaintColumnHeaders(renderer);

    if (cc->notProcessed)
    {
        ProcessItemsToBeDrawn(InvalidItemHandle, true);
        cc->notProcessed = false;
    }

    if ((cc->treeFlags & TreeViewFlags::HideScrollBar) == TreeViewFlags::None)
    {
        if (cc->itemsToDrew.size() > cc->maxItemsToDraw)
        {
            if ((cc->Flags & GATTR_VSCROLL) == 0)
            {
                cc->Flags |= GATTR_VSCROLL;
            }
        }
        else
        {
            if ((cc->Flags & GATTR_VSCROLL) == GATTR_VSCROLL)
            {
                cc->Flags ^= GATTR_VSCROLL;
            }
        }
    }

    ItemsPainting(renderer, InvalidItemHandle);
    PaintColumnSeparators(renderer);

    if ((cc->treeFlags & TreeViewFlags::HideBorder) == TreeViewFlags::None)
    {
        renderer.DrawRectSize(0, 0, cc->Layout.Width, cc->Layout.Height, cc->Cfg->Border.Normal, LineType::Single);
    }

    if (cc->Focused)
    {
        if (cc->Layout.Width > TreeSearchBarWidth && cc->filter.mode != TreeControlContext::FilterMode::None)
        {
            renderer.FillHorizontalLine(1, cc->Layout.Height - 1, TreeSearchBarWidth, ' ', cc->Cfg->SearchBar.Normal);

            if (const auto searchTextLen = cc->filter.searchText.Len(); searchTextLen > 0)
            {
                if (const auto searchText = cc->filter.searchText.ToStringView();
                    searchText.length() < TreeSearchBarWidth - 2ULL)
                {
                    renderer.WriteSingleLineText(2, cc->Layout.Height - 1, searchText, cc->Cfg->Text.Normal);
                    renderer.SetCursor((int) (2 + searchText.length()), cc->Layout.Height - 1);
                }
                else
                {
                    renderer.WriteSingleLineText(
                          2,
                          cc->Layout.Height - 1,
                          searchText.substr(searchText.length() - TreeSearchBarWidth + 2, TreeSearchBarWidth - 2),
                          cc->Cfg->Text.Normal);
                    renderer.SetCursor(TreeSearchBarWidth, cc->Layout.Height - 1);
                }
            }
            else if (cc->isMouseOn == TreeControlContext::IsMouseOn::SearchField)
            {
                renderer.SetCursor(2, cc->Layout.Height - 1);
            }
        }
    }
}

bool TreeView::OnKeyEvent(Input::Key keyCode, char16 character)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    switch (keyCode)
    {
    case Key::Up:
        return MoveUp();
    case Key::Down:
        if (cc->separatorIndexSelected != InvalidIndex)
        {
            cc->separatorIndexSelected = InvalidIndex;
            return true;
        }
        else
        {
            return MoveDown();
        }
    case Key::Ctrl | Key::Up:
        if (cc->itemsToDrew.size() > 0)
        {
            if (cc->offsetTopToDraw > 0)
            {
                cc->offsetTopToDraw--;
                cc->offsetBotToDraw--;
                return true;
            }
        }
        break;
    case Key::Ctrl | Key::Down:
        if (cc->itemsToDrew.size() > 0)
        {
            if (cc->offsetBotToDraw < cc->itemsToDrew.size())
            {
                cc->offsetTopToDraw++;
                cc->offsetBotToDraw++;
                return true;
            }
        }
        break;
    case Key::PageUp:
        if (cc->itemsToDrew.size() == 0)
        {
            break;
        }

        if (static_cast<size_t>(cc->offsetTopToDraw) > cc->maxItemsToDraw)
        {
            const auto it    = find(cc->itemsToDrew.begin(), cc->itemsToDrew.end(), cc->currentSelectedItemHandle);
            const auto index = static_cast<uint32>(it - cc->itemsToDrew.begin()) - cc->maxItemsToDraw;
            cc->currentSelectedItemHandle = cc->itemsToDrew[index];

            cc->offsetTopToDraw -= cc->maxItemsToDraw;
            cc->offsetBotToDraw -= cc->maxItemsToDraw;
        }
        else if (cc->offsetTopToDraw > 0)
        {
            const auto difference = cc->offsetTopToDraw;

            const auto it    = find(cc->itemsToDrew.begin(), cc->itemsToDrew.end(), cc->currentSelectedItemHandle);
            const auto index = static_cast<uint32>(it - cc->itemsToDrew.begin()) - difference;
            cc->currentSelectedItemHandle = cc->itemsToDrew[index];

            cc->offsetTopToDraw -= difference;
            cc->offsetBotToDraw -= difference;
        }
        else
        {
            cc->currentSelectedItemHandle = cc->itemsToDrew[0];
        }

        return true;
    case Key::PageDown:
        if (cc->itemsToDrew.size() == 0)
        {
            break;
        }

        if (static_cast<size_t>(cc->offsetBotToDraw) + cc->maxItemsToDraw < cc->itemsToDrew.size())
        {
            const auto it    = find(cc->itemsToDrew.begin(), cc->itemsToDrew.end(), cc->currentSelectedItemHandle);
            const auto index = static_cast<uint32>(it - cc->itemsToDrew.begin()) + cc->maxItemsToDraw;
            cc->currentSelectedItemHandle = cc->itemsToDrew[index];

            cc->offsetTopToDraw += cc->maxItemsToDraw;
            cc->offsetBotToDraw += cc->maxItemsToDraw;
        }
        else if (static_cast<size_t>(cc->offsetBotToDraw) < cc->itemsToDrew.size())
        {
            const auto difference = cc->itemsToDrew.size() - cc->offsetBotToDraw;

            const auto it    = find(cc->itemsToDrew.begin(), cc->itemsToDrew.end(), cc->currentSelectedItemHandle);
            const auto index = static_cast<uint32>(it - cc->itemsToDrew.begin()) + difference;
            cc->currentSelectedItemHandle = cc->itemsToDrew[index];

            cc->offsetTopToDraw += static_cast<uint32>(difference);
            cc->offsetBotToDraw += static_cast<uint32>(difference);
        }
        else
        {
            cc->currentSelectedItemHandle = cc->itemsToDrew[cc->itemsToDrew.size() - 1];
        }

        return true;

    case Key::Home:
        if (cc->itemsToDrew.size() == 0)
        {
            break;
        }
        cc->offsetTopToDraw           = 0;
        cc->offsetBotToDraw           = cc->maxItemsToDraw;
        cc->currentSelectedItemHandle = cc->itemsToDrew[0];
        return true;

    case Key::End:
        if (cc->itemsToDrew.size() == 0)
        {
            break;
        }
        cc->offsetTopToDraw           = static_cast<uint32>(cc->itemsToDrew.size()) - cc->maxItemsToDraw;
        cc->offsetBotToDraw           = cc->offsetTopToDraw + cc->maxItemsToDraw;
        cc->currentSelectedItemHandle = cc->itemsToDrew[cc->itemsToDrew.size() - 1];
        return true;

    case Key::Ctrl | Key::Space:
        ToggleExpandRecursive(cc->currentSelectedItemHandle);
        {
            if (cc->filter.searchText.Len() > 0 && cc->filter.mode != TreeControlContext::FilterMode::None)
            {
                SearchItems();
            }
        }
        return true;
    case Key::Space:
        ToggleItem(cc->currentSelectedItemHandle);
        ProcessItemsToBeDrawn(InvalidItemHandle);
        if (cc->filter.searchText.Len() > 0 && cc->filter.mode != TreeControlContext::FilterMode::None)
        {
            SearchItems();
        }
        return true;

    case Key::Ctrl | Key::Left:
        cc->SelectColumnSeparator(-1);
        return true;
    case Key::Ctrl | Key::Right:
        cc->SelectColumnSeparator(1);
        return true;
    case Key::Escape:
        if (cc->filter.mode == TreeControlContext::FilterMode::Search)
        {
            if (cc->filter.searchText.Len() > 0)
            {
                cc->filter.searchText.Clear();
                SetColorForItems(cc->Cfg->Text.Normal);
                return true;
            }
        }
        else if (cc->filter.mode == TreeControlContext::FilterMode::Filter)
        {
            if (cc->filter.searchText.Len() > 0)
            {
                cc->filter.searchText.Clear();
                SetColorForItems(cc->Cfg->Text.Normal);
                ProcessItemsToBeDrawn(InvalidItemHandle);
                return true;
            }
        }

        if (cc->separatorIndexSelected != InvalidIndex)
        {
            cc->separatorIndexSelected = InvalidIndex;
            return true;
        }
        break;

    case Key::Left:
    case Key::Right:
        if (cc->separatorIndexSelected != InvalidIndex)
        {
            if (AddToColumnWidth(cc->separatorIndexSelected, keyCode == Key::Left ? -1 : 1))
            {
                return true;
            }
        }
        break;

    case Key::Ctrl | Key::C:
        if (const auto it = cc->items.find(cc->currentSelectedItemHandle); it != cc->items.end())
        {
            LocalUnicodeStringBuilder<1024> lusb;
            for (const auto& value : it->second.values)
            {
                if (lusb.Len() > 0)
                {
                    lusb.Add(" ");
                    lusb.Add(value);
                }
            }
            if (OS::Clipboard::SetText(lusb) == false)
            {
                const std::string input{ lusb };
                LOG_WARNING("Fail to copy string [%s] to the clipboard!", input.c_str());
            }
        }
        break;

    case Key::Backspace:
        if (cc->filter.mode != TreeControlContext::FilterMode::None)
        {
            if (cc->filter.searchText.Len() > 0)
            {
                cc->filter.searchText.Truncate(cc->filter.searchText.Len() - 1);
                SearchItems();
                return true;
            }
        }
        break;

    case Key::Ctrl | Key::Enter:
        if (cc->filter.mode == TreeControlContext::FilterMode::Search)
        {
            if (cc->filter.searchText.Len() > 0)
            {
                bool foundCurrent = false;

                for (const auto& handle : cc->orderedItems)
                {
                    if (foundCurrent == false)
                    {
                        foundCurrent = cc->currentSelectedItemHandle == handle;
                        continue;
                    }

                    const auto& item = cc->items[handle];
                    if (item.markedAsFound == true)
                    {
                        cc->currentSelectedItemHandle = handle;
                        return true;
                    }
                }

                // there's no next so go back to the first
                for (const auto& handle : cc->orderedItems)
                {
                    const auto& item = cc->items[handle];
                    if (item.markedAsFound == true)
                    {
                        cc->currentSelectedItemHandle = handle;
                        return true;
                    }
                }
            }
        }
        break;

    case Key::Ctrl | Key::Shift | Key::Enter:
        if (cc->filter.mode == TreeControlContext::FilterMode::Search)
        {
            if (cc->filter.searchText.Len() > 0)
            {
                bool foundCurrent = false;

                for (auto it = cc->orderedItems.rbegin(); it != cc->orderedItems.rend(); ++it)
                {
                    const auto handle = *it;
                    if (foundCurrent == false)
                    {
                        foundCurrent = cc->currentSelectedItemHandle == handle;
                        continue;
                    }

                    const auto& item = cc->items[handle];
                    if (item.markedAsFound == true)
                    {
                        cc->currentSelectedItemHandle = handle;
                        return true;
                    }
                }

                // there's no previous so go back to the last
                for (auto it = cc->orderedItems.rbegin(); it != cc->orderedItems.rend(); ++it)
                {
                    const auto handle = *it;
                    const auto& item  = cc->items[handle];
                    if (item.markedAsFound == true)
                    {
                        cc->currentSelectedItemHandle = handle;
                        return true;
                    }
                }
            }
        }
        break;

    default:
        break;
    }

    if (cc->filter.mode != TreeControlContext::FilterMode::None)
    {
        if (character > 0)
        {
            cc->filter.searchText.AddChar(character);
            SetColorForItems(cc->Cfg->Text.Normal);
            if (SearchItems() == false)
            {
                cc->filter.searchText.Truncate(cc->filter.searchText.Len() - 1);
                if (cc->filter.searchText.Len() > 0)
                {
                    SearchItems();
                }
                else
                {
                    cc->notProcessed = true;
                    SetColorForItems(cc->Cfg->Text.Normal);
                }
            }
            return true;
        }
    }

    if ((cc->treeFlags & TreeViewFlags::Sortable) != TreeViewFlags::None)
    {
        if (cc->filter.mode != TreeControlContext::FilterMode::Filter)
        {
            for (uint32 i = 0; i < cc->columns.size(); i++)
            {
                if (cc->columns[i].hotKeyCode == keyCode)
                {
                    cc->ColumnSort(i);
                    return true;
                }
            }
        }
    }

    return false;
}

void TreeView::OnFocus()
{
    CHECKRET(Context != nullptr, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    if (cc->itemsToDrew.size() > 0)
    {
        if (cc->currentSelectedItemHandle == InvalidItemHandle)
        {
            cc->currentSelectedItemHandle = cc->itemsToDrew[cc->offsetTopToDraw];
        }
    }
}

void TreeView::OnMousePressed(int x, int y, Input::MouseButton button)
{
    CHECKRET(Context != nullptr, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    switch (button)
    {
    case Input::MouseButton::None:
        break;
    case Input::MouseButton::Left:
        switch (cc->isMouseOn)
        {
        case TreeControlContext::IsMouseOn::Border:
            break;
        case TreeControlContext::IsMouseOn::ColumnHeader:
            if (cc->mouseOverColumnIndex != InvalidIndex)
            {
                cc->sortAscendent = !cc->sortAscendent;
                cc->ColumnSort(cc->mouseOverColumnIndex);
            }
            break;
        case TreeControlContext::IsMouseOn::ColumnSeparator:
            break;
        case TreeControlContext::IsMouseOn::ToggleSymbol:
        {
            const uint32 index    = y - 2;
            const auto itemHandle = cc->itemsToDrew[static_cast<size_t>(cc->offsetTopToDraw) + index];
            const auto it         = cc->items.find(itemHandle);
            ToggleItem(it->second.handle);
            if (it->second.expanded == false)
            {
                if (IsAncestorOfChild(it->second.handle, cc->currentSelectedItemHandle))
                {
                    cc->currentSelectedItemHandle = it->second.handle;
                }
            }
            ProcessItemsToBeDrawn(InvalidItemHandle);
            SearchItems();
        }
        break;
        case TreeControlContext::IsMouseOn::Item:
        {
            const uint32 index = y - 2;
            if (index >= cc->offsetBotToDraw || index >= cc->itemsToDrew.size())
            {
                break;
            }

            const auto itemHandle = cc->itemsToDrew[static_cast<size_t>(cc->offsetTopToDraw) + index];
            const auto it         = cc->items.find(itemHandle);

            if (x > static_cast<int>(it->second.depth * ItemSymbolOffset + ItemSymbolOffset) &&
                x < static_cast<int>(cc->Layout.Width))
            {
                cc->currentSelectedItemHandle = itemHandle;
            }
        }
        break;
        default:
            break;
        }
        break;
    case Input::MouseButton::Center:
        break;
    case Input::MouseButton::Right:
        break;
    case Input::MouseButton::DoubleClicked:
        break;
    default:
        break;
    }
}

bool TreeView::OnMouseOver(int x, int y)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    if (IsMouseOnBorder(x, y))
    {
        cc->isMouseOn = TreeControlContext::IsMouseOn::Border;
    }
    else if (IsMouseOnColumnSeparator(x, y))
    {
        cc->isMouseOn = TreeControlContext::IsMouseOn::ColumnSeparator;
    }
    else if (IsMouseOnToggleSymbol(x, y))
    {
        cc->isMouseOn = TreeControlContext::IsMouseOn::ToggleSymbol;
    }
    else if (IsMouseOnItem(x, y))
    {
        cc->isMouseOn = TreeControlContext::IsMouseOn::Item;
    }
    else if (IsMouseOnColumnHeader(x, y))
    {
        cc->isMouseOn = TreeControlContext::IsMouseOn::ColumnHeader;
    }
    else if (IsMouseOnSearchField(x, y))
    {
        cc->isMouseOn = TreeControlContext::IsMouseOn::SearchField;
    }
    else
    {
        cc->isMouseOn = TreeControlContext::IsMouseOn::None;
    }

    return false;
}

bool TreeView::OnMouseWheel(int /*x*/, int /*y*/, Input::MouseWheel direction)
{
    switch (direction)
    {
    case Input::MouseWheel::None:
        break;
    case Input::MouseWheel::Up:
        return MoveUp();
    case Input::MouseWheel::Down:
        return MoveDown();
    case Input::MouseWheel::Left:
        break;
    case Input::MouseWheel::Right:
        break;
    default:
        break;
    }

    return false;
}

bool TreeView::OnMouseDrag(int x, int, Input::MouseButton button)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    switch (button)
    {
    case Input::MouseButton::None:
        break;
    case Input::MouseButton::Left:
        switch (cc->isMouseOn)
        {
        case TreeControlContext::IsMouseOn::Border:
            break;
        case TreeControlContext::IsMouseOn::ColumnHeader:
            break;
        case TreeControlContext::IsMouseOn::ColumnSeparator:
            if (cc->mouseOverColumnSeparatorIndex != InvalidIndex)
            {
                const auto xs    = cc->columns[cc->mouseOverColumnSeparatorIndex].x;
                const auto w     = cc->columns[cc->mouseOverColumnSeparatorIndex].width;
                const auto delta = -(static_cast<int32>((xs + w)) - x);
                if (AddToColumnWidth(cc->mouseOverColumnSeparatorIndex, delta))
                {
                    return true;
                }
            }
            break;
        case TreeControlContext::IsMouseOn::ToggleSymbol:
            break;
        case TreeControlContext::IsMouseOn::Item:
            break;
        default:
            break;
        }
        break;
    case Input::MouseButton::Center:
        break;
    case Input::MouseButton::Right:
        break;
    case Input::MouseButton::DoubleClicked:
        break;
    default:
        break;
    }

    return false;
}

void TreeView::OnUpdateScrollBars()
{
    CHECKRET(Context != nullptr, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    const auto it     = find(cc->itemsToDrew.begin(), cc->itemsToDrew.end(), cc->currentSelectedItemHandle);
    const int64 index = it - cc->itemsToDrew.begin();
    UpdateVScrollBar(index, std::max<size_t>(cc->itemsToDrew.size() - 1, 0));
}

void TreeView::OnAfterResize(int newWidth, int newHeight)
{
    CHECKRET(AdjustElementsOnResize(newWidth, newHeight), "");
}

Handlers::TreeView* TreeView::Handlers()
{
    GET_CONTROL_HANDLERS(Handlers::TreeView);
}

ItemHandle TreeView::AddItem(
      const ItemHandle parent,
      std::initializer_list<ConstString> values,
      const ConstString metadata,
      bool process,
      bool isExpandable)
{
    CHECK(values.size() > 0, InvalidItemHandle, "");

    CHECK(Context != nullptr, InvalidItemHandle, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    std::vector<CharacterBuffer> cbvs;
    cbvs.reserve(values.size());
    CharacterBuffer cb;
    for (const auto& value : values)
    {
        cb.Set(value);
        cbvs.emplace_back(std::move(cb));
    }

    cc->items[cc->nextItemHandle]              = { parent, cc->nextItemHandle, std::move(cbvs) };
    cc->items[cc->nextItemHandle].isExpandable = isExpandable;
    CHECK(cc->items[cc->nextItemHandle].metadata.Set(metadata), false, "");

    if (parent == RootItemHandle)
    {
        cc->roots.emplace_back(cc->items[cc->nextItemHandle].handle);
    }
    else
    {
        auto& parentItem                    = cc->items[parent];
        cc->items[cc->nextItemHandle].depth = parentItem.depth + 1;
        parentItem.children.emplace_back(cc->items[cc->nextItemHandle].handle);
        parentItem.isExpandable = true;
    }

    if (cc->items.size() == 1)
    {
        cc->currentSelectedItemHandle = cc->items[cc->nextItemHandle].handle;
    }

    if (process)
    {
        ProcessItemsToBeDrawn(InvalidItemHandle);
    }
    else
    {
        cc->notProcessed = true;
    }

    return cc->items[cc->nextItemHandle++].handle;
}

bool TreeView::RemoveItem(const ItemHandle handle, bool process)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    std::queue<ItemHandle> ancestorRelated;
    ancestorRelated.push(handle);

    while (ancestorRelated.empty() == false)
    {
        ItemHandle current = ancestorRelated.front();
        ancestorRelated.pop();

        if (const auto it = cc->items.find(handle); it != cc->items.end())
        {
            for (const auto& handle : cc->items[current].children)
            {
                ancestorRelated.push(handle);
            }
            cc->items.erase(it);

            if (const auto rootIt = std::find(cc->roots.begin(), cc->roots.end(), handle); rootIt != cc->roots.end())
            {
                cc->roots.erase(rootIt);
            }
        }
    }

    if (process)
    {
        ProcessItemsToBeDrawn(InvalidItemHandle);
    }
    else
    {
        cc->notProcessed = true;
    }

    return true;
}

bool TreeView::ClearItems()
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    cc->items.clear();

    cc->nextItemHandle = 1ULL;

    cc->currentSelectedItemHandle = InvalidItemHandle;

    cc->roots.clear();

    ProcessItemsToBeDrawn(InvalidItemHandle);

    return true;
}

ItemHandle TreeView::GetCurrentItem()
{
    CHECK(Context != nullptr, InvalidItemHandle, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    return cc->currentSelectedItemHandle;
}

const ConstString TreeView::GetItemText(const ItemHandle handle)
{
    CHECK(Context != nullptr, u"", "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    const auto it = cc->items.find(handle);
    if (it != cc->items.end())
    {
        return it->second.values.at(0);
    }

    static const ConstString cs{ "u" };
    return cs;
}

GenericRef TreeView::GetItemDataAsPointer(const ItemHandle handle) const
{
    CHECK(Context != nullptr, nullptr, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    const auto it = cc->items.find(handle);
    if (it != cc->items.end())
    {
        if (std::holds_alternative<GenericRef>(it->second.data))
            return std::get<GenericRef>(it->second.data);
    }

    return nullptr;
}

uint64 TreeView::GetItemData(const size_t index, uint64 errorValue)
{
    CHECK(Context != nullptr, errorValue, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    auto it = cc->items.begin();
    std::advance(it, index);
    if (it != cc->items.end())
    {
        if (std::holds_alternative<uint64>(it->second.data))
            return std::get<uint64>(it->second.data);
    }

    return errorValue;
}

ItemHandle TreeView::GetItemHandleByIndex(const uint32 index) const
{
    CHECK(Context != nullptr, InvalidItemHandle, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);
    CHECK(index < cc->items.size(), InvalidItemHandle, "");

    auto it = cc->items.begin();
    std::advance(it, index);
    if (it != cc->items.end())
    {
        return it->second.handle;
    }

    return InvalidItemHandle;
}

bool TreeView::SetItemDataAsPointer(ItemHandle item, GenericRef value)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    auto it = cc->items.find(item);
    if (it == cc->items.end())
        return false;
    it->second.data = value;
    return true;
}

bool TreeView::SetItemData(ItemHandle item, uint64 value)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    auto it = cc->items.find(item);
    if (it == cc->items.end())
        return false;
    it->second.data = value;
    return true;
}

uint32 TreeView::GetItemsCount() const
{
    CHECK(Context != nullptr, 0, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);
    return static_cast<uint32>(cc->items.size());
}

bool TreeView::AddColumnData(
      const uint32 index,
      const ConstString title,
      const TextAlignament headerAlignment,
      const TextAlignament contentAlignment,
      const uint32 width)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    CHECK(index < cc->columns.size(), false, "");

    auto& column = cc->columns[index];

    CHECK(column.headerValue.SetWithHotKey(title, column.hotKeyOffset, column.hotKeyCode, Key::Ctrl), false, "");
    column.headerAlignment  = headerAlignment;
    column.contentAlignment = contentAlignment;
    if (width != 0xFFFFFFFF)
    {
        column.width       = std::max<>(width, MinColumnWidth);
        column.customWidth = true;

        // shifts columns
        uint32 currentX = column.x + column.width;
        for (auto i = index + 1; i < cc->columns.size(); i++)
        {
            auto& col       = cc->columns[i];
            col.customWidth = true;
            if (currentX < col.x)
            {
                const auto diff = col.x - currentX;
                col.x -= diff;
            }
            else
            {
                const auto diff = currentX - col.x;
                col.x += diff;
            }
            currentX = col.x + col.width + 1;
        }
    }

    // shift columns back if needed
    auto maxRightX = cc->Layout.Width - BorderOffset;
    for (auto i = static_cast<int>(cc->columns.size()) - 1; i >= 0; i--)
    {
        auto& col                = cc->columns[i];
        const auto currentRightX = col.x + col.width;
        if (currentRightX > maxRightX)
        {
            const auto diff = currentRightX - maxRightX;
            if (col.width > diff && col.width - diff >= MinColumnWidth)
            {
                col.width -= diff;
            }
            else
            {
                col.x -= diff;
            }
        }
        maxRightX = col.x;
    }

    return true;
}

bool TreeView::ToggleItem(const ItemHandle handle)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    auto& item = cc->items[handle];
    CHECK(item.isExpandable, true, "");

    if (cc->treeFlags && TreeViewFlags::DynamicallyPopulateNodeChildren)
    {
        for (const auto& child : item.children)
        {
            RemoveItem(child);
        }
        item.children.clear();
    }

    item.expanded = !item.expanded;

    if (item.expanded)
    {
        if (cc->treeFlags && TreeViewFlags::DynamicallyPopulateNodeChildren)
        {
            if (cc->handlers != nullptr)
            {
                auto handler = reinterpret_cast<Controls::Handlers::TreeView*>(cc->handlers.get());
                if (handler->OnTreeItemToggle.obj)
                {
                    handler->OnTreeItemToggle.obj->OnTreeItemToggle(this, handle);
                }
            }
        }
    }

    return true;
}

bool TreeView::IsMouseOnToggleSymbol(int x, int y) const
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    const uint32 index = y - 2;
    if (index >= cc->offsetBotToDraw || index >= cc->itemsToDrew.size())
    {
        return false;
    }

    const auto itemHandle = cc->itemsToDrew[static_cast<size_t>(cc->offsetTopToDraw) + index];
    const auto it         = cc->items.find(itemHandle);

    if (x > static_cast<int>(it->second.depth * ItemSymbolOffset + ItemSymbolOffset) &&
        x < static_cast<int>(cc->Layout.Width))
    {
        return false; // on item
    }

    if (x >= static_cast<int>(it->second.depth * ItemSymbolOffset) &&
        x < static_cast<int>(it->second.depth * ItemSymbolOffset + ItemSymbolOffset - 1U))
    {
        return true;
    }

    return false;
}

bool TreeView::IsMouseOnItem(int x, int y) const
{
    CHECK(Context != nullptr, false, "");
    const auto cc      = reinterpret_cast<TreeControlContext*>(Context);
    const uint32 index = y - 2;
    if (index >= cc->offsetBotToDraw || index >= cc->itemsToDrew.size())
    {
        return false;
    }

    const auto itemHandle = cc->itemsToDrew[static_cast<size_t>(cc->offsetTopToDraw) + index];
    const auto it         = cc->items.find(itemHandle);

    return (
          x > static_cast<int>(it->second.depth * ItemSymbolOffset + ItemSymbolOffset) &&
          x < static_cast<int>(cc->Layout.Width));
}

bool TreeView::IsMouseOnBorder(int x, int y) const
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    return (x == 0 || x == cc->Layout.Width - BorderOffset) || (y == 0 || y == cc->Layout.Width - BorderOffset);
}

bool TreeView::IsMouseOnColumnHeader(int x, int y) const
{
    CHECK(Context != nullptr, false, "");
    const auto cc            = reinterpret_cast<TreeControlContext*>(Context);
    cc->mouseOverColumnIndex = InvalidIndex;
    CHECK(x >= 0, false, "");
    CHECK(y == 1, false, "");

    CHECK((cc->treeFlags & TreeViewFlags::HideColumns) == TreeViewFlags::None, false, "");

    auto i                   = 0U;
    cc->mouseOverColumnIndex = InvalidIndex;
    for (auto& col : cc->columns)
    {
        if (static_cast<uint32>(x) >= col.x && static_cast<uint32>(x) <= col.x + col.width)
        {
            cc->mouseOverColumnIndex = i;
            return true;
        }
        i++;
    }

    return false;
}

bool TreeView::IsMouseOnColumnSeparator(int x, int y) const
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    cc->mouseOverColumnSeparatorIndex = InvalidIndex;
    for (auto i = 0U; i < cc->columns.size(); i++)
    {
        const auto& col = cc->columns[i];
        const auto xs   = col.x + col.width;

        if (xs == x)
        {
            cc->mouseOverColumnSeparatorIndex = i;
            return true;
        }
    }

    return false;
}

bool TreeView::IsMouseOnSearchField(int x, int y) const
{
    if (this->HasFocus())
    {
        if (y == GetHeight() - 1)
        {
            if (x > 0 && x < TreeSearchBarWidth)
            {
                return true;
            }
        }
    }

    return false;
}

bool TreeView::AdjustElementsOnResize(const int /*newWidth*/, const int /*newHeight*/)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    CHECK(AdjustItemsBoundsOnResize(), false, "");

    const uint32 width =
          (static_cast<uint32>(cc->Layout.Width)) / static_cast<uint32>(std::max<>(cc->columns.size(), size_t(1U)));

    uint32 xPreviousColumn       = 0;
    uint32 widthOfPreviousColumn = 0;
    for (auto i = 0U; i < cc->columns.size(); i++)
    {
        auto& col  = cc->columns[i];
        col.height = static_cast<uint32>(cc->Layout.Height - 2);
        col.x      = static_cast<uint32>(xPreviousColumn + widthOfPreviousColumn + BorderOffset);
        if (col.customWidth == false)
        {
            col.width = std::max<>(width, MinColumnWidth);
        }
        xPreviousColumn       = col.x;
        widthOfPreviousColumn = col.width;
    }

    if (cc->Layout.Width <= TreeScrollbarLeftOffset)
    {
        if (cc->hidSearchBarOnResize == false)
        {
            if ((cc->treeFlags & TreeViewFlags::HideSearchBar) != TreeViewFlags::None)
            {
                cc->filter.mode = TreeControlContext::FilterMode::None;
                cc->filter.searchText.Clear();
            }
            cc->hidSearchBarOnResize = true;
        }
    }
    else
    {
        if (cc->hidSearchBarOnResize)
        {
            cc->treeFlags ^= static_cast<uint32>(TreeViewFlags::HideSearchBar);
            cc->hidSearchBarOnResize = false;
        }

        if ((cc->treeFlags & TreeViewFlags::FilterSearch) != TreeViewFlags::None)
        {
            cc->filter.mode = TreeControlContext::FilterMode::Filter;
        }
        else if ((cc->treeFlags & TreeViewFlags::Searchable) != TreeViewFlags::None)
        {
            cc->filter.mode = TreeControlContext::FilterMode::Search;
        }
    }

    return true;
}

bool TreeView::AdjustItemsBoundsOnResize()
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    cc->maxItemsToDraw  = cc->Layout.Height - 1 - 1 - 1; // 0 - border top | 1 - column header | 2 - border bottom
    cc->offsetBotToDraw = cc->offsetTopToDraw + cc->maxItemsToDraw;

    return true;
}

bool TreeView::AddToColumnWidth(const uint32 columnIndex, const int32 value)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    auto& currentColumn = cc->columns[columnIndex];
    if (value < 0 && currentColumn.width == MinColumnWidth)
    {
        return true;
    }

    const auto newWidth = static_cast<int32>(currentColumn.width) + value;
    currentColumn.width = std::max<>(newWidth, static_cast<int32>(MinColumnWidth));

    auto previousX = currentColumn.x + currentColumn.width +
                     ((cc->treeFlags & TreeViewFlags::HideColumnsSeparator) == TreeViewFlags::None);
    for (auto i = columnIndex + 1; i < cc->columns.size(); i++)
    {
        auto& column = cc->columns[i];
        column.x     = previousX;
        previousX += (column.width + ((cc->treeFlags & TreeViewFlags::HideColumnsSeparator) == TreeViewFlags::None));
    }

    return true;
}

bool TreeView::SetColorForItems(const ColorPair& color)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    for (auto& item : cc->items)
    {
        for (auto& value : item.second.values)
        {
            value.SetColor(color);
        }
    }

    return true;
}

bool TreeView::SearchItems()
{
    bool found = false;

    MarkAllItemsAsNotFound();

    CHECK(Context != nullptr, found, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    std::set<ItemHandle> toBeExpanded;
    if (cc->filter.searchText.Len() > 0)
    {
        for (auto& item : cc->items)
        {
            for (auto& value : item.second.values)
            {
                if (const auto index = value.Find(cc->filter.searchText.ToStringView(), true); index >= 0)
                {
                    item.second.markedAsFound = true;
                    if (cc->filter.mode == TreeControlContext::FilterMode::Filter)
                    {
                        MarkAllAncestorsWithChildFoundInFilterSearch(item.second.handle);
                    }
                    if (found == false)
                    {
                        cc->currentSelectedItemHandle = item.second.handle;
                        SetColorForItems(cc->Cfg->Text.Normal);
                    }
                    found = true;
                    value.SetColor(index, index + cc->filter.searchText.Len(), cc->Cfg->Text.Highlighted);

                    ItemHandle ancestorHandle = item.second.parent;
                    do
                    {
                        if (const auto& it = cc->items.find(ancestorHandle); it != cc->items.end())
                        {
                            const auto& ancestor = it->second;
                            if (ancestor.isExpandable && ancestor.expanded == false &&
                                (cc->treeFlags & TreeViewFlags::DynamicallyPopulateNodeChildren) == TreeViewFlags::None)
                            {
                                toBeExpanded.insert(ancestorHandle);
                            }
                            ancestorHandle = ancestor.parent;
                        }
                        else
                        {
                            break;
                        }
                    } while (ancestorHandle != InvalidItemHandle);
                }
            }
        }
    }

    for (const auto handle : toBeExpanded)
    {
        ToggleItem(handle);
    }

    if (toBeExpanded.size() > 0 || cc->filter.mode == TreeControlContext::FilterMode::Filter)
    {
        ProcessItemsToBeDrawn(InvalidItemHandle);
    }

    cc->ProcessOrderedItems(InvalidItemHandle, true);

    return found;
}

bool TreeView::MarkAllItemsAsNotFound()
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    for (auto& [handle, item] : cc->items)
    {
        item.markedAsFound = false;

        if (cc->filter.mode == TreeControlContext::FilterMode::Filter)
        {
            item.hasAChildThatIsMarkedAsFound = false;
        }
    }

    return true;
}

bool TreeView::MarkAllAncestorsWithChildFoundInFilterSearch(const ItemHandle handle)
{
    CHECK(handle != InvalidItemHandle, false, "");
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);

    const auto& item          = cc->items[handle];
    ItemHandle ancestorHandle = item.parent;
    do
    {
        if (const auto& it = cc->items.find(ancestorHandle); it != cc->items.end())
        {
            auto& ancestor                        = it->second;
            ancestor.hasAChildThatIsMarkedAsFound = true;
            ancestorHandle                        = ancestor.parent;
        }
        else
        {
            break;
        }
    } while (ancestorHandle != InvalidItemHandle);

    return true;
}

const Utils::UnicodeStringBuilder& TreeView::GetItemMetadata(ItemHandle handle)
{
    CHECK(Context != nullptr, cb, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);
    if (cc->items.find(handle) == cc->items.end())
    {
        return cb;
    }

    auto& item = cc->items.at(handle);
    return item.metadata;
}

bool TreeView::SetItemMetadata(ItemHandle handle, const ConstString& metadata)
{
    CHECK(Context != nullptr, false, "");
    const auto cc = reinterpret_cast<TreeControlContext*>(Context);
    if (cc->items.find(handle) != cc->items.end())
    {
        return false;
    }

    auto& item = cc->items.at(handle);
    item.metadata.Set(metadata);

    return true;
}
} // namespace AppCUI::Controls

namespace AppCUI
{
void TreeControlContext::ColumnSort(uint32 columnIndex)
{
    if ((treeFlags & TreeViewFlags::Sortable) == TreeViewFlags::None)
    {
        columnIndexToSortBy = InvalidIndex;
        return;
    }

    if (columnIndex != columnIndexToSortBy)
    {
        SetSortColumn(columnIndex);
    }

    Sort();
}

void TreeControlContext::SetSortColumn(uint32 columnIndex)
{
    if ((treeFlags & TreeViewFlags::Sortable) == TreeViewFlags::None)
    {
        columnIndexToSortBy = InvalidIndex;
    }
    else
    {
        if (columnIndexToSortBy >= columns.size())
        {
            columnIndexToSortBy = InvalidIndex;
        }
        else
        {
            columnIndexToSortBy = columnIndex;
        }
    }
}

void TreeControlContext::SelectColumnSeparator(int32 offset)
{
    if (separatorIndexSelected == InvalidIndex)
    {
        separatorIndexSelected = 0;
        return;
    }

    separatorIndexSelected += offset;
    if (separatorIndexSelected < 0)
    {
        separatorIndexSelected = static_cast<int32>(columns.size() - 1);
    }
    else if (separatorIndexSelected >= columns.size())
    {
        separatorIndexSelected = 0;
    }
}

void TreeControlContext::Sort()
{
    SortByColumn(InvalidItemHandle);
    notProcessed = true;
}

bool TreeControlContext::SortByColumn(const ItemHandle handle)
{
    CHECK(columnIndexToSortBy != InvalidIndex, false, "");
    CHECK(items.size() > 0, false, "");

    const auto Comparator = [this](ItemHandle i1, ItemHandle i2) -> bool
    {
        const auto& a = items[i1];
        const auto& b = items[i2];

        const auto result = a.values[columnIndexToSortBy].CompareWith(b.values[columnIndexToSortBy], true);

        if (result == 0)
        {
            return false;
        }

        if (sortAscendent)
        {
            return result < 0;
        }
        else
        {
            return result > 0;
        }
    };

    if (handle == InvalidItemHandle)
    {
        std::sort(roots.begin(), roots.end(), Comparator);

        for (const auto& rootHandle : roots)
        {
            auto& root = items[rootHandle];
            std::sort(root.children.begin(), root.children.end(), Comparator);
            for (auto& childHandle : root.children)
            {
                SortByColumn(childHandle);
            }
        }
    }
    else
    {
        auto& item = items[handle];
        std::sort(item.children.begin(), item.children.end(), Comparator);
        for (auto& childHandle : item.children)
        {
            SortByColumn(childHandle);
        }
    }

    return true;
}

bool TreeControlContext::ProcessOrderedItems(const ItemHandle handle, const bool clear)
{
    if (clear)
    {
        orderedItems.clear();
        orderedItems.reserve(items.size());
    }

    CHECK(items.size() > 0, true, "");

    if (handle == InvalidItemHandle)
    {
        for (const auto& rootHandle : roots)
        {
            orderedItems.emplace_back(rootHandle);

            const auto& root = items[rootHandle];
            for (auto& childHandle : root.children)
            {
                ProcessOrderedItems(childHandle, false);
            }
        }
    }
    else
    {
        const auto& item = items[handle];
        orderedItems.emplace_back(item.handle);
        for (auto& childHandle : item.children)
        {
            ProcessOrderedItems(childHandle, false);
        }
    }

    return true;
}
} // namespace AppCUI
