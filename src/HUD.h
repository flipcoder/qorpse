#ifndef HUD_H_OWVM6H1E
#define HUD_H_OWVM6H1E

#include <memory>
#include "Qor/Node.h"
#include "Qor/Window.h"
#include "Qor/Canvas.h"
#include "Character.h"

class HUD:
    public Node
{
    public:
        
        HUD(Window* win, const std::shared_ptr<Character>& character);
        virtual void logic_self(Freq::Time t) override;
        
        void target(const std::shared_ptr<Character>& character) {
            m_pCharacter = character;
        }
        std::shared_ptr<Character> target() {
            return m_pCharacter.lock();
        }

        void dirty(bool b) { m_Dirty = true; }
        
    private:

        void setup_character();
        
        float m_Fade = 0.0f;
        
        Window* m_pWindow = nullptr;
        std::shared_ptr<Canvas> m_pCanvas;
        std::weak_ptr<Character> m_pCharacter;

        boost::signals2::scoped_connection m_HPChange;
        boost::signals2::scoped_connection m_AmmoChange;

        bool m_Dirty = true;
};

#endif

