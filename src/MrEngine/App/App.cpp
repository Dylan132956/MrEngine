#include "AppImplementGLTF.h"

namespace moonriver
{
    App::App(Engine* pEngine)
    {
        m_implement = std::make_shared<AppImplementGLTF>(pEngine);
    }
    
    void App::Update()
    {
        std::dynamic_pointer_cast<AppImplementGLTF>(m_implement)->Update();
    }
}
