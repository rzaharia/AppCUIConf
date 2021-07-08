#include "AppCUI.h"

using namespace AppCUI::Console;

void AppCUI::Application::Config::SetDarkTheme()
{
    this->Desktop.Color = ColorPair{ Color::Gray, Color::Black };
    this->Desktop.DesktopFillCharacterCode = 186;

    this->CommandBar.BackgroundColor = ColorPair{ Color::Black,Color::White };
    this->CommandBar.ShiftKeysColor = ColorPair{ Color::Gray, Color::White };
    this->CommandBar.Normal.KeyColor = ColorPair{ Color::DarkRed, Color::White };
    this->CommandBar.Normal.NameColor = ColorPair{ Color::Black, Color::White };
    this->CommandBar.Hover.KeyColor = ColorPair{ Color::DarkRed, Color::Silver };
    this->CommandBar.Hover.NameColor = ColorPair{ Color::Black, Color::Silver };
    this->CommandBar.Pressed.KeyColor = ColorPair{ Color::White, Color::Magenta };
    this->CommandBar.Pressed.NameColor = ColorPair{ Color::Yellow, Color::Magenta };

    this->Window.ActiveColor = ColorPair{ Color::White, Color::DarkBlue };
    this->Window.InactiveColor = ColorPair{ Color::Silver, Color::Black };
    this->Window.TitleActiveColor = ColorPair{Color::Yellow, Color::DarkBlue};
    this->Window.TitleInactiveColor = ColorPair{Color::Silver, Color::Black};
    this->Window.ControlButtonColor = ColorPair{Color::Aqua, Color::DarkBlue};
    this->Window.ControlButtonHoverColor = ColorPair{Color::Black, Color::Aqua};
    this->Window.ControlButtonPressedColor = ColorPair{Color::Black, Color::Yellow};
    this->Window.ControlButtonInactiveColor = ColorPair{Color::Silver, Color::Black};

    this->DialogError.ActiveColor = ColorPair{Color::White, Color::DarkRed};
    this->DialogError.InactiveColor = ColorPair{Color::Silver, Color::DarkRed};
    this->DialogError.TitleActiveColor = ColorPair{Color::Yellow, Color::DarkRed};
    this->DialogError.TitleInactiveColor = ColorPair{Color::Silver, Color::DarkRed};
    this->DialogError.ControlButtonColor = ColorPair{Color::Aqua, Color::DarkRed};
    this->DialogError.ControlButtonHoverColor = ColorPair{Color::Black, Color::Aqua};
    this->DialogError.ControlButtonPressedColor = ColorPair{Color::Black, Color::Yellow};
    this->DialogError.ControlButtonInactiveColor = ColorPair{Color::Silver, Color::Black};

    this->DialogNotify.ActiveColor = ColorPair{Color::White, Color::DarkGreen};
    this->DialogNotify.InactiveColor = ColorPair{Color::Silver, Color::DarkGreen};
    this->DialogNotify.TitleActiveColor = ColorPair{Color::Yellow, Color::DarkGreen};
    this->DialogNotify.TitleInactiveColor = ColorPair{Color::Silver, Color::DarkGreen};
    this->DialogNotify.ControlButtonColor = ColorPair{Color::Aqua, Color::DarkGreen};
    this->DialogNotify.ControlButtonHoverColor = ColorPair{Color::Black, Color::Aqua};
    this->DialogNotify.ControlButtonPressedColor = ColorPair{Color::Black, Color::Yellow};
    this->DialogNotify.ControlButtonInactiveColor = ColorPair{Color::Silver, Color::Black};

    this->DialogWarning.ActiveColor = ColorPair{Color::White, Color::Olive};
    this->DialogWarning.InactiveColor = ColorPair{Color::Silver, Color::Olive};
    this->DialogWarning.TitleActiveColor = ColorPair{Color::Yellow, Color::Olive};
    this->DialogWarning.TitleInactiveColor = ColorPair{Color::Silver, Color::Olive};
    this->DialogWarning.ControlButtonColor = ColorPair{Color::Aqua, Color::Olive};
    this->DialogWarning.ControlButtonHoverColor = ColorPair{Color::Black, Color::Aqua};
    this->DialogWarning.ControlButtonPressedColor = ColorPair{Color::Black, Color::Yellow};
    this->DialogWarning.ControlButtonInactiveColor = ColorPair{Color::Silver, Color::Black};


    this->Label.NormalColor = ColorPair{Color::Silver, Color::Transparent};
    this->Label.HotKeyColor = ColorPair{Color::Yellow, Color::Transparent};

    this->Button.Normal.TextColor = ColorPair{Color::White, Color::Gray};
    this->Button.Normal.HotKeyColor = ColorPair{Color::Yellow, Color::Gray};
    this->Button.Focused.TextColor = ColorPair{Color::Black, Color::White};
    this->Button.Focused.HotKeyColor = ColorPair{Color::Magenta, Color::White};
    this->Button.Inactive.TextColor = ColorPair{Color::Gray, Color::Black};
    this->Button.Inactive.HotKeyColor = ColorPair{Color::Gray, Color::Black};
    this->Button.Hover.TextColor = ColorPair{Color::Black, Color::Yellow};
    this->Button.Hover.HotKeyColor = ColorPair{Color::Magenta, Color::Yellow};

    this->StateControl.Normal.TextColor = ColorPair{Color::Silver, Color::Transparent};
    this->StateControl.Normal.HotKeyColor = ColorPair{Color::Aqua, Color::Transparent};
    this->StateControl.Normal.StateSymbolColor = ColorPair{Color::Green, Color::Transparent};
    this->StateControl.Focused.TextColor = ColorPair{Color::White, Color::Transparent};
    this->StateControl.Focused.HotKeyColor = ColorPair{Color::Aqua, Color::Transparent};
    this->StateControl.Focused.StateSymbolColor = ColorPair{Color::Green, Color::Transparent};
    this->StateControl.Hover.TextColor = ColorPair{Color::Yellow, Color::Transparent};
    this->StateControl.Hover.HotKeyColor = ColorPair{Color::Aqua, Color::Transparent};
    this->StateControl.Hover.StateSymbolColor = ColorPair{Color::Green, Color::Transparent};
    this->StateControl.Inactive.TextColor = ColorPair{Color::Gray, Color::Transparent};
    this->StateControl.Inactive.HotKeyColor = ColorPair{Color::Gray, Color::Transparent};
    this->StateControl.Inactive.StateSymbolColor = ColorPair{Color::Gray, Color::Transparent};

    this->Splitter.NormalColor = ColorPair{Color::Silver, Color::Transparent};
    this->Splitter.ClickColor = ColorPair{Color::White, Color::Magenta};
    this->Splitter.HoverColor = ColorPair{Color::Yellow, Color::Transparent};

    this->Panel.NormalColor = ColorPair{Color::Silver, Color::Transparent};
    this->Panel.TextColor = ColorPair{Color::White, Color::Transparent};

    this->TextField.SelectionColor = ColorPair{Color::Yellow, Color::Magenta};
    this->TextField.NormalColor = ColorPair{Color::Silver, Color::Black};
    this->TextField.FocusColor = ColorPair{Color::White, Color::Black};
    this->TextField.InactiveColor = ColorPair{Color::Gray, Color::Transparent};
    this->TextField.HoverColor = ColorPair{Color::Yellow, Color::Black};

    this->Tab.PageColor = ColorPair{Color::White, Color::Blue};
    this->Tab.PageHotKeyColor = ColorPair{Color::Yellow, Color::Blue};
    this->Tab.TabBarColor = ColorPair{Color::Silver, Color::Gray};
    this->Tab.TabBarHotKeyColor = ColorPair{Color::White, Color::Gray};
    this->Tab.HoverColor = ColorPair{Color::Yellow, Color::Magenta};
    this->Tab.HoverHotKeyColor = ColorPair{Color::White, Color::Magenta};
    this->Tab.ListSelectedPageColor = ColorPair{Color::Black, Color::White};
    this->Tab.ListSelectedPageHotKey = ColorPair{Color::DarkRed, Color::White};

    this->View.Normal.Border = ColorPair{ Color::Silver, Color::Transparent };
    this->View.Normal.Hotkey = ColorPair{ Color::Yellow, Color::Transparent };
    this->View.Normal.Text = ColorPair{ Color::Silver, Color::Transparent };
    this->View.Focused.Border = ColorPair{ Color::White, Color::Transparent };
    this->View.Focused.Hotkey = ColorPair{ Color::Yellow, Color::Transparent };
    this->View.Focused.Text = ColorPair{ Color::Yellow, Color::Transparent };
    this->View.Hover.Border = ColorPair{ Color::Yellow, Color::Transparent };
    this->View.Hover.Hotkey = ColorPair{ Color::Red, Color::Transparent };
    this->View.Hover.Text = ColorPair{ Color::White, Color::Transparent };
    this->View.Inactive.Border = ColorPair{ Color::Gray, Color::Transparent };
    this->View.Inactive.Hotkey = ColorPair{ Color::Gray, Color::Transparent };
    this->View.Inactive.Text = ColorPair{ Color::Gray, Color::Transparent };
    this->View.InactiveCanvasColor = ColorPair{ Color::Black, Color::Transparent };

    this->ScrollBar.Arrows = ColorPair{ Color::White, Color::Teal };
    this->ScrollBar.Bar = ColorPair{ Color::Black, Color::Teal };
    this->ScrollBar.Position = ColorPair{ Color::Green, Color::Teal };
}
