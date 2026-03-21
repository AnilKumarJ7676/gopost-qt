#pragma once

#include <QObject>
#include <QTimer>

namespace gopost::diagnostics {

class DiagnosticEngine;

class OverlayDataProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(double fps READ fps NOTIFY updated)
    Q_PROPERTY(double frameTime READ frameTime NOTIFY updated)
    Q_PROPERTY(QString memoryUsed READ memoryUsed NOTIFY updated)
    Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY updated)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
    explicit OverlayDataProvider(DiagnosticEngine* engine, QObject* parent = nullptr);

    double fps() const { return fps_; }
    double frameTime() const { return frameTime_; }
    QString memoryUsed() const { return memoryUsed_; }
    int droppedFrames() const { return droppedFrames_; }
    bool visible() const { return visible_; }
    void setVisible(bool v);

signals:
    void updated();
    void visibleChanged();

private slots:
    void refresh();

private:
    DiagnosticEngine* engine_;
    QTimer timer_;
    double fps_ = 0.0;
    double frameTime_ = 0.0;
    QString memoryUsed_;
    int droppedFrames_ = 0;
    bool visible_ = true;
};

} // namespace gopost::diagnostics
