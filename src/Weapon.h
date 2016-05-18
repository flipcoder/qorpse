#ifndef WEAPON_H_V9PBVMZN
#define WEAPON_H_V9PBVMZN

#include <string>
#include "Qor/kit/freq/animation.h"
#include "Qor/kit/meta/meta.h"
#include "Qor/kit/cache/cache.h"

class Weapon:
    public IRealtime
{
    public:

        enum {
            DUAL = kit::bit(0)
        };

        Weapon() = default;
        
        Weapon(
            std::string name,
            Cache<Resource, std::string>* resources
        ){
            //auto cfg = std::make_shared<Meta>(resources->transform("weapons.json"));
        }
        Weapon(
            std::string name,
            std::string ammo_type,
            std::string bullet_gfx,
            std::shared_ptr<int> ammo,
            int clip,
            int clip_size,
            int chamber_size,
            float range,
            float fire_speed,
            float reload_speed,
            float bullet_speed,
            float accuracy=1.0f,
            unsigned flags=0,
            int order = 0
        ):
            m_Name(name),
            m_AmmoType(ammo_type),
            m_BulletGFX(bullet_gfx),
            m_Clip(clip),
            m_ClipSize(clip_size),
            m_ChamberSize(chamber_size),
            m_Range(range),
            m_FireSpeed(fire_speed),
            m_ReloadSpeed(reload_speed),
            m_BulletSpeed(bullet_speed),
            m_Timeline(std::make_shared<Freq::Timeline>()),
            //m_FireDelay(Freq::Alarm(&m_Timeline)),
            //m_ReloadDelay(Freq::Alarm(&m_Timeline)),
            m_Ammo(ammo),
            m_Accuracy(accuracy),
            m_Flags(flags),
            m_Order(order)
        {
            m_FireDelay = Freq::Alarm(m_Timeline.get());
            m_ReloadDelay = Freq::Alarm(m_Timeline.get());
            m_FireDelay.set(Freq::Time::ms(0));
            m_ReloadDelay.set(Freq::Time::ms(0));
        }
        virtual ~Weapon() {}

        Weapon(const Weapon&) = default;
        Weapon(Weapon&&) = default;
        Weapon& operator=(const Weapon&) = default;
        Weapon& operator=(Weapon&&) = default;

        std::string name() const {
            return m_Name;
        }
        int ammo() const {
            return *m_Ammo;
        }
        std::string ammo_type() {
            return m_AmmoType;
        }
        void clip(int c) {
            m_Clip = c;
        }
        int clip() const {
            return m_Clip;
        }
        int clip_size() const {
            return m_ClipSize;
        }

        float range() const {
            return m_Range;
        }
        float bullet_speed() const {
            return m_BulletSpeed;
        }

        float accuracy() const {
            return m_Accuracy;
        }

        bool reload() {
            if(delayed())
                return false;

            if(*m_Ammo) {
                auto ofs = std::min(
                    std::min(*m_Ammo, m_ClipSize),
                    m_ClipSize - m_Clip
                );
                if(ofs)
                {
                    *m_Ammo -= ofs;
                    m_Clip += ofs;
                    m_ReloadDelay.set(Freq::Time::seconds(1.0f / m_ReloadSpeed));
                    return true;
                }
            }
            return false;
        }

        bool delayed() const {
            //return false;
            return !m_FireDelay.elapsed() || !m_ReloadDelay.elapsed();
        }
        
        bool can_fire() const {
            return !delayed() && m_Clip;
        }
        
        int shoot()
        {
            if(delayed())
                return 0;
            
            if(m_Clip)
            {
                auto ofs = std::min(m_Clip, m_ChamberSize);
                m_Clip -= ofs;
                
                m_FireDelay.set(Freq::Time::seconds(1.0f / m_FireSpeed));
                return ofs;
            }
            return 0;
        }
        
        virtual void logic(Freq::Time t) override {
            m_Timeline->logic(t);
        }
        
        std::string bullet_gfx() const{
            return m_BulletGFX;
        }

        unsigned flags() const {
            return m_Flags;
        }
        
    private:

        std::string m_Name;
        std::string m_AmmoType;
        std::string m_BulletGFX;
        
        int m_ChamberSize = 1;
        int m_Clip = 1;
        int m_ClipSize = 1;
        int m_Order = 0;
        float m_Range = 1.0f;
        float m_FireSpeed = 1.0f;
        float m_ReloadSpeed = 1.0f;
        float m_BulletSpeed = 1.0f;
        float m_Accuracy = 1.0f;
        unsigned m_Flags = 0;

        std::shared_ptr<Freq::Timeline> m_Timeline;
        Freq::Alarm m_FireDelay;
        Freq::Alarm m_ReloadDelay;
        
        // common ammo type are shared with player inventory
        std::shared_ptr<int> m_Ammo;
};

#endif

