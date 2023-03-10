#include "Application.h"

#ifdef NDEBUG
int WinMain()
#else
int main()
#endif
{
    Application::Get().Run();

    return 0;
}