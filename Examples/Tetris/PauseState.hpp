#pragma once

#include "State.hpp"
#include "Game.hpp"

class PauseState : public State, public AppCUI::Controls::Handlers::OnKeyEventInterface
{
  public:
    explicit PauseState(const std::shared_ptr<GameData>& data);

    PauseState(const PauseState& other)     = default;
    PauseState(PauseState&& other) noexcept = default;
    PauseState& operator=(const PauseState& other) = default;
    PauseState& operator=(PauseState&& other) noexcept = default;

    ~PauseState();

    void Init() override;

    bool HandleEvent(
          AppCUI::Utils::Reference<AppCUI::Controls::Control> ctrl,
          AppCUI::Controls::Event eventType,
          int controlID) override;
    bool Update() override;
    void Draw(AppCUI::Graphics::Renderer& renderer) override;

    void Pause() override;
    void Resume() override;

    bool OnKeyEvent(
          AppCUI::Controls::Reference<AppCUI::Controls::Control> control,
          AppCUI::Input::Key keyCode,
          char16_t unicodeChar);

  private:
    const std::shared_ptr<GameData>& data;

    AppCUI::Utils::Reference<AppCUI::Controls::TabPage> page = nullptr;
    AppCUI::Utils::Reference<AppCUI::Controls::Label> label  = nullptr;
};
