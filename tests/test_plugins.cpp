/***************************************************************************
                          test_plugins.cpp  -  description
                             -------------------
    purpose              : Tests for loading file format plugins and reading
                           traces through them
    copyright            : (C) 2026 by Joscha Schmiedt
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "pluginregistry.h"
#include "plugintracesprovider.h"
#include "testplugin.h"
#include "testutils.h"

#include <QTemporaryDir>
#include <QtTest>

#include <vector>

using namespace testutils;

namespace
{

QString pluginDirectory(const QString& name)
{
    return QStringLiteral(NEUROSCOPE_TEST_PLUGIN_DIR "/") + name;
}

std::int64_t expectedFast(std::int64_t sample, int channel)
{
    return roundHalfAway(test_fast_value(sample, channel));
}

Matrix request(PluginTracesProvider& provider, std::int64_t startTime, std::int64_t endTime)
{
    Matrix result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &TracesProvider::dataReady,
                                       [&](Array<dataType>& data, QObject*)
                                       {
                                           result = toMatrix(data);
                                           ++emitted;
                                       });
    provider.requestData(static_cast<long>(startTime), static_cast<long>(endTime), nullptr, 0);
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("dataReady emitted %d times", emitted);
    return result;
}

} // namespace

class TestPlugins : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;
    PluginRegistry registry;

    QString path(const QString& name) const { return dir.filePath(name); }

    std::shared_ptr<PluginFile> openTestFile()
    {
        QString error;
        auto file = PluginFile::open(registry.pluginFor(path("session.nstest")), path("session.nstest"), &error);
        if (!file)
            qFatal("Cannot open the test file: %s", qPrintable(error));
        return file;
    }

  private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(dir.isValid());
        writeTextFile(path("session.nstest"), "NSTEST");
        writeTextFile(path("UPPER.NSTEST"), "NSTEST");
        writeTextFile(path("other.nstest"), "something else");
        writeTextFile(path("session.dat"), "NSTEST");
        writeTextFile(path("broken.nstest"), "NSTEST-broken");

        registry.loadDirectory(pluginDirectory("good"));
        QCOMPARE(registry.errors(), QStringList());
        QCOMPARE(registry.plugins().size(), 1);
    }

    void pluginDescription()
    {
        const auto plugin = registry.plugins().first();
        QCOMPARE(plugin->name(), QString(TEST_PLUGIN_NAME));
        QCOMPARE(plugin->extensions(), QStringList({"nstest"}));
        QCOMPARE(registry.fileFilters(), QStringList({"Test format (*.nstest)"}));
    }

    void pluginForFile()
    {
        QVERIFY(registry.pluginFor(path("session.nstest")));
        QVERIFY(registry.pluginFor(path("UPPER.NSTEST")));
        // The extension matches, but the probe rejects the content.
        QVERIFY(!registry.pluginFor(path("other.nstest")));
        // The content matches, but no plugin has the extension.
        QVERIFY(!registry.pluginFor(path("session.dat")));
        QVERIFY(!registry.pluginFor(path("missing.nstest")));
    }

    void pluginWithSameNameIsSkipped()
    {
        PluginRegistry twice;
        twice.loadDirectory(pluginDirectory("good"));
        twice.loadDirectory(pluginDirectory("duplicate"));
        QCOMPARE(twice.errors(), QStringList());
        QCOMPARE(twice.plugins().size(), 1);
        QVERIFY(twice.plugins().first()->libraryPath().startsWith(pluginDirectory("good")));
    }

    void unusableLibrariesAreRejected()
    {
        PluginRegistry bad;
        bad.loadDirectory(pluginDirectory("bad"));
        QCOMPARE(bad.plugins().size(), 0);
        const QStringList errors = bad.errors();
        QCOMPARE(errors.size(), 2);
        const QString all = errors.join('\n');
        QVERIFY2(all.contains("version 99"), qPrintable(all));
        QVERIFY2(all.contains("no neuroscope_plugin function"), qPrintable(all));
    }

    void missingDirectoryIsIgnored()
    {
        PluginRegistry none;
        none.loadDirectory(path("no such directory"));
        QCOMPARE(none.plugins().size(), 0);
        QCOMPARE(none.errors(), QStringList());
    }

    void defaultDirectoriesStartWithEnvironment()
    {
        const QByteArray previous = qgetenv("NEUROSCOPE_PLUGIN_PATH");
        qputenv("NEUROSCOPE_PLUGIN_PATH", (pluginDirectory("good") + QDir::listSeparator() + pluginDirectory("duplicate")).toUtf8());
        const QStringList directories = PluginRegistry::defaultDirectories();
        qputenv("NEUROSCOPE_PLUGIN_PATH", previous);
        QVERIFY(directories.size() >= 3);
        QCOMPARE(directories.at(0), pluginDirectory("good"));
        QCOMPARE(directories.at(1), pluginDirectory("duplicate"));
    }

    void streams()
    {
        const auto file = openTestFile();
        const QList<PluginFile::Stream> streams = file->streams();
        QCOMPARE(streams.size(), 2);
        QCOMPARE(streams[0].id, QString("1000Hz"));
        QCOMPARE(streams[0].label, QString("1 kHz: 2 channels"));
        QCOMPARE(streams[0].channelCount, 2);
        QCOMPARE(streams[0].samplingRate, 1000.0);
        QCOMPARE(std::int64_t(streams[0].sampleCount), 3000_i64);
        QCOMPARE(streams[1].id, QString("250Hz"));
        QCOMPARE(file->defaultStream(), 0);
        QCOMPARE(file->streamIndex("250Hz"), 1);
        QCOMPARE(file->streamIndex("1Hz"), -1);
        QCOMPARE(file->channelLabels(0), QStringList({"A0", "A1"}));
        // Channels without a label are numbered.
        QCOMPARE(file->channelLabels(1), QStringList({"0"}));
        QCOMPARE(file->channelGroups(0), QList<int>({1, 0}));
        QCOMPARE(file->channelGroups(1), QList<int>({0}));
    }

    void noGroupsFromOlderPlugin()
    {
        PluginRegistry other;
        other.loadDirectory(pluginDirectory("slowdefault"));
        QString error;
        const auto file = PluginFile::open(other.pluginFor(path("session.nstest")), path("session.nstest"), &error);
        QVERIFY2(file, qPrintable(error));
        QVERIFY(file->channelGroups(0).isEmpty());
    }

    void defaultStreamFromPlugin()
    {
        PluginRegistry other;
        other.loadDirectory(pluginDirectory("slowdefault"));
        QString error;
        const auto file = PluginFile::open(other.pluginFor(path("session.nstest")), path("session.nstest"), &error);
        QVERIFY2(file, qPrintable(error));
        QCOMPARE(file->defaultStream(), 1);
    }

    void openFailure()
    {
        QString error;
        QVERIFY(!PluginFile::open(registry.pluginFor(path("broken.nstest")), path("broken.nstest"), &error));
        QCOMPARE(error, QString("broken test file"));
    }

    void readOutsideStreamIsZero()
    {
        const auto file = openTestFile();
        // Two samples before the start of the stream and two after its end.
        std::vector<double> before(4 * 2, -1), after(4 * 2, -1);
        QString error;
        QVERIFY(file->read(0, -2, 4, before.data(), &error));
        QVERIFY(file->read(0, 2998, 4, after.data(), &error));
        QCOMPARE(before, std::vector<double>({0, 0, 0, 0, 0, 0, 0, 0}));
        QCOMPARE(after[0], test_fast_value(2998, 0));
        QCOMPARE(after[3], test_fast_value(2999, 1));
        QCOMPARE(std::vector<double>(after.begin() + 4, after.end()), std::vector<double>({0, 0, 0, 0}));
    }

    void providerMetadata()
    {
        PluginTracesProvider provider(openTestFile(), 0);
        QCOMPARE(provider.getNbChannels(), 2);
        QCOMPARE(provider.getSamplingRate(), 1000.0);
        QCOMPARE(std::int64_t(provider.recordingLength()), 3000_i64);
        QCOMPARE(std::int64_t(provider.getTotalNbSamples()), 3000_i64);
        QCOMPARE(provider.getLabels(), QStringList({"A0", "A1"}));

        // The file describes itself, so the parameters cannot be changed.
        provider.setNbChannels(5);
        provider.setSamplingRate(20000);
        QCOMPARE(provider.getNbChannels(), 2);
        QCOMPARE(provider.getSamplingRate(), 1000.0);
        QCOMPARE(std::int64_t(provider.recordingLength()), 3000_i64);
    }

    void providerReadWindow_data()
    {
        QTest::addColumn<std::int64_t>("startTime");
        QTest::addColumn<std::int64_t>("endTime");
        QTest::addColumn<std::int64_t>("firstSample");
        QTest::addColumn<std::int64_t>("nbSamples");

        // As for .dat files, the samples at the start and the end time are included,
        // except at the end of the recording.
        QTest::newRow("window") << 200_i64 << 300_i64 << 200_i64 << 101_i64;
        QTest::newRow("before the recording starts") << 0_i64 << 150_i64 << 0_i64 << 151_i64;
        QTest::newRow("pause") << 900_i64 << 1600_i64 << 900_i64 << 701_i64;
        QTest::newRow("end of recording") << 2900_i64 << 3000_i64 << 2900_i64 << 100_i64;
    }

    void providerReadWindow()
    {
        QFETCH(std::int64_t, startTime);
        QFETCH(std::int64_t, endTime);
        QFETCH(std::int64_t, firstSample);
        QFETCH(std::int64_t, nbSamples);

        PluginTracesProvider provider(openTestFile(), 0);
        const Matrix data = request(provider, startTime, endTime);
        QCOMPARE(data.rows, nbSamples);
        QCOMPARE(data.cols, 2_i64);
        for (std::int64_t s = 0; s < nbSamples; ++s)
            for (int c = 0; c < 2; ++c)
                QVERIFY2(data(s + 1, c + 1) == expectedFast(firstSample + s, c),
                         qPrintable(QString("sample %1, channel %2: %3").arg(firstSample + s).arg(c).arg(data(s + 1, c + 1))));
    }

    void providerRounding()
    {
        PluginTracesProvider provider(openTestFile(), 0);
        const Matrix data = request(provider, 500, 500);
        QCOMPARE(data.rows, 1_i64);
        // 500.5 µV and -10500.5 µV, rounded half away from zero.
        QCOMPARE(data(1, 1), 501_i64);
        QCOMPARE(data(1, 2), -10501_i64);
    }

    void providerOtherStream()
    {
        PluginTracesProvider provider(openTestFile(), 1);
        QCOMPARE(provider.getNbChannels(), 1);
        QCOMPARE(provider.getSamplingRate(), 250.0);
        QCOMPARE(std::int64_t(provider.recordingLength()), 3000_i64);
        QCOMPARE(provider.getLabels(), QStringList({"0"}));
        // 100 ms to 200 ms are samples 25 to 50.
        const Matrix data = request(provider, 100, 200);
        QCOMPARE(data.rows, 26_i64);
        QCOMPARE(data(1, 1), 50_i64);
        QCOMPARE(data(26, 1), 100_i64);
    }
};

QTEST_GUILESS_MAIN(TestPlugins)
#include "test_plugins.moc"
