#include "WorldManager.h"
#include "triangle.h"
#include "cube.h"

namespace moonriver
{
    WorldManager::WorldManager()
    {
        m_triangle = std::make_shared<triangle>();
        m_cube = std::make_shared<cube>();
    }
    WorldManager::~WorldManager()
    {

    }
    void WorldManager::run()
    {
        m_triangle->run();
        //m_cube->run();
    }
}