#ifndef THING_H_MPZJ6QOR
#define THING_H_MPZJ6QOR

#include <memory>
#include "Qor/Sprite.h"
#include "Character.h" 
#include "Qor/TileMap.h" 
#include "Qor/BasicPartitioner.h"
class World;

class Thing:
    public Node
{
    public:

        Thing(
            const std::shared_ptr<Meta>& config,
            MapTile* placeholder,
            World* world,
            TileMap* map,
            BasicPartitioner* partitioner,
            Cache<Resource, std::string>* resources
        );
        //Thing(
        //    const std::string& fn,
        //    Cache<Resource, std::string>* resources
        //);
        //Thing(
        //    const std::shared_ptr<Mesh>& cfg,
        //    Cache<Resource, std::string>* resources
        //);

        virtual ~Thing() {}

        virtual void logic_self(Freq::Time t) override;
        
        //void partitioner(BasicPartitioner* p){
        //    m_pPartitioner = p;
        //}
        void setup_player(const std::shared_ptr<Character>& player);
        void setup_map(const std::shared_ptr<TileMap>& map);
        void setup_other(const std::shared_ptr<Thing>& thing);
        
        // set placeholder tile used as placeholder
        //void set_placeholder(MapTile* t) {m_pPlaceholder=t;}

        static unsigned get_id(const std::shared_ptr<Meta>& config);
        static bool is_thing(std::string name);
        
        enum Type
        {
            INVALID_THING = 0,
            
            MONSTERS,
            RAVEN = MONSTERS,
            SKELETON,
            ZOMBIE,
            GHOST,
            MEAT_FLY,
            BIGHEAD,
            SEVERED_HEAD,
            MASKKID,
            HOODED_PRIEST,
            BERNIE,
            FETUS_MAXIMUS,
            MONSTERS_END,
            
            ITEMS = MONSTERS_END,
            MEDKIT = ITEMS,
            KEY,
            LOCK,

            WEAPONS,
            RIFLE = WEAPONS,
            GRENADE_LAUNCHER,
            FLAMETHROWER,
            SHOTGUN,
            BAZOOKA,
            UZIS,
            MINIGUN,
            
            WEAPONS_END,
            ITEMS_END = WEAPONS_END
        };
        const static std::vector<std::string> s_TypeNames;

        bool is_monster() const {
            return m_ThingID >= MONSTERS && m_ThingID < MONSTERS_END;
        }
        bool is_item() const {
            return m_ThingID >= ITEMS && m_ThingID < ITEMS_END;
        }
        bool is_weapon() const {
            return m_ThingID >= WEAPONS && m_ThingID < WEAPONS_END;
        }
        
        
        void init_thing();
        void orient(glm::vec3 vec);
        
        void sound(const std::string& fn);
        void chase(Node* node);

        static void cb_to_player(Node* player_node, Node* thing_node);
        static void cb_to_static(Node* thing_node, Node* static_node);
        static void cb_to_bullet(Node* thing_node, Node* bullet_node);
        
        World* world();

        void kill()
        {
            m_Dying = true;
            velocity(glm::vec3(0.0f));
        }

        // ignore m_Dying when check alive
        bool alive() const { return not m_Dead; }

        bool damage(int dmg) {
            if(m_HP <= 0 || dmg < 0)
                return false;
            m_HP = std::max(m_HP-dmg, 0);
            if(m_HP <= 0){
                m_Dying = true;
                velocity(glm::vec3(0.0f));
            }
            return true;
        }
        
        void gib(Node* bullet);

        bool solid() const { return m_Solid; }
        unsigned id() const { return m_ThingID; }

        static std::shared_ptr<Thing> find_thing(Node* n);
        
    private:

        int m_HP = 1;
        bool m_Dying = false;
        bool m_Dead = false;
        Cache<Resource, std::string>* m_pResources = nullptr;
        std::weak_ptr<Node> m_pTarget;
        std::string m_Identity;
        unsigned m_ThingID = 0;
        bool m_bActive = false;
        bool m_bFirstPlayer = false;
        MapTile* m_pPlaceholder = nullptr;
        BasicPartitioner* m_pPartitioner = nullptr;
        World* m_pWorld = nullptr;
        TileMap* m_pMap = nullptr;
        
        bool m_Solid = false;

        std::shared_ptr<Sprite> m_pSprite; // optional for thing type
        
};

#endif

