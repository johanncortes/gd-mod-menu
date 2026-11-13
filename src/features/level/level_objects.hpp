#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Tres opciones que actúan sobre los objetos del nivel mientras PlayLayer los carga (avisos de
// core::game::onAddObject). Las tres se aplican al entrar al nivel: cambiarlas a mitad de uno no
// hace nada hasta volver a entrar.

// Localiza offsets e instala el hook que comparten. Llamar antes de los init() de las clases.
void initLevelObjects(bool gameAvailable);
// Una vez por frame: olvida los objetos del nivel cuando se sale de él.
void updateLevelObjects();

// Show layout: no se dibuja la decoración (objetos de tipo "decoración"), solo lo que afecta al
// juego. Esos objetos siguen existiendo (grupos, triggers que los mueven), pero no entran en las
// secciones de dibujo, igual que hace el juego con los triggers. Trampa.
class ShowLayout {
public:
    void init();
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    bool m_available = false;
    bool m_enabled = false;
};

// Hitbox multiplier: multiplica el tamaño de la hitbox de los peligros (pinchos, sierras...) por
// un factor, con los factores de escala de hitbox que tiene cada objeto. Trampa.
class HitboxMultiplier {
public:
    void init();
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    bool m_available = false;
    bool m_enabled = false;
    float m_factor = 0.8f;
};

// Show triggers: marca en pantalla dónde están los triggers del nivel (el juego no los dibuja),
// con su nombre o su número de objeto. Solo dibuja encima: no toca los triggers.
class ShowTriggers {
public:
    void init();
    void enable();
    void disable();
    bool drawUI();      // true si el usuario cambió algo
    void drawOverlay(); // cada frame, bajo el panel

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_enabled = false;
};

} // namespace features
