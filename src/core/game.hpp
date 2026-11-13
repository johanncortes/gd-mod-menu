#pragma once

// Acceso a objetos del juego y hooks de nivel que usan varias features.
namespace core::game {

// Localiza el puntero global a GameManager (firma kSwitchStartPos) e instala los hooks de
// nivel (PlayLayer::resetLevel y GJBaseGameLayer::update). Llamar antes que las features.
// Devuelve false si no se encontró GameManager.
bool init();

// PlayLayer del nivel que se está jugando, o nullptr si no hay ninguno (menús, editor).
void* playLayer();

// LevelEditorLayer abierto (el editor, también al probar el nivel en él), o nullptr.
void* editorLayer();

// Jugador 1 (PlayerObject*) de un nivel, o nullptr si no se pudo localizar.
void* player1(void* gameLayer);

// Jugador 2 (existe siempre, aunque el nivel no sea dual), o nullptr si no se pudo localizar.
void* player2(void* gameLayer);

// --- Hooks de nivel (false si sus firmas no aparecieron: los avisos no llegan nunca) ---
bool levelHooksAvailable();

// Avisos, en el hilo del juego. Cada feature registra los suyos una vez, en su init().
using ResetCallback = void (*)(void* playLayer);            // empieza un intento
using UpdateCallback = void (*)(void* gameLayer, float dt); // un frame de nivel, después del juego
void onResetLevel(ResetCallback callback);
void onUpdate(UpdateCallback callback);

// Filtros antes de GJBaseGameLayer::update, en el orden en que se añaden: si uno devuelve false ese
// frame el nivel no avanza ni se llaman los avisos; pueden cambiar dt.
using UpdateFilter = bool (*)(void* gameLayer, float& dt);
void addUpdateFilter(UpdateFilter filter);

// --- Carga del nivel (false si faltan sus firmas: los avisos no llegan nunca) ---
bool loadHooksAvailable();

// Avisos mientras PlayLayer carga un nivel, en el hilo del juego.
using LevelInitCallback = void (*)(void* playLayer);               // empieza la carga (PlayLayer::init)
using AddObjectCallback = void (*)(void* playLayer, void* object); // antes de añadir cada objeto
void onLevelInit(LevelInitCallback callback);
// Solo los objetos de la carga inicial (hasta el primer intento): esos viven tanto como el nivel.
void onAddObject(AddObjectCallback callback);

// Datos de un GameObject; -1 si no se pudo localizar el campo.
int objectId(void* object);   // GameObject::m_objectID
int objectType(void* object); // GameObject::m_objectType (0 sólido, 2 peligro, 7 decoración, 0x14 trigger...)

// GJGameLevel del nivel que se juega, o nullptr.
void* level(void* playLayer);

// Segundos de juego del intento actual (suma de los dt, así que sigue al speedhack).
double attemptTime();

// true si el nivel está corriendo ahora mismo (ni en pausa ni fuera del nivel).
bool levelRunning();

} // namespace core::game
