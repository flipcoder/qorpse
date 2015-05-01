#ifndef _POINTCLICKSTATE_H_VZ3QNB09
#define _POINTCLICKSTATE_H_VZ3QNB09

#include "Qor/State.h"
#include "Qor/Input.h"
#include "Qor/TileMap.h"
#include "Qor/Camera.h"
#include "Qor/Pipeline.h"
#include "Qor/Mesh.h"
#include "Qor/Interpreter.h"
#include "Qor/Physics.h"
#include "Qor/ViewModel.h"
//#include "Qor/BasicPhysics.h"
#include "Qor/Sprite.h"
#include "CharacterInterface.h"
//#include "Qor/PlayerInterface3D.h"
#include "TextScroller.h"
class BasicPartitioner;
#include "HUD.h"
#include "Character.h"
#include "World.h"

class Qor;

class QorpseState:
    public State
{
    public:
        
        QorpseState(Qor* engine);
        virtual ~QorpseState();

        virtual void preload() override;
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
        
        void reset_thunder();
        
    private:
        
        Qor* m_pQor = nullptr;
        Input* m_pInput = nullptr;
        Pipeline* m_pPipeline = nullptr;

        std::shared_ptr<World> m_pRoot;
        //Interpreter* m_pInterpreter;
        //std::shared_ptr<Interpreter::Context> m_pScript;
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<Mesh> m_pGun;
        std::shared_ptr<ViewModel> m_pViewModel;
        //std::shared_ptr<TileMap> m_pMap;
        std::shared_ptr<Character> m_pPlayer;
        std::shared_ptr<Sound> m_pMusic;
        BasicPartitioner* m_pPartitioner = nullptr;
        
        std::shared_ptr<CharacterInterface> m_pPlayerInterface;
        //std::shared_ptr<PlayerInterface3D> m_pPlayerInterface;
        std::shared_ptr<TextScroller> m_pTextScroller;
        std::shared_ptr<HUD> m_pHUD;
        //std::shared_ptr<Physics> m_pPhysics;
        
        // TODO: move to world or scene
        glm::vec2 m_WrapAccum;
        Animation<float> m_Thunder;
        std::shared_ptr<Sound> m_pThunderSound;

        kit::reactive<Color> m_Ambient;
        
        std::string m_Filename;
        std::shared_ptr<Sound> m_pRain;

        //  Testing level or writing to profile?
        bool m_bTestMode = false;
};

#endif

