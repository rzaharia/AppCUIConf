#pragma once

#include "../../Internal.hpp"
#include <queue>
#include <optional>

namespace AppCUI
{
namespace Internal
{
    class TestTerminal : public AbstractTerminal
    {
      public:
        enum class CommandID : uint8
        {
            None,
            MouseHold,
            MouseRelease,
            MouseClick,
            MouseDrag,
            MouseMove,
            KeyPress,
            KeyPressMultipleTimes,
            KeyType,
            KeyHold,
            KeyRelease,
            Print,
            PrintScreenHash
        };
        struct Command
        {
            CommandID id;
            union
            {
                int32 i32Value;
                uint32 u32Value;
                AppCUI::Input::Key keyValue;
                char16 charValue;
                AppCUI::Input::MouseButton mouseButtonValue;
                bool boolValue;
            } Params[8];
            Command();
            Command(CommandID id);
        };

      protected:
        std::queue<Command> commandsQueue;

        uint64 ComputeHash(bool useColors);

        void AddMouseHoldCommand(const std::string_view* params);
        void AddMouseReleaseCommand(const std::string_view* params);
        void AddMouseMoveCommand(const std::string_view* params);
        void AddMouseDragCommand(const std::string_view* params);
        void AddKeyPressCommand(const std::string_view* params);
        void AddKeyPressMultipleTimesCommand(const std::string_view* params);
        void AddKeyTypeCommand(const std::string_view* params);
        void AddKeyHoldCommand(const std::string_view* params);
        void AddPrintScreenHashCommand(const std::string_view* params);
        void PrintCurrentScreen();
        void PrintScreenHash(bool withColors);
      public:
        TestTerminal();

        void CreateEventsQueue(std::string_view commandsScript);

        virtual bool OnInit(const Application::InitializationData& initData) override;
        virtual void RestoreOriginalConsoleSettings() override;
        virtual void OnUninit() override;
        virtual void OnFlushToScreen() override;
        virtual void OnFlushToScreen(const Graphics::Rect& r) override;
        virtual bool OnUpdateCursor() override;
        virtual void GetSystemEvent(Internal::SystemEvent& evnt) override;
        virtual bool IsEventAvailable() override;
        virtual bool HasSupportFor(Application::SpecialCharacterSetType type) override;
        virtual ~TestTerminal();
    };
} // namespace Internal
} // namespace AppCUI
