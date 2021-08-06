#include "ControlContext.hpp"

using namespace AppCUI::Controls;
using namespace AppCUI::Console;
using namespace AppCUI::Input;

#define ITEM_FLAG_CHECKED    0x0001
#define ITEM_FLAG_SELECTED   0x0002
#define COLUMN_DONT_COPY     1
#define COLUMN_DONT_FILTER   2
#define INVALID_COLUMN_INDEX 0xFFFFFFFF
#define MINIM_COLUMN_WIDTH   3
#define MAXIM_COLUMN_WIDTH   256
#define NO_HOTKEY_FOR_COLUMN 0xFF

#define PREPARE_LISTVIEW_ITEM(index, returnValue)                                                                      \
    CHECK(index < Items.List.size(), returnValue, "Invalid index: %d", index);                                         \
    ListViewItem& i = Items.List[index];

#define WRAPPER ((ListViewControlContext*) this->Context)

AppCUI::Console::CharacterBuffer _temp_Buffer_;

void ListViewColumn::Reset()
{
    this->HotKeyCode   = Key::None;
    this->HotKeyOffset = NO_HOTKEY_FOR_COLUMN;
    this->Name[0]      = 0;
    this->NameLength   = 0;
    this->Flags        = 0;
    this->Align        = TextAlignament::Left;
    this->Width        = 10;
}
bool ListViewColumn::SetName(const char* text)
{
    CHECK(text, false, "Expecting a valid name for the column (non null)");

    this->HotKeyCode   = Key::None;
    this->HotKeyOffset = NO_HOTKEY_FOR_COLUMN;
    char* p            = this->Name;
    char* e            = p + MAX_LISTVIEW_HEADER_TEXT - 2;
    while ((p < e) && (*text))
    {
        if ((*text) == '&')
        {
            char hotKey = *(text + 1);
            if ((hotKey >= 'a') && (hotKey <= 'z'))
            {
                this->HotKeyCode   = (Key)(((unsigned int) Key::Ctrl) | ((unsigned int) Key::A + (hotKey - 'a')));
                this->HotKeyOffset = (unsigned char) (p - this->Name);
            }
            if ((hotKey >= 'A') && (hotKey <= 'Z'))
            {
                this->HotKeyCode   = (Key)(((unsigned int) Key::Ctrl) | ((unsigned int) Key::A + (hotKey - 'A')));
                this->HotKeyOffset = (unsigned char) (p - this->Name);
            }
            if ((hotKey >= '0') && (hotKey <= '9'))
            {
                this->HotKeyCode   = (Key)(((unsigned int) Key::Ctrl) | ((unsigned int) Key::N0 + (hotKey - '0')));
                this->HotKeyOffset = (unsigned char) (p - this->Name);
            }
            text++;
            continue;
        }
        *p = *text;
        p++;
        text++;
    }
    *p               = 0;
    this->NameLength = (unsigned char) (p - this->Name);
    return true;
}
bool ListViewColumn::SetAlign(TextAlignament align)
{
    if ((align == TextAlignament::Left) || (align == TextAlignament::Right) || (align == TextAlignament::Center))
    {
        this->Align = align;
        return true;
    }
    RETURNERROR(
          false,
          "align parameter can only be one of the following: TextAlignament::Left, TextAlignament::Right or "
          "TextAlignament::Center");
}
void ListViewColumn::SetWidth(unsigned int width)
{
    width       = MAXVALUE(width, MINIM_COLUMN_WIDTH);
    width       = MINVALUE(width, MAXIM_COLUMN_WIDTH);
    this->Width = (unsigned char) width;
}

ListViewItem::ListViewItem()
{
    this->Flags            = 0;
    this->Type             = ListViewItemType::REGULAR;
    this->ItemColor        = DefaultColorPair;
    this->Height           = 1;
    this->Data.UInt64Value = 0;
    this->XOffset          = 0;
}
ListViewItem::ListViewItem(const ListViewItem& obj)
{
    this->Flags     = obj.Flags;
    this->Type      = obj.Type;
    this->ItemColor = obj.ItemColor;
    this->Height    = obj.Height;
    this->Data      = obj.Data;
    this->XOffset   = obj.XOffset;
    for (unsigned int tr = 0; tr < MAX_LISTVIEW_COLUMNS; tr++)
    {
        this->SubItem[tr] = obj.SubItem[tr];
    }
}
ListViewItem::ListViewItem(ListViewItem&& obj) noexcept
{
    this->Flags     = obj.Flags;
    this->Type      = obj.Type;
    this->ItemColor = obj.ItemColor;
    this->Height    = obj.Height;
    this->Data      = obj.Data;
    this->XOffset   = obj.XOffset;
    for (unsigned int tr = 0; tr < MAX_LISTVIEW_COLUMNS; tr++)
    {
        this->SubItem[tr] = obj.SubItem[tr];
    }
}

ListViewItem* ListViewControlContext::GetFilteredItem(unsigned int index)
{
    unsigned int idx;
    CHECK(Items.Indexes.Get(index, idx), nullptr, "Fail to get index value for item with ID: %d", index);
    CHECK(idx < Items.List.size(), nullptr, "Invalid index (%d)", idx);
    return &Items.List[idx];
}

