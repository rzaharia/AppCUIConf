#include "AppCUI.hpp"

using namespace AppCUI;
using namespace AppCUI::Application;
using namespace AppCUI::Controls;

class SimpleWin : public Window
{
  public:
    SimpleWin() : Window("Test", "d:c,w:40,h:10", WindowFlags::None)
    {
        Factory::CheckBox::Create(this, "A checkbox", "x:1,y:1,w:20");
    }
};
int main()
{
    if (!Application::InitForTests(60,20, InitializationFlags::None, true))
        return 1;
    Application::AddWindow(std::make_unique<SimpleWin>());
    Application::RunTestScript(R"SCRIPT(
            Print()
            # click on the checkbox
            Mouse.Click(14,7,Left)
            Print()
    )SCRIPT");
    return 0;
}
