#pragma once

#include <QTest>

class TestIO : public QObject {
    Q_OBJECT
private slots:
    // Section A: AsyncReadPool
    void asyncReadPool();
    // Section B: BufferedReader
    void bufferedReader();
    // Section C: ProjectDatabase
    void projectDatabase();
    // Section D: DuplicateDetector
    void duplicateDetector();
    // Section E: FileValidator
    void fileValidator();
};
