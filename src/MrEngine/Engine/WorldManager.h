#pragma once

#include <memory>

namespace moonriver
{
    class triangle;
    class cube;
    class WorldManager
    {
    public:
        WorldManager();
        ~WorldManager();
        void run();
    private:
        std::shared_ptr<triangle> m_triangle;
        std::shared_ptr<cube> m_cube;
    };

}


