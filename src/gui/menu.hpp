#pragma once

namespace gui {

void toggleMenu();
bool isMenuOpen();

// Dibuja el panel. Se llama una vez por frame entre ImGui::NewFrame() y ImGui::Render().
void draw();

// Reordena todas las ventanas del panel a su rejilla limpia predeterminada
void resetWindowPositions();

} // namespace gui
