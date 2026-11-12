#include "screenshot.hpp"

#include <windows.h>
#include <wincodec.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <GL/gl.h>
#include <imgui.h>

#include "config/config.hpp"
#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

// Píxeles BGRA de arriba abajo
struct Image {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> bgra;
};

// Lee el back buffer (lo que el juego acaba de dibujar en este frame)
bool readBackBuffer(Image& image) {
    const ImGuiIO& io = ImGui::GetIO();
    image.width = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    image.height = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (image.width <= 0 || image.height <= 0) return false;
    std::vector<uint8_t> rgba(static_cast<size_t>(image.width) * image.height * 4);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, image.width, image.height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    if (glGetError() != GL_NO_ERROR) return false;
    // OpenGL da las filas de abajo arriba y en RGBA: se voltea y se pasa a BGRA opaco
    image.bgra.resize(rgba.size());
    const size_t row = static_cast<size_t>(image.width) * 4;
    for (int y = 0; y < image.height; ++y) {
        const uint8_t* src = rgba.data() + (image.height - 1 - y) * row;
        uint8_t* dst = image.bgra.data() + y * row;
        for (int x = 0; x < image.width; ++x) {
            dst[x * 4 + 0] = src[x * 4 + 2];
            dst[x * 4 + 1] = src[x * 4 + 1];
            dst[x * 4 + 2] = src[x * 4 + 0];
            dst[x * 4 + 3] = 255;
        }
    }
    return true;
}

// PNG con Windows Imaging Component (viene con Windows, sin dependencias)
bool savePng(const Image& image, const std::wstring& path) {
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool ok = false;
    IWICImagingFactory* factory = nullptr;
    IWICStream* stream = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) &&
        SUCCEEDED(factory->CreateStream(&stream)) &&
        SUCCEEDED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) &&
        SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
        SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) &&
        SUCCEEDED(encoder->CreateNewFrame(&frame, nullptr)) && SUCCEEDED(frame->Initialize(nullptr)) &&
        SUCCEEDED(frame->SetSize(image.width, image.height))) {
        WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
        ok = SUCCEEDED(frame->SetPixelFormat(&format)) && format == GUID_WICPixelFormat32bppBGRA &&
             SUCCEEDED(frame->WritePixels(image.height, image.width * 4,
                                          static_cast<UINT>(image.bgra.size()),
                                          const_cast<BYTE*>(image.bgra.data()))) &&
             SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit());
    }
    if (frame) frame->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    if (SUCCEEDED(init)) CoUninitialize();
    return ok;
}

// Al portapapeles como DIB de 32 bits (filas de abajo arriba)
bool copyToClipboard(const Image& image) {
    const size_t row = static_cast<size_t>(image.width) * 4;
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + image.bgra.size());
    if (!memory) return false;
    auto* header = static_cast<BITMAPINFOHEADER*>(GlobalLock(memory));
    *header = {};
    header->biSize = sizeof(BITMAPINFOHEADER);
    header->biWidth = image.width;
    header->biHeight = image.height;
    header->biPlanes = 1;
    header->biBitCount = 32;
    header->biCompression = BI_RGB;
    auto* pixels = reinterpret_cast<uint8_t*>(header + 1);
    for (int y = 0; y < image.height; ++y)
        memcpy(pixels + y * row, image.bgra.data() + (image.height - 1 - y) * row, row);
    GlobalUnlock(memory);
    if (!OpenClipboard(nullptr)) {
        GlobalFree(memory);
        return false;
    }
    EmptyClipboard();
    const bool ok = SetClipboardData(CF_DIB, memory) != nullptr;
    CloseClipboard();
    if (!ok) GlobalFree(memory); // si se aceptó, ahora es del portapapeles
    return ok;
}

} // namespace

void Screenshot::update() {
    if (!m_pending) return;
    m_pending = false;
    Image image;
    if (!readBackBuffer(image)) {
        core::logMsg("Screenshot: no se pudo leer la imagen");
        snprintf(m_lastFile, sizeof(m_lastFile), "error al leer la imagen");
        return;
    }
    const std::filesystem::path dir = config::siblingPath("screenshots");
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    SYSTEMTIME now;
    GetLocalTime(&now);
    char name[64];
    snprintf(name, sizeof(name), "gd_%04d%02d%02d_%02d%02d%02d_%03d.png", now.wYear, now.wMonth, now.wDay, now.wHour,
             now.wMinute, now.wSecond, now.wMilliseconds);
    const bool saved = savePng(image, (dir / name).wstring());
    const bool copied = copyToClipboard(image);
    snprintf(m_lastFile, sizeof(m_lastFile), "%s%s", saved ? name : "no se pudo guardar el PNG",
             copied ? " (copiada)" : "");
    core::logMsg("Screenshot: %dx%d, archivo %s, portapapeles %s", image.width, image.height, saved ? name : "error",
                 copied ? "sí" : "no");
}

void Screenshot::drawUI() {
    if (gui::button("Screenshot", true)) m_pending = true;
    if (m_lastFile[0]) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", m_lastFile);
    }
}

} // namespace features
