#pragma once

namespace features {

// Screenshot: guarda lo que se ve del juego (sin el panel ni los labels) como PNG en la carpeta
// "screenshots" junto a la DLL y lo copia al portapapeles. Se lee el back buffer de OpenGL al
// empezar el frame del panel, antes de que ImGui dibuje encima.
class Screenshot {
public:
    void update(); // una vez por frame, desde features::update (antes de dibujar ImGui)
    void drawUI();

private:
    bool m_pending = false;
    char m_lastFile[160] = {}; // nombre de la última captura, para mostrarlo
};

} // namespace features
