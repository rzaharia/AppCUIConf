#include "AppCUI.h"
#include "Internal.h"
#include "IO.h"
#include "ControlContext.h"

using namespace AppCUI;

AppCUI::Internal::Application*   app;

bool AppCUI::Application::Init(Application::Flags::Type flags, EventHandler eventCallback)
{
    CHECK(app == nullptr, false, "Application has already been initialized !");
    app = new AppCUI::Internal::Application();
    CHECK(app, false, "Fail to allocate space for application object !");
    CHECK(app->Init(flags, eventCallback), false, "Fail to initialize application !");
    return true;
}
bool AppCUI::Application::Run()
{
    CHECK(app, false, "Application has not been initialized !");
    CHECK(app->Inited,false,"Application has not been corectly initialized !");
    app->ExecuteEventLoop();
    return true;
}
bool AppCUI::Application::GetApplicationSize(AppCUI::Console::Size & size)
{
    CHECK(app, false, "Application has not been initialized !");
    size.Width = app->consoleRenderer.GetConsoleSize().Width;
    size.Height = app->consoleRenderer.GetConsoleSize().Height;
    return true;
}
bool AppCUI::Application::GetDesktopSize(AppCUI::Console::Size & size)
{
    CHECK(app, false, "Application has not been initialized !");
    size.Width = app->consoleRenderer.GetConsoleSize().Width;
    size.Height = app->consoleRenderer.GetConsoleSize().Height;
    if (app->CommandBar.IsVisible())
        size.Height--;
    //GDT: trebuie scazut meniul si bara de jos
    return true;
}
bool AppCUI::Application::SetCommand(AppCUI::Input::Key::Type keyCode, const char* Name, int CommandID)
{
    CHECK(app, false, "Application has not been initialized !");
    return app->CommandBar.Set(keyCode, Name, CommandID);
}
void AppCUI::Application::Close()
{
    if (app)
        app->Terminate();
}
bool AppCUI::Application::AddWindow(AppCUI::Controls::Window * wnd)
{
    return app->Desktop.AddControl(wnd);
}

AppCUI::Application::Config* AppCUI::Application::GetAppConfig()
{
    CHECK(app, nullptr, "Application has not been initialized !");
    return &app->config;
}
void AppCUI::Application::RecomputeControlsLayout()
{
    app->ComputePositions();
}
void AppCUI::Application::Repaint()
{
    app->Paint();
}
void AppCUI::Application::RaiseEvent(AppCUI::Controls::Control *control, AppCUI::Controls::Control *sourceControl, AppCUI::Controls::Events::Event eventType, int controlID)
{
    app->RaiseEvent(control, sourceControl, eventType, controlID);
}