void ListViewControlContext::DrawColumnSeparatorsForResizeMode(Console::Renderer& renderer)
{
    int x                  = 1 - Columns.XOffset;
    ListViewColumn* column = this->Columns.List;
    for (unsigned int tr = 0; (tr < Columns.Count) && (x < (int) this->Layout.Width); tr++, column++)
    {
        x += column->Width;
        if (((Columns.ResizeModeEnabled) && (tr == Columns.ResizeColumnIndex)) ||
            (tr == Columns.HoverSeparatorColumnIndex))
            renderer.DrawVerticalLineWithSpecialChar(
                  x, 1, Layout.Height, SpecialChars::BoxVerticalSingleLine, Cfg->ListView.ColumnHover.Separator);
        x++;
    }
}
void ListViewControlContext::DrawColumn(Console::Renderer& renderer)
{
    auto* defaultCol = &this->Cfg->ListView.ColumnNormal;
    if (!(this->Flags & GATTR_ENABLE))
        defaultCol = &this->Cfg->ListView.ColumnInactive;
    auto* lvCol = defaultCol;

    renderer.DrawHorizontalLine(1, 1, Layout.Width - 2, ' ', defaultCol->Text);

    int x = 1 - Columns.XOffset;

    ListViewColumn* column = this->Columns.List;
    for (unsigned int tr = 0; (tr < Columns.Count) && (x < (int) this->Layout.Width); tr++, column++)
    {
        if (this->Focused)
        {
            if (tr == SortParams.ColumnIndex)
            {
                lvCol = &this->Cfg->ListView.ColumnSort;
                renderer.DrawHorizontalLineSize(x, 1, column->Width, ' ', lvCol->Text); // highlight the column
            }
            else if (tr == Columns.HoverColumnIndex)
            {
                lvCol = &this->Cfg->ListView.ColumnHover;
                renderer.DrawHorizontalLineSize(x, 1, column->Width, ' ', lvCol->Text); // highlight the column
            }
            else
                lvCol = defaultCol;
        }
        if ((column->HotKeyOffset == NO_HOTKEY_FOR_COLUMN) ||
            ((Flags & ListViewFlags::SORTABLE) == ListViewFlags::NONE))
            renderer.WriteSingleLineText(
                  x + 1, 1, column->Name, column->Width - 2, lvCol->Text, column->Align, column->NameLength);
        else
            renderer.WriteSingleLineTextWithHotKey(
                  x + 1,
                  1,
                  column->Name,
                  column->Width - 2,
                  lvCol->Text,
                  lvCol->HotKey,
                  column->HotKeyOffset,
                  column->Align,
                  column->NameLength);
        x += column->Width;
        if ((this->Focused) && (tr == SortParams.ColumnIndex))
            renderer.WriteSpecialCharacter(
                  x - 1,
                  1,
                  this->SortParams.Ascendent ? SpecialChars::TriangleUp : SpecialChars::TriangleDown,
                  lvCol->HotKey);

        if ((Flags & ListViewFlags::HIDE_COLUMNS_SEPARATORS) == ListViewFlags::NONE)
        {
            renderer.DrawVerticalLineWithSpecialChar(
                  x, 1, Layout.Height, SpecialChars::BoxVerticalSingleLine, defaultCol->Separator);
        }
        x++;
    }
}
void ListViewControlContext::DrawItem(Console::Renderer& renderer, ListViewItem* item, int y, bool currentItem)
{
    int x = 1 - Columns.XOffset;
    int itemStarts;
    ListViewColumn* column   = this->Columns.List;
    CharacterBuffer* subitem = item->SubItem;
    ColorPair itemCol        = Cfg->ListView.Item.Regular;
    ColorPair checkCol, uncheckCol;
    //WriteCharacterBufferParams params;
    if (Flags & GATTR_ENABLE)
    {
        checkCol   = Cfg->ListView.CheckedSymbol;
        uncheckCol = Cfg->ListView.UncheckedSymbol;
    }
    else
    {
        checkCol = uncheckCol = Cfg->ListView.InactiveColor;
    }
    // select color based on item type
    switch (item->Type)
    {
    case ListViewItemType::REGULAR:
        itemCol = item->ItemColor;
        break;
    case ListViewItemType::HIGHLIGHT:
        itemCol = Cfg->ListView.Item.Highligheted;
        break;
    case ListViewItemType::ERROR_INFORMATION:
        itemCol = Cfg->ListView.Item.Error;
        break;
    case ListViewItemType::WARNING_INFORMATION:
        itemCol = Cfg->ListView.Item.Warning;
        break;
    case ListViewItemType::INACTIVE:
        itemCol = Cfg->ListView.Item.Inactive;
        break;
    default:
        break;
    }
    // disable is not active
    if (!(Flags & GATTR_ENABLE))
        itemCol = Cfg->ListView.Item.Inactive;
    
    // prepare params
    //params.Flags = WriteCharacterBufferFlags::SINGLE_LINE | WriteCharacterBufferFlags::WRAP_TO_WIDTH |
    //               WriteCharacterBufferFlags::OVERWRITE_COLORS;
    //params.Color = itemCol;
    
    // first column
    int end_first_column = x + ((int) column->Width);
    x += (int) item->XOffset;
    if ((Flags & ListViewFlags::HAS_CHECKBOXES) != ListViewFlags::NONE)
    {
        if (x < end_first_column)
        {
            if (item->Flags & ITEM_FLAG_CHECKED)
                renderer.WriteSpecialCharacter(x, y, SpecialChars::CheckMark, checkCol);
            else
                renderer.WriteCharacter(x, y, 'x', uncheckCol);
        }
        x += 2;
    }
    itemStarts = x;
    if (x < end_first_column)
    {
        //renderer.WriteSingleLineText( x, y, subitem->GetText(), end_first_column - x, itemCol, column->Align, subitem->Len());
        //params.Width = end_first_column - x;
        //renderer.WriteCharacterBuffer(x, y, *subitem, params);    
        renderer.WriteCharacterBuffer(x, y, end_first_column - (x+1), *subitem, itemCol, column->Align);
    }

    // rest of the columns
    x = end_first_column + 1;
    subitem++;
    column++;
    for (unsigned int tr = 1; (tr < Columns.Count) && (x < (int) this->Layout.Width); tr++, column++)
    {
        //renderer.WriteSingleLineText(x, y, subitem->GetText(), column->Width, itemCol, column->Align, subitem->Len());
        //params.Width = column->Width;
        //renderer.WriteCharacterBuffer(x, y, *subitem, params);
        if ((column->Align & TextAlignament::Center) == TextAlignament::Center)
            renderer.WriteCharacterBuffer(x + column->Width/2, y, column->Width, *subitem, itemCol, column->Align);
        else if ((column->Align & TextAlignament::Right) == TextAlignament::Right)
            renderer.WriteCharacterBuffer(x + column->Width - 1, y, column->Width, *subitem, itemCol, column->Align);
        else
            renderer.WriteCharacterBuffer(x, y, column->Width-1, *subitem, itemCol, column->Align);
        x += column->Width;
        x++;
        subitem++;
    }
    if (Focused)
    {
        if (currentItem)
        {
            if (((Flags & ListViewFlags::MULTIPLE_SELECTION_MODE) != ListViewFlags::NONE) &&
                (item->Flags & ITEM_FLAG_SELECTED))
                renderer.DrawHorizontalLine(itemStarts, y, this->Layout.Width, -1, Cfg->ListView.FocusAndSelectedColor);
            else
                renderer.DrawHorizontalLine(itemStarts, y, this->Layout.Width, -1, Cfg->ListView.FocusColor);
            if ((Flags & ListViewFlags::HAS_CHECKBOXES) != ListViewFlags::NONE)
                renderer.SetCursor(itemStarts - 2, y); // point the cursor to the check/uncheck
        }
        else
        {
            if (((Flags & ListViewFlags::MULTIPLE_SELECTION_MODE) != ListViewFlags::NONE) &&
                (item->Flags & ITEM_FLAG_SELECTED))
                renderer.DrawHorizontalLine(itemStarts, y, this->Layout.Width, -1, Cfg->ListView.SelectionColor);
        }
    }
    else
    {
        if (Flags & GATTR_ENABLE)
        {
            if (((Flags & ListViewFlags::HIDECURRENTITEM) == ListViewFlags::NONE) && (currentItem))
                renderer.DrawHorizontalLine(itemStarts, y, this->Layout.Width, -1, Cfg->ListView.SelectionColor);
            if (((Flags & ListViewFlags::MULTIPLE_SELECTION_MODE) != ListViewFlags::NONE) &&
                (item->Flags & ITEM_FLAG_SELECTED))
                renderer.DrawHorizontalLine(itemStarts, y, this->Layout.Width, -1, Cfg->ListView.SelectionColor);
        }
    }
    if ((Flags & ListViewFlags::ITEM_SEPARATORS) != ListViewFlags::NONE)
    {
        y++;
        ColorPair col = this->Cfg->ListView.ColumnNormal.Separator;
        if (!(this->Flags & GATTR_ENABLE))
            col = this->Cfg->ListView.ColumnInactive.Separator;
        renderer.DrawHorizontalLineWithSpecialChar(1, y, Layout.Width - 2, SpecialChars::BoxHorizontalSingleLine, col);
        // draw crosses
        if ((Flags & ListViewFlags::HIDE_COLUMNS_SEPARATORS) == ListViewFlags::NONE)
        {
            x                      = 1 - Columns.XOffset;
            ListViewColumn* column = this->Columns.List;
            for (unsigned int tr = 0; (tr < Columns.Count) && (x < (int) this->Layout.Width); tr++, column++)
            {
                x += column->Width;
                renderer.WriteSpecialCharacter(x, y, SpecialChars::BoxCrossSingleLine, col);
                x++;
            }
        }
    }
}

