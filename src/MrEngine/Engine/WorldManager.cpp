#include "WorldManager.h"
#include "triangle.h"
#include "cube.h"
#include "shader.h"

namespace moonriver
{
    WorldManager::WorldManager()
    {
        m_triangle = std::make_shared<triangle>();
        m_cube = std::make_shared<cube>();
        Shader shader;
    }
    WorldManager::~WorldManager()
    {

    }
    void WorldManager::run()
    {
        m_cube->run();
        m_triangle->run();
    }
}