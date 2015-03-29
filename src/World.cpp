#include "World.h"
#include "Thing.h"
#include "Qor/Sound.h"
#include <algorithm>
using namespace std;
using namespace glm;
namespace _ = std::placeholders;

World :: World(
    Qor* qor,
    BasicPartitioner* partitioner
):
    m_pQor(qor),
    m_pResources(qor->resources()),
    m_pPartitioner(partitioner)
{
    //m_pMap = m_pQor->make<TileMap>("modern.tmx");
    m_pMap = m_pQor->make<TileMap>("theGraveyard.tmx");
    //m_pMap = m_pQor->make<TileMap>("test.tmx");
    if(m_pMap)
    {
        add(m_pMap);
        
        vector<vector<shared_ptr<TileLayer>>*> layer_types {
            &m_pMap->layers(),
            &m_pMap->object_layers()
        };
        for(auto&& layers: layer_types)
        for(auto&& layer: *layers)
        {
            for(auto&& tile: *layer)
            {
                // read object properties and replace
                auto obj = std::dynamic_pointer_cast<MapTile>(tile);
                if(obj)
                {
                    auto obj_cfg = obj->config();
                    obj->box() = obj->mesh()->box();
                    if(obj_cfg->at<string>("name","")=="player_start")
                    {
                        obj->visible(false);
                        obj->mesh()->visible(false);
                        m_Spawns.push_back(obj.get());
                        continue;
                    }
                    else
                    {
                        if(Thing::get_id(obj_cfg))
                        {
                            auto thing = make_shared<Thing>(
                                obj_cfg,
                                obj.get(),
                                this,
                                m_pMap.get(),
                                partitioner,
                                qor->resources()
                            );
                            obj->add(thing);
                            //LOGf("mat: %s", Matrix::to_string(*obj->matrix()));
                            ////obj->each([this](Node* n){
                            ////    n->visible(false);
                            ////}, Node::Each::RECURSIVE | Node::Each::INCLUDE_SELF);
                            setup_thing(thing);
                            continue;
                        }
                    }
                    
                    bool depth = layer->depth() || obj_cfg->has("depth");
                    if(depth)
                    {
                        auto n = make_shared<Node>();
                        n->name("mask");
                        auto mask = obj_cfg->at<shared_ptr<Meta>>("mask", shared_ptr<Meta>());
                        bool hflip = obj->orientation() & (unsigned)MapTile::Orientation::H;
                        bool vflip = obj->orientation() & (unsigned)MapTile::Orientation::V;
                        if(obj_cfg->has("sidewall") && not mask)
                        {
                            hflip ^= obj_cfg->at<string>("sidewall","")=="right";
                            mask = make_shared<Meta>();
                            mask->append<double>({0.0, 0.0, 0.25, 1.0});
                        }
                        if(mask && mask->size()==4)
                        {
                            n->box() = Box(
                                vec3(mask->at<double>(0), mask->at<double>(1), K_EPSILON * 5.0f),
                                vec3(mask->at<double>(2), mask->at<double>(3), 0.5f)
                            );
                        }
                        else
                        {
                            n->box() = Box(
                                vec3(0.0f, 0.5f, K_EPSILON * 5.0f),
                                vec3(1.0f, 1.0f, 0.5f)
                            );
                        }
                        if(hflip){
                            n->box().min().x = 1.0f - n->box().min().x;
                            n->box().max().x = 1.0f - n->box().max().x;
                        }
                        if(vflip){
                            n->box().min().y = 1.0f - n->box().min().y;
                            n->box().max().y = 1.0f - n->box().max().y;
                        }
                        obj->mesh()->add(n);
                        if(obj_cfg->has("door"))
                        {
                            m_pPartitioner->register_object(n, DOOR);
                        }else{
                            m_pPartitioner->register_object(n, STATIC);
                            obj_cfg->set<string>("static", "");
                        }
                    }
                    else {
                        m_pPartitioner->register_object(obj->mesh(), GROUND);
                        obj->mesh()->config()->set<string>("static", "");
                    }
                }
            }
        }

        m_pPartitioner->on_collision(
            CHARACTER, STATIC,
            std::bind(&World::cb_to_tile, this, _::_1, _::_2)
        );
        m_pPartitioner->on_collision(
            THING, STATIC,
            std::bind(&Thing::cb_to_static, _::_1, _::_2)
        );
        m_pPartitioner->on_collision(
            BULLET, STATIC,
            std::bind(&World::cb_bullet_to_static, this, _::_1, _::_2)
        );
        m_pPartitioner->on_collision(
            THING, BULLET,
            std::bind(&Thing::cb_to_bullet, _::_1, _::_2)
        );
        m_pPartitioner->on_collision(
            CHARACTER, THING,
            std::bind(&Thing::cb_to_player, _::_1, _::_2)
        );
        m_pPartitioner->on_collision(
            CHARACTER, DOOR,
            std::bind(&World::cb_open_door, this, _::_1, _::_2),
            std::bind(&World::cb_not_on_door, this, _::_1, _::_2)
        );

        m_pPartitioner->on_collision(
            CHARACTER_HEAD, GROUND,
            function<void(Node*,Node*)>(),
            function<void(Node*,Node*)>(),
            [this](Node* a, Node*){
                //LOGf("a %s, b %s", string(a->world_box()) % string(b->world_box()));
                Character* playerptr = (Character*)(a->parent()->parent());
                bool was_indoors = playerptr->indoors();
                playerptr->indoors(true);
                if(was_indoors != playerptr->indoors()) // changed
                {
                    //LOG("indoors");
                }
            },
            [this](Node* a, Node*){
                //LOGf("a %s, b %s", string(a->world_box()) % string(b->world_box()));
                Character* playerptr = (Character*)(a->parent()->parent());
                bool was_indoors = playerptr->indoors();
                playerptr->indoors(false);
                if(was_indoors != playerptr->indoors()) // chnaged
                {
                    //LOG("outdoors");
                }
            }
        );
        
        // TODO: bake each map layer independently
        for(auto&& layers: layer_types)
            for(auto&& layer: *layers)
            {
                Mesh::bake(layer, m_pQor->pipeline(), [](Node* n){
                    return n->visible() && n->config()->has("static");
                });
            }
    }
}

