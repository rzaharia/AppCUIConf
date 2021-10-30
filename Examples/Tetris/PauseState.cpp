#include "PauseState.hpp"

PauseState::PauseState(const std::shared_ptr<GameData>& data) : data(data)
{
    page = AppCUI::Controls::Factory::TabPage::Create(data->tab, "");
    label =
          AppCUI::Controls::Factory::Label::Create(page, "Pause state. Please Esc to resume...", "x:50%,y:50%, w:20%");

    page->Handlers()->OnKeyEvent = this;
}

PauseState::~PauseState()
{
    data->tab->RemoveControl(page);
}

void PauseState::Init()
{
    data->tab->SetCurrentTabPage(page);
}

bool PauseState::HandleEvent(
      AppCUI::Utils::Reference<AppCUI::Controls::Control> ctrl, AppCUI::Controls::Event eventType, int controlID)
{
    return false;
}

bool PauseState::Update()
{
    return false;
}

void PauseState::Draw(AppCUI::Graphics::Renderer& renderer)
{
    renderer.HideCursor();
    renderer.Clear(
          ' ', AppCUI::Graphics::ColorPair{ AppCUI::Graphics::Color::White, AppCUI::Graphics::Color::DarkBlue });
}

void PauseState::Pause()
{
}

void PauseState::Resume()
{
    data->tab->SetCurrentTabPage(page);
}

bool PauseState::OnKeyEvent(
      AppCUI::Controls::Reference<AppCUI::Controls::Control> control, AppCUI::Input::Key keyCode, char16_t unicodeChar)
{
    switch (keyCode)
    {
    case AppCUI::Input::Key::Escape:
        this->data->machine->PopState();
        return true;
    default:
        break;
    }

    return false;
}
