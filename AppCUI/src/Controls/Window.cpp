#include "../../include/ControlContext.h"


using namespace AppCUI::Controls;
using namespace AppCUI::Console;
using namespace AppCUI::Input;

#define WINBUTTON_STATE_NONE        0
#define WINBUTTON_STATE_CLOSE       1
#define WINBUTTON_STATE_MAXIMIZE    2
#define WINBUTTON_STATE_RESIZE      4
#define WINBUTTON_STATE_CLICKED     32

Control* FindNextControl(Control *parent, bool forward, bool startFromCurrentOne, bool rootLevel, bool noSteps)
{
	if (parent == nullptr)
		return nullptr;
	CREATE_CONTROL_CONTEXT(parent, Members, nullptr);
	// daca am copii
	if (Members->ControlsCount != 0)
	{
		int start, end;
		if (startFromCurrentOne)
			start = Members->CurrentControlIndex;
		else
			start = -1;
		if (start < 0)
		{
			if (forward)
				start = 0;
			else
				start = Members->ControlsCount - 1;
		}
		// calculez si end
		if (forward)
			end = Members->ControlsCount;
		else
			end = -1;
		// sanity check
		if (((forward) && (start >= end)) || ((!forward) && (start <= end)))
			return nullptr;
		// ma plimb intre elemente
		bool firstElement = true;
		while (true)
		{
			Control* copil = Members->Controls[start];
			if ((copil != nullptr) && (copil->Context != nullptr))
			{
				ControlContext *cMembers = (ControlContext *)copil->Context;
				// am un element posibil ok
				if (((cMembers->Flags & (GATTR_VISIBLE | GATTR_ENABLE)) == (GATTR_VISIBLE | GATTR_ENABLE)))
				{
					Control* res = FindNextControl(copil, forward, startFromCurrentOne & firstElement, false, noSteps & firstElement);
					if (res != nullptr)
						return res;
				}
			}


			if (forward)
				start++;
			else
				start--;
			noSteps = false;
			if (start == end)				
			{
				if ((!rootLevel) || (startFromCurrentOne==false))
					return nullptr; 
				// am ajuns la finalul listei si nu am gasit sau am parcurs toata lista
				// daca nu - parcurg si restul listei
				if (forward) {
					start = 0;
					end = Members->CurrentControlIndex + 1;
				}
				else {
					start = Members->ControlsCount - 1;
					end = Members->CurrentControlIndex + 1;
				}
				// sanity check
				if (((forward) && (start >= end)) || ((!forward) && (start <= end)))
					return nullptr;
				rootLevel = false;
				firstElement = false;
			}
			firstElement = false;
		}
	}
	// daca nu am copii
	if (((Members->Flags & GATTR_TABSTOP) != 0) && (noSteps == false))
		return parent;
	return nullptr;
}
bool ProcessHotKey(Control *ctrl,int KeyCode)
{
	if (ctrl == nullptr)
		return false;
	CREATE_CONTROL_CONTEXT(ctrl, Members, false);
	if (((Members->Flags & (GATTR_VISIBLE | GATTR_ENABLE)) != (GATTR_VISIBLE | GATTR_ENABLE)))
		return false;
	for (unsigned int tr = 0; tr < Members->ControlsCount; tr++)
	{
		if (ProcessHotKey(Members->Controls[tr], KeyCode))
			return true;
	}
	if (ctrl->GetHotKey() == KeyCode)
	{
		ctrl->SetFocus();
		ctrl->OnHotKey();
		return true;
	}
	return false;
}
void UpdateWindowsButtonsPoz(WindowControlContext *wcc)
{
    if (!(wcc->Flags & WindowFlags::NOCLOSEBUTTON))
    {
        wcc->rCloseButton.Y = 0;
        wcc->rCloseButton.Left = wcc->Layout.Width - 4;
        wcc->rCloseButton.Right = wcc->Layout.Width - 2;
    }
    if (wcc->Flags & WindowFlags::SIZEABLE) {
        wcc->rMaximizeButton.Y = 0;
        wcc->rMaximizeButton.Left = 1;
        wcc->rMaximizeButton.Right = 3;
        wcc->rResizeButton.Y = wcc->Layout.Height - 1;
        wcc->rResizeButton.Left = wcc->Layout.Width - 1;
        wcc->rResizeButton.Right = wcc->rResizeButton.Left;
    }
}
//=========================================================================================================================================================
Window::~Window()
{
	DELETE_CONTROL_CONTEXT(WindowControlContext);
}
bool Window::Create(const char* text, const char * layout, WindowFlags::Type Flags)
{
	CONTROL_INIT_CONTEXT(WindowControlContext);
	CHECK(Init(nullptr, text, layout, false), false, "Failed to create window !");
    CHECK(SetMargins(1, 1, 1, 1), false, "Failed to set margins !");
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
	Members->Flags = GATTR_ENABLE | GATTR_VISIBLE | GATTR_TABSTOP | Flags;
	Members->MinWidth = 12; // left_corner(1 char), maximize button(3chars),OneSpaceLeftPadding, title, OneSpaceRightPadding, close button(char),right_corner(1 char) = 10+szTitle (szTitle = min 2 chars)
	Members->MinHeight = 3;
	Members->MaxWidth = 100000;
	Members->MaxHeight = 100000;
	Members->Maximized = false;
	Members->dragStatus = WINDOW_DRAG_STATUS_NONE;
	Members->DialogResult = -1;
    Members->winButtonState = WINBUTTON_STATE_NONE;
    UpdateWindowsButtonsPoz(Members);
    if (Flags & WindowFlags::CENTERED)
    {
        CHECK(CenterScreen(), false, "Fail to center window to screen !");
    }
    if (Flags & WindowFlags::MAXIMIZED)
    {
        CHECK(MaximizeRestore(), false, "Fail to maximize window !");
    }
	return true;
}
void Window::Paint(Console::Renderer & renderer)
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, );
    auto * wcfg = &Members->Cfg->Window;
    unsigned int colorTitle, colorWindow, colorWindowButton, c1,c2;
    bool doubleLine;

    if ((Members->Flags & WindowFlags::WARNINGBOX) != 0)
        wcfg = &Members->Cfg->DialogWarning;
    else if ((Members->Flags & WindowFlags::ERRORBOX) != 0)
        wcfg = &Members->Cfg->DialogError;
    else if ((Members->Flags & WindowFlags::NOTIFYBOX) != 0)
        wcfg = &Members->Cfg->DialogNotify;

    if (Members->Focused)
    {
        colorTitle = wcfg->TitleActiveColor;
        colorWindow = wcfg->ActiveColor;
        colorWindowButton = wcfg->ControlButtonColor;
        if (Members->dragStatus == WINDOW_DRAG_STATUS_SIZE) {
            colorWindow = wcfg->ControlButtonColor;
            doubleLine = false;
        }
        else {
            doubleLine = true;
        }
    }
    else {
        colorTitle = wcfg->TitleInactiveColor;
        colorWindow = wcfg->InactiveColor;
        colorWindowButton = wcfg->ControlButtonInactiveColor;
        doubleLine = false;
    }
    renderer.Clear(' ', colorWindow);
    renderer.DrawRectWidthHeight(0, 0, Members->Layout.Width, Members->Layout.Height, colorWindow, doubleLine);
    int txW = Members->Text.Len();
    txW = MINVALUE(txW, Members->Layout.Width - 10);
    int txX = (Members->Layout.Width - txW) / 2;
    renderer.WriteSingleLineText(txX, 0, Members->Text.GetText(), colorTitle, txW);
    renderer.WriteCharacter(txX - 1, 0, ' ', colorTitle);
    renderer.WriteCharacter(txX + txW, 0, ' ', colorTitle);
    // close button
    if (!(Members->Flags & WindowFlags::NOCLOSEBUTTON))
    {
        switch (Members->winButtonState)
        {
            case WINBUTTON_STATE_CLOSE: c1 = c2 = wcfg->ControlButtonHoverColor; break;
            case WINBUTTON_STATE_CLOSE | WINBUTTON_STATE_CLICKED: c1 = c2 = wcfg->ControlButtonPressedColor; break;
            default: c1 = colorTitle; c2 = colorWindowButton; break;
        }
        renderer.WriteSingleLineText(Members->rCloseButton.Left, Members->rCloseButton.Y, "[ ]", c1, Members->rCloseButton.Right - Members->rCloseButton.Left + 1);
        renderer.WriteCharacter(Members->rCloseButton.Left + 1, Members->rCloseButton.Y, 'x', c2);
    }
    // maximize button
    if (Members->Flags & WindowFlags::SIZEABLE)
    {
        switch (Members->winButtonState)
        {
            case WINBUTTON_STATE_MAXIMIZE: c1 = c2 = wcfg->ControlButtonHoverColor; break;
            case WINBUTTON_STATE_MAXIMIZE | WINBUTTON_STATE_CLICKED: c1 = c2 = wcfg->ControlButtonPressedColor; break;
            default: c1 = colorTitle; c2 = colorWindowButton; break;
        }
        renderer.WriteSingleLineText(Members->rMaximizeButton.Left, Members->rMaximizeButton.Y, "[ ]", c1, Members->rMaximizeButton.Right - Members->rMaximizeButton.Left + 1);
        if (Members->Maximized)
            renderer.WriteSpecialCharacter(Members->rMaximizeButton.Left + 1, Members->rMaximizeButton.Y, SpecialChars::ArrowUpDown, c2);
        else
            renderer.WriteSpecialCharacter(Members->rMaximizeButton.Left + 1, Members->rMaximizeButton.Y, SpecialChars::ArrowUp, c2);
        if (Members->Focused)
        {
            if (Members->dragStatus == WINDOW_DRAG_STATUS_SIZE)
                c1 = wcfg->ControlButtonPressedColor;
            else if (Members->winButtonState == WINBUTTON_STATE_RESIZE)
                c1 = wcfg->ControlButtonHoverColor;
            else
                c1 = colorWindowButton;
            renderer.WriteSpecialCharacter(Members->rResizeButton.Left, Members->rResizeButton.Y, SpecialChars::BoxBottomRightCornerSingleLine, c1);
        }
    }
}
bool Window::MaximizeRestore()
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
	if (Members->Maximized == false)
	{
		Members->oldPosX = GetX();
		Members->oldPosY = GetY();
		Members->oldW = GetWidth();
		Members->oldH = GetHeight();
        Size sz;
        CHECK(AppCUI::Application::GetDesktopSize(sz), false, "Fail to get desktop size");
		this->MoveTo(0, 0);
		if (this->Resize(sz.Width, sz.Height))
			Members->Maximized = true;		        
	}
	else 
	{
		this->MoveTo(Members->oldPosX, Members->oldPosY);
		this->Resize(Members->oldW, Members->oldH);
		Members->Maximized = false;
	}
    UpdateWindowsButtonsPoz(Members);
    AppCUI::Application::RecomputeControlsLayout();
	return true;
}
bool Window::CenterScreen()
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
    Size sz;
    CHECK(Application::GetDesktopSize(sz), false, "Fail to get desktop size !");
	MoveTo(((sz.Width - Members->Layout.Width) / 2) , ((sz.Height - Members->Layout.Height) / 2));
    UpdateWindowsButtonsPoz(Members);
	return true;
}
void Window::OnMousePressed(int x, int y, int butonState)
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, );
	Members->dragStatus = WINDOW_DRAG_STATUS_NONE;
    if ((!(Members->Flags & WindowFlags::NOCLOSEBUTTON)) && (Members->rCloseButton.Contains(x, y)))
    {
        Members->winButtonState = WINBUTTON_STATE_CLICKED | WINBUTTON_STATE_CLOSE;
        return;
    }
    if (Members->Flags & WindowFlags::SIZEABLE)  
    {
        if (Members->rMaximizeButton.Contains(x, y))
        {
            Members->winButtonState = WINBUTTON_STATE_CLICKED | WINBUTTON_STATE_MAXIMIZE;
            return;
        }
        if (Members->rResizeButton.Contains(x, y))
        {
            Members->dragStatus = WINDOW_DRAG_STATUS_SIZE;
            Members->winButtonState = WINBUTTON_STATE_NONE;
            return;
        }
    }
	//if (Members->fnMousePressedHandler != nullptr)
	//{
	//	// daca vreau sa tratez eu evenimentul
	//	if (Members->fnMousePressedHandler(this, x, y, butonState,Members->fnMouseHandlerContext))
	//		return;
	//}
    if (!(Members->Flags & WindowFlags::FIXED))
    {
        Members->dragStatus = WINDOW_DRAG_STATUS_MOVE;
        Members->dragOffsetX = x;
        Members->dragOffsetY = y;
    }
}
void Window::OnMouseReleased(int x, int y, int butonState)
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, );
	if (Members->dragStatus != WINDOW_DRAG_STATUS_NONE)
	{
		Members->dragStatus = WINDOW_DRAG_STATUS_NONE;
		return;
	}
    if (Members->winButtonState == (WINBUTTON_STATE_CLICKED | WINBUTTON_STATE_MAXIMIZE)) {
        MaximizeRestore();
        return;
    }
	if (Members->winButtonState == (WINBUTTON_STATE_CLICKED | WINBUTTON_STATE_CLOSE)) {
		RaiseEvent(Events::EVENT_WINDOW_CLOSE);
		return;
	}
	//if (Members->fnMouseReleaseHandler != nullptr)
	//{
	//	// daca vreau sa tratez eu evenimentul
	//	if (Members->fnMouseReleaseHandler(this, x, y, butonState, Members->fnMouseHandlerContext))
	//		return;
	//}
}
bool Window::OnMouseDrag(int x, int y,  int butonState)
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
	if (Members->dragStatus == WINDOW_DRAG_STATUS_SIZE)
	{
		bool res = Resize(x+1, y+1);
        UpdateWindowsButtonsPoz(Members);
        return res;
	}
	if (Members->dragStatus == WINDOW_DRAG_STATUS_MOVE)
	{
		this->MoveTo(x + Members->ScreenClip.ScreenPosition.X - Members->dragOffsetX, y + Members->ScreenClip.ScreenPosition.Y - Members->dragOffsetY);
		return true;
	}
	return false;
}
bool Window::OnMouseOver(int x, int y)
{
    CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
    if ((!(Members->Flags & WindowFlags::NOCLOSEBUTTON)) && (Members->rCloseButton.Contains(x, y)))
    {
        if (Members->winButtonState == WINBUTTON_STATE_CLOSE)
            return false; // suntem deja pe buton
        Members->winButtonState = WINBUTTON_STATE_CLOSE;
        return true;
    }
    if (Members->Flags & WindowFlags::SIZEABLE)
    {
        if (Members->rMaximizeButton.Contains(x, y))
        {
            if (Members->winButtonState == WINBUTTON_STATE_MAXIMIZE)
                return false; // suntem deja pe buton
            Members->winButtonState = WINBUTTON_STATE_MAXIMIZE;
            return true;
        }
        if (Members->rResizeButton.Contains(x, y))
        {
            if (Members->winButtonState == WINBUTTON_STATE_RESIZE)
                return false; // suntem deja pe buton
            Members->winButtonState = WINBUTTON_STATE_RESIZE;
            return true;
        }
    }
    if (Members->winButtonState == WINBUTTON_STATE_NONE)
        return false; // suntem deja in afara
    Members->winButtonState = WINBUTTON_STATE_NONE;
    return true;
}
bool Window::OnMouseLeave()
{
    CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
    if (Members->winButtonState == WINBUTTON_STATE_NONE)
        return false;
    Members->winButtonState = WINBUTTON_STATE_NONE;
    return true;
}
bool Window::OnBeforeResize(int newWidth,int newHeight)
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
	if ((Members->Flags & WindowFlags::SIZEABLE) == 0)
		return false;
	return (newWidth >= Members->MinWidth) && (newWidth <= Members->MaxWidth) && (newHeight >= Members->MinHeight) && (newHeight <= Members->MaxHeight);
}
void Window::OnAfterResize(int newWidth, int newHeight)
{
    WindowControlContext * Members = (WindowControlContext*)this->Context;
    if (Members) 
    {
        UpdateWindowsButtonsPoz(Members);
    }
}
bool Window::OnKeyEvent(AppCUI::Input::Key::Type KeyCode, char AsciiCode)
{
	Control* tmp;
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);

	switch (KeyCode)
	{
	case Key::Tab|Key::Shift:
		tmp = FindNextControl(this, false, true, true, true);
		if (tmp != nullptr) tmp->SetFocus();
		return true;
	case Key::Tab:
		tmp = FindNextControl(this, true, true, true, true);
		if (tmp != nullptr) tmp->SetFocus();
		return true;
	case Key::Escape:
		RaiseEvent(Events::EVENT_WINDOW_CLOSE);
		return true;
	case Key::Enter:
		RaiseEvent(Events::EVENT_WINDOW_ACCEPT);
		return true;
	}
	if ((KeyCode & (Key::Shift | Key::Alt | Key::Ctrl)) == Key::Alt)
	{
		return ProcessHotKey(this, KeyCode);
	}

	return false;
}
bool Window::Exit(int dialogResult)
{
	CHECK(dialogResult >= 0, false, "Dialog result code must be bigger than 0 !");
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
	Members->DialogResult = dialogResult;
    _asm int 3;
    //GetExecutionLoop()->LoopStatus = LOOP_STATUS_STOP_CURRENT;
	return true;
}
int  Window::Show()
{
	CHECK(GetParent() == nullptr, -1, "Unable to run modal window if it is attached to another control !");
    _asm int 3;
    //ExecutionLoop *loop = GetExecutionLoop();
    _asm int 3;
    //CHECK(loop->ExecuteUI(this), -1, "Failed to start execution loop !");
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, -1);
	return Members->DialogResult;
}
int  Window::GetDialogResult()
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, -1);
	return Members->DialogResult;
}

bool Window::IsWindowInResizeMode()
{
	CREATE_TYPECONTROL_CONTEXT(WindowControlContext, Members, false);
	return (Members->dragStatus == WINDOW_DRAG_STATUS_SIZE);
}