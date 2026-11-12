#pragma once

namespace gui {

void toggleMenu();
bool isMenuOpen();

// Dibuja el panel. Se llama una vez por frame entre ImGui::NewFrame() y ImGui::Render().
void draw();

// Recoloca las ventanas del panel en columnas, sin que se tapen (tarda un frame).
void resetWindowPositions();

} // namespace gui
