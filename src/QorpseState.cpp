#include "QorpseState.h"
#include "Qor/Input.h"
#include "Qor/Qor.h"
#include "Qor/TileMap.h"
#include "Qor/Sound.h"
#include "Qor/Sprite.h"
#include <glm/glm.hpp>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "Qor/BasicPartitioner.h"
#include "World.h"
#include "Thing.h"
using namespace std;
using namespace glm;

QorpseState :: QorpseState(
    Qor* engine
    //std::string fn
):
    m_pQor(engine),
    m_pInput(engine->input()),
    m_pPipeline(engine->pipeline())
{
}

QorpseState :: ~QorpseState()
{
    m_Ambient = Color::white(); // reset shader
    m_pRoot = nullptr;
    m_pPipeline->partitioner()->clear();
}

void QorpseState :: preload()
{
    float sw = m_pQor->window()->size().x;
    float sh = m_pQor->window()->size().y;

    m_pPartitioner = m_pPipeline->partitioner();
    m_pRoot = make_shared<World>(m_pQor, m_pPartitioner);
    m_pCamera = make_shared<Camera>(m_pQor->resources(), m_pQor->window());
    m_pRoot->add(m_pCamera);

    //auto num = rand() % 2 + 1;
    //m_pMusic = m_pQor->make<Sound>(string("sh_tribute")+to_string(num)+".ogg");
    //m_pMusic = m_pQor->make<Sound>("boss_dead_find_exit.ogg");
    m_pMusic = m_pQor->make<Sound>("ingame1.ogg");
    m_pRoot->add(m_pMusic);
    
    m_pPlayer = make_shared<Character>(
        m_pQor->session(),
        m_pQor->session()->profile(0).get(),
        m_pQor->resource_path("actor.json"),
        m_pQor->resources(),
        vec3(0.0f)
        //vec3(500.0f, 128.0f, 10.0f + 2.0f)
    );
    m_pPlayer->on_death([this](const bool& b){
        m_pMusic->detach();
        m_pMusic = m_pQor->make<Sound>("rust_ambient.ogg");
        Sound::play(m_pRoot.get(), m_pPlayer->skin() + "-death.wav", m_pQor->resources());
        m_pRoot->add(m_pMusic);
        m_pMusic->play();
        // fade out
        auto fade = make_shared<float>(0.0f);
        on_tick.connect([this, fade](Freq::Time t){
            const float inc = t.s() / 5.0f;
            *fade += inc; // fade seconds
            m_Ambient = m_Ambient * Color(1.0f - *fade);
            auto scale = 300.0f / std::max<float>(
                m_pQor->window()->size().x * 1.0f,1.0f
            );
            scale *= 1.0f + *fade;
            m_pCamera->track(nullptr);
            m_pCamera->rescale(glm::vec3(
                scale, scale,
                1.0f
            ));
            if(*fade > 1.0f)
                m_pQor->change_state(m_pQor->states().class_id("menu"));
        });
    });
    if(m_pRoot->map())
        m_pPlayer->mesh()->set_geometry(m_pRoot->map()->tilted_tile_geometry());

    m_pPlayer->add_camera(m_pCamera);
    m_pHUD = make_shared<HUD>(
        m_pQor->window(),
        m_pPlayer
    );
    m_pHUD->move(vec3(0.0f, 0.0f, 55.0f));
    m_pCamera->add(m_pHUD);
    m_pHUD->each([](Node* n){
        n->layer(World::INDICATOR);
    }, Node::Each::RECURSIVE | Node::Each::INCLUDE_SELF);
    
    //m_pPlayer->scale(2.0f);
    //m_pRoot->rescale(glm::vec3(4.0f, 4.0f, 1.0f));
    
    m_pPlayerInterface = make_shared<CharacterInterface>(
        m_pQor->session()->profile(0)->controller(),
        m_pPlayer,
        m_pRoot.get(), // world
        m_pQor->resources()
    );
    m_pPlayerInterface->plug();
    
    // update direction, this is not automatic yet until we
    // make a custom PlayerInteface2D
    //{
    //    auto playerptr = m_pPlayer.get();
    //    auto playerint = m_pPlayerInterface.get();
    //    m_pPlayer->on_tick.connect([playerptr, playerint](Freq::Time){
    //        playerptr->set_direction(playerint->direction());
    //    });
    //}
    
    auto scale = 300.0f / std::max<float>(sw * 1.0f,1.0f);
    m_pCamera->rescale(glm::vec3(
        scale, scale,
        1.0f
    ));
    m_pCamera->mode(Tracker::FOLLOW);
    m_pCamera->track(m_pPlayerInterface->crosshair()->mesh()->as_node());
    m_pCamera->focal_offset(vec3(
        -m_pQor->window()->center().x * 1.0f,
        -m_pQor->window()->center().y * 1.0f,
        0.0f
    ));
    m_pCamera->listen(true);
    m_pRoot->setup_camera(m_pCamera.get());
    m_pRoot->spawn_player(m_pPlayer);
    
    m_Ambient.on_change.connect([this](const Color& c){
        int u = m_pPipeline->shader(1)->uniform("Ambient");
        float fade = m_Thunder.get();
        if(u != -1)
            m_pPipeline->shader(1)->uniform(u, vec4(c.vec3() * fade, c.a()));
    });
    
    m_pRain = m_pQor->make<Sound>("rain.ogg");
    m_pCamera->add(m_pRain);

    m_pTextScroller = make_shared<TextScroller>(
        m_pQor->window(),
        m_pQor->session()->profile(0)->controller().get(),
        "Press Start 2P",
        m_pQor->resources(),
        TextScroller::TIMED
    );
    m_pCamera->add(m_pTextScroller);
    m_pTextScroller->each([](Node* n){
        n->layer(World::INDICATOR);
    }, Node::Each::RECURSIVE | Node::Each::INCLUDE_SELF);
    
    m_pTextScroller->move(vec3(0.0f, 0.0f, 55.0f));
    m_pTextScroller->write("portrait_grampire.png", "Where is everyone?");
    
    m_pPlayer->on_speak([this](const std::string& s){
        m_pTextScroller->write("portrait_grampire.png", s);
    });
    
    auto bg = make_shared<Mesh>(
        make_shared<MeshGeometry>(Prefab::quad(vec2(sw, sh))),
        vector<shared_ptr<IMeshModifier>>{
            make_shared<Wrap>(Prefab::quad_wrap())
        },
        make_shared<MeshMaterial>("rain.png", m_pQor->resources())
    );
    //auto bg2 = bg->instance();
    bg->position(vec3(0.0f,0.0f,50.0f));
    m_pCamera->add(bg);
    bg->each([](Node* n){
        n->layer(World::WEATHER); // weather effect layer
    }, Node::Each::RECURSIVE | Node::Each::INCLUDE_SELF);

    on_tick.connect([this, bg](Freq::Time t){
        m_WrapAccum.x += t.seconds() * 0.5f;
        m_WrapAccum.y -= t.seconds() * 0.5f;
        
        m_WrapAccum.x = std::fmod(m_WrapAccum.x, 1.0f);
        m_WrapAccum.y = std::fmod(m_WrapAccum.y, 1.0f);

        // TODO: offset this by camera world position
        
        bg->swap_modifier(0, make_shared<Wrap>(Prefab::quad_wrap(
             vec2(0.0f, 0.0f), vec2(1.0f, 1.0f),
             vec2(1.6f, 0.9f), //scale
             vec2(m_WrapAccum.x * 1.0f, m_WrapAccum.y * 1.0f) //offset
        )));
    });
    m_pThunderSound = m_pQor->make<Sound>("thunder.wav");
    m_pCamera->add(m_pThunderSound);
    on_tick.connect([this](Freq::Time t){
        m_Thunder.logic(t);
        
        float fade = m_Thunder.get();
        int fadev = m_pPipeline->shader(1)->uniform("Ambient");
        //vec3 randv(
        //    //(rand() % 200) / 1000.f,
        //    //(rand() % 200) / 1000.f,
        //    (rand() % 100) / 1000.f
        //);
        if(fadev != -1)
            m_pPipeline->shader(1)->uniform(
                fadev,
                vec4(m_Ambient.get().vec3() * fade/* + randv*/, 1.0f)
            );
    });

    m_pRoot->cache();
    m_pPartitioner->preload();
}