void ListViewControlContext::Paint(Console::Renderer& renderer)
{
    int y       = 1;
    auto* lvCol = &this->Cfg->ListView.Normal;
    if (!(this->Flags & GATTR_ENABLE))
        lvCol = &this->Cfg->ListView.Inactive;
    else if (this->Focused)
        lvCol = &this->Cfg->ListView.Focused;
    else if (this->MouseIsOver)
        lvCol = &this->Cfg->ListView.Hover;

    renderer.DrawRectSize(0, 0, this->Layout.Width, this->Layout.Height, lvCol->Border, false);
    renderer.SetClipMargins(1, 1, 1, 1);
    if ((Flags & ListViewFlags::HIDE_COLUMNS) == ListViewFlags::NONE)
    {
        DrawColumn(renderer);
        y++;
    }
    unsigned int index = this->Items.FirstVisibleIndex;
    unsigned int count = this->Items.Indexes.Len();
    while ((y < this->Layout.Height) && (index < count))
    {
        ListViewItem* item = GetFilteredItem(index);
        DrawItem(renderer, item, y, index == this->Items.CurentItemIndex);
        y++;
        if ((Flags & ListViewFlags::ITEM_SEPARATORS) != ListViewFlags::NONE)
            y++;
        index++;
    }
    // columns separators
    if ((this->Focused) && ((Columns.HoverSeparatorColumnIndex != INVALID_COLUMN_INDEX) || (Columns.ResizeModeEnabled)))
        DrawColumnSeparatorsForResizeMode(renderer);

    // filtering & status
    if (Focused)
    {
        int x_ofs = 2;
        int yPoz  = ((int) this->Layout.Height) - 1;
        renderer.ResetClip();

        // search bar
        if ((this->Layout.Width > 20) && ((Flags & ListViewFlags::HIDE_SEARCH_BAR) == ListViewFlags::NONE))
        {
            renderer.DrawHorizontalLine(x_ofs, yPoz, 15, ' ', Cfg->ListView.FilterText);
            unsigned int len = this->Filter.SearchText.Len();
            const char* txt  = this->Filter.SearchText.GetText();
            if (len < 12)
            {
                renderer.WriteSingleLineText(3, yPoz, txt, Cfg->ListView.FilterText, len);
                if (Filter.FilterModeEnabled)
                    renderer.SetCursor(3 + len, yPoz);
            }
            else
            {
                renderer.WriteSingleLineText(3, yPoz, txt + (len - 12), Cfg->ListView.FilterText, 12);
                if (Filter.FilterModeEnabled)
                    renderer.SetCursor(3 + 12, yPoz);
            }
            x_ofs = 17;
        }
        // status information
        if ((this->Flags & ListViewFlags::MULTIPLE_SELECTION_MODE) != ListViewFlags::NONE)
        {
            renderer.WriteSingleLineText(
                  x_ofs, yPoz, this->Selection.Status, Cfg->ListView.StatusColor, this->Selection.StatusLength);
        }
    }
}
// coloane
bool ListViewControlContext::AddColumn(const char* text, TextAlignament Align, unsigned int width)
{
    CHECK(text != nullptr, false, "");
    CHECK(Columns.Count < MAX_LISTVIEW_COLUMNS, false, "");
    Columns.List[Columns.Count].Reset();
    CHECK(Columns.List[Columns.Count].SetName(text), false, "Fail to set column name: %s", text);
    CHECK(Columns.List[Columns.Count].SetAlign(Align), false, "Fail to set alignament to: %d", Align);
    Columns.List[Columns.Count].SetWidth(width);
    Columns.Count++;
    UpdateColumnsWidth();
    return true;
}
void ListViewControlContext::UpdateColumnsWidth()
{
    this->Columns.TotalWidth = 0;
    for (unsigned int tr = 0; tr < this->Columns.Count; tr++)
        this->Columns.TotalWidth += ((unsigned int) (this->Columns.List[tr].Width)) + 1;
}
bool ListViewControlContext::DeleteColumn(unsigned int index)
{
    CHECK(index < Columns.Count, false, "Invalid column index: %d (should be smaller than %d)", index, Columns.Count);
    for (unsigned int tr = index; tr < Columns.Count; tr++)
        Columns.List[tr] = Columns.List[tr + 1];
    Columns.Count--;
    UpdateColumnsWidth();
    return true;
}
void ListViewControlContext::DeleteAllColumns()
{
    Columns.Count = 0;
    UpdateColumnsWidth();
}
int ListViewControlContext::GetNrColumns()
{
    return Columns.Count;
}

ItemHandle ListViewControlContext::AddItem(const AppCUI::Utils::ConstString& text)
{
    ItemHandle idx = (unsigned int) Items.List.size();
    Items.List.push_back(ListViewItem(Cfg->ListView.Item.Regular));
    Items.Indexes.Push(idx);
    SetItemText(idx, 0, text);
    return idx;
}
bool ListViewControlContext::SetItemText(ItemHandle item, unsigned int subItem, const AppCUI::Utils::ConstString& text)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    CHECK(subItem < Columns.Count,
          false,
          "Invalid column index (%d), should be smaller than %d",
          subItem,
          Columns.Count);
    CHECK(i.SubItem[subItem].Set(text), false, "Fail to set text to a sub-item: %s", text);
    return true;
}
AppCUI::Console::CharacterBuffer* ListViewControlContext::GetItemText(ItemHandle item, unsigned int subItem)
{
    PREPARE_LISTVIEW_ITEM(item, nullptr);
    CHECK(subItem < Columns.Count,
          nullptr,
          "Invalid column index (%d), should be smaller than %d",
          subItem,
          Columns.Count);
    return &i.SubItem[subItem];
}
bool ListViewControlContext::SetItemCheck(ItemHandle item, bool check)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    if (check)
        i.Flags |= ITEM_FLAG_CHECKED;
    else
        i.Flags &= (0xFFFFFFFF - ITEM_FLAG_CHECKED);
    return true;
}
bool ListViewControlContext::SetItemSelect(ItemHandle item, bool check)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    if (check)
        i.Flags |= ITEM_FLAG_SELECTED;
    else
        i.Flags &= (0xFFFFFFFF - ITEM_FLAG_SELECTED);
    UpdateSelectionInfo();
    return true;
}
bool ListViewControlContext::SetItemColor(ItemHandle item, ColorPair color)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    CHECK(i.Type == ListViewItemType::REGULAR,
          false,
          "Item color only applies to regular item. Use SetItemType to change item type !");
    i.ItemColor = color;
    return true;
}
bool ListViewControlContext::SetItemType(ItemHandle item, ListViewItemType type)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    i.Type = type;
    return true;
}

bool ListViewControlContext::SetItemData(ItemHandle item, ItemData Data)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    i.Data = Data;
    return true;
}
ItemData* ListViewControlContext::GetItemData(ItemHandle item)
{
    PREPARE_LISTVIEW_ITEM(item, nullptr);
    return &i.Data;
}
bool ListViewControlContext::SetItemXOffset(ItemHandle item, unsigned int XOffset)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    i.XOffset = (unsigned int) XOffset;
    return true;
}
unsigned int ListViewControlContext::GetItemXOffset(ItemHandle item)
{
    PREPARE_LISTVIEW_ITEM(item, 0);
    return i.XOffset;
}
bool ListViewControlContext::SetItemHeight(ItemHandle item, unsigned int itemHeight)
{
    CHECK(itemHeight > 0, false, "Item height should be bigger than 0");
    PREPARE_LISTVIEW_ITEM(item, false);
    i.Height = itemHeight;
    return true;
}
unsigned int ListViewControlContext::GetItemHeight(ItemHandle item)
{
    PREPARE_LISTVIEW_ITEM(item, 0);
    return i.Height;
}
void ListViewControlContext::SetClipboardSeparator(char ch)
{
    clipboardSeparator = ch;
}
bool ListViewControlContext::SetColumnClipboardCopyState(unsigned int columnIndex, bool allowCopy)
{
    CHECK(columnIndex < Columns.Count,
          false,
          "Invalid column index: %d (should be smaller than %d)",
          columnIndex,
          Columns.Count);
    if (allowCopy)
        Columns.List[columnIndex].Flags -= Columns.List[columnIndex].Flags & COLUMN_DONT_COPY;
    else
        Columns.List[columnIndex].Flags |= COLUMN_DONT_COPY;
    return true;
}
bool ListViewControlContext::SetColumnFilterMode(unsigned int columnIndex, bool allowFilterForThisColumn)
{
    CHECK(columnIndex < Columns.Count,
          false,
          "Invalid column index (%d), should be smaller than %d",
          columnIndex,
          Columns.Count);
    if (allowFilterForThisColumn)
        Columns.List[columnIndex].Flags -= Columns.List[columnIndex].Flags & COLUMN_DONT_FILTER;
    else
        Columns.List[columnIndex].Flags |= COLUMN_DONT_FILTER;
    return true;
}
bool ListViewControlContext::IsItemChecked(ItemHandle item)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    return (bool) ((i.Flags & ITEM_FLAG_CHECKED) != 0);
}
bool ListViewControlContext::IsItemSelected(ItemHandle item)
{
    PREPARE_LISTVIEW_ITEM(item, false);
    return (bool) ((i.Flags & ITEM_FLAG_SELECTED) != 0);
}
void ListViewControlContext::SelectAllItems()
{
    UpdateSelection(0, Items.Indexes.Len(), true);
}
void ListViewControlContext::UnSelectAllItems()
{
    UpdateSelection(0, Items.Indexes.Len(), false);
}
void ListViewControlContext::CheckAllItems()
{
    unsigned int sz       = Items.Indexes.Len();
    unsigned int* indexes = Items.Indexes.GetUInt32Array();
    if (indexes == nullptr)
        return;
    for (unsigned int tr = 0; tr < sz; tr++, indexes++)
        SetItemCheck(*indexes, true);
}
void ListViewControlContext::UncheckAllItems()
{
    unsigned int sz       = Items.Indexes.Len();
    unsigned int* indexes = Items.Indexes.GetUInt32Array();
    if (indexes == nullptr)
        return;
    for (unsigned int tr = 0; tr < sz; tr++, indexes++)
        SetItemCheck(*indexes, false);
}
unsigned int ListViewControlContext::GetCheckedItemsCount()
{
    unsigned int count    = 0;
    unsigned int sz       = Items.Indexes.Len();
    unsigned int* indexes = Items.Indexes.GetUInt32Array();
    if (indexes == nullptr)
        return 0;
    for (unsigned int tr = 0; tr < sz; tr++, indexes++)
    {
        if ((*indexes) < sz)
        {
            if ((Items.List[*indexes].Flags & ITEM_FLAG_CHECKED) != 0)
                count++;
        }
    }

    return count;
}

