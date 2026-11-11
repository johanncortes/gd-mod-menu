#pragma once

namespace gui {

void toggleMenu();
bool isMenuOpen();

// Dibuja el panel. Se llama una vez por frame entre ImGui::NewFrame() y ImGui::Render().
void draw();

} // namespace gui
