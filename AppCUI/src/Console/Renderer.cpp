#include "../../include/AppCUI.h"
#include "../../include/Internal.h"

using namespace AppCUI::Console;

#define CRND  ((AppCUI::Internal::ConsoleRenderer*)this->consoleRenderer)
#define ConsoleRendererCall(x) if (this->consoleRenderer) CRND->x;

Renderer::Renderer()
{
    this->consoleRenderer = nullptr;
}
void Renderer::Init(void* _consoleRenderer)
{
    if ((_consoleRenderer) && !(this->consoleRenderer))
        this->consoleRenderer = _consoleRenderer;
}
void Renderer::FillRect(int left, int top, int right, int bottom, int charCode, unsigned int color)
{
    ConsoleRendererCall(FillRect(left, top, right, bottom, charCode, color));
}
void Renderer::FillRectWidthHeight(int x, int y, int width, int height, int charCode, unsigned int color)
{
    ConsoleRendererCall(FillRect(x, y, x + width - 1, y + height - 1, charCode, color));
}
void Renderer::Clear(int charCode, unsigned int color)
{
    ConsoleRendererCall(ClearClipRectangle(charCode, color));
}
void Renderer::WriteSingleLineText(int x, int y, const char * text, unsigned int color, int textSize)
{
    ConsoleRendererCall(WriteSingleLineText(x, y, text, color, textSize));
}
void Renderer::FillHorizontalLine(int left, int y, int right, int charCode, unsigned int color)
{
    ConsoleRendererCall(FillHorizontalLine(left, y, right, charCode, color));
}
void Renderer::FillHorizontalLineSize(int x, int y, int size, int charCode, unsigned int color)
{
    ConsoleRendererCall(FillHorizontalLine(x, y, x+size-1, charCode, color));
}
void Renderer::FillVerticalLine(int x, int top, int bottom, int charCode, unsigned int color)
{
    ConsoleRendererCall(FillVerticalLine(x, top, bottom, charCode, color));
}
void Renderer::FillVerticalLineSize(int x, int y, int size, int charCode, unsigned int color)
{
    ConsoleRendererCall(FillVerticalLine(x, y, y+size-1, charCode, color));
}
#undef CRND