bool ListViewControlContext::SetCurrentIndex(ItemHandle item)
{
    CHECK((unsigned int) item < Items.Indexes.Len(),
          false,
          "Invalid index: %d (should be smaller than %d)",
          item,
          Items.Indexes.Len());
    MoveTo((int) item);
    return true;
}
int ListViewControlContext::GetFirstVisibleLine()
{
    return Items.FirstVisibleIndex;
}
bool ListViewControlContext::SetFirstVisibleLine(ItemHandle item)
{
    if (SetCurrentIndex(item) == false)
        return false;
    Items.FirstVisibleIndex = Items.CurentItemIndex;
    return true;
}
void ListViewControlContext::DeleteAllItems()
{
    Items.List.clear();
    Items.Indexes.Clear();
    Columns.XOffset          = 0;
    Items.FirstVisibleIndex  = 0;
    Items.CurentItemIndex    = 0;
    Filter.FilterModeEnabled = false;
    Filter.SearchText.Clear();
}
// movement
int ListViewControlContext::GetVisibleItemsCount()
{
    int vis = Layout.Height - 3;
    if ((Flags & ListViewFlags::HIDE_COLUMNS) != ListViewFlags::NONE)
        vis++;
    int dim = 0, poz = Items.FirstVisibleIndex, nrItems = 0;
    int sz = (int) Items.Indexes.Len();
    while ((dim < vis) && (poz < sz))
    {
        ListViewItem* i = GetFilteredItem(poz);
        if (i)
            dim += i->Height;
        if ((Flags & ListViewFlags::ITEM_SEPARATORS) != ListViewFlags::NONE)
            dim++;
        nrItems++;
        poz++;
    }
    return nrItems;
}
void ListViewControlContext::UpdateSelectionInfo()
{
    unsigned int count = 0;
    unsigned int size  = (unsigned int) this->Items.List.size();
    for (unsigned int tr = 0; tr < size; tr++)
    {
        if (this->Items.List[tr].Flags & ITEM_FLAG_SELECTED)
            count++;
    }
    Utils::String tmp;
    while (true)
    {
        CHECKBK(
              tmp.Create(this->Selection.Status, sizeof(this->Selection.Status), true),
              "Fail to create selection string ");
        CHECKBK(tmp.Format("[%u/%u]", count, size), "Fail to format selection status string !");
        this->Selection.StatusLength = tmp.Len();
        return;
    }
    this->Selection.Status[0]    = 0;
    this->Selection.StatusLength = 0;
}
void ListViewControlContext::UpdateSelection(int start, int end, bool select)
{
    ListViewItem* i;
    int totalItems = Items.Indexes.Len();

    while ((start != end) && (start >= 0) && (start < totalItems))
    {
        i = GetFilteredItem(start);
        if (i != nullptr)
        {
            if (select)
                i->Flags |= ITEM_FLAG_SELECTED;
            else
                i->Flags &= (0xFFFFFFFF - ITEM_FLAG_SELECTED);
        }
        if (start < end)
            start++;
        else
            start--;
    }
    UpdateSelectionInfo();
}
void ListViewControlContext::MoveTo(int index)
{
    int count = Items.Indexes.Len();
    if (count <= 0)
        return;
    if (index >= count)
        index = count - 1;
    if (index < 0)
        index = 0;
    if (index == Items.CurentItemIndex)
        return;
    int vis               = GetVisibleItemsCount();
    int rel               = index - Items.FirstVisibleIndex;
    int originalPoz       = Items.CurentItemIndex;
    Items.CurentItemIndex = index;
    if (rel < 0)
        Items.FirstVisibleIndex = index;
    if (rel >= vis)
        Items.FirstVisibleIndex = (index - vis) + 1;
    if (originalPoz != index)
        SendMsg(Event::EVENT_LISTVIEW_CURRENTITEM_CHANGED);
}
bool ListViewControlContext::OnKeyEvent(AppCUI::Input::Key keyCode, char AsciiCode)
{
    Utils::String temp;
    ListViewItem* lvi;
    bool selected;

    if (Columns.ResizeModeEnabled)
    {
        Filter.FilterModeEnabled = false;
        switch (keyCode)
        {
        case Key::Ctrl | Key::Right:
            Columns.ResizeColumnIndex++;
            if (Columns.ResizeColumnIndex >= Columns.Count)
                Columns.ResizeColumnIndex = 0;
            return true;
        case Key::Ctrl | Key::Left:
            if (Columns.ResizeColumnIndex > 0)
                Columns.ResizeColumnIndex--;
            else
                Columns.ResizeColumnIndex = Columns.Count - 1;
            return true;
        case Key::Left:
            Columns.List[Columns.ResizeColumnIndex].SetWidth(
                  ((unsigned int) Columns.List[Columns.ResizeColumnIndex].Width) - 1);
            UpdateColumnsWidth();
            return true;
        case Key::Right:
            Columns.List[Columns.ResizeColumnIndex].SetWidth(
                  ((unsigned int) Columns.List[Columns.ResizeColumnIndex].Width) + 1);
            UpdateColumnsWidth();
            return true;
        };
        if ((AsciiCode > 0) || (keyCode != Key::None))
        {
            Columns.ResizeModeEnabled = false;
            Columns.ResizeColumnIndex = INVALID_COLUMN_INDEX;
            return true;
        }
    }
    else
    {
        if ((Flags & ListViewFlags::MULTIPLE_SELECTION_MODE) != ListViewFlags::NONE)
        {
            lvi = GetFilteredItem(Items.CurentItemIndex);
            if (lvi != nullptr)
                selected = ((lvi->Flags & ITEM_FLAG_SELECTED) != 0);
            else
                selected = false;
            switch (keyCode)
            {
            case Key::Shift | Key::Up:
                UpdateSelection(Items.CurentItemIndex, Items.CurentItemIndex - 1, !selected);
                MoveTo(Items.CurentItemIndex - 1);
                Filter.FilterModeEnabled = false;
                SendMsg(Event::EVENT_LISTVIEW_SELECTION_CHANGED);
                return true;
            case Key::Insert:
            case Key::Down | Key::Shift:
                UpdateSelection(Items.CurentItemIndex, Items.CurentItemIndex + 1, !selected);
                MoveTo(Items.CurentItemIndex + 1);
                Filter.FilterModeEnabled = false;
                SendMsg(Event::EVENT_LISTVIEW_SELECTION_CHANGED);
                return true;
            case Key::PageUp | Key::Shift:
                UpdateSelection(Items.CurentItemIndex, Items.CurentItemIndex - GetVisibleItemsCount(), !selected);
                MoveTo(Items.CurentItemIndex - GetVisibleItemsCount());
                Filter.FilterModeEnabled = false;
                SendMsg(Event::EVENT_LISTVIEW_SELECTION_CHANGED);
                return true;
            case Key::PageDown | Key::Shift:
                UpdateSelection(Items.CurentItemIndex, Items.CurentItemIndex + GetVisibleItemsCount(), !selected);
                MoveTo(Items.CurentItemIndex + GetVisibleItemsCount());
                Filter.FilterModeEnabled = false;
                SendMsg(Event::EVENT_LISTVIEW_SELECTION_CHANGED);
                return true;
            case Key::Home | Key::Shift:
                UpdateSelection(Items.CurentItemIndex, 0, !selected);
                MoveTo(0);
                Filter.FilterModeEnabled = false;
                SendMsg(Event::EVENT_LISTVIEW_SELECTION_CHANGED);
                return true;
            case Key::End | Key::Shift:
                UpdateSelection(Items.CurentItemIndex, Items.Indexes.Len(), !selected);
                MoveTo(Items.Indexes.Len());
                Filter.FilterModeEnabled = false;
                SendMsg(Event::EVENT_LISTVIEW_SELECTION_CHANGED);
                return true;
            };
        }
        switch (keyCode)
        {
        case Key::Up:
            MoveTo(Items.CurentItemIndex - 1);
            Filter.FilterModeEnabled = false;
            return true;
        case Key::Down:
            MoveTo(Items.CurentItemIndex + 1);
            Filter.FilterModeEnabled = false;
            return true;
        case Key::PageUp:
            MoveTo(Items.CurentItemIndex - GetVisibleItemsCount());
            Filter.FilterModeEnabled = false;
            return true;
        case Key::PageDown:
            MoveTo(Items.CurentItemIndex + GetVisibleItemsCount());
            Filter.FilterModeEnabled = false;
            return true;
        case Key::Left:
            if (Columns.XOffset > 0)
                Columns.XOffset--;
            Filter.FilterModeEnabled = false;
            return true;
        case Key::Right:
            UpdateColumnsWidth();
            Columns.XOffset          = MINVALUE(Columns.XOffset + 1, (int) Columns.TotalWidth);
            Filter.FilterModeEnabled = false;
            return true;
        case Key::Home:
            MoveTo(0);
            Filter.FilterModeEnabled = false;
            return true;
        case Key::End:
            MoveTo(((int) Items.Indexes.Len()) - 1);
            Filter.FilterModeEnabled = false;
            return true;
        case Key::Backspace:
            if ((Flags & ListViewFlags::HIDE_SEARCH_BAR) == ListViewFlags::NONE)
            {
                Filter.FilterModeEnabled = true;
                if (Filter.SearchText.Len() > 0)
                {
                    Filter.SearchText.Truncate(Filter.SearchText.Len() - 1);
                    UpdateSearch(0);
                }
            }
            return true;
        case Key::Space:
            if (!Filter.FilterModeEnabled)
            {
                if ((Flags & ListViewFlags::HAS_CHECKBOXES) == ListViewFlags::NONE)
                    return false;
                lvi = GetFilteredItem(Items.CurentItemIndex);
                if (lvi != nullptr)
                {
                    if ((lvi->Flags & ITEM_FLAG_CHECKED) != 0)
                        lvi->Flags -= ITEM_FLAG_CHECKED;
                    else
                        lvi->Flags |= ITEM_FLAG_CHECKED;
                }
                SendMsg(Event::EVENT_LISTVIEW_ITEM_CHECKED);
            }
            else
            {
                if ((Flags & ListViewFlags::HIDE_SEARCH_BAR) == ListViewFlags::NONE)
                {
                    Filter.SearchText.AddChar(' ');
                    UpdateSearch(0);
                }
            }
            return true;
        case Key::Enter | Key::Ctrl:
            if ((Filter.FilterModeEnabled) && ((Flags & ListViewFlags::HIDE_SEARCH_BAR) == ListViewFlags::NONE))
            {
                UpdateSearch(Items.CurentItemIndex + 1);
                return true; // de vazut daca are send
            }
            return false;
        case Key::Enter:
            SendMsg(Event::EVENT_LISTVIEW_ITEM_CLICKED);
            return true;
        case Key::Escape:
            if ((Flags & ListViewFlags::HIDE_SEARCH_BAR) == ListViewFlags::NONE)
            {
                if (Filter.FilterModeEnabled)
                {
                    if (Filter.SearchText.Len() > 0)
                    {
                        Filter.SearchText.Clear();
                        UpdateSearch(0);
                    }
                    else
                    {
                        Filter.FilterModeEnabled = false;
                    }
                    return true;
                }
                if ((Flags & ListViewFlags::SEARCHMODE) == ListViewFlags::NONE)
                {
                    if (Filter.SearchText.Len() > 0)
                    {
                        Filter.SearchText.Clear();
                        UpdateSearch(0);
                        return true;
                    }
                }
            }
            return false;

        case Key::Ctrl | Key::Right:
        case Key::Ctrl | Key::Left:
            Columns.ResizeColumnIndex = 0;
            Columns.ResizeModeEnabled = true;
            return true;
        case Key::Ctrl | Key::C:
        case Key::Ctrl | Key::Insert:
            //temp.Create(256);
            //if (Items.Indexes.Len() > 0)
            //{
            //    for (unsigned int tr = 0; tr < Columns.Count; tr++)
            //    {
            //        if ((Columns.List[tr].Flags & COLUMN_DONT_COPY) == 0)
            //        {
            //            temp.Add(GetFilteredItem(Items.CurentItemIndex)->SubItem[tr].GetText());
            //            if (clipboardSeparator != 0)
            //                temp.AddChar(clipboardSeparator);
            //        }
            //    }
            //    AppCUI::OS::Clipboard::SetText(temp);
            //}
            return true;
        case Key::Ctrl | Key::Alt | Key::Insert:
            //temp.Create(4096);
            //for (unsigned int gr = 0; gr < Items.Indexes.Len(); gr++)
            //{
            //    for (unsigned int tr = 0; tr < Columns.Count; tr++)
            //    {
            //        if ((Columns.List[tr].Flags & COLUMN_DONT_COPY) == 0)
            //        {
            //            temp.Add(GetFilteredItem(gr)->SubItem[tr].GetText());
            //            if (clipboardSeparator != 0)
            //                temp.AddChar(clipboardSeparator);
            //        }
            //    }
            //    temp.Add("\r\n");
            //}
            //AppCUI::OS::Clipboard::SetText(temp);
            return true;
        };
        // caut sort
        if ((Flags & ListViewFlags::HIDE_SEARCH_BAR) == ListViewFlags::NONE)
        {
            if (Filter.FilterModeEnabled == false)
            {
                for (unsigned int tr = 0; tr < Columns.Count; tr++)
                {
                    if (Columns.List[tr].HotKeyCode == keyCode)
                    {
                        ColumnSort(tr);
                        return true;
                    }
                }
            }
            // search mode
            if (AsciiCode > 0)
            {
                Filter.FilterModeEnabled = true;
                Filter.SearchText.AddChar(AsciiCode);
                UpdateSearch(0);
                return true;
            }
        }
    }
    return false;
}
bool ListViewControlContext::MouseToHeader(int x, int y, unsigned int& HeaderIndex, unsigned int& HeaderColumnIndex)
{
    int xx                 = 1 - Columns.XOffset;
    ListViewColumn* column = this->Columns.List;
    for (unsigned int tr = 0; tr < Columns.Count; tr++, column++)
    {
        if ((x >= xx) && (x <= xx + column->Width) && (x > 1) && (x < (this->Layout.Width - 2)))
        {
            if (x == (xx + column->Width))
            {
                HeaderColumnIndex = tr;
                HeaderIndex       = INVALID_COLUMN_INDEX;
            }
            else
            {
                HeaderColumnIndex = INVALID_COLUMN_INDEX;
                HeaderIndex       = tr;
            }
            return true;
        }
        xx += column->Width;
        xx++;
    }
    HeaderColumnIndex = INVALID_COLUMN_INDEX;
    HeaderIndex       = INVALID_COLUMN_INDEX;
    return false;
}
void ListViewControlContext::OnMouseReleased(int x, int y, int butonState)
{
    Columns.ResizeModeEnabled         = false;
    Columns.ResizeColumnIndex         = INVALID_COLUMN_INDEX;
    Columns.HoverSeparatorColumnIndex = INVALID_COLUMN_INDEX;
    Columns.HoverColumnIndex          = INVALID_COLUMN_INDEX;
}
void ListViewControlContext::OnMousePressed(int x, int y, int butonState)
{
    if (((Flags & ListViewFlags::HIDE_COLUMNS) == ListViewFlags::NONE))
    {
        unsigned int hIndex, hColumn;
        if (MouseToHeader(x, y, hIndex, hColumn))
        {
            if ((y == 1) && (hIndex != INVALID_COLUMN_INDEX))
            {
                ColumnSort(hIndex);
                return;
            }
            if (hColumn != INVALID_COLUMN_INDEX)
                return;
        }
    }
    // check is the search bar was pressed
    if ((this->Layout.Width > 20) && ((Flags & ListViewFlags::HIDE_SEARCH_BAR) == ListViewFlags::NONE) &&
        (y == (this->Layout.Height - 1)))
    {
        if ((x >= 2) && (x <= 15))
        {
            this->Filter.FilterModeEnabled = true;
            return;
        }
    }

    // check if items are pressed
    if ((Flags & ListViewFlags::HIDE_COLUMNS) != ListViewFlags::NONE)
        y--;
    else
        y -= 2;
    if ((Flags & ListViewFlags::ITEM_SEPARATORS) != ListViewFlags::NONE)
        y = y / 2;

    if (y < GetVisibleItemsCount())
    {
        this->Filter.FilterModeEnabled = false;
        MoveTo(y + Items.FirstVisibleIndex);
    }
}
bool ListViewControlContext::OnMouseDrag(int x, int y, int butonState)
{
    if (Columns.HoverSeparatorColumnIndex != INVALID_COLUMN_INDEX)
    {
        int xx                 = 1 - Columns.XOffset;
        ListViewColumn* column = this->Columns.List;
        for (unsigned int tr = 0; tr < Columns.HoverSeparatorColumnIndex; tr++, column++)
            xx += (((unsigned int) column->Width) + 1);
        // xx = the start of column
        if (x > xx)
        {
            column->SetWidth((unsigned int) (x - xx));
            UpdateColumnsWidth();
        }
        return true;
    }
    return false;
}
bool ListViewControlContext::OnMouseOver(int x, int y)
{
    if ((Flags & ListViewFlags::HIDE_COLUMNS) == ListViewFlags::NONE)
    {
        unsigned int hIndex, hColumn;
        MouseToHeader(x, y, hIndex, hColumn);
        if ((hIndex != Columns.HoverColumnIndex) && (y == 1) &&
            ((Flags & ListViewFlags::SORTABLE) != ListViewFlags::NONE))
        {
            Columns.HoverColumnIndex          = hIndex;
            Columns.HoverSeparatorColumnIndex = INVALID_COLUMN_INDEX;
            return true;
        }
        if (hColumn != Columns.HoverSeparatorColumnIndex)
        {
            Columns.HoverColumnIndex          = INVALID_COLUMN_INDEX;
            Columns.HoverSeparatorColumnIndex = hColumn;
            return true;
        }
    }
    return false;
}
// sort
void ListViewControlContext::SetSortColumn(unsigned int colIndex)
{
    if (colIndex >= Columns.Count)
        this->SortParams.ColumnIndex = INVALID_COLUMN_INDEX;
    else
        this->SortParams.ColumnIndex = colIndex;
    if ((Flags & ListViewFlags::SORTABLE) == ListViewFlags::NONE)
        this->SortParams.ColumnIndex = INVALID_COLUMN_INDEX;
}

