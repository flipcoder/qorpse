#ifndef _QORPSEINTERFACE_H_W5TRRMBU
#define _QORPSEINTERFACE_H_W5TRRMBU

#include <memory>
#include <cstdint>
#include <boost/optional.hpp>
#include "Qor/kit/cache/cache.h"
#include "Qor/NodeInterface.h"
#include "Qor/Input.h"
#include "Qor/Sprite.h"
#include "Qor/kit/freq/animation.h"
#include "Character.h"
class World;

class CharacterInterface:
    public NodeInterface,
    public std::enable_shared_from_this<CharacterInterface>
{
    public:

        enum class Button: unsigned int
        {
            UP,
            DOWN,
            LEFT,
            RIGHT,

            SHOOT,
            SPRINT,
            ACTION,
            STRAFE,
            RELOAD,
            PREVIOUS_WEAPON,
            NEXT_WEAPON,

            MAX,

        };

        enum class State: unsigned int
        {
            UP,
            DOWN,
            LEFT,
            RIGHT,

            STAND,
            WALK,
            
            DEAD,

            MAX
        };

        CharacterInterface(
            const std::shared_ptr<Controller>& input,
            const std::shared_ptr<Node>& character,
            World* world,
            Cache<Resource, std::string>* resources
        );
        virtual ~CharacterInterface() {}

        virtual void event() override;
        virtual void logic(Freq::Time t) override;

        const Input::Switch& button(
            Button btn
        ){
            assert(m_pInput); // should be locked

            try{
                unsigned int idx = m_Buttons.at((unsigned int)btn);
                if(idx != std::numeric_limits<unsigned int>::max())
                    return m_pInput->button(idx);
            }catch(const std::out_of_range&){}

            return m_pInput->input()->dummy_switch();
        }

        void set_state(State state) {
            assert(m_pCharacter);
            unsigned int idx = m_States.at((unsigned int)state);
            assert(idx != std::numeric_limits<unsigned int>::max());
            m_pCharacter->set_state(idx);
        }

        /*
         * Turns NodeInterface's controller() weak_ptr into m_pInput
         */
        void lock_input() { m_pInput = controller(); }
        void unlock_input() { m_pInput.reset(); }
        void lock_sprite() { m_pCharacter= std::static_pointer_cast<Character>(node()); }
        void unlock_sprite() { m_pCharacter.reset(); }

        /*
         * Make sure Input can call this interface's logic
         *
         * Warning: can't do this in constructor, so we do it on the fly
         */
        void plug() {
            if(!m_InterfaceID)
                m_InterfaceID = controller()->add_interface(
                    std::static_pointer_cast<IInterface>(
                        shared_from_this()
                    )
                );
        }
        /*
         * Manually unplugs interface from input system
         *
         * This does not need to be called since Input system's weak_ptr's
         * will allow Interfaces that go out of scope to auto-unplug
         */
        void unplug() {
            if(m_InterfaceID)
            {
                controller()->remove_interface(
                    *m_InterfaceID
                );
                m_InterfaceID = boost::optional<unsigned int>();
            }
        }

        std::shared_ptr<Character> character() {
            return std::static_pointer_cast<Character>(node());
        }

        std::shared_ptr<Sprite> crosshair() {
            return m_pCrosshair.lock();
        }

        glm::vec2 direction() const {
            return m_vDir;
        }
        
        void reload();
        
    private:

        World* m_pWorld;
        
        std::shared_ptr<Controller> m_pInput;
        std::shared_ptr<Character> m_pCharacter;

        std::array<unsigned int, (unsigned int)Button::MAX> m_Buttons;
        std::array<unsigned int, (unsigned int)State::MAX> m_States;
        std::vector<std::string> m_ButtonNames;
        std::vector<std::string> m_StateNames;

        glm::vec2 m_vMove;
        glm::vec2 m_vDir;

        float m_fSprintMult = 1.5f;
        float m_fWalkSpeed = 100.0f;
        float m_fSpeed = 100.0f;
        float m_fCrosshairDist = 32.0f;

        Animation<Angle> m_CrosshairEase;
        std::weak_ptr<Sprite> m_pCrosshair;
        boost::optional<unsigned int> m_InterfaceID;

        Cache<Resource, std::string>* m_pResources;

};

#endif

