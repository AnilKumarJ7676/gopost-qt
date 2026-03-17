import QtQuick

/// Canvas-based painter for adjustment layer clips on the timeline.
/// Renders animated patterns: colorWave, blurPulse, distortScan, stylizeDots, presetStrip.
Canvas {
    id: root

    property string style: "colorWave"   // colorWave | blurPulse | distortScan | stylizeDots | presetStrip
    property color baseColor: "#6C63FF"
    property real clipDuration: 1.0
    property real pixelsPerSecond: 100
    property real animationValue: 0.0
    property int effectCount: 0
    property real intensity: 1.0

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.save()
        ctx.beginPath()
        ctx.rect(0, 0, width, height)
        ctx.clip()

        switch (root.style) {
        case "colorWave":
            internal.paintColorWave(ctx)
            break
        case "blurPulse":
            internal.paintBlurPulse(ctx)
            break
        case "distortScan":
            internal.paintDistortScan(ctx)
            break
        case "stylizeDots":
            internal.paintStylizeDots(ctx)
            break
        case "presetStrip":
            internal.paintPresetStrip(ctx)
            break
        }

        internal.paintEffectIcons(ctx)
        ctx.restore()
    }

    onAnimationValueChanged: requestPaint()
    onStyleChanged: requestPaint()
    onBaseColorChanged: requestPaint()

    QtObject {
        id: internal

        function paintColorWave(ctx) {
            var grad = ctx.createLinearGradient(0, 0, root.width, root.height)
            grad.addColorStop(0, Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.25))
            grad.addColorStop(0.5, Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.08))
            grad.addColorStop(1, Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.20))
            ctx.fillStyle = grad
            ctx.fillRect(0, 0, root.width, root.height)

            for (var wave = 0; wave < 3; wave++) {
                var phaseOffset = wave * 0.33 + root.animationValue
                var amplitude = (root.height * 0.15) * (1 + wave * 0.3)
                var midY = root.height * (0.3 + wave * 0.2)
                var alpha = 0.15 + wave * 0.08

                ctx.strokeStyle = Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, alpha)
                ctx.lineWidth = 1.5
                ctx.beginPath()
                ctx.moveTo(0, midY)
                for (var x = 0; x <= root.width; x += 2) {
                    var t = x / root.width
                    var y = midY + Math.sin((t * 4 * Math.PI) + (phaseOffset * 2 * Math.PI)) * amplitude
                    ctx.lineTo(x, y)
                }
                ctx.stroke()
            }
        }

        function paintBlurPulse(ctx) {
            ctx.fillStyle = Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.1)
            ctx.fillRect(0, 0, root.width, root.height)

            var cx = root.width * 0.5
            var cy = root.height * 0.5
            var maxRadius = Math.max(root.width, root.height) * 0.6

            for (var ring = 0; ring < 5; ring++) {
                var phase = (root.animationValue + ring * 0.2) % 1.0
                var radius = maxRadius * phase
                var alpha = (1.0 - phase) * 0.25

                ctx.strokeStyle = Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, alpha)
                ctx.lineWidth = 1.5
                ctx.beginPath()
                ctx.arc(cx, cy, radius, 0, 2 * Math.PI)
                ctx.stroke()
            }
        }

        function paintDistortScan(ctx) {
            ctx.fillStyle = Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.08)
            ctx.fillRect(0, 0, root.width, root.height)

            ctx.strokeStyle = Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.2)
            ctx.lineWidth = 1
            var seed = Math.floor(root.animationValue * 100)
            for (var y = 0; y < root.height; y += 4) {
                var offset = Math.sin(seed + y * 0.1 + root.animationValue * Math.PI * 2) * 6
                ctx.beginPath()
                ctx.moveTo(offset, y)
                ctx.lineTo(root.width + offset, y)
                ctx.stroke()
            }

            var glitchY = root.height * ((root.animationValue * 3) % 1.0)
            ctx.fillStyle = Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.35)
            ctx.fillRect(0, glitchY, root.width, 3)
        }

        function paintStylizeDots(ctx) {
            ctx.fillStyle = Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.06)
            ctx.fillRect(0, 0, root.width, root.height)

            var spacing = 10
            var phase = root.animationValue * spacing
            ctx.fillStyle = Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.3)

            for (var y = -spacing + phase; y < root.height + spacing; y += spacing) {
                var rowOffset = (Math.floor(y / spacing) % 2 !== 0) ? spacing * 0.5 : 0
                for (var x = -spacing + rowOffset; x < root.width + spacing; x += spacing) {
                    var dist = Math.sqrt(Math.pow(x - root.width / 2, 2) + Math.pow(y - root.height / 2, 2))
                    var maxDist = Math.sqrt(Math.pow(root.width / 2, 2) + Math.pow(root.height / 2, 2))
                    var sizeFactor = 1.0 - Math.min(1.0, Math.max(0.0, dist / maxDist))
                    var r = 1.0 + sizeFactor * 2.0
                    ctx.beginPath()
                    ctx.arc(x, y, r, 0, 2 * Math.PI)
                    ctx.fill()
                }
            }
        }

        function paintPresetStrip(ctx) {
            var grad = ctx.createLinearGradient(0, 0, root.width, 0)
            grad.addColorStop(0, Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.3))
            grad.addColorStop(0.35, root.baseColor)
            grad.addColorStop(0.65, root.baseColor)
            grad.addColorStop(1, Qt.rgba(root.baseColor.r, root.baseColor.g, root.baseColor.b, 0.5))
            ctx.fillStyle = grad
            ctx.fillRect(0, 0, root.width, root.height)

            var shimmerX = root.width * root.animationValue
            var shimmerWidth = 40
            var shimmerGrad = ctx.createLinearGradient(shimmerX - shimmerWidth, 0, shimmerX + shimmerWidth, 0)
            shimmerGrad.addColorStop(0, "transparent")
            shimmerGrad.addColorStop(0.5, Qt.rgba(1, 1, 1, 0.15))
            shimmerGrad.addColorStop(1, "transparent")
            ctx.fillStyle = shimmerGrad
            ctx.fillRect(shimmerX - shimmerWidth, 0, shimmerWidth * 2, root.height)
        }

        function paintEffectIcons(ctx) {
            if (root.effectCount <= 0) return
            var iconCount = Math.min(root.effectCount, 6)
            var iconSize = 10
            var padding = 4
            var startX = root.width - (iconCount * (iconSize + padding)) - padding
            var y = root.height - iconSize - padding

            ctx.fillStyle = Qt.rgba(1, 1, 1, 0.7)
            for (var i = 0; i < iconCount; i++) {
                var x = startX + i * (iconSize + padding)
                ctx.beginPath()
                ctx.roundedRect(x, y, iconSize, iconSize, 2, 2)
                ctx.fill()
            }
        }
    }
}
