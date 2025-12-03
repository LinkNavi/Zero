#pragma once
#include <raylib.h>

// Lightweight wrapper around a Raylib Model.
// Move-only. Destructor unloads the Model if valid.
struct ZeroMesh {
    Model model{};
    bool isValid = false;

    ZeroMesh() = default;

    // Deleting copy operations to avoid accidental duplicates
    ZeroMesh(const ZeroMesh&) = delete;
    ZeroMesh& operator=(const ZeroMesh&) = delete;

    // Move operations
    ZeroMesh(ZeroMesh&& other) noexcept {
        model = other.model;
        isValid = other.isValid;
        other.isValid = false;
    }
    ZeroMesh& operator=(ZeroMesh&& other) noexcept {
        if (this != &other) {
            // If we already own a model, unload it first
            if (isValid) UnloadModel(model);
            model = other.model;
            isValid = other.isValid;
            other.isValid = false;
        }
        return *this;
    }

    // Convenience factory from a Mesh
    static ZeroMesh FromMesh(const Mesh& mesh) {
        ZeroMesh m;
        m.model = LoadModelFromMesh(mesh);
        m.isValid = true;
        return m;
    }

    // Destructor unloads the model if still valid
    ~ZeroMesh() {
        if (isValid) {
            UnloadModel(model);
            isValid = false;
        }
    }
};
