#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QTimer>

#include "rendering_bridge/qprocess_video_decoder.h"
#include "rendering_bridge/render_decode_thread.h"

using namespace gopost::rendering;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    QString testVideo = QStringLiteral("C:/Users/abcd/Documents/CapCut/Videos/0313.mp4");

    qDebug() << "=== Test 1: QProcessVideoDecoder direct ===";
    {
        QProcessVideoDecoder dec;
        bool ok = dec.open(testVideo);
        qDebug() << "open:" << ok;
        if (ok) {
            qDebug() << "  width:" << dec.width() << "height:" << dec.height()
                     << "fps:" << dec.frameRate() << "dur:" << dec.duration();

            // Decode a few frames (on main thread — fine for testing)
            for (int i = 0; i < 3; i++) {
                auto frame = dec.decodeNextFrame();
                if (frame.has_value()) {
                    qDebug() << "  frame" << i << ": " << frame->width << "x" << frame->height
                             << "ts:" << frame->timestamp_seconds
                             << "pixels:" << frame->pixels.size() << "bytes";
                } else {
                    qDebug() << "  frame" << i << ": NONE (eof:" << dec.isEof() << ")";
                }
            }
            dec.close();
        }
    }

    qDebug() << "\n=== Test 2: RenderDecodeThread ===";
    {
        RenderDecodeThread dt(4);
        bool ok = dt.open(testVideo);
        qDebug() << "open:" << ok;
        if (ok) {
            qDebug() << "  width:" << dt.width() << "height:" << dt.height()
                     << "fps:" << dt.frameRate() << "dur:" << dt.duration();

            // Wait for some frames to be decoded
            QThread::msleep(3000);

            qDebug() << "  buffered:" << dt.bufferedFrameCount() << "eof:" << dt.isEof();

            // Pop frames
            for (int i = 0; i < 5; i++) {
                auto frame = dt.popFrame();
                if (frame.has_value()) {
                    qDebug() << "  popped frame" << i << ":" << frame->width << "x" << frame->height
                             << "ts:" << frame->timestamp_seconds
                             << "pixels:" << frame->pixels.size();
                } else {
                    qDebug() << "  popped frame" << i << ": EMPTY";
                }
            }

            // Test seek
            qDebug() << "  seeking to 2.0s...";
            dt.seekTo(2.0);
            QThread::msleep(2000);
            qDebug() << "  after seek — buffered:" << dt.bufferedFrameCount();

            auto frame = dt.popFrame();
            if (frame.has_value()) {
                qDebug() << "  post-seek frame:" << frame->width << "x" << frame->height
                         << "ts:" << frame->timestamp_seconds;
            } else {
                qDebug() << "  post-seek frame: EMPTY";
            }

            dt.close();
        }
    }

    qDebug() << "\n=== DONE ===";
    return 0;
}
