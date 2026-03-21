#include "TestBridge.h"
#include "core/bridges/common/bridge_factory.h"
#include "core/bridges/common/file_picker_options_builder.h"
#include "core/bridges/common/path_sanitizer.h"
#include "core/interfaces/IPlatformBridge.h"
#include "core/interfaces/IResult.h"

#include <QDebug>

using namespace gopost::bridges;
using namespace gopost::core::interfaces;

void TestBridge::factoryCreates() {
    auto bridge = BridgeFactory::create();
    QVERIFY2(bridge != nullptr, "BridgeFactory::create() returned null");
}

void TestBridge::directoryResolution() {
    auto bridge = BridgeFactory::create();
    QVERIFY(bridge != nullptr);

    auto cache = bridge->getAppCacheDirectory();
    QVERIFY2(cache.ok(), cache.error.c_str());
    QVERIFY(!cache.get().empty());
    qDebug() << "  Cache dir :" << QString::fromStdString(cache.get());

    auto docs = bridge->getAppDocumentsDirectory();
    QVERIFY2(docs.ok(), docs.error.c_str());
    QVERIFY(!docs.get().empty());
    qDebug() << "  Docs dir  :" << QString::fromStdString(docs.get());

    auto temp = bridge->getTempDirectory();
    QVERIFY2(temp.ok(), temp.error.c_str());
    QVERIFY(!temp.get().empty());
    qDebug() << "  Temp dir  :" << QString::fromStdString(temp.get());
}

void TestBridge::storageInfo() {
    auto bridge = BridgeFactory::create();
    QVERIFY(bridge != nullptr);

    auto space = bridge->getAvailableStorageBytes();
    QVERIFY2(space.ok(), space.error.c_str());
    QVERIFY(space.get() > 0);

    double gb = static_cast<double>(space.get()) / (1024.0 * 1024.0 * 1024.0);
    qDebug().nospace() << "  Available storage: " << QString::number(gb, 'f', 1) << " GB";
}

void TestBridge::deviceInfo() {
    auto bridge = BridgeFactory::create();
    QVERIFY(bridge != nullptr);

    auto model = bridge->getDeviceModel();
    QVERIFY2(model.ok(), model.error.c_str());
    QVERIFY(!model.get().empty());

    auto os = bridge->getOsVersionString();
    QVERIFY2(os.ok(), os.error.c_str());
    QVERIFY(!os.get().empty());

    qDebug() << "  Device:" << QString::fromStdString(model.get())
             << "running" << QString::fromStdString(os.get());
}

void TestBridge::fileRead() {
    auto bridge = BridgeFactory::create();
    QVERIFY(bridge != nullptr);

    // Read this test's source file
    std::string thisFile = __FILE__;
    auto bytes = bridge->readFileBytes(thisFile, 0, 4096);
    QVERIFY2(bytes.ok(), bytes.error.c_str());
    QVERIFY(bytes.get().size() == 4096);

    // Show a preview of the first 20 characters
    std::string preview(bytes.get().begin(), bytes.get().begin() + std::min<size_t>(20, bytes.get().size()));
    qDebug() << "  Read 4096 bytes from test source file — first 20 chars:"
             << QString::fromStdString(preview);
}

void TestBridge::fileMetadata() {
    auto bridge = BridgeFactory::create();
    QVERIFY(bridge != nullptr);

    std::string thisFile = __FILE__;
    auto meta = bridge->getFileMetadata(thisFile);
    QVERIFY2(meta.ok(), meta.error.c_str());
    QVERIFY(meta.get().sizeBytes > 0);
    QVERIFY(!meta.get().name.empty());

    qDebug() << "  File:" << QString::fromStdString(meta.get().name)
             << "," << meta.get().sizeBytes << "bytes"
             << ", MIME:" << QString::fromStdString(meta.get().mimeType)
             << ", modified:" << meta.get().lastModified;
}

void TestBridge::filePickerManual() {
    qDebug() << "  File picker test: MANUAL (requires UI interaction)";
    // Verify the builder compiles and creates valid options
    auto opts = FilePickerOptionsBuilder()
        .title("Import Media")
        .allowMultiple(true)
        .extensions({".mp4", ".mov", ".jpg", ".png"})
        .build();
    QVERIFY(opts.allowMultiple);
    QVERIFY(opts.allowedExtensions.size() == 4);
    QVERIFY(opts.title == "Import Media");
}

void TestBridge::pathSanitizer() {
    // Valid paths
    auto ok1 = PathSanitizer::sanitize("C:\\Users\\test\\file.txt");
    QVERIFY(ok1.ok());
    QVERIFY(!ok1.get().empty());

    auto ok2 = PathSanitizer::sanitize("/tmp/test.txt");
    QVERIFY(ok2.ok());

    // Invalid: empty
    auto fail1 = PathSanitizer::sanitize("");
    QVERIFY(!fail1.ok());

    // Invalid: null bytes
    std::string nullPath = "C:\\test";
    nullPath += '\0';
    nullPath += "evil.exe";
    auto fail2 = PathSanitizer::sanitize(nullPath);
    QVERIFY(!fail2.ok());

    // Invalid: whitespace only
    auto fail3 = PathSanitizer::sanitize("   ");
    QVERIFY(!fail3.ok());

    // UTF-8 validation
    QVERIFY(PathSanitizer::isValidUtf8("Hello world"));
    QVERIFY(PathSanitizer::isValidUtf8(""));
    // Japanese text in raw UTF-8
    QVERIFY(PathSanitizer::isValidUtf8(std::string("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e")));

    qDebug() << "  PathSanitizer: all validation tests passed";
}

QTEST_GUILESS_MAIN(TestBridge)
#include "TestBridge.moc"