void ListViewControlContext::ColumnSort(unsigned int columnIndex)
{
    if ((Flags & ListViewFlags::SORTABLE) == ListViewFlags::NONE)
    {
        this->SortParams.ColumnIndex = INVALID_COLUMN_INDEX;
        return;
    }
    if (columnIndex != SortParams.ColumnIndex)
        SetSortColumn(columnIndex);
    else
        SortParams.Ascendent = !SortParams.Ascendent;
    Sort();
}
//----------------------

int SortIndexesCompareFunction(unsigned int indx1, unsigned int indx2, void* context)
{
    ListViewControlContext* lvcc = (ListViewControlContext*) context;
    if (lvcc->SortParams.CompareCallbak)
    {
        return lvcc->SortParams.CompareCallbak(
              (ListView*) lvcc->Host,
              indx1,
              indx2,
              lvcc->SortParams.ColumnIndex,
              lvcc->SortParams.CompareCallbakContext);
    }
    else
    {
        unsigned int itemsCount = (unsigned int) lvcc->Items.List.size();
        if ((indx1 < itemsCount) && (indx2 < itemsCount) && (lvcc->SortParams.ColumnIndex != INVALID_COLUMN_INDEX))
        {
            return lvcc->Items.List[indx1].SubItem[lvcc->SortParams.ColumnIndex].CompareWith(
                  lvcc->Items.List[indx2].SubItem[lvcc->SortParams.ColumnIndex], true);
        }
        else
        {
            if (indx1 < indx2)
                return -1;
            else if (indx1 > indx2)
                return 1;
            else
                return 0;
        }
    }
}
bool ListViewControlContext::Sort()
{
    if (SortParams.CompareCallbak == nullptr)
    {
        // sanity check
        CHECK(SortParams.ColumnIndex < Columns.Count, false, "No sort column or custom sort function defined !");
    }
    Items.Indexes.Sort(SortIndexesCompareFunction, SortParams.Ascendent, this);
    return true;
}
int  ListViewControlContext::SearchItem(unsigned int startPoz, unsigned int colIndex)
{
    unsigned int originalStartPoz;
    ListViewItem* i;

    unsigned int count = Items.Indexes.Len();
    if (startPoz >= count)
        startPoz = 0;
    if (count == 0)
        return -1;
    originalStartPoz = startPoz;

    do
    {
        if ((i = GetFilteredItem(startPoz)) != nullptr)
        {
            if (i->SubItem[colIndex].Contains(Filter.SearchText.GetText(), true))
                return (int) startPoz;
        }
        startPoz++;
        if (startPoz >= count)
            startPoz = 0;

    } while (startPoz != originalStartPoz);
    return -1;
}
void ListViewControlContext::FilterItems()
{
    Items.Indexes.Clear();
    unsigned int count = (unsigned int) Items.List.size();
    if (this->Filter.SearchText.Len() == 0)
    {
        Items.Indexes.Reserve((unsigned int) Items.List.size());
        for (unsigned int tr = 0; tr < count; tr++)
            Items.Indexes.Push(tr);
    }
    else
    {
        for (unsigned int tr = 0; tr < count; tr++)
        {
            bool isOK         = false;
            ListViewItem& lvi = Items.List[tr];
            for (unsigned int gr = 0; gr < Columns.Count; gr++)
            {
                if ((Columns.List[gr].Flags & COLUMN_DONT_FILTER) != 0)
                    continue;
                if (lvi.SubItem[gr].Contains(this->Filter.SearchText.GetText(), true))
                {
                    isOK = true;
                    break;
                }
            }
            if (isOK)
                Items.Indexes.Push(tr);
        }
    }
    this->Items.FirstVisibleIndex = 0;
    this->Items.CurentItemIndex   = 0;
    SendMsg(Event::EVENT_LISTVIEW_CURRENTITEM_CHANGED);
}
void ListViewControlContext::UpdateSearch(int startPoz)
{
    int index;
    int cCol = 0;

    // daca fac filtrare
    if ((Flags & ListViewFlags::SEARCHMODE) == ListViewFlags::NONE)
    {
        FilterItems();
    }
    else
    {
        if (SortParams.ColumnIndex < Columns.Count)
            cCol = SortParams.ColumnIndex;

        if ((index = SearchItem(startPoz, cCol)) >= 0)
        {
            MoveTo(index);
        }
        else
        {
            if (Filter.SearchText.Len() > 0)
                Filter.SearchText.Truncate(Filter.SearchText.Len() - 1);
        }
    }
}
void ListViewControlContext::SendMsg(Event eventType)
{
    Host->RaiseEvent(eventType);
}

