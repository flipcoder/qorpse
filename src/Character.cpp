#include "Character.h"
using namespace std;
using namespace glm;

Character :: Character(
    Session* session,
    Profile* profile,
    const string& fn,
    Cache<Resource, std::string>* resources,
    glm::vec3 pos = glm::vec3(0.0f)
):
    m_HP(100),
    m_MaxHP(100),
    m_Armor(0),
    m_MaxArmor(0),
    Sprite(fn, resources, profile->config()->at<string>("actor"), pos),
    m_pSession(session),
    m_pProfile(profile),
    //m_pController(profile->controller().get()),
    m_pResources(resources)
{
    auto cfg = make_shared<Meta>(
        m_pResources->transform("weapons.json")
    );
    for(auto&& e: *cfg)
    {
        auto wpn = e.as<shared_ptr<Meta>>();
        m_Weapons.emplace_back(
            e.key,
            wpn->at<string>("ammo-type"),
            wpn->at<string>("gfx"),
            make_shared<int>(wpn->at<int>("ammo")),
            wpn->at<int>("in-clip"),
            wpn->at<int>("clip-capacity"),
            wpn->at<int>("chamber"),
            wpn->at<double>("range"),
            wpn->at<double>("fire-speed"),
            wpn->at<double>("reload-speed"),
            wpn->at<double>("bullet-speed"),
            wpn->at<double>("accuracy")
        );
    }

    //m_Weapon.on_change.connect([this](const int&){
    //    if(m_Weapons.empty()){
    //        m_Ammo = 0;
    //        return;
    //    }
    //    m_Ammo = m_Weapons[m_Weapon].ammo();
    //});
    set_states({"stand","down"});
    m_Dead.on_change.connect([this](const bool& b){
        if(b)
            set_states({"death"});
    });
}

void Character :: logic_self(Freq::Time t)
{
    Sprite::logic_self(t);
    for(auto& w: m_Weapons)
        w.logic(t);
}

bool Character :: indoors(bool b)
{
    //if(m_bIndoors == b)
    //    return false;
    
    m_Indoors += b?1:-1;
    return m_Indoors;

    //bool clean = true;
    //for(auto&& c: m_Cameras) {
    //    auto cam = c.lock();
    //    if(not cam) {
    //        clean = false;
    //        continue;
    //    }
    //    cam->range(
    //        -(b ? position().z: 100.0f),
    //        100.0f
    //    );
    //}
    //if(not clean)
    //    kit::remove_if(m_Cameras, [](weak_ptr<Camera> cam){
    //        return not cam.lock();
    //    });
}

void Character :: add_camera(std::weak_ptr<Camera> camera)
{
    m_Cameras.push_back(camera);
    kit::remove_if(m_Cameras, [](weak_ptr<Camera> const& cam){
        return not cam.lock();
    });
}