void World :: setup_thing(std::shared_ptr<Thing> thing)
{
    thing->init_thing();
    m_Things.push_back(thing);

    for(auto&& player: m_Characters)
        setup_player_to_thing(player,thing);
}

void World :: spawn_player(std::shared_ptr<Character> player)
{
    try{
        m_Spawns.at(0)->add(player);
    }catch(...){
        WARNING("Map has no spawn points");
        add(player);
    }
    player->collapse(Space::WORLD);
    player->rescale(vec3(1.0f));
    auto pos = player->position();
    player->position(vec3(pos.x, pos.y, floor(pos.z)));
    m_Characters.push_back(player);
    setup_player(player);
}

std::shared_ptr<Node> World :: find_mask(std::shared_ptr<Node> n, std::string mask_name)
{
    auto mask_itr = find_if(ENTIRE(*n),
        [mask_name](const std::shared_ptr<Node>& n){
            return n->name() == mask_name;
        }
    );
    if(mask_itr == n->end())
        return nullptr;
    return *mask_itr;
}

void World :: setup_player(std::shared_ptr<Character> player)
{
    // create masks
    {
        auto n = make_shared<Node>();
        n->name("mask");
        n->box() = Box(
            vec3(0.25f, 0.5f, K_EPSILON * 5.0f),
            vec3(0.75f, 1.0f, 1.0f - K_EPSILON * 5.0f)
        );
        player->mesh()->add(n);
        //LOGf("player box: %s", string(n->box()));
        //LOGf("player box: %s", string(n->world_box()));
        m_pPartitioner->register_object(n, CHARACTER);
    }
    {
        auto n = make_shared<Node>();
        n->name("head_mask");
        n->box() = Box(
            vec3(0.4f, 0.0f, 1.0f - K_EPSILON * 5.0f),
            vec3(0.6f, 0.1f, 1.8f - K_EPSILON * 5.0f)
        );
        player->mesh()->add(n);
        m_pPartitioner->register_object(n, CHARACTER_HEAD);
    }
    
    //setup_player_to_map(player);
    for(auto&& thing: m_Things)
        setup_player_to_thing(player,thing);
}

