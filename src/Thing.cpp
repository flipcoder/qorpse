#include "Thing.h"
#include "World.h"
#include "Qor/Filesystem.h"
#include "Qor/Sound.h"
#include <memory>
using namespace std;

const std::vector<std::string> Thing :: s_TypeNames({
    "",
    
    // monsters
    "raven",
    "skeleton",
    "zombie",
    "ghost",
    "meat_fly",
    "bighead",
    "severed_head",
    "mask_kid",
    "hooded_priest",
    "bernie",
    "fetus_maximus",
    
    // items
    "medkit",
    "key",
    "lock",
    "rifle",
    "grenade_launcher",
    "flamethrower",
    "shotgun",
    "bazooka",
    "uzis",
    "minigun",
});

Thing :: Thing(
    const std::shared_ptr<Meta>& config,
    MapTile* placeholder,
    World* world,
    TileMap* map,
    BasicPartitioner* partitioner,
    Cache<Resource, std::string>* resources
):
    Node(config),
    m_pPlaceholder(placeholder),
    m_pPartitioner(partitioner),
    m_pWorld(world),
    m_pMap(map),
    m_pResources(resources),
    m_Identity(config->at<string>("name","")),
    m_ThingID(get_id(config))
{
}

//Thing :: Thing(
//    const std::string& fn,
//    Cache<Resource, std::string>* resources
//):
//    Node(fn),
//    m_Identity(Filesystem::getFileNameNoExt(fn)),
//    m_pResources(resources)
//{
//    on_add.connect([this]{
//        init_thing();
//    });
//}

void Thing :: init_thing()
{
    // precond: thing attached, placeholder is node parent
    assert(m_pPartitioner);
    
    //LOGf("?: %s", m_pConfig->serialize(MetaFormat::JSON));
    //LOGf("this: %s", this);
    
    m_Box = m_pPlaceholder->box();
    
    if(is_monster())
    {
        m_pSprite = make_shared<Sprite>(
            m_pResources->transform(m_Identity+".json"),
            m_pResources
        );
        add(m_pSprite);
        m_pSprite->set_state(0);
        if(m_pPlaceholder->tile_layer()->depth() || m_pConfig->has("depth"))
            m_pSprite->mesh()->set_geometry(m_pMap->tilted_tile_geometry());
        collapse(); // detach from placeholder
        rescale(glm::vec3(1.0f));
        position(m_pPlaceholder->position(Space::WORLD)); // inherit placeholder pos
        // adding a sprite will spawn its center on 0,0...
        // so we offset
        move(glm::vec3(
            m_pSprite->origin().x * m_pSprite->size().x,
            m_pSprite->origin().y * m_pSprite->size().y,
            0.0f
        ));
        //m_pPlaceholder->detach(); // don't want to invalidate iterator
        m_pPlaceholder->mesh()->visible(false); // remove placeholder
        m_pPartitioner->register_object(m_pSprite->mesh(), World::THING);
        m_Solid = true;
    }
    else if(m_ThingID == LOCK)
    {
        // on top of doors
        m_pPlaceholder->move(glm::vec3(0.0f, 0.0f, 0.1f));
        m_pPartitioner->register_object(shared_from_this(), World::THING);
        m_Solid = true;
    }
    
    // setup behavior (based on identity)
}

void Thing :: orient(glm::vec3 vec)
{
    float s = atan2<float>(vec.y, vec.x) / K_TAU;
    auto frame = kit::round_int(4.0f * s);
    //LOGf("turn %s / frame %s", s % frame);
    if(frame==0)
        m_pSprite->set_state("right");
    else if(frame==1)
        m_pSprite->set_state("down");
    else if(frame==2 || frame==-2)
        m_pSprite->set_state("left");
    else if(frame==-1)
        m_pSprite->set_state("up");
}

void Thing :: sound(const std::string& fn)
{
    Sound::play(this, fn, m_pResources);
}

void Thing :: chase(Node* node)
{
    auto vec = node->position(Space::WORLD) - position(Space::WORLD);
    vec.z = 0.0f;
    velocity(glm::normalize(vec) * 50.0f);
    orient(vec);
}

void Thing :: setup_player(const std::shared_ptr<Character>& player)
{
    m_bFirstPlayer = true;
    if(is_monster()){
        weak_ptr<Character> chrwp(player);
        auto* playerptr = player.get();
        function<void()> on_activate = [this](){
            sound(m_Identity + ".wav");
        };
        function<void(Freq::Time)> active_tick = [this](Freq::Time t){
            // ...
        };
        function<void(Freq::Time)> inactive_tick = [this, chrwp, active_tick, on_activate](Freq::Time t){
            auto chr = chrwp.lock();
            if(not chr)
                return; // TODO: remove this from on_tick signal
            float dist = glm::distance(position(Space::WORLD), chr->position(Space::WORLD));
            m_pTarget = static_pointer_cast<Node>(chr->as_node());
            if(dist <= 200.0f){
                chase(chr.get());
                on_tick.sync([this, active_tick, on_activate]{
                    on_tick.clear();
                    on_tick.connect(std::move(active_tick));
                    on_activate();
                });   
            }
        };
        m_pPartitioner->on_collision(
            World::find_mask(player->mesh(), "mask"), m_pSprite->mesh(),
            function<void(Node*,Node*)>(),
            function<void(Node*,Node*)>(),
            [this,playerptr](Node* a, Node* b){ // on enter
                if(playerptr->alive()){
                    if(alive())
                    {
                        playerptr->damage(25);
                        sound(playerptr->skin() + "-hurt.wav");
                        kill();
                    }else{
                        sound("squash.wav");
                    }
                }
            }
        );
        on_tick.connect(inactive_tick);
    
    } else if (m_ThingID == MEDKIT) {
        auto* playerptr = player.get();
        assert(m_pPartitioner);
        m_pPartitioner->on_collision(
            World::find_mask(player->mesh(), "mask"), m_pPlaceholder->mesh(),
            [this,playerptr](Node* a, Node* b){
                if(playerptr->heal()){
                    sound("medkit.wav");
                    m_pPlaceholder->detach();
                }
            }
        );
    } else if (m_ThingID == KEY) {
        auto* playerptr = player.get();
        assert(m_pPartitioner);
        m_pPartitioner->on_collision(
            World::find_mask(player->mesh(), "mask"), m_pPlaceholder->mesh(),
            [this,playerptr](Node* a, Node* b){
                if(playerptr->give("key")) {
                    sound("key.wav");
                    playerptr->say("Picked up " + m_Identity + ".");
                    m_pPlaceholder->detach();
                }
            }
        );
    } else if (is_weapon()) {
        auto* playerptr = player.get();
        assert(m_pPartitioner);
        m_pPartitioner->on_collision(
            World::find_mask(player->mesh(), "mask"), m_pPlaceholder->mesh(),
            [this,playerptr](Node* a, Node* b){
                if(playerptr->give(m_Identity)) {
                    sound("reload.wav");
                    playerptr->say("Picked up " + m_Identity + ".");
                    m_pPlaceholder->detach();
                }
            }
        );
    }
}

