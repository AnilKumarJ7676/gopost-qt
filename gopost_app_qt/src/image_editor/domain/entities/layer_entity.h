#pragma once

#include "rendering_bridge/engine_types.h"

#include <QString>

#include <optional>

namespace gopost::image_editor {

/// Domain entity mirroring engine's LayerInfo with identity semantics.
class LayerEntity {
public:
    int id = 0;
    rendering::LayerType type = rendering::LayerType::Image;
    QString name;
    double opacity = 1.0;
    rendering::BlendMode blendMode = rendering::BlendMode::Normal;
    bool visible = true;
    bool locked = false;
    double tx = 0.0;
    double ty = 0.0;
    double sx = 1.0;
    double sy = 1.0;
    double rotation = 0.0;
    int contentWidth = 0;
    int contentHeight = 0;

    /// Non-null when this layer is a template placeholder. Value is the unique
    /// key that maps to an EditableField.
    std::optional<QString> placeholderKey;

    LayerEntity() = default;

    LayerEntity(int id, rendering::LayerType type, const QString& name,
                double opacity = 1.0,
                rendering::BlendMode blendMode = rendering::BlendMode::Normal,
                bool visible = true, bool locked = false,
                double tx = 0.0, double ty = 0.0,
                double sx = 1.0, double sy = 1.0,
                double rotation = 0.0,
                int contentWidth = 0, int contentHeight = 0,
                std::optional<QString> placeholderKey = std::nullopt)
        : id(id), type(type), name(name)
        , opacity(opacity), blendMode(blendMode)
        , visible(visible), locked(locked)
        , tx(tx), ty(ty), sx(sx), sy(sy), rotation(rotation)
        , contentWidth(contentWidth), contentHeight(contentHeight)
        , placeholderKey(std::move(placeholderKey)) {}

    [[nodiscard]] bool isPlaceholder() const { return placeholderKey.has_value(); }

    struct CopyWithArgs {
        std::optional<QString> name;
        std::optional<double> opacity;
        std::optional<rendering::BlendMode> blendMode;
        std::optional<bool> visible;
        std::optional<bool> locked;
        std::optional<double> tx, ty, sx, sy, rotation;
        std::optional<QString> placeholderKey;
        bool clearPlaceholder = false;
    };

    [[nodiscard]] LayerEntity copyWith(const CopyWithArgs& args) const {
        return LayerEntity(
            id, type,
            args.name.value_or(this->name),
            args.opacity.value_or(this->opacity),
            args.blendMode.value_or(this->blendMode),
            args.visible.value_or(this->visible),
            args.locked.value_or(this->locked),
            args.tx.value_or(this->tx),
            args.ty.value_or(this->ty),
            args.sx.value_or(this->sx),
            args.sy.value_or(this->sy),
            args.rotation.value_or(this->rotation),
            contentWidth, contentHeight,
            args.clearPlaceholder
                ? std::nullopt
                : (args.placeholderKey.has_value()
                    ? args.placeholderKey
                    : this->placeholderKey));
    }

    [[nodiscard]] static LayerEntity fromLayerInfo(const rendering::LayerInfo& info) {
        return LayerEntity(
            info.id, info.type, info.name,
            info.opacity, info.blendMode,
            info.visible, info.locked,
            info.tx, info.ty, info.sx, info.sy, info.rotation,
            info.contentWidth, info.contentHeight);
    }

    bool operator==(const LayerEntity& other) const = default;
};

} // namespace gopost::image_editor
