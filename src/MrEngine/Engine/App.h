#pragma once

#include <memory>

namespace moonriver
{
    class Scene;
    class Engine;
	class AppImplement
	{
	public:
		virtual ~AppImplement() { }
    protected:
        std::shared_ptr<Scene> m_pScene;
        Engine* m_pEngine = nullptr;
	};
    
	class App
	{
	public:
        App(Engine* pEngine);
        virtual void Update();
        
    private:
        std::shared_ptr<AppImplement> m_implement;
	};
}
