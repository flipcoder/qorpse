#include "HUD.h"
#include <boost/algorithm/string.hpp>
#include <glm/glm.hpp>
using namespace std;
using namespace glm;

HUD :: HUD(Window* win, const std::shared_ptr<Character>& character):
    m_pWindow(win),
    m_pCanvas(make_shared<Canvas>(win->size().x, win->size().y)),
    m_pCharacter(character)
{
    add(m_pCanvas);
    setup_character();
}

void HUD :: logic_self(Freq::Time t)
{
    //if(m_Dirty)
    {
        auto cairo = m_pCanvas->context();
        cairo->save();
        cairo->set_source_rgba(1.0f, 1.0f, 1.0f, 0.0f);
        cairo->set_operator(Cairo::OPERATOR_SOURCE);
        cairo->paint();
        cairo->restore();
        
        auto ch = m_pCharacter.lock();
        if(!ch) return;
        
        m_Fade += t.s() * 2.5f *(1.0f-ch->hp_percent()/100.0f);
        m_Fade = fmod(m_Fade, 1.0f);
        
        cairo->select_font_face(
            "Press Start",
            Cairo::FONT_SLANT_NORMAL,
            Cairo::FONT_WEIGHT_NORMAL
        );
        float sz = kit::round_int(m_pWindow->size().y / 24.0f);
        const float shadow = 4.0f;
        
        // text with shadow offset
        for(int i=1; i>=0; --i)
        {
            cairo->set_font_size(sz);
            Color c = Color::white();
            if(i == 1)
                c = Color::black();
            cairo->set_source_rgba(c.r(), c.g(), c.b(), c.a());
            m_pCanvas->text(
                boost::to_upper_copy(ch->weapon()->name()) + " " + to_string(+ ch->clip()) + " / " + to_string(ch->ammo()),
                vec2(sz/2.0f - i*shadow, m_pWindow->size().y - sz/2.0f + i*shadow)
            );
            if(i == 0){
                c *= Color(
                    1.0f,
                    ch->hp_percent() / 100.0f,
                    ch->hp_percent() / 100.0f
                );
            }
            cairo->set_source_rgba(c.r(), c.g(), c.b(), c.a());
            cairo->set_font_size(
                sz + (1.0f - ch->hp_percent() / 100.0f) * 4.0f * (1.0f+sin(m_Fade * K_TAU)
                //+ sz * (1.0f - ch->hp_percent() / 100.0f)
            ));
            m_pCanvas->text(
                to_string(ch->hp_percent()) + "%",
                vec2(m_pWindow->size().x - sz/2.0f - i*shadow, m_pWindow->size().y - sz/2.0f + i*shadow),
                Canvas::Align::RIGHT
            );
        }

        //m_Dirty = false;
        m_pCanvas->dirty(true);
    }
    //m_Dirty = false;
}

void HUD :: setup_character()
{
    auto ch = m_pCharacter.lock();
    if(!ch) return;

    //auto dirty_cb = [this]{
    //    m_Dirty = true;
    //};

    // setup change signals
    //m_pWindow->on_resize(dirty_cb);
    //ch->on_hp_change(dirty_cb);
    //ch->on_ammo_change(dirty_cb);
}

