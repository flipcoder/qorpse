#include <boost/scope_exit.hpp>
#include "CharacterInterface.h"
#include "Qor/Sound.h"
#include <glm/gtx/vector_angle.hpp>
#include "World.h"
using namespace std;
using namespace glm;

CharacterInterface :: CharacterInterface(
    const shared_ptr<Controller>& input,
    const shared_ptr<Node>& character,
    World* world,
    Cache<Resource, std::string>* resources
):
    NodeInterface(input, character),
    m_pWorld(world),
    m_pResources(resources)
{
    lock_sprite();
    BOOST_SCOPE_EXIT(this_) {
        this_->unlock_sprite();
    } BOOST_SCOPE_EXIT_END

    m_ButtonNames = {
        "up",
        "down",
        "left",
        "right",

        "shoot",
        "sprint",
        "action",
        "strafe",
        "reload",
        "previous_weapon",
        "next_weapon"
    };

    m_StateNames = {
        "up",
        "down",
        "left",
        "right",

        "stand",
        "walk",
        
        "death"
    };

    for(unsigned int i=0; i<(unsigned int)Button::MAX; ++i)
        try{
            m_Buttons[i] = input->button_id(m_ButtonNames[i]);
        }catch(const out_of_range&){
            m_Buttons[i] = numeric_limits<unsigned int>::max();
        }
    for(unsigned int i=0; i<(unsigned int)State::MAX; ++i)
        try{
            m_States[i] = m_pCharacter->state_id(m_StateNames[i]);
        }catch(const out_of_range&){
            m_States[i] = numeric_limits<unsigned int>::max();
        }
    
    set_state(State::STAND);
    set_state(State::DOWN);

    auto crosshair = make_shared<Sprite>(
        resources->transform("arrow.png"),
        resources,
        "", // no skin
        vec3(0.0f, 0.0f, 10.0f)
    );
    crosshair->mesh()->offset(vec3(0.0f, -m_fCrosshairDist, 0.0f));
    m_pCharacter->add(crosshair);
    crosshair->each([](Node* n){
        n->layer(World::INDICATOR);
    }, Node::Each::RECURSIVE | Node::Each::INCLUDE_SELF);

    m_pCrosshair = crosshair;

    m_vDir = vec2(0.0f, 1.0f);
    m_CrosshairEase.stop(Angle::degrees(180));
}

void CharacterInterface :: event()
{
    lock_input();
    BOOST_SCOPE_EXIT_ALL(this) {
        unlock_input();
    };
    lock_sprite();
    BOOST_SCOPE_EXIT_ALL(this) {
        unlock_sprite();
    };

    if(m_pCharacter->dead()) // block control on death
    {
        m_pCharacter->velocity(glm::vec3(0.0f));
        return;
    }
    
    auto crosshair = m_pCrosshair.lock();
    assert(crosshair);

    const bool strafe = button(Button::STRAFE);

    m_vMove = vec2(0.0f);
    if(button(Button::LEFT))
        m_vMove += vec2(-button(Button::LEFT).pressure(), 0.0f);
    if(button(Button::RIGHT))
        m_vMove += vec2(button(Button::RIGHT).pressure(), 0.0f);
    if(button(Button::UP))
        m_vMove += vec2(0.0f, -button(Button::UP).pressure());
    if(button(Button::DOWN))
        m_vMove += vec2(0.0f, button(Button::DOWN).pressure());

    if(m_vMove != vec2())
        m_vMove = glm::normalize(m_vMove);

    if(!strafe)
    {
        m_CrosshairEase.resume();

        if(m_vMove != vec2())
        {
            vec2 old_dir = m_vDir;
            m_vDir = normalize(m_vMove);

            Angle a(angle(old_dir, m_vDir), Angle::RADIANS);
            if(fabs(a.degrees()) > K_EPSILON)
            {
                //auto atest = orientedAngle(
                //    vec2(m_vDir.x, -m_vDir.y),
                //    vec2(0.0f, 1.0f)
                //);
                //LOGf("atest: %s", atest);
                m_CrosshairEase.stop(
                    Angle::radians(
                        orientedAngle(
                            vec2(m_vDir.x, -m_vDir.y),
                            vec2(0.0f, 1.0f)
                        )
                    ),
                    Freq::Time(100),
                    //Freq::Time((fabs(a.degrees())  > 90.0f + K_EPSILON) ? 0 : 100),
                    INTERPOLATE(linear<Angle>)
                    //[](const Angle& a, const Angle& b, float t){
                    //    return a + (b-a)*t;
                    //}
                );
            }
        }

        // TODO: get direction from crosshair angle
        if(fabs(m_vDir.y) > K_EPSILON)
            set_state(m_vDir.y > 0.0f ? State::DOWN : State::UP);
        else if(fabs(m_vDir.x) > K_EPSILON)
            set_state(m_vDir.x > 0.0f ? State::RIGHT: State::LEFT);
    }
    else
    {
        m_CrosshairEase.pause();
    }

    //crosshair->reset_orientation();
    //*crosshair->matrix() = rotate(mat4(),
    //    orientedAngle(
    //        vec2(m_vDir.x, -m_vDir.y), vec2(0.0f, 1.0f)
    //    ),
    //    Axis::Z
    //);

    m_fSpeed = m_fWalkSpeed * std::max<float>(
        std::max<float>(button(Button::LEFT).pressure(), button(Button::RIGHT).pressure()),
        std::max<float>(button(Button::UP).pressure(), button(Button::DOWN).pressure())
    );
    if(button(Button::SPRINT) && !strafe)
    {
        m_fSpeed = m_fWalkSpeed * m_fSprintMult;
        m_pCharacter->speed(m_fSprintMult);
    }
    else
    {
        m_pCharacter->resume();
    }

    set_state(length(m_vMove) > K_EPSILON ? State::WALK : State::STAND);

    if(button(Button::PREVIOUS_WEAPON).pressed_now())
        m_pCharacter->switch_weapon(-1);
    if(button(Button::NEXT_WEAPON).pressed_now())
        m_pCharacter->switch_weapon(1);
}