void Thing :: setup_map(const std::shared_ptr<TileMap>& map)
{
}

void Thing :: setup_other(const std::shared_ptr<Thing>& thing)
{
}

//unsigned Thing :: get_id(std::string identity)
//{
//    return 1;
//}

unsigned Thing :: get_id(const std::shared_ptr<Meta>& config)
{
    string name = config->at<string>("name","");
    
    if(name.empty())
        return INVALID_THING;
    auto itr = std::find(ENTIRE(s_TypeNames),name);
    if(itr == s_TypeNames.end())
        return INVALID_THING;
    
    return std::distance(s_TypeNames.begin(), itr);
}

void Thing :: logic_self(Freq::Time t)
{
    if(is_monster())
    {
        if(m_Dying && not m_Dead)
        {
            m_Dead = true;

            // do death animation stuff here
            sound(m_Identity + "-death.wav");
            try{
                m_pSprite->set_state("death");
                //m_pSprite->on_cycle_done_once([this]{
                //    detach();
                //});
            }catch(const std::out_of_range&){
            }
        }
    }
}

void Thing :: cb_to_player(Node* player_node, Node* thing_node)
{
    //LOG("cb_to_player");
    shared_ptr<Character> player;
    
    player = dynamic_pointer_cast<Character>(player_node->parent()->parent()->as_node());
    assert(player);
    auto thing = find_thing(thing_node);
    assert(thing);
    
    if(thing->id() ==  LOCK)
    {
        //LOG("locked door coll");
        if(player->has("key"))
            return; // no collide
    }
    if(thing->solid() && not thing->is_monster())
        thing->world()->cb_to_static(player_node, thing_node, player.get());
}

void Thing :: cb_to_static(Node* thing_node, Node* static_node)
{
    auto thing = find_thing(thing_node);
    assert(thing);
    if(thing->solid() || thing->is_monster())
    {
        glm::vec3 pos_before = thing->position(Space::WORLD);
        thing->world()->cb_to_static(thing_node, static_node, thing.get());
        glm::vec3 pos_after = thing->position(Space::WORLD);
        if(thing->velocity() != glm::vec3(0.0f)){
            glm::vec3 vel = thing->velocity();
            if(pos_after.x != pos_before.x){
                vel.x = -vel.x;
                thing->velocity(vel);
                thing->orient(vel);
            }
            if(pos_after.y != pos_before.y){
                vel.y = -vel.y;
                thing->velocity(vel);
                thing->orient(vel);
            }
            
        }
    }
}

std::shared_ptr<Thing> Thing :: find_thing(Node* n)
{
    shared_ptr<Thing> thing;
    thing = dynamic_pointer_cast<Thing>(n->as_node());
    if(not thing){
        thing = dynamic_pointer_cast<Thing>(n->parent()->as_node());
        if(not thing)
            thing = dynamic_pointer_cast<Thing>(n->parent()->parent()->as_node());
    }
    return thing;
}

void Thing :: gib(Node* bullet)
{
    int gib_idx = rand() % 3;
    auto gib = make_shared<Sprite>("gib"+to_string(gib_idx+1)+".png", m_pResources);
    auto dir = Angle::degrees(1.0f * (std::rand() % 360)).vector();
    add(gib);
    gib->velocity(glm::vec3(dir, 0.0f) * 15.0f);
    auto life = make_shared<float>(0.25f);
    auto gibptr = gib.get();
    gib->on_tick.connect([gibptr, life](Freq::Time t){
        *life -= t.s();
        if(*life < 0.0f)
            gibptr->detach();
    });
}

void Thing :: cb_to_bullet(Node* thing_node, Node* bullet_node)
{
    Thing* thing = (Thing*)thing_node->parent()->parent();
    Node* bullet = bullet_node->parent();
    //a->parent()->parent()->detach();
    //b->parent()->detach();
    if(thing->is_monster() && thing->alive())
    {
        int gib_count = rand() % 3 + 1;
        for(int i=0; i < gib_count; ++i)
            thing->gib(bullet);
        
        if(thing->damage(bullet->config()->at("damage",1)))
        {
            if(not bullet->config()->at("penetration",false))
                bullet->on_tick.connect([bullet](Freq::Time){
                    bullet->detach();
                });
        }
    }
}

World* Thing :: world()
{
    return m_pWorld;
}