void PaintControl(AppCUI::Controls::Control *ctrl, AppCUI::Console::Renderer & renderer, bool focused)
{
    if (ctrl == nullptr)
        return;
    CREATE_CONTROL_CONTEXT(ctrl, Members, );
    if ((Members->Flags & GATTR_VISIBLE) == 0)
        return;
    // ma desenez pe mine    
    app->consoleRenderer.SetClip(Members->ScreenClip);
    app->consoleRenderer.SetTranslate(Members->ScreenClip.ScreenPosition.X, Members->ScreenClip.ScreenPosition.Y);
    if (focused != Members->Focused)
    {
        if (focused) {
            if (Members->Handlers.OnFocusHandler != nullptr)
                Members->Handlers.OnFocusHandler(ctrl, Members->Handlers.OnFocusHandlerContext);
            else
                ctrl->OnFocus();
        }
        else
        {
            if (Members->Handlers.OnLoseFocusHandler != nullptr)
            {
                Members->Handlers.OnLoseFocusHandler(ctrl, Members->Handlers.OnFocusHandlerContext);
            }
            else
            {
                ctrl->OnLoseFocus();
            }
        }
        Members->Focused = focused;
    }
    if (Members->Handlers.OnPaintHandler != nullptr)
        Members->Handlers.OnPaintHandler(ctrl, Members->Handlers.OnPaintHandlerContext);
    else
        ctrl->Paint(renderer);
    // desenez pe rand fiecare copil
    // ISO C++17 does not allow 'register' storage class specifier
    int /*register*/ cnt = Members->ControlsCount;
    int /*register*/ idx = Members->CurrentControlIndex;
    if ((!focused) || (idx < 0))
    {
        for (int tr = 1; tr <= cnt; tr++)
            PaintControl(Members->Controls[(tr + idx) % cnt], renderer, false);
    }
    else {
        for (int tr = 1; tr < cnt; tr++)
            PaintControl(Members->Controls[(tr + idx) % cnt], renderer, false);
        PaintControl(Members->Controls[idx], renderer, true);
    }

}
void ComputeControlLayout(AppCUI::Console::Clip &parentClip, Control *ctrl)
{
    if (ctrl == nullptr)
        return;
    CREATE_CONTROL_CONTEXT(ctrl, Members, );
    // calculez clip-ul meu
    Members->ScreenClip.Set(parentClip, Members->Layout.X, Members->Layout.Y, Members->Layout.Width, Members->Layout.Height);
    // calculez clip-ul client
    AppCUI::Console::Clip client;
    client.Set(parentClip, Members->Layout.X + Members->Margins.Left, Members->Layout.Y + Members->Margins.Top, Members->Layout.Width - (Members->Margins.Right+Members->Margins.Left), Members->Layout.Height - (Members->Margins.Bottom+Members->Margins.Top));
    // calculez pentru fiecare copil
    for (unsigned int tr = 0; tr < Members->ControlsCount; tr++)
        ComputeControlLayout(client, Members->Controls[tr]);
}
AppCUI::Controls::Control *CoordinatesToControl(AppCUI::Controls::Control *ctrl, int x, int y)
{
    if (ctrl == nullptr)
        return nullptr;
    CREATE_CONTROL_CONTEXT(ctrl, Members, nullptr);
    if ((Members->Flags & (GATTR_ENABLE | GATTR_VISIBLE)) != (GATTR_ENABLE | GATTR_VISIBLE))
        return nullptr;
    if (Members->ScreenClip.Visible == false)
        return nullptr;
    if ((x < Members->ScreenClip.ClipRect.X) || (y < Members->ScreenClip.ClipRect.Y) || 
        (x >= (Members->ScreenClip.ClipRect.X + Members->ScreenClip.ClipRect.Width)) ||
        (y >= (Members->ScreenClip.ClipRect.Y + Members->ScreenClip.ClipRect.Height)))
        return nullptr;
    // controlul e ok - verific acuma pentru copii
    if (Members->ControlsCount > 0)
    {
        unsigned int idx = Members->CurrentControlIndex;
        if (idx >= Members->ControlsCount)
            idx = 0;
        for (unsigned int tr = 0; tr < Members->ControlsCount; tr++)
        {
            Control *res = CoordinatesToControl(Members->Controls[idx], x, y);
            if (res != nullptr)
                return res;
            idx++;
            if (idx >= Members->ControlsCount)
                idx = 0;
        }
    }
    // daca nu e ok pe nici un copil sau nu am copii - atunci ma returnez pe mine
    return ctrl;
}
AppCUI::Controls::Control *GetFocusedControl(AppCUI::Controls::Control *ctrl)
{
    if (ctrl == nullptr)
        return nullptr;
    CREATE_CONTROL_CONTEXT(ctrl, Members, nullptr);
    if ((Members->Flags & (GATTR_ENABLE | GATTR_VISIBLE)) != (GATTR_ENABLE | GATTR_VISIBLE))
        return nullptr;
    // altfel ma uit la copii lui
    if (Members->ControlsCount > 0)
    {
        if ((Members->CurrentControlIndex >= 0) && (Members->CurrentControlIndex < Members->ControlsCount))
        {
            Control* c = GetFocusedControl(Members->Controls[Members->CurrentControlIndex]);
            if (c != nullptr)
                return c;
        }
    }
    // altfel nici un copil nu e ok - cer eu
    return ctrl;
}
void UpdateCommandBar(AppCUI::Controls::Control *obj)
{
    app->CommandBar.Clear();
    while (obj != nullptr)
    {
        if (((ControlContext*)(obj->Context))->Handlers.OnUpdateCommandBarHandler != nullptr)
        {
            if (((ControlContext*)(obj->Context))->Handlers.OnUpdateCommandBarHandler(obj, ((ControlContext*)(obj->Context))->Handlers.OnUpdateCommandBarHandlerContext) == true)
                break;
        }
        else {
            if (obj->OnUpdateCommandBar() == true)
                break;
        }
        obj = ((ControlContext*)(obj->Context))->Parent;
    }
    app->RepaintStatus |= REPAINT_STATUS_DRAW;
}