//=====================================================================================================
ListView::~ListView()
{
    DeleteAllItems();
    DeleteAllColumns();
    DELETE_CONTROL_CONTEXT(ListViewControlContext);
}
bool ListView::Create(Control* parent, const std::string_view& layout, ListViewFlags flags)
{
    CONTROL_INIT_CONTEXT(ListViewControlContext);
    CREATE_TYPECONTROL_CONTEXT(ListViewControlContext, Members, false);
    Members->Layout.MinWidth  = 5;
    Members->Layout.MinHeight = 3;
    CHECK(Init(parent, "", layout, false), false, "Failed to create list view !");
    Members->Flags =
          GATTR_ENABLE | GATTR_VISIBLE | GATTR_TABSTOP | GATTR_HSCROLL | GATTR_VSCROLL | (unsigned int) flags;
    Members->ScrollBars.LeftMargin = 25;
    // allocate items
    CHECK(Members->Items.Indexes.Create(32), false, "Fail to allocate indexes");
    Members->Items.List.reserve(32);

    // initialize
    Members->Columns.Count                     = 0;
    Members->Columns.XOffset                   = 0;
    Members->Items.FirstVisibleIndex           = 0;
    Members->Items.CurentItemIndex             = 0;
    Members->SortParams.ColumnIndex            = INVALID_COLUMN_INDEX;
    Members->Columns.HoverColumnIndex          = INVALID_COLUMN_INDEX;
    Members->Columns.HoverSeparatorColumnIndex = INVALID_COLUMN_INDEX;
    Members->SortParams.Ascendent              = true;
    Members->SortParams.CompareCallbak         = nullptr;
    Members->SortParams.CompareCallbakContext  = nullptr;
    Members->Columns.ResizeModeEnabled         = false;
    Members->Filter.FilterModeEnabled          = false;
    Members->Columns.ResizeColumnIndex         = INVALID_COLUMN_INDEX;
    Members->clipboardSeparator                = '\t';
    Members->Columns.TotalWidth                = 0;
    Members->Host                              = this;
    Members->Filter.SearchText.Clear();
    Members->Selection.Status[0]    = 0;
    Members->Selection.StatusLength = 0;

    // all is good
    return true;
}
void ListView::Paint(Console::Renderer& renderer)
{
    CREATE_TYPECONTROL_CONTEXT(ListViewControlContext, Members, );
    Members->Paint(renderer);
}
bool ListView::OnKeyEvent(AppCUI::Input::Key keyCode, char AsciiCode)
{
    return WRAPPER->OnKeyEvent(keyCode, AsciiCode);
}
void ListView::OnUpdateScrollBars()
{
    CREATE_TYPECONTROL_CONTEXT(ListViewControlContext, Members, );
    // compute XOfsset
    unsigned int left_margin = 2;
    if ((Members->Flags & ListViewFlags::HIDE_SEARCH_BAR) == ListViewFlags::NONE)
        left_margin = 17;
    if ((Members->Flags & ListViewFlags::MULTIPLE_SELECTION_MODE) != ListViewFlags::NONE)
        left_margin += (Members->Selection.StatusLength + 1);

    Members->ScrollBars.LeftMargin = left_margin;
    UpdateHScrollBar(Members->Columns.XOffset, Members->Columns.TotalWidth);
    unsigned int count = Members->Items.Indexes.Len();
    if (count > 0)
        count--;
    UpdateVScrollBar(Members->Items.CurentItemIndex, count);
}

