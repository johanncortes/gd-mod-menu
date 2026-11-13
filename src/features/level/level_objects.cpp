#include "level_objects.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <vector>

#include <MinHook.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

// GameObjectType (ver docs/firmas.md)
constexpr int kTypeHazard = 2, kTypeDecoration = 7, kTypeTrigger = 0x14, kTypeAnimatedHazard = 0x2f;
constexpr int kCheckpointId = 44; // el checkpoint de práctica es un objeto de decoración
constexpr float kMinFactor = 0.1f, kMaxFactor = 3.f;
constexpr int kMaxDrawnTriggers = 400;

struct Point {
    float x, y; // mismo formato que cocos2d::CCPoint
};

// Offsets de GameObject leídos de las firmas
int32_t g_sectionOffset = 0; // índice de sección (-1 = en ninguna: ni se dibuja ni colisiona)
int32_t g_scaleXOffset = 0;  // factores de escala de la hitbox
int32_t g_scaleYOffset = 0;
int32_t g_radiusOffset = 0;  // radio de las hitbox circulares (0 = no es circular)
int32_t g_rectDirtyOffset = 0; // "hay que recalcular el rectángulo guardado"

// libcocos2d
using GetPositionFn = const Point* (*)(void* node);
using ToWorldFn = Point* (*)(void* node, Point* out, const Point* in);
using GetParentFn = void* (*)(void* node);
GetPositionFn g_getPosition = nullptr;
ToWorldFn g_toWorld = nullptr;
GetParentFn g_getParent = nullptr;

using AddToSectionFn = void (*)(void* gameLayer, void* object);
AddToSectionFn g_origAddToSection = nullptr;
bool g_hooked = false; // hook de addToSection instalado y avisos de carga disponibles

// Estado del nivel cargado (solo hilo del juego)
void* g_layer = nullptr; // PlayLayer al que pertenecen los objetos apuntados
bool g_loaded = false;   // ya empezó su primer intento
std::unordered_set<void*> g_hidden; // decoración que Show layout dejó fuera de las secciones
std::vector<void*> g_triggers;

bool g_layoutEnabled = false;
bool g_hitboxEnabled = false;
float g_hitboxFactor = 1.f;
bool g_triggersEnabled = false;

template <typename T>
T& fieldAt(void* object, int32_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
}

void forget() {
    g_layer = nullptr;
    g_loaded = false;
    g_hidden.clear();
    g_triggers.clear();
}

void onLevelInit(void* playLayer) {
    forget();
    g_layer = playLayer;
}

void onResetLevel(void* playLayer) {
    if (playLayer == g_layer) g_loaded = true;
}

void scaleHitbox(void* object, float factor) {
    for (int32_t offset : {g_scaleXOffset, g_scaleYOffset, g_radiusOffset}) {
        float& value = fieldAt<float>(object, offset);
        if (std::isfinite(value) && value > 0.f) value *= factor;
    }
    fieldAt<bool>(object, g_rectDirtyOffset) = true;
}

void onAddObject(void* playLayer, void* object) {
    if (playLayer != g_layer) return;
    const int type = core::game::objectType(object);
    if (type == kTypeTrigger) {
        g_triggers.push_back(object);
    } else if (type == kTypeDecoration) {
        if (g_layoutEnabled && core::game::objectId(object) != kCheckpointId) g_hidden.insert(object);
    } else if (type == kTypeHazard || type == kTypeAnimatedHazard) {
        if (g_hitboxEnabled) scaleHitbox(object, g_hitboxFactor);
    }
}

// El juego llama aquí al añadir el objeto y cada vez que cambia de sección (o al reactivar su grupo)
void addToSectionHook(void* gameLayer, void* object) {
    if (gameLayer == g_layer && !g_hidden.empty() && g_hidden.count(object)) {
        fieldAt<int32_t>(object, g_sectionOffset) = -1; // lo mismo que hace el juego con los triggers
        return;
    }
    g_origAddToSection(gameLayer, object);
}

