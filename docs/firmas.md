# Registro de firmas

| Función | Módulo | Firma (patrón) | Versión GD | Notas |
|---|---|---|---|---|
| CCScheduler::update | libcocos2d.dll | export por nombre | — | estable |
| wglSwapBuffers | opengl32.dll | export por nombre | — | hook de render: aquí se dibuja ImGui (fase 1) |
| GetRawInputData | user32.dll | export por nombre | — | hook de input: GD lee clics y teclas por raw input en un hilo aparte (fase 1) |