bool ListView::AddColumn(const char* text, TextAlignament Align, unsigned int Size)
{
    return WRAPPER->AddColumn(text, Align, Size);
}
bool ListView::SetColumnText(unsigned int columnIndex, const char* text)
{
    CHECK(columnIndex < WRAPPER->Columns.Count,
          false,
          "Invalid column index:%d (should be smaller than %d)",
          columnIndex,
          WRAPPER->Columns.Count);
    return WRAPPER->Columns.List[columnIndex].SetName(text);
}
bool ListView::SetColumnAlignament(unsigned int columnIndex, TextAlignament Align)
{
    CHECK(columnIndex < WRAPPER->Columns.Count,
          false,
          "Invalid column index:%d (should be smaller than %d)",
          columnIndex,
          WRAPPER->Columns.Count);
    return WRAPPER->Columns.List[columnIndex].SetAlign(Align);
}
bool ListView::SetColumnWidth(unsigned int columnIndex, unsigned int width)
{
    CHECK(columnIndex < WRAPPER->Columns.Count,
          false,
          "Invalid column index:%d (should be smaller than %d)",
          columnIndex,
          WRAPPER->Columns.Count);
    WRAPPER->Columns.List[columnIndex].SetWidth(width);
    WRAPPER->UpdateColumnsWidth();
    return true;
}
bool ListView::SetColumnClipboardCopyState(unsigned int columnIndex, bool allowCopy)
{
    return WRAPPER->SetColumnClipboardCopyState(columnIndex, allowCopy);
}
bool ListView::SetColumnFilterMode(unsigned int columnIndex, bool allowFilterForThisColumn)
{
    return WRAPPER->SetColumnFilterMode(columnIndex, allowFilterForThisColumn);
}
bool ListView::DeleteColumn(unsigned int columnIndex)
{
    return WRAPPER->DeleteColumn(columnIndex);
}
void ListView::DeleteAllColumns()
{
    if (Context != nullptr)
    {
        WRAPPER->DeleteAllColumns();
    }
}
unsigned int ListView::GetColumnsCount()
{
    return WRAPPER->GetNrColumns();
}

ItemHandle ListView::AddItem(const AppCUI::Utils::ConstString& text)
{
    return WRAPPER->AddItem(text);
}
ItemHandle ListView::AddItem(const AppCUI::Utils::ConstString& text, const AppCUI::Utils::ConstString& subItem1)
{
    int handle = WRAPPER->AddItem(text);
    CHECK(handle != InvalidItemHandle, InvalidItemHandle, "Fail to allocate item for ListView");
    CHECK(WRAPPER->SetItemText(handle, 1, subItem1), InvalidItemHandle, "");
    return handle;
}
ItemHandle ListView::AddItem(const AppCUI::Utils::ConstString& text, const AppCUI::Utils::ConstString& subItem1, const AppCUI::Utils::ConstString& subItem2)
{
    ItemHandle handle = WRAPPER->AddItem(text);
    CHECK(handle != InvalidItemHandle, InvalidItemHandle, "Fail to allocate item for ListView");
    CHECK(WRAPPER->SetItemText(handle, 1, subItem1), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 2, subItem2), InvalidItemHandle, "");
    return handle;
}
ItemHandle ListView::AddItem(const AppCUI::Utils::ConstString& text, const AppCUI::Utils::ConstString& subItem1, const AppCUI::Utils::ConstString& subItem2, const AppCUI::Utils::ConstString& subItem3)
{
    ItemHandle handle = WRAPPER->AddItem(text);
    CHECK(handle != InvalidItemHandle, InvalidItemHandle, "Fail to allocate item for ListView");
    CHECK(WRAPPER->SetItemText(handle, 1, subItem1), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 2, subItem2), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 3, subItem3), InvalidItemHandle, "");
    return handle;
}
ItemHandle ListView::AddItem(
      const AppCUI::Utils::ConstString& text, const AppCUI::Utils::ConstString& subItem1, const AppCUI::Utils::ConstString& subItem2, const AppCUI::Utils::ConstString& subItem3, const AppCUI::Utils::ConstString& subItem4)
{
    ItemHandle handle = WRAPPER->AddItem(text);
    CHECK(handle != InvalidItemHandle, InvalidItemHandle, "Fail to allocate item for ListView");
    CHECK(WRAPPER->SetItemText(handle, 1, subItem1), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 2, subItem2), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 3, subItem3), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 4, subItem4), InvalidItemHandle, "");
    return handle;
}
ItemHandle ListView::AddItem(
      const AppCUI::Utils::ConstString& text,
      const AppCUI::Utils::ConstString& subItem1,
      const AppCUI::Utils::ConstString& subItem2,
      const AppCUI::Utils::ConstString& subItem3,
      const AppCUI::Utils::ConstString& subItem4,
      const AppCUI::Utils::ConstString& subItem5)
{
    ItemHandle handle = WRAPPER->AddItem(text);
    CHECK(handle != InvalidItemHandle, InvalidItemHandle, "Fail to allocate item for ListView");
    CHECK(WRAPPER->SetItemText(handle, 1, subItem1), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 2, subItem2), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 3, subItem3), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 4, subItem4), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 5, subItem5), InvalidItemHandle, "");
    return handle;
}
ItemHandle ListView::AddItem(
      const AppCUI::Utils::ConstString& text,
      const AppCUI::Utils::ConstString& subItem1,
      const AppCUI::Utils::ConstString& subItem2,
      const AppCUI::Utils::ConstString& subItem3,
      const AppCUI::Utils::ConstString& subItem4,
      const AppCUI::Utils::ConstString& subItem5,
      const AppCUI::Utils::ConstString& subItem6)
{
    ItemHandle handle = WRAPPER->AddItem(text);
    CHECK(handle != InvalidItemHandle, InvalidItemHandle, "Fail to allocate item for ListView");
    CHECK(WRAPPER->SetItemText(handle, 1, subItem1), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 2, subItem2), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 3, subItem3), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 4, subItem4), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 5, subItem5), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 6, subItem6), InvalidItemHandle, "");
    return handle;
}
ItemHandle ListView::AddItem(
      const AppCUI::Utils::ConstString& text,
      const AppCUI::Utils::ConstString& subItem1,
      const AppCUI::Utils::ConstString& subItem2,
      const AppCUI::Utils::ConstString& subItem3,
      const AppCUI::Utils::ConstString& subItem4,
      const AppCUI::Utils::ConstString& subItem5,
      const AppCUI::Utils::ConstString& subItem6,
      const AppCUI::Utils::ConstString& subItem7)
{
    ItemHandle handle = WRAPPER->AddItem(text);
    CHECK(handle != InvalidItemHandle, InvalidItemHandle, "Fail to allocate item for ListView");
    CHECK(WRAPPER->SetItemText(handle, 1, subItem1), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 2, subItem2), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 3, subItem3), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 4, subItem4), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 5, subItem5), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 6, subItem6), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 7, subItem7), InvalidItemHandle, "");
    return handle;
}
ItemHandle ListView::AddItem(
      const AppCUI::Utils::ConstString& text,
      const AppCUI::Utils::ConstString& subItem1,
      const AppCUI::Utils::ConstString& subItem2,
      const AppCUI::Utils::ConstString& subItem3,
      const AppCUI::Utils::ConstString& subItem4,
      const AppCUI::Utils::ConstString& subItem5,
      const AppCUI::Utils::ConstString& subItem6,
      const AppCUI::Utils::ConstString& subItem7,
      const AppCUI::Utils::ConstString& subItem8)
{
    ItemHandle handle = WRAPPER->AddItem(text);
    CHECK(handle != InvalidItemHandle, InvalidItemHandle, "Fail to allocate item for ListView");
    CHECK(WRAPPER->SetItemText(handle, 1, subItem1), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 2, subItem2), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 3, subItem3), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 4, subItem4), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 5, subItem5), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 6, subItem6), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 7, subItem7), InvalidItemHandle, "");
    CHECK(WRAPPER->SetItemText(handle, 8, subItem8), InvalidItemHandle, "");
    return handle;
}