void World :: setup_bullet(std::shared_ptr<Sprite> bullet)
{
    m_pPartitioner->register_object(bullet->mesh(), BULLET);
}

void World :: setup_player_to_map(std::shared_ptr<Character> player)
{
    if(not m_pMap)
        return;
    
    //auto player_mask = find_mask(player->mesh());
    //auto player_head_mask = find_mask(player->mesh(), "head_mask");
    //m_pPartitioner->register_object(player_head_mask, CHARACTER_HEAD);
    
    //vector<vector<std::shared_ptr<TileLayer>>*> layer_types {
    //    &m_pMap->layers(),
    //    &m_pMap->object_layers()
    //};
    
    
    //for(auto&& layers: layer_types)
    //for(auto&& layer: *layers)
    //{
        //if(not layer->depth())
        //    continue;
    //    for(auto&& tile: *layer)
    //    {
    //        auto obj = std::dynamic_pointer_cast<MapTile>(tile);
    //        if(not obj || not obj->mesh() || not obj->visible())
    //            continue;

    //        auto mask = find_mask(obj->mesh());
            
    //        if(player_mask && mask)
    //        {
    //            if(not obj->config()->has("door"))
    //            {
    //                m_pPartitioner->on_collision(
    //                    player_mask, mask,
    //                    std::bind(&World::cb_player_to_static, this, _::_1, _::_2)
    //                );
    //            } else {
    //                m_pPartitioner->on_collision(
    //                    player_mask, mask,
    //                    std::bind(&World::cb_player_to_door, this, _::_1, _::_2),
    //                    function<void(Node*,Node*)>(),
    //                    std::bind(&World::cb_player_enter_door, this, _::_1, _::_2),
    //                    std::bind(&World::cb_player_leave_door, this, _::_1, _::_2)
    //                );
    //            }
    //        }
                
    //        if(player_head_mask)
    //        {
    //            //LOGf("mesh box: %s", string(obj->mesh()->world_box()));
    //            auto playerptr = player.get();
    //            m_pPartitioner->on_collision(
    //                // player_head_mask
    //                player_head_mask, obj->mesh(), // use full object, not mask
    //                function<void(Node*,Node*)>(),
    //                function<void(Node*,Node*)>(),
    //                [playerptr, this](Node* a, Node* b){
    //                    //LOGf("a %s, b %s", string(a->world_box()) % string(b->world_box()));
    //                    bool was_indoors = playerptr->indoors();
    //                    playerptr->indoors(true);
    //                    if(was_indoors != playerptr->indoors()) // changed
    //                    {
    //                        //LOG("indoors");
    //                    }
    //                },
    //                [playerptr, this](Node* a, Node* b){
    //                    //LOGf("a %s, b %s", string(a->world_box()) % string(b->world_box()));
    //                    bool was_indoors = playerptr->indoors();
    //                    playerptr->indoors(false);
    //                    if(was_indoors != playerptr->indoors()) // chnaged
    //                    {
    //                        //LOG("outdoors");
    //                    }
    //                }
    //            );
    //        }
    //    }
    //}
}

void World :: setup_player_to_thing(std::shared_ptr<Character> player, std::shared_ptr<Thing> thing)
{
    thing->setup_player(player);
}

