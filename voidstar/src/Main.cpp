#include "Application.h"

#ifdef NDEBUG
int WinMain()
#else
int main()
#endif
{
    Application& app = Application::Get();
    app.Run();

    return 0;
}