bool ListView::SetItemText(ItemHandle item, unsigned int subItem, const AppCUI::Utils::ConstString& text)
{
    return WRAPPER->SetItemText(item, subItem, text);
}
const AppCUI::Console::CharacterBuffer& ListView::GetItemText(ItemHandle item, unsigned int subItemIndex)
{
    auto obj = WRAPPER->GetItemText(item, subItemIndex);
    if (obj)
        return *obj;
    else
    {
        _temp_Buffer_.Destroy();
        return _temp_Buffer_;
    }
}
bool ListView::SetItemCheck(ItemHandle item, bool check)
{
    return WRAPPER->SetItemCheck(item, check);
}
bool ListView::SetItemSelect(ItemHandle item, bool select)
{
    return WRAPPER->SetItemSelect(item, select);
}
bool ListView::SetItemColor(ItemHandle item, ColorPair col)
{
    return WRAPPER->SetItemColor(item, col);
}
bool ListView::SetItemType(ItemHandle item, ListViewItemType type)
{
    return WRAPPER->SetItemType(item, type);
}
bool ListView::IsItemChecked(ItemHandle item)
{
    return WRAPPER->IsItemChecked(item);
}
bool ListView::IsItemSelected(ItemHandle item)
{
    return WRAPPER->IsItemSelected(item);
}
bool ListView::SetItemData(ItemHandle item, ItemData Data)
{
    return WRAPPER->SetItemData(item, Data);
}
ItemData* ListView::GetItemData(ItemHandle item)
{
    return WRAPPER->GetItemData(item);
}
bool ListView::SetItemXOffset(ItemHandle item, unsigned int XOffset)
{
    return WRAPPER->SetItemXOffset(item, XOffset);
}
unsigned int ListView::GetItemXOffset(ItemHandle item)
{
    return WRAPPER->GetItemXOffset(item);
}
bool ListView::SetItemHeight(ItemHandle item, unsigned int Height)
{
    return WRAPPER->SetItemHeight(item, Height);
}
unsigned int ListView::GetItemHeight(ItemHandle item)
{
    return WRAPPER->GetItemHeight(item);
}
void ListView::DeleteAllItems()
{
    if (Context != nullptr)
    {
        WRAPPER->DeleteAllItems();
    }
}
unsigned int ListView::GetItemsCount()
{
    if (Context != nullptr)
    {
        return (unsigned int) WRAPPER->Items.List.size();
    }
    return 0;
}
unsigned int ListView::GetCurrentItem()
{
    ListViewControlContext* lvcc = ((ListViewControlContext*) this->Context);
    if (lvcc->Items.CurentItemIndex < 0)
        return -1;
    unsigned int* indexes = lvcc->Items.Indexes.GetUInt32Array();
    return indexes[lvcc->Items.CurentItemIndex];
}
bool ListView::SetCurrentItem(ItemHandle item)
{
    ListViewControlContext* lvcc = ((ListViewControlContext*) this->Context);
    unsigned int* indexes        = lvcc->Items.Indexes.GetUInt32Array();
    unsigned int count           = lvcc->Items.Indexes.Len();
    if (count <= 0)
        return false;
    // caut indexul
    for (unsigned int tr = 0; tr < count; tr++, indexes++)
    {
        if ((*indexes) == item)
            return WRAPPER->SetCurrentIndex(tr);
    }
    return false;
}
void ListView::SelectAllItems()
{
    WRAPPER->SelectAllItems();
}
void ListView::UnSelectAllItems()
{
    WRAPPER->UnSelectAllItems();
}
void ListView::CheckAllItems()
{
    WRAPPER->CheckAllItems();
}
void ListView::UncheckAllItems()
{
    WRAPPER->UncheckAllItems();
}
unsigned int ListView::GetCheckedItemsCount()
{
    return WRAPPER->GetCheckedItemsCount();
}

void ListView::SetClipboardSeparator(char ch)
{
    WRAPPER->SetClipboardSeparator(ch);
}
void ListView::OnMouseReleased(int x, int y, int butonState)
{
    WRAPPER->OnMouseReleased(x, y, butonState);
}
void ListView::OnMousePressed(int x, int y, int butonState)
{
    WRAPPER->OnMousePressed(x, y, butonState);
}
bool ListView::OnMouseDrag(int x, int y, int butonState)
{
    return WRAPPER->OnMouseDrag(x, y, butonState);
}
bool ListView::OnMouseOver(int x, int y)
{
    return WRAPPER->OnMouseOver(x, y);
}
bool ListView::OnMouseLeave()
{
    WRAPPER->Columns.HoverSeparatorColumnIndex = INVALID_COLUMN_INDEX;
    WRAPPER->Columns.HoverColumnIndex          = INVALID_COLUMN_INDEX;
    return true;
}
void ListView::OnFocus()
{
    WRAPPER->Columns.HoverSeparatorColumnIndex = INVALID_COLUMN_INDEX;
    WRAPPER->Columns.HoverColumnIndex          = INVALID_COLUMN_INDEX;
    WRAPPER->Filter.FilterModeEnabled          = false;
    if ((WRAPPER->Flags & ListViewFlags::MULTIPLE_SELECTION_MODE) != ListViewFlags::NONE)
        WRAPPER->UpdateSelectionInfo();
}
void ListView::SetItemCompareFunction(Handlers::ListViewItemComparer fnc, void* Context)
{
    WRAPPER->SortParams.CompareCallbak        = fnc;
    WRAPPER->SortParams.CompareCallbakContext = Context;
}
bool ListView::Sort()
{
    return WRAPPER->Sort();
}
bool ListView::Sort(unsigned int columnIndex, bool ascendent)
{
    CHECK(columnIndex < WRAPPER->Columns.Count,
          false,
          "Invalid column index (%d). Should be smaller than %d",
          columnIndex,
          WRAPPER->Columns.Count);
    CHECK((WRAPPER->Flags & ListViewFlags::SORTABLE) != ListViewFlags::NONE,
          false,
          "Can not sort items (have you set 'ListViewFlags::SORTABLE' when creating ListView ?)");
    WRAPPER->SortParams.ColumnIndex = columnIndex;
    WRAPPER->SortParams.Ascendent   = ascendent;
    return WRAPPER->Sort();
}
bool ListView::Reserve(unsigned int itemsCount)
{
    WRAPPER->Items.List.reserve(itemsCount);
    return WRAPPER->Items.Indexes.Reserve(itemsCount);
}