// --- Show triggers: nombre por número de objeto (los que no están salen como "#número") ---
struct TriggerName {
    int id;
    const char* name;
};
constexpr TriggerName kTriggerNames[] = {
    {899, "Color"},        {901, "Move"},          {1006, "Pulse"},      {1007, "Alpha"},
    {1049, "Toggle"},      {1268, "Spawn"},        {1346, "Rotate"},     {1347, "Follow"},
    {1520, "Shake"},       {1585, "Animate"},      {1595, "Touch"},      {1611, "Count"},
    {1612, "Hide"},        {1613, "Show"},         {1616, "Stop"},       {1811, "Inst. count"},
    {1812, "On death"},    {1814, "Follow Y"},     {1815, "Collision"},  {1817, "Pickup"},
    {1912, "Random"},      {1913, "Zoom"},         {1914, "Static"},     {1916, "Offset"},
    {1917, "Reverse"},     {1934, "Song"},         {1935, "Time warp"},  {2015, "Cam rotate"},
    {2016, "Cam guide"},   {2062, "Cam edge"},     {2066, "Gravity"},    {2067, "Scale"},
    {2068, "Adv. random"}, {2899, "Options"},      {2903, "Gradient"},   {3016, "Adv. follow"},
    {3600, "End"},         {3602, "SFX"},          {3604, "Event"},      {3607, "Sequence"},
    {3608, "Spawn part."}, {3609, "Inst. collis."}, {3614, "Time"},      {3615, "Time event"},
    {3617, "Time ctrl"},   {3618, "Reset"},        {3619, "Item edit"},  {3620, "Item comp."},
    {3641, "Persist. item"},
};

const char* triggerName(int id, char* buffer, size_t size) {
    for (const TriggerName& entry : kTriggerNames)
        if (entry.id == id) return entry.name;
    snprintf(buffer, size, "#%d", id);
    return buffer;
}

} // namespace

void initLevelObjects(bool gameAvailable) {
    HMODULE exe = GetModuleHandleW(nullptr);
    const auto readOffset = [exe](const char* signature, int at) -> int32_t {
        const uint8_t* found = core::memory::findPattern(exe, signature);
        int32_t offset = 0;
        if (found) memcpy(&offset, found + at, 4);
        return offset > 0 && offset < 0x10000 ? offset : 0;
    };
    g_sectionOffset = readOffset(sig::kAddObjectSection, sig::kAddObjectSectionIndex);
    g_scaleXOffset = readOffset(sig::kObjectRectScale, sig::kObjectRectScaleX);
    g_scaleYOffset = readOffset(sig::kObjectRectScale, sig::kObjectRectScaleY);
    g_radiusOffset = readOffset(sig::kObjectRadius, sig::kObjectRadiusOffset);
    g_rectDirtyOffset = readOffset(sig::kObjectRadius, sig::kObjectRadiusDirty);
    void* addToSection = core::memory::findPattern(exe, sig::kAddToSection);

    if (HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll")) {
        g_getPosition = reinterpret_cast<GetPositionFn>(
            GetProcAddress(cocos, "?getPosition@CCNode@cocos2d@@UEAAAEBVCCPoint@2@XZ"));
        g_toWorld = reinterpret_cast<ToWorldFn>(
            GetProcAddress(cocos, "?convertToWorldSpace@CCNode@cocos2d@@QEAA?AVCCPoint@2@AEBV32@@Z"));
        g_getParent = reinterpret_cast<GetParentFn>(GetProcAddress(cocos, "?getParent@CCNode@cocos2d@@UEAAPEAV12@XZ"));
    }

    if (!gameAvailable || !core::game::loadHooksAvailable()) {
        core::logMsg("Objetos del nivel: no disponibles (hooks de carga)");
        return;
    }
    core::game::onLevelInit(&onLevelInit);
    core::game::onAddObject(&onAddObject);
    core::game::onResetLevel(&onResetLevel);
    g_hooked = g_sectionOffset && addToSection &&
               MH_CreateHook(addToSection, &addToSectionHook, reinterpret_cast<void**>(&g_origAddToSection)) == MH_OK &&
               MH_EnableHook(addToSection) == MH_OK;
}

void updateLevelObjects() {
    // Al salir del nivel sus objetos se destruyen: no se guarda ningún puntero a ellos
    if (g_loaded && core::game::playLayer() != g_layer) forget();
}

// --- Show layout ---
void ShowLayout::init() {
    m_available = g_hooked;
    core::logMsg("Show layout: %s", m_available ? "listo" : "no disponible");
}

void ShowLayout::enable() {
    m_enabled = true;
    g_layoutEnabled = m_available;
}

void ShowLayout::disable() {
    m_enabled = false;
    g_layoutEnabled = false;
}

bool ShowLayout::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Show layout", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void ShowLayout::load(const nlohmann::json& j) {
    j.value("show_layout", false) ? enable() : disable();
}

void ShowLayout::save(nlohmann::json& j) const {
    j["show_layout"] = m_enabled;
}

