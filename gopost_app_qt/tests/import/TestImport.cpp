#include "TestImport.h"
#include "core/import/media_import_engine.h"
#include "core/interfaces/IMediaImport.h"
#include "core/interfaces/IResult.h"

using namespace gopost::media_import;
using namespace gopost::core::interfaces;

void TestImport::importStub() {
    MediaImportEngine engine;
    std::vector<ImportRequest> requests = {{"file:///test.mp4", "test.mp4", true}};
    auto handle = engine.importFiles(requests);
    QVERIFY(handle == -1);
}

void TestImport::statusStub() {
    MediaImportEngine engine;
    auto status = engine.getImportStatus(-1);
    QVERIFY(status.state == ImportState::PENDING);
    QVERIFY(status.totalFiles == 0);
}

QTEST_GUILESS_MAIN(TestImport)
#include "TestImport.moc"