void QorpseState :: enter()
{
    float sw = m_pQor->window()->size().x;
    float sh = m_pQor->window()->size().y;

    m_pPipeline->blend(false);
    
    m_pCamera->ortho();
    m_pPipeline->winding(true);
    
    m_pCamera->focus_time(Freq::Time::seconds(0.0));
    m_pCamera->finish();
    m_pCamera->focus_time(Freq::Time::seconds(0.5));
    
    if(m_pMusic)
        m_pMusic->play();
    
    m_pRain->play();
    
    reset_thunder();
    m_Ambient = Color(0.6f, 0.5f, 0.6f);
}

void QorpseState :: reset_thunder()
{
    m_Thunder.stop(1.0f);
    m_Thunder.frame(Frame<float>(
        1.0f, Freq::Time::seconds(2),
        INTERPOLATE(linear<float>))
    );
    m_Thunder.frame(Frame<float>(
        5.0f, Freq::Time::ms(20), 
        INTERPOLATE(in_sine<float>),[this]{
            m_pThunderSound->source()->stop();
            m_pThunderSound->play();
        }
    ));
    m_Thunder.frame(Frame<float>(1.0f,
        Freq::Time::ms(200),
        INTERPOLATE(out_sine<float>)
    ));
    m_Thunder.frame(Frame<float>(
        1.0f, Freq::Time::seconds(2 + (rand() % 4) * 2), 
        INTERPOLATE(linear<float>),
        std::bind(&QorpseState::reset_thunder, this)
    ));
}

void QorpseState :: logic(Freq::Time t)
{
    Actuation::logic(t);
    
    //auto pp = m_pPlayerInterface->crosshair()->position(Space::WORLD);
    //LOGf("player: (%s, %s, %s)",
    //    pp.x %
    //    pp.y %
    //    pp.z
    //);

    m_pPlayer->set_direction(m_pPlayerInterface->direction());
    //if(m_pPlayer->on_dead())
    //    m_pQor->change_state(m_pQor->states().class_id("menu"));
    
    if(m_pInput->key(SDLK_ESCAPE))
        m_pQor->quit();
    
    m_pRoot->logic(t);
    m_pPipeline->logic(t);
}

void QorpseState :: render() const
{
    m_pPipeline->render(m_pRoot.get(), m_pCamera.get());
}

