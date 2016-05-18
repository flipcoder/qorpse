#ifndef WORLD_H_FJLWGZSZ
#define WORLD_H_FJLWGZSZ

#include <memory>
#include "Qor/Qor.h"
#include "Qor/Node.h"
#include "Qor/BasicPartitioner.h"
#include "Qor/TileMap.h"
#include "Character.h"
class Thing;

class World:
    public Node
{
    public:
        
        enum Layers {
            DEFAULT = 0, // clippable based on character's vision
            WEATHER = 1,
            INDICATOR = 2
        };
        
        enum ObjectTypes {
            GROUND,
            STATIC,
            CHARACTER,
            CHARACTER_HEAD,
            DOOR,
            BULLET,
            THING
        };

        World(
            Qor* qor,
            BasicPartitioner* partitioner
        );
        virtual ~World() {}
        static World* world_of(Node* n) {
            while(n->parent())
                n = n->parent();
            return dynamic_cast<World*>(n);
        }

        static std::shared_ptr<Node> find_mask(std::shared_ptr<Node> n, std::string mask_name = "mask");
        void setup_thing(std::shared_ptr<Thing> thing);
        void setup_player(std::shared_ptr<Character> player);
        void setup_bullet(std::shared_ptr<Sprite> bullet);
        void spawn_player(std::shared_ptr<Character> player);
        void setup_player_to_map(std::shared_ptr<Character> player);
        void setup_player_to_thing(std::shared_ptr<Character> player, std::shared_ptr<Thing> thing);
        void setup_thing_to_map(std::shared_ptr<Thing> thing);
        
        void cb_to_static(Node* a, Node* b, Node* m = nullptr);
        void cb_to_tile(Node* a, Node* b);
        void cb_bullet_to_static(Node* a, Node* b);
        void cb_open_door(Node* a, Node* b);
        void cb_not_on_door(Node* a, Node* b);
        //void cb_player_enter_door(Node* a, Node* b);
        //void cb_player_leave_door(Node* a, Node* b);
        
        TileMap* map() {return m_pMap.get();}
        
        void setup_camera(Camera* camera);
        void sound(Node* n, std::string fn);

        //virtual void logic(Freq::Time t) override {}
        
    private:

        Qor* m_pQor;
        Cache<Resource, std::string>* m_pResources;
        BasicPartitioner* m_pPartitioner = nullptr;
        std::vector<MapTile*> m_Spawns;
        std::shared_ptr<TileMap> m_pMap;

        std::vector<std::shared_ptr<Character>> m_Characters;
        std::vector<std::shared_ptr<Thing>> m_Things;
        //std::vector<std::shared_ptr<Node>> m_Bullets;
};

#endif