void CharacterInterface :: logic(Freq::Time t)
{
    lock_sprite();
    BOOST_SCOPE_EXIT_ALL(this) {
        unlock_sprite();
    };
    lock_input();
    BOOST_SCOPE_EXIT_ALL(this) {
        unlock_input();
    };

    if(m_pCharacter->dead()) // block control on death
        return;

    m_CrosshairEase.logic(t);
    auto crosshair = m_pCrosshair.lock();
    assert(crosshair);
    //*crosshair->matrix() = m_CrosshairEase.get();

    crosshair->reset_orientation();
    glm::vec3 pos = Matrix::translation(*crosshair->matrix());

    *crosshair->matrix() = glm::rotate(m_CrosshairEase.get().radians(), Axis::Z);
    //crosshair->pend(); // will pend below
    crosshair->position(pos);

    //if(length(m_vMove) > K_EPSILON)
    m_pCharacter->velocity(vec3(m_vMove * m_fSpeed, 0.0f));

    //if(!m_pCharacter->clip())
    //    reload();

    if(button(Button::RELOAD).pressed())
        reload();
    
    if(m_pCharacter->weapon() && !m_pCharacter->weapon()->delayed() && button(Button::SHOOT).pressed())
    {
        if(m_pCharacter->clip())
        {
            unsigned num_bullets = m_pCharacter->shoot();
            
            if(num_bullets)
            {
                auto bulletsound = make_shared<Sound>(
                    m_pResources->transform(m_pCharacter->weapon()->name() + ".wav"),
                    m_pResources
                );
                m_pCharacter->add(bulletsound);
                bulletsound->play();
                auto bulletsoundptr = bulletsound.get();
                bulletsound->on_tick.connect([bulletsoundptr](Freq::Time){
                    if(not bulletsoundptr->source()->playing())
                        bulletsoundptr->detach();
                });
            }
            
            auto path = m_pResources->transform(m_pCharacter->weapon()->bullet_gfx() + ".png");
            for(unsigned i=0; i<num_bullets; i++)
            {
                auto bullet = make_shared<Sprite>(path, m_pResources);
                float vary = (rand() % (1+kit::round_int((1.0f - m_pCharacter->weapon()->accuracy()) * 45.0f))) * 1.0f;
                vary *= (rand() % 2) ? 1.0f : -1.0f;
                auto dir = (m_CrosshairEase.get() - Angle(90.0f + vary, Angle::DEGREES)).vector();
                auto bulletptr = bullet.get();
                auto range = make_shared<float>(m_pCharacter->weapon()->range());
                auto bullet_speed = m_pCharacter->weapon()->bullet_speed();
                bullet->on_tick.connect([bulletptr, dir, range, bullet_speed, vary] (Freq::Time t){
                    bulletptr->move(vec3(
                        dir.x * bullet_speed * t.s(),
                        dir.y * bullet_speed * t.s(),
                        0.0f
                    ));
                    *range -= t.s() * bullet_speed;
                    if(*range <= 0.0f)
                        bulletptr->detach();
                });
                crosshair->add(bullet);
                bullet->position(glm::vec3(0.0f, 0.0f, 0.0f));
                bullet->collapse(); // collapse to player
                bullet->collapse(); // collapse to root / map
                auto bpos = bullet->position();
                bullet->position(glm::vec3(
                    bpos.x, bpos.y, m_pCharacter->position().z + 0.25f
                ));
                m_pWorld->setup_bullet(bullet);
            }
        }else{
            reload();
        }
    }
}

void CharacterInterface :: reload()
{
    if(m_pCharacter->reload())
    {
        auto sound = make_shared<Sound>(
            "reload.wav",
            m_pResources
        );
        m_pCharacter->add(sound);
        sound->play();
        auto soundptr = sound.get();
        sound->on_tick.connect([soundptr](Freq::Time){
            if(not soundptr->source()->playing())
                soundptr->detach();
        });
    }
}