void World :: setup_thing_to_map(std::shared_ptr<Thing> thing)
{
    thing->setup_map(m_pMap);
}

void World :: cb_to_static(Node* a, Node* b, Node* m)
{
    if(not m) m = a;
    
    auto p = m->position(Space::PARENT);
    auto col = [this, a, b]() -> bool {
        return (not m_pPartitioner->get_collisions_for(a, STATIC).empty()) ||
            a->world_box().collision(b->world_box());
    };
    
    Box overlap = a->world_box().intersect(b->world_box());
    //LOGf("collision: %s", string(a->world_box()));
    //LOGf("collision: %s", string(b->world_box()));
    
    vec3 overlap_sz = overlap.size() + vec3(1.0f);
    if(not floatcmp(m->velocity().x, 0.0f))
    {
        auto np = vec3(p.x - sgn(m->velocity().x) * overlap_sz.x, p.y, p.z);
        m->position(np);
        if(not col())
            return;
    }
    if(not floatcmp(m->velocity().y, 0.0f))
    {
        m->position(vec3(p.x, p.y - sgn(m->velocity().y) * overlap_sz.y, p.z));
        if(not col())
            return;
    }
    
    try{
        auto old_pos = Matrix::translation(kit::safe_ptr(m->snapshot(0))->world_transform);
        m->position(vec3(old_pos.x, old_pos.y, p.z));
    }catch(const kit::null_ptr_exception&){}
}

void World :: cb_to_tile(Node* a, Node* b)
{
    cb_to_static(a, b, a->parent()->parent());
}

void World :: cb_open_door(Node* a, Node* b)
{
    Mesh* door = ((Mesh*)b->parent());
    if(door->visible()){
        door->visible(false);
        sound(door, "door.wav");
    }
}

void World :: cb_not_on_door(Node* a, Node* b)
{
    Mesh* door = ((Mesh*)b->parent());
    if(not door->visible())
        door->visible(true);
}

void World :: sound(Node* n, std::string fn)
{
    shared_ptr<Sound> snd;
    try{
        snd = make_shared<Sound>(fn, m_pResources);
    }catch(...){
        WARNINGf("missing sound: %s", fn);
        return; // TEMP: ignore missing sounds
    }
    n->add(snd);
    snd->collapse(Space::WORLD);
    snd->play();
    auto sndptr = snd.get();
    snd->on_tick.connect([sndptr](Freq::Time t){
        if(not sndptr->source()->playing())
            sndptr->detach();
    });
}

void World :: cb_bullet_to_static(Node* a, Node* b)
{
    Node* bullet = a->parent();
    sound(bullet, "hit.wav");
    if(not bullet->config()->at("penetration",false))
        bullet->on_tick.connect([bullet](Freq::Time){
            bullet->detach();
        });
}

//void World :: cb_player_enter_door(Node* a, Node* b)
//{
//    LOG("door");
//    // if door closed and unlocked open door
//    ((Mesh*)b->parent())->visible(false);
//}

//void World :: cb_player_leave_door(Node* a, Node* b)
//{
//    // if door open shut door?
//    ((Mesh*)b->parent())->visible(true);
//}

void World :: setup_camera(Camera* camera)
{
    camera->set_node_visible_func([this, camera](const Node* n, Node::LoopCtrl* lc){
        auto npos = n->position(Space::WORLD).z;

        // indicators are always visible
        if(n->layer() == INDICATOR){
            //if(lc) *lc = LC_SKIP;
            return true;
        }
        if(not camera->target())
            return true;
        auto player = camera->target()->parent()->parent();
        auto playerpos = player->position(Space::WORLD).z;
        Character* c = dynamic_cast<Character*>(player);
        if(c->indoors())
        {
            if(n->layer() == WEATHER){
                //if(lc) *lc = LC_SKIP;
                return false;
            }
            return npos < playerpos + 1.0f - K_EPSILON || npos >= 49.0f;
        }
        return true;
    });
}

