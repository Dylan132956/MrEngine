#include "SceneManager.h"
#include "triangle.h"
#include "cube.h"
#include "gltfview.h"
#include "graphics/shader.h"
#include "Scene.h"
#include "Entity.h"
#include "App.h"

namespace moonriver
{

    SceneManager* SceneManager::m_instance = nullptr;

    SceneManager::SceneManager()
    {
        m_instance = this;
        m_triangle = std::make_shared<triangle>();
        m_cube = std::make_shared<cube>();
        m_gltfview = std::make_shared<gltfview>();   
    }

    SceneManager* SceneManager::Instance()
    {
        return m_instance;
    }

    SceneManager::~SceneManager()
    {

    }

    std::shared_ptr<Scene> SceneManager::CreateScene()
    {
        m_Scene = std::make_shared<Scene>();
        return m_Scene;
    }

    std::shared_ptr<Scene> SceneManager::GetScene()
    {
        return m_Scene;
    }

    void SceneManager::Update()
    {
        //m_triangle->Update();
        //m_cube->Update();
        m_Scene->Update();
    }
}