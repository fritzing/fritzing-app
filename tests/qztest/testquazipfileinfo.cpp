#include "testquazipfileinfo.h"

#include "qztest.h"

#include <QByteArray>
#include <QDir>
#include <QFileInfo>

#include <QtTest/QtTest>

#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "quazip/quazipfileinfo.h"
#include "quazip/quazipnewinfo.h"

TestQuaZipFileInfo::TestQuaZipFileInfo(QObject *parent) :
    QObject(parent)
{
}

void TestQuaZipFileInfo::getNTFSTime()
{
    QString zipName = "newtimes.zip";
    QStringList testFiles;
    testFiles << "test.txt";
    QDir curDir;
    if (curDir.exists(zipName)) {
        if (!curDir.remove(zipName))
            QFAIL("Can't remove zip file");
    }
    if (!createTestFiles(testFiles)) {
        QFAIL("Can't create test file");
    }
    QDateTime base(QDate(1601, 1, 1), QTime(0, 0), Qt::UTC);
    quint64 mTicks, aTicks, cTicks;
    QFileInfo fileInfo("tmp/test.txt");
    {
        // create
        QuaZip zip(zipName);
        QVERIFY(zip.open(QuaZip::mdCreate));
        QuaZipFile zipFile(&zip);
        QDateTime lm = fileInfo.lastModified().toUTC();
        QDateTime lr = fileInfo.lastRead().toUTC();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        QDateTime cr = fileInfo.birthTime().toUTC();
#else
        QDateTime cr = fileInfo.created().toUTC();
#endif
        mTicks = (static_cast<qint64>(base.date().daysTo(lm.date()))
                * Q_UINT64_C(86400000)
                + static_cast<qint64>(base.time().msecsTo(lm.time())))
            * Q_UINT64_C(10000);
        aTicks = (static_cast<qint64>(base.date().daysTo(lr.date()))
                * Q_UINT64_C(86400000)
                + static_cast<qint64>(base.time().msecsTo(lr.time())))
            * Q_UINT64_C(10000);
        cTicks = (static_cast<qint64>(base.date().daysTo(cr.date()))
                * Q_UINT64_C(86400000)
                + static_cast<qint64>(base.time().msecsTo(cr.time())))
            * Q_UINT64_C(10000);
        QuaZipNewInfo newInfo("test.txt", "tmp/test.txt");
        QByteArray extra(36, 0);
        extra[0] = 0x0A; // magic
        extra[1] = 0x00;
        extra[2] = 32; // size
        extra[3] = 0;
        extra[4] = extra[5] = extra[6] = extra[7] = 0; // reserved
        extra[8] = 0x01; // time tag
        extra[9] = 0x00;
        extra[10] = 24; // time tag size
        extra[11] = 0;
        for (int i = 12; i < 36; i += 8) {
            quint64 ticks;
            if (i == 12) {
                ticks = mTicks;
            } else if (i == 20) {
                ticks = aTicks;
            } else if (i == 28) {
                ticks = cTicks;
            } else {
                QFAIL("Stupid programming bug here");
            }
            extra[i] = static_cast<char>(ticks);
            extra[i + 1] = static_cast<char>(ticks >> 8);
            extra[i + 2] = static_cast<char>(ticks >> 16);
            extra[i + 3] = static_cast<char>(ticks >> 24);
            extra[i + 4] = static_cast<char>(ticks >> 32);
            extra[i + 5] = static_cast<char>(ticks >> 40);
            extra[i + 6] = static_cast<char>(ticks >> 48);
            extra[i + 7] = static_cast<char>(ticks >> 56);
        }
        newInfo.extraLocal = extra;
        newInfo.extraGlobal = extra;
        QVERIFY(zipFile.open(QIODevice::WriteOnly, newInfo));
        zipFile.close();
        zip.close();
    }
    {
        // check
        QuaZip zip(zipName);
        QVERIFY(zip.open(QuaZip::mdUnzip));
        zip.goToFirstFile();
        QuaZipFileInfo64 zipFileInfo;
        QVERIFY(zip.getCurrentFileInfo(&zipFileInfo));
        zip.close();
        QCOMPARE(zipFileInfo.getNTFSmTime(), fileInfo.lastModified());
        QCOMPARE(zipFileInfo.getNTFSaTime(), fileInfo.lastRead());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        QCOMPARE(zipFileInfo.getNTFScTime(), fileInfo.birthTime());
#else
        QCOMPARE(zipFileInfo.getNTFScTime(), fileInfo.created());
#endif
    }
    removeTestFiles(testFiles);
    curDir.remove(zipName);
}
