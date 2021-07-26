#ifndef _SDL_TERMINAL_INCLUDED_
#define _SDL_TERMINAL_INCLUDED_

#include <map>
#include <memory>
#include <iterator>
#include <string>

#include "Internal.hpp"
#include "SDL.h"
#include "SDL_ttf.h"

namespace AppCUI
{
    namespace Internal
    {
        const static std::string fontName = "CourierNew.ttf";

        class SDLTerminal : public AbstractTerminal
        {
        private:
            std::map<SDL_Scancode, AppCUI::Input::Key> KeyTranslation;
            std::map<SDL_Scancode, AppCUI::Input::Key> AsciiTranslation;
            AppCUI::Input::Key oldShiftState;

            SDL_Window* window;
            SDL_Renderer* renderer;
            TTF_Font* font;
            size_t charWidth;
            size_t charHeight;

        public:
            virtual bool OnInit(const InitializationData & initData) override;
            virtual void OnUninit() override;
            virtual void OnFlushToScreen() override;
            virtual bool OnUpdateCursor() override;
            virtual void GetSystemEvent(AppCUI::Internal::SystemEvents::Event & evnt) override;
            virtual bool IsEventAvailable() override;
            virtual void RestoreOriginalConsoleSettings() override;

        private:
            bool initScreen(const InitializationData & initData);
            bool initInput(const InitializationData & initData);

            bool initFont(const InitializationData & initData);

            void uninitScreen();
            void uninitInput();

            void handleMouse(SystemEvents::Event& evt, const SDL_Event& eSdl);
            void handleKeyUp(SystemEvents::Event &evt, const SDL_Event& eSdl);
            void handleKeyDown(SystemEvents::Event &evt, const SDL_Event& eSdl);
        };
    }
}

#endif // _SDL_TERMINAL_INCLUDED_