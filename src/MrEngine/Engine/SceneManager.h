#pragma once

#include <memory>

namespace moonriver
{
    class triangle;
    class cube;
    class gltfview;
    class Scene;
    class SceneManager
    {
    public:
        SceneManager();
        ~SceneManager();
        std::shared_ptr<Scene> CreateScene();
        std::shared_ptr<Scene> GetScene();
        static SceneManager* Instance();
        void Update();
        void Render();
    private:
        static SceneManager* m_instance;
        std::shared_ptr<triangle> m_triangle;
        std::shared_ptr<cube> m_cube;
        std::shared_ptr<gltfview> m_gltfview;
        std::shared_ptr<Scene> m_Scene = nullptr;
    };

}