bool AppCUI::Internal::Application::Init(AppCUI::Application::Flags::Type flags, AppCUI::Application::EventHandler handler)
{
    CHECK(this->consoleRenderer.Init(), false, "Fail to create OS-Specific Console Renderer");
    CHECK(this->inputReader.Init(), false, "Fail to initialize OS-Specific input reader");
    this->renderer.Init(&this->consoleRenderer);
    this->config.SetDarkTheme();    
    this->CommandBar.Init(consoleRenderer.GetConsoleSize().Width, consoleRenderer.GetConsoleSize().Height, &this->config, (flags & AppCUI::Application::Flags::HAS_COMMANDBAR)!=0);
    
    
    CHECK(Desktop.Create(consoleRenderer.GetConsoleSize().Width, consoleRenderer.GetConsoleSize().Height), false, "Failed to create desktop !");
    LoopStatus = LOOP_STATUS_NORMAL;
    RepaintStatus = REPAINT_STATUS_ALL;
    MouseLockedObject = MOUSE_LOCKED_OBJECT_NONE;
    MouseLockedControl = nullptr;
    MouseOverControl = nullptr;
    ModalControlsCount = 0;
    Handler = handler;
    
    this->Inited = true;
    return true;
}
void AppCUI::Internal::Application::Paint()
{
    this->consoleRenderer.Prepare();
    // controalele

    if (ModalControlsCount > 0)
    {
        PaintControl(&Desktop, this->renderer, false);
        if (ModalControlsCount > 1)
        {
            unsigned int tmp = ModalControlsCount - 1;
            for (unsigned int tr = 0; tr < tmp; tr++)
                PaintControl(ModalControlsStack[tr], this->renderer, false);
        }
        // desenez si pe cel activ
        //Console::Gray();
        PaintControl(ModalControlsStack[ModalControlsCount - 1], this->renderer, true);
    }
    else {
        PaintControl(&Desktop, this->renderer, true);
    }
    // Acceleratorii
    this->consoleRenderer.ResetClip();
    this->consoleRenderer.SetTranslate(0, 0);
    this->CommandBar.Paint(this->renderer);
}
void AppCUI::Internal::Application::ComputePositions()
{
    AppCUI::Console::Clip full;
    full.Set(0, 0, app->consoleRenderer.GetConsoleSize().Width, app->consoleRenderer.GetConsoleSize().Height);
    ComputeControlLayout(full, &Desktop);
    for (unsigned int tr = 0; tr < ModalControlsCount; tr++)
        ComputeControlLayout(full, ModalControlsStack[tr]);
}
void AppCUI::Internal::Application::ProcessKeyPress(AppCUI::Input::Key::Type KeyCode, int AsciiCode)
{
    Control *ctrl = nullptr;

    if (ModalControlsCount == 0)
        ctrl = GetFocusedControl(&Desktop);
    else
        ctrl = GetFocusedControl(ModalControlsStack[ModalControlsCount - 1]);

    bool found = false;
    while (ctrl != nullptr)
    {
        if (((ControlContext*)(ctrl->Context))->Handlers.OnKeyEventHandler != nullptr)
        {
            if (((ControlContext*)(ctrl->Context))->Handlers.OnKeyEventHandler(ctrl, KeyCode, AsciiCode, ((ControlContext*)(ctrl->Context))->Handlers.OnKeyEventHandlerContext))
            {
                // daca a acceptat cineva o tasta - fac si o redesenare
                found = true;
                RepaintStatus |= REPAINT_STATUS_DRAW;
                break;
            }
        }
        if (ctrl->OnKeyEvent(KeyCode, AsciiCode))
        {
            // daca a acceptat cineva o tasta - fac si o redesenare
            found = true;
            RepaintStatus |= REPAINT_STATUS_DRAW;
            break;
        }
        ctrl = ctrl->GetParent();
    }
    if (!found)
    {
        // verific in accelarator
        int cmd = this->CommandBar.GetCommandForKey(KeyCode);
        if (cmd > 0)
            SendCommand(cmd);
    }
}
void AppCUI::Internal::Application::OnMouseDown(int x, int y, int buttonState)
{
    if (this->CommandBar.OnMouseDown())
    {
        RepaintStatus |= REPAINT_STATUS_DRAW;
        MouseLockedObject = MOUSE_LOCKED_OBJECT_ACCELERATOR;
        return;
    }
    // verific daca nu e un control	
    if (ModalControlsCount == 0)
        MouseLockedControl = CoordinatesToControl(&Desktop, x, y);
    else
        MouseLockedControl = CoordinatesToControl(ModalControlsStack[ModalControlsCount - 1], x, y);

    if (MouseLockedControl != nullptr)
    {
        MouseLockedControl->SetFocus();
        ControlContext* cc = ((ControlContext*)(MouseLockedControl->Context));
        if (cc->Handlers.OnMousePressedHandler != nullptr)
            cc->Handlers.OnMousePressedHandler(MouseLockedControl, x - cc->ScreenClip.ScreenPosition.X, y - cc->ScreenClip.ScreenPosition.Y, buttonState, cc->Handlers.OnMousePressedHandlerContext);
        else
            MouseLockedControl->OnMousePressed(x - cc->ScreenClip.ScreenPosition.X, y - cc->ScreenClip.ScreenPosition.Y, buttonState);
        MouseLockedObject = MOUSE_LOCKED_OBJECT_CONTROL;
        RepaintStatus |= REPAINT_STATUS_DRAW;
        return;
    }

    // daca nu e -> curat
    MouseLockedObject = MOUSE_LOCKED_OBJECT_NONE;
}
void AppCUI::Internal::Application::OnMouseUp(int x, int y, int buttonState)
{
    int commandID;
    switch (MouseLockedObject)
    {
    case MOUSE_LOCKED_OBJECT_ACCELERATOR:
        if (this->CommandBar.OnMouseUp(commandID))
            RepaintStatus |= REPAINT_STATUS_DRAW;
        if (commandID>0)
            SendCommand(commandID);
        break;
    case MOUSE_LOCKED_OBJECT_CONTROL:
        ControlContext* cc = ((ControlContext*)(MouseLockedControl->Context));
        if (cc->Handlers.OnMouseReleasedHandler != nullptr)
            cc->Handlers.OnMouseReleasedHandler(MouseLockedControl, x - cc->ScreenClip.ScreenPosition.X, y - cc->ScreenClip.ScreenPosition.Y, buttonState, cc->Handlers.OnMouseReleasedHandlerContext);
        else
            MouseLockedControl->OnMouseReleased(x - cc->ScreenClip.ScreenPosition.X, y - cc->ScreenClip.ScreenPosition.Y, buttonState);
        RepaintStatus |= REPAINT_STATUS_DRAW;
        break;
    }
    MouseLockedControl = nullptr;
    MouseLockedObject = MOUSE_LOCKED_OBJECT_NONE;
}
void AppCUI::Internal::Application::OnMouseMove(int x, int y, int buttonState)
{
    AppCUI::Controls::Control * ctrl;
    bool repaint;
    switch (MouseLockedObject)
    {
    case MOUSE_LOCKED_OBJECT_ACCELERATOR:
        break;
    case MOUSE_LOCKED_OBJECT_CONTROL:
        if (MouseLockedControl->OnMouseDrag(x - ((ControlContext*)(MouseLockedControl->Context))->ScreenClip.ScreenPosition.X, y - ((ControlContext*)(MouseLockedControl->Context))->ScreenClip.ScreenPosition.Y, buttonState))
            RepaintStatus |= (REPAINT_STATUS_DRAW | REPAINT_STATUS_COMPUTE_POSITION);
        break;
    case MOUSE_LOCKED_OBJECT_NONE:
        // validez acceleratorii mai intai
        if (this->CommandBar.OnMouseOver(x, y, repaint))
        {
            if (this->MouseOverControl)
            {
                if (this->MouseOverControl->OnMouseLeave())
                    RepaintStatus |= REPAINT_STATUS_DRAW;
                ((ControlContext*)(MouseOverControl->Context))->MouseIsOver = false;
            }
            this->MouseOverControl = nullptr;
            if (repaint)
                RepaintStatus |= REPAINT_STATUS_DRAW;
            break;
        }
        else {
            if (repaint)
                RepaintStatus |= REPAINT_STATUS_DRAW;
        }

        if (ModalControlsCount == 0)
            ctrl = CoordinatesToControl(&Desktop, x, y);
        else
            ctrl = CoordinatesToControl(ModalControlsStack[ModalControlsCount - 1], x, y);
        if (ctrl != this->MouseOverControl)
        {
            if (this->MouseOverControl)
            {
                if (this->MouseOverControl->OnMouseLeave())
                    RepaintStatus |= REPAINT_STATUS_DRAW;
                ((ControlContext*)(MouseOverControl->Context))->MouseIsOver = false;
            }
            this->MouseOverControl = ctrl;
            if (this->MouseOverControl)
            {
                if (this->MouseOverControl->OnMouseEnter())
                    RepaintStatus |= REPAINT_STATUS_DRAW;
                ControlContext* cc = ((ControlContext*)(MouseOverControl->Context));
                cc->MouseIsOver = true;
                if (MouseOverControl->OnMouseOver(x - cc->ScreenClip.ScreenPosition.X, y - cc->ScreenClip.ScreenPosition.Y))
                    RepaintStatus |= REPAINT_STATUS_DRAW;
            }
        }
        else {
            if (this->MouseOverControl)
            {
                ControlContext* cc = ((ControlContext*)(MouseOverControl->Context));
                if (MouseOverControl->OnMouseOver(x - cc->ScreenClip.ScreenPosition.X, y - cc->ScreenClip.ScreenPosition.Y))
                    RepaintStatus |= REPAINT_STATUS_DRAW;
            }
        }
        break;
    }
}
void AppCUI::Internal::Application::OnMouseWheel()
{
    if (MouseLockedObject != MOUSE_LOCKED_OBJECT_NONE)
        return;
}
void AppCUI::Internal::Application::ProcessShiftState(AppCUI::Input::Key::Type ShiftState)
{
    if (this->CommandBar.SetShiftKey(ShiftState))
        RepaintStatus |= REPAINT_STATUS_DRAW;
}
bool AppCUI::Internal::Application::ExecuteEventLoop(Control *ctrl)
{
    AppCUI::Internal::SystemEvents::Event evnt;
    if (ctrl != nullptr)
    {
        CHECK(ModalControlsCount < MAX_MODAL_CONTROLS_STACK, false, "Too many modal calls !");
        ModalControlsStack[ModalControlsCount] = ctrl;
        ModalControlsCount++;
    }
    // update la acceleratori
    if (ModalControlsCount == 0)
        UpdateCommandBar(GetFocusedControl(&Desktop));
    else
        UpdateCommandBar(GetFocusedControl(ModalControlsStack[ModalControlsCount - 1]));

    while (LoopStatus == LOOP_STATUS_NORMAL)
    {
        if (RepaintStatus != REPAINT_STATUS_NONE) {
            if ((RepaintStatus & REPAINT_STATUS_COMPUTE_POSITION) != 0)
                ComputePositions();
            if ((RepaintStatus & REPAINT_STATUS_DRAW) != 0)
            {
                RepaintStatus = REPAINT_STATUS_NONE;
                this->Paint();
                // pentru cazul in care OnFocus sau OnLoseFocus schimba repaint status
                if ((RepaintStatus & REPAINT_STATUS_COMPUTE_POSITION) != 0)
                    ComputePositions();
                if ((RepaintStatus & REPAINT_STATUS_DRAW) != 0)
                    Paint();
                this->consoleRenderer.FlushToScreen();
            }
            RepaintStatus = REPAINT_STATUS_NONE;
        }
        this->inputReader.GetSystemEvent(evnt);
        switch (evnt.eventType)
        {
            case SystemEvents::APP_CLOSE:
                LoopStatus = LOOP_STATUS_STOP_APP;
                break;
            case SystemEvents::APP_RESIZED:
                if (((evnt.newWidth != this->consoleRenderer.GetConsoleSize().Width) || (evnt.newHeight != this->consoleRenderer.GetConsoleSize().Height)) && (evnt.newWidth>0) && (evnt.newHeight>0))
                {
                    this->consoleRenderer.SetSize(evnt.newWidth,evnt.newHeight);
                    this->Desktop.Resize(this->consoleRenderer.GetConsoleSize().Width, this->consoleRenderer.GetConsoleSize().Height);
                    this->CommandBar.SetDesktopSize(this->consoleRenderer.GetConsoleSize().Width, this->consoleRenderer.GetConsoleSize().Height);
                    this->RepaintStatus = REPAINT_STATUS_ALL;
                }
                break;
            case SystemEvents::MOUSE_DOWN:
                OnMouseDown(evnt.mouseX, evnt.mouseY, evnt.mouseButtonState);
                break;
            case SystemEvents::MOUSE_UP:
                OnMouseUp(evnt.mouseX, evnt.mouseY, evnt.mouseButtonState);
                break;
            case SystemEvents::MOUSE_MOVE:
                OnMouseMove(evnt.mouseX, evnt.mouseY, evnt.mouseButtonState);
                break;
            case SystemEvents::KEY_PRESSED:
                ProcessKeyPress(evnt.keyCode, evnt.asciiCode);
                break;
            case SystemEvents::SHIFT_STATE_CHANGED:
                ProcessShiftState(evnt.keyCode);
                break;
        }
    }
    if (ctrl != nullptr)
    {
        ModalControlsCount--;
        if (ModalControlsCount < 0) ModalControlsCount = 0;
        // update la acceleratori
        if (ModalControlsCount == 0)
            UpdateCommandBar(GetFocusedControl(&Desktop));
        else
            UpdateCommandBar(GetFocusedControl(ModalControlsStack[ModalControlsCount - 1]));
        Paint();
        RepaintStatus |= REPAINT_STATUS_DRAW;
    }
    // daca vreau sa opresc doar bucla curenta
    if (LoopStatus == LOOP_STATUS_STOP_CURRENT)
        LoopStatus = LOOP_STATUS_NORMAL;
    //*/
    return true;
}
void AppCUI::Internal::Application::SendCommand(int command)
{
    Control *ctrl = nullptr;

    if (ModalControlsCount == 0)
        ctrl = GetFocusedControl(&Desktop);
    else
        ctrl = GetFocusedControl(ModalControlsStack[ModalControlsCount - 1]);
    if (ctrl != nullptr)
    {
        RaiseEvent(ctrl, nullptr, AppCUI::Controls::Events::EVENT_COMMAND, command);
        // refac si command bar-ul
        // update la acceleratori
        if (ModalControlsCount == 0)
            UpdateCommandBar(GetFocusedControl(&Desktop));
        else
            UpdateCommandBar(GetFocusedControl(ModalControlsStack[ModalControlsCount - 1]));
    }
}
void AppCUI::Internal::Application::RaiseEvent(AppCUI::Controls::Control *control, AppCUI::Controls::Control *sourceControl, AppCUI::Controls::Events::Event eventType, int controlID)
{
    while (control != nullptr)
    {
        if (((ControlContext*)(control->Context))->Handlers.OnEventHandler != nullptr)
        {
            if (((ControlContext*)(control->Context))->Handlers.OnEventHandler(control, sourceControl, eventType, controlID, ((ControlContext*)(control->Context))->Handlers.OnEventHandlerContext) == true)
            {
                RepaintStatus |= REPAINT_STATUS_DRAW;
                return;
            }
        }
        else {
            if (control->OnEvent(sourceControl, eventType, controlID) == true)
            {
                RepaintStatus |= REPAINT_STATUS_DRAW;
                return;
            }
        }
        control = control->GetParent();
    }
    // daca am ajuns aici - verific daca nu am si un handler
    if (Handler != nullptr)
        Handler(sourceControl, eventType, controlID);
}
void AppCUI::Internal::Application::Terminate()
{
    LoopStatus = LOOP_STATUS_STOP_APP;
}