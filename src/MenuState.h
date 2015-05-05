#ifndef _MENUSTATE_H_VZ3QNB09
#define _MENUSTATE_H_VZ3QNB09

#include "Qor/State.h"
#include "Qor/Input.h"
#include "Qor/TileMap.h"
#include "Qor/Camera.h"
#include "Qor/Pipeline.h"
#include "Qor/Mesh.h"
#include "Qor/Interpreter.h"
#include "Qor/Physics.h"
#include "Qor/Canvas.h"
//#include "Qor/BasicPhysics.h"
#include "Qor/Sprite.h"
#include "Qor/PlayerInterface2D.h"
#include "Qor/Sound.h"
#include "Qor/Scene.h"
#include "Qor/Profile.h"
#include "Qor/Menu.h"

class Qor;

class MenuState:
    public State
{
    public:
        
        MenuState(Qor* engine);
        virtual ~MenuState();

        virtual void preload() override;
        //virtual void start() override;
        virtual void enter() override;
        virtual void logic(Freq::Time t) override;
        virtual void render() const override;
        virtual bool needs_load() const override {
            return true;
        }

        virtual Pipeline* pipeline() {
            return m_pPipeline;
        }
        virtual const Pipeline* pipeline() const {
            return m_pPipeline;
        }
        
        virtual std::shared_ptr<Node> root() override {
            return m_pRoot;
        }
        virtual std::shared_ptr<const Node> root() const override {
            return m_pRoot;
        }
        
        virtual std::shared_ptr<Node> camera() override {
            return m_pCamera;
        }
        virtual std::shared_ptr<const Node> camera() const override {
            return m_pCamera;
        }

        virtual void camera(const std::shared_ptr<Node>& camera)override{
            m_pCamera = std::dynamic_pointer_cast<Camera>(camera);
        }
        
        void init_controls_menu();
        
    private:
        
        Qor* m_pQor = nullptr;
        Input* m_pInput = nullptr;
        Pipeline* m_pPipeline = nullptr;
        Cache<Resource, std::string>* m_pResources = nullptr;

        std::shared_ptr<Node> m_pRoot;
        //Interpreter* m_pInterpreter;
        //std::shared_ptr<Interpreter::Context> m_pScript;
        std::shared_ptr<Camera> m_pCamera;
        //std::shared_ptr<Physics> m_pPhysics;

        std::string m_Filename;
        std::shared_ptr<Sound> m_pMusic;
        //std::shared_ptr<Scene> m_pScene;
        std::shared_ptr<Canvas> m_pCanvas;
        glm::vec2 m_WrapAccum;

        Profile* m_pProfile = nullptr;

        glm::vec2 m_TextOffset;

        std::shared_ptr<MenuGUI> m_pMenuGUI;
        MenuContext m_MenuContext;
        Menu m_MainMenu;
        Menu m_OptionsMenu;
        Menu m_ControlsMenu;
        
        kit::reactive<Color> m_Ambient;

        float m_Fade = 0.0f;
        std::shared_ptr<std::function<void()>> m_pDone;
        std::shared_ptr<std::string> m_pVolumeText;
        std::shared_ptr<std::string> m_pSoundText;
        std::shared_ptr<std::string> m_pMusicText;
        
        std::map<std::string, std::vector<std::string>> m_Binds;
};

#endif

