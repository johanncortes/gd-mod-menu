#pragma once

namespace features {

// Display: VSync, pantalla completa y ventana sin bordes, con las mismas funciones de libcocos2d
// que usan los ajustes gráficos del juego (CCApplication::toggleVerticalSync y
// CCEGLView::toggleFullScreen). Las casillas muestran el estado real. El cambio es para esta
// sesión: el juego guarda su propia opción solo cuando se cambia desde sus ajustes.
class Display {
public:
    void init();
    void update(); // aplica el cambio pendiente, fuera del dibujado del panel
    void drawUI();

private:
    bool m_available = false;
    int m_pending = 0; // 0 = nada; 1 = VSync, 2 = pantalla completa, 3 = sin bordes
};

} // namespace features
