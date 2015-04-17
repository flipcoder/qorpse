#ifndef PLAYER_H_R0CEOUXE
#define PLAYER_H_R0CEOUXE

#include <string>
#include "Qor/kit/cache/cache.h"
#include "Qor/Sprite.h"
#include "Qor/Session.h"
#include "Qor/Profile.h"
#include "Qor/Session.h"
#include "Weapon.h"
#include "Qor/kit/reactive/reactive.h"

class Character:
    public Sprite
{
    public:
        
        Character(
            Session* session,
            Profile* profile,
            const std::string& fn,
            Cache<Resource, std::string>* resources,
            glm::vec3 pos
        );
        virtual ~Character() {}

        virtual void logic_self(Freq::Time t) override;

        void set_direction(glm::vec2 dir) {
            m_Dir = dir;
        }

        int clip() const {
            return weapon() ? weapon()->clip() : 0;
        }
        int ammo() const {
            return weapon() ? weapon()->ammo() : 0;
        }
        int clip_size() const {
            return weapon() ? weapon()->clip_size() : 0;
        }
        bool reload() {
            if(weapon())
                return weapon()->reload();
            return false;
        }

        void clip(int a) {
            if(weapon())
                weapon()->clip(a);
        }

        int hp() const {
            return m_HP;
        }
        void hp(int a) {
            m_HP = std::min<int>(m_HP+a, m_MaxHP);
        }
        bool heal() {
            if(m_HP < m_MaxHP){
                m_HP = m_MaxHP;
                return true;
            }
            return false;
        }
        bool heal(int hp) {
            if(hp >= 0 && m_HP == m_MaxHP)
                return false;
            m_HP = std::min(m_HP+hp, m_MaxHP);
            if(m_HP <= 0) {
                m_HP = std::max<int>(m_HP, 0);
                m_Dead = true;
                return false;
            }
            return true;
        }
        bool damage(int hp) { return heal(-hp); }

        int hp_percent() {
            // dont round
            return (
                (m_MaxHP ? (m_HP*1.0f) / (m_MaxHP*1.0f) : 0) +
                (m_MaxArmor ? (m_Armor*1.0f) / (m_MaxArmor*1.0f) : 0)
            ) * 100.0f;
        }

        int max_hp() const {
            return m_MaxHP;
        }
        void max_hp(int a) {
            m_MaxHP = a;
        }

        Weapon* weapon() {
            assert(m_Weapon < m_Weapons.size());
            if(!m_Weapons.empty())
                return &m_Weapons[m_Weapon];
            assert(false);
            return nullptr;
        }
        const Weapon* weapon() const {
            return const_cast<Character*>(this)->weapon();
        }
        
        bool switch_weapon(int idx) {
            int old_idx = m_Weapon;
            m_Weapon = kit::mod<int>(
                m_Weapon + idx, m_Weapons.size()
            );
            return idx == old_idx;
        }
        
        int shoot() {
            if(weapon())
                return weapon()->shoot();
            return 0;
        }
        
        bool indoors() const { return m_Indoors; }
        
        bool indoors(bool b);
        
        void add_camera(std::weak_ptr<Camera> camera);

        bool alive() const { return not m_Dead; }
        bool dead() const { return m_Dead; }
        
        KIT_REACTIVE_SIGNAL(on_hp_change, m_HP)
        KIT_REACTIVE_SIGNAL(on_weapon_change, m_Weapon)
        KIT_REACTIVE_SIGNAL(on_ammo_change, m_Ammo)
        
        KIT_REACTIVE_SIGNAL(on_speak, m_Speak)
        KIT_REACTIVE_SIGNAL(on_death, m_Dead)
        
        void say(std::string msg) {
            m_Speak = msg; // reactive
        }
        bool give(std::string id) {
            if(not kit::has(m_Items, id)){
                m_Items.push_back(id);
                return true;
            }
            return false;
        }
        bool has(std::string id) const {
            return kit::has(m_Items, id);
        }
        std::string skin() const {
            return m_Skin;
        }
        
    private:

        kit::reactive<std::string> m_Speak;

        kit::reactive<bool> m_Dead = false;
        kit::reactive<int> m_HP;
        int m_MaxHP;
        kit::reactive<int> m_Ammo; // current weapon ammo
        kit::reactive<int> m_Armor;
        int m_MaxArmor;
        int m_Indoors = 0;

        std::vector<Weapon> m_Weapons;
        std::vector<std::string> m_Items;
        kit::reactive<int> m_Weapon = 0;
        
        std::string m_Skin;
        Session* m_pSession = nullptr;
        Profile* m_pProfile = nullptr;
        Controller* m_pController = nullptr;
        Cache<Resource, std::string>* m_pResources = nullptr;

        glm::vec2 m_Dir;

        std::vector<std::weak_ptr<Camera>> m_Cameras;
};

#endif