// --- Hitbox multiplier ---
void HitboxMultiplier::init() {
    m_available = core::game::loadHooksAvailable() && g_scaleXOffset && g_scaleYOffset && g_radiusOffset &&
                  g_rectDirtyOffset;
    core::logMsg("Hitbox multiplier: %s", m_available ? "listo" : "no disponible");
}

void HitboxMultiplier::enable() {
    m_enabled = true;
    g_hitboxFactor = m_factor;
    g_hitboxEnabled = m_available;
}

void HitboxMultiplier::disable() {
    m_enabled = false;
    g_hitboxEnabled = false;
}

bool HitboxMultiplier::drawUI() {
    bool enabled = m_enabled;
    const bool changed =
        gui::toggleValue("Hitbox multiplier", m_available, enabled, m_factor, kMinFactor, kMaxFactor, "x%.2f");
    if (changed) enabled ? enable() : disable();
    return changed;
}

void HitboxMultiplier::load(const nlohmann::json& j) {
    m_factor = std::clamp(j.value("factor", m_factor), kMinFactor, kMaxFactor);
    j.value("enabled", false) ? enable() : disable();
}

void HitboxMultiplier::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"factor", m_factor}};
}

// --- Show triggers ---
void ShowTriggers::init() {
    m_available = core::game::loadHooksAvailable() && g_sectionOffset && g_getPosition && g_toWorld && g_getParent;
    core::logMsg("Show triggers: %s", m_available ? "listo" : "no disponible");
}

void ShowTriggers::enable() {
    m_enabled = true;
    g_triggersEnabled = m_available;
}

void ShowTriggers::disable() {
    m_enabled = false;
    g_triggersEnabled = false;
}

bool ShowTriggers::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Show triggers", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void ShowTriggers::drawOverlay() {
    void* playLayer = core::game::playLayer();
    if (!g_triggersEnabled || !g_loaded || !playLayer || playLayer != g_layer || g_triggers.empty()) return;
    void* player = core::game::player1(playLayer);
    void* objectLayer = player ? g_getParent(player) : nullptr;
    if (!objectLayer) return;

    // La conversión capa de objetos -> pantalla es afín: se calcula una vez con tres puntos
    const Point zero{0.f, 0.f}, unitX{1.f, 0.f}, unitY{0.f, 1.f};
    Point origin{}, ex{}, ey{};
    g_toWorld(objectLayer, &origin, &zero);
    g_toWorld(objectLayer, &ex, &unitX);
    g_toWorld(objectLayer, &ey, &unitY);
    ex = {ex.x - origin.x, ex.y - origin.y};
    ey = {ey.x - origin.x, ey.y - origin.y};

    // GD dibuja con 320 puntos de alto
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const float scale = screen.y / 320.f;
    ImFont* font = ImGui::GetFont();
    const float fontSize = std::max(11.f, screen.y / 70.f);
    const float pad = fontSize * 0.25f;
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    int drawn = 0;
    char buffer[16];
    for (void* trigger : g_triggers) {
        if (fieldAt<int32_t>(trigger, g_sectionOffset) >= 0) continue; // ese ya lo dibuja el juego
        const Point* pos = g_getPosition(trigger);
        const float worldX = origin.x + ex.x * pos->x + ey.x * pos->y;
        const float worldY = origin.y + ex.y * pos->x + ey.y * pos->y;
        const ImVec2 center{worldX * scale, screen.y - worldY * scale};
        if (center.x < -50.f || center.x > screen.x + 50.f || center.y < -20.f || center.y > screen.y + 20.f) continue;

        const char* name = triggerName(core::game::objectId(trigger), buffer, sizeof(buffer));
        const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, name);
        const ImVec2 min{center.x - size.x * 0.5f - pad, center.y - size.y * 0.5f - pad};
        const ImVec2 max{center.x + size.x * 0.5f + pad, center.y + size.y * 0.5f + pad};
        draw->AddRectFilled(min, max, IM_COL32(15, 20, 30, 190), 3.f);
        draw->AddRect(min, max, IM_COL32(90, 200, 255, 230), 3.f);
        draw->AddText(font, fontSize, {min.x + pad, min.y + pad}, IM_COL32(255, 255, 255, 255), name);
        if (++drawn >= kMaxDrawnTriggers) break;
    }
}

void ShowTriggers::load(const nlohmann::json& j) {
    j.value("show_triggers", false) ? enable() : disable();
}

void ShowTriggers::save(nlohmann::json& j) const {
    j["show_triggers"] = m_enabled;
}

} // namespace features
