/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "private/qavdemuxer_p.h"
#include "qavaudioframe.h"
#include "qavvideoframe.h"
#include "private/qaviodevice_p.h"
#include "private/qavvideocodec_p.h"
#include "private/qavaudiocodec_p.h"

#include <QDebug>
#include <QtTest/QtTest>

QT_USE_NAMESPACE

class tst_QAVDemuxer : public QObject
{
    Q_OBJECT
private slots:
    void construction();
    void loadIncorrect();
    void loadAudio();
    void loadVideo();
    void fileIO();
    void qrcIO();
};

void tst_QAVDemuxer::construction()
{
    QAVDemuxer d;
    QVERIFY(d.currentVideoStreamIndex() < 0);
    QVERIFY(d.currentAudioStreamIndex() < 0);
    QVERIFY(d.currentSubtitleStreamIndex() < 0);
    QCOMPARE(d.duration(), 0);
    QCOMPARE(d.seekable(), false);
    QCOMPARE(d.eof(), false);
    QVERIFY(!d.read());
    QVERIFY(d.seek(0) < 0);

    QAVPacket p;
    QVERIFY(!p);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.bytes(), 0);
    QVERIFY(p.streamIndex() < 0);
    QVERIFY(!p.decode());

    QAVFrame f;
    QVERIFY(!f);
    QVERIFY(isnan(f.pts()));

    QAVAudioFrame af;
    QVERIFY(!af);
    QVERIFY(isnan(af.pts()));

    QAVVideoFrame vf;
    QVERIFY(!vf);
}

void tst_QAVDemuxer::loadIncorrect()
{
    QAVDemuxer d;
    QVERIFY(d.load(QUrl("unknown.mp4")) < 0);
    QVERIFY(d.currentVideoStreamIndex() < 0);
    QVERIFY(d.currentAudioStreamIndex() < 0);
    QVERIFY(d.currentSubtitleStreamIndex() < 0);
    QVERIFY(!d.read());
    QVERIFY(d.seek(0) < 0);
}

void tst_QAVDemuxer::loadAudio()
{
    QAVDemuxer d;

    QFileInfo file(QLatin1String("../testdata/test.wav"));

    QVERIFY(d.load(QUrl::fromLocalFile(file.absoluteFilePath())) >= 0);
    QVERIFY(d.currentVideoStreamIndex() < 0);
    QVERIFY(d.currentAudioStreamIndex() >= 0);
    QVERIFY(d.currentSubtitleStreamIndex() < 0);
    QVERIFY(d.duration() > 0);
    QCOMPARE(d.seekable(), true);
    QCOMPARE(d.eof(), false);

    QAVPacket p;
    QAVPacket p2;
    QAVFrame f2;
    while ((p = d.read())) {
        QVERIFY(p);
        QVERIFY(p.packet());
        QVERIFY(p.duration() > 0);
        QVERIFY(p.pts() >= 0);
        QVERIFY(p.bytes() > 0);
        QCOMPARE(p.streamIndex(), d.currentAudioStreamIndex());

        p2 = p;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        QAVPacket p3 = p2;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        auto f = p.decode();
        QVERIFY(f);
        QVERIFY(f.frame());
        QVERIFY(f.pts() >= 0);

        f2 = f;
        QVERIFY(f2);
        QVERIFY(f2.frame());
        QCOMPARE(f2.pts(), f.pts());

        QAVFrame f3 = f2;
        QVERIFY(f3);
        QVERIFY(f3.frame());
        QCOMPARE(f3.pts(), f.pts());

        QAVAudioFrame af = f;
        QVERIFY(af);
        QVERIFY(af.frame());
        QCOMPARE(af.pts(), f.pts());
        QVERIFY(af.codec()->codec() != nullptr);

        auto format = af.format();
        QCOMPARE(format.sampleFormat(), QAVAudioFormat::Int32);
        auto data = af.data();
        QVERIFY(!data.isEmpty());

        QCOMPARE(d.eof(), false);
    }

    QVERIFY(p2);
    QVERIFY(p2.packet());
    QVERIFY(p2.duration() > 0);
    QVERIFY(p2.bytes() > 0);
    QCOMPARE(p2.streamIndex(), d.currentAudioStreamIndex());

    QVERIFY(f2);
    QVERIFY(f2.frame());
    QVERIFY(f2.pts() > 0);

    QCOMPARE(d.eof(), true);
    QVERIFY(d.seek(0) >= 0);
}

void tst_QAVDemuxer::loadVideo()
{
    QAVDemuxer d;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));

    QVERIFY(d.load(QUrl::fromLocalFile(file.absoluteFilePath())) >= 0);
    QVERIFY(d.currentVideoStreamIndex() >= 0);
    QVERIFY(d.currentAudioStreamIndex() >= 0);
    QVERIFY(d.currentSubtitleStreamIndex() < 0);
    QVERIFY(d.duration() > 0);
    QCOMPARE(d.seekable(), true);
    QCOMPARE(d.eof(), false);

    auto p = d.read();
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.bytes() > 0);
    QVERIFY(p.streamIndex() >= 0);

    auto f = p.decode();
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d.eof(), false);

    QVERIFY(d.seek(0) >= 0);
    while ((p = d.read())) {
        QCOMPARE(d.eof(), false);
        if (p.streamIndex() == d.currentVideoStreamIndex()) {
            auto f = p.decode();
            QVERIFY(f);
            QVERIFY(f.frame());
            QVERIFY(f.pts() >= 0);

            QAVVideoFrame vf = f;
            QVERIFY(vf);
            QVERIFY(vf.frame());
            QCOMPARE(vf.pts(), f.pts());
            QVERIFY(vf.size().isValid());
        }
    }

    QCOMPARE(d.eof(), true);
}

void tst_QAVDemuxer::fileIO()
{
    QAVDemuxer d;

    QFile file(QLatin1String("../testdata/colors.mp4"));
    if (!file.open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }

    QAVIODevice dev(file);
    QVERIFY(d.load(QUrl("colors.mp4"), &dev) >= 0);

    QVERIFY(d.currentVideoStreamIndex() >= 0);
    QVERIFY(d.currentAudioStreamIndex() >= 0);
    QVERIFY(d.currentSubtitleStreamIndex() < 0);
    QVERIFY(d.duration() > 0);
    QCOMPARE(d.seekable(), true);
    QCOMPARE(d.eof(), false);

    auto p = d.read();
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.bytes() > 0);
    QVERIFY(p.streamIndex() >= 0);

    auto f = p.decode();
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d.eof(), false);

    QVERIFY(d.seek(0) >= 0);
    while ((p = d.read())) {
        QCOMPARE(d.eof(), false);
        if (p.streamIndex() == d.currentVideoStreamIndex()) {
            auto f = p.decode();
            QVERIFY(f);
            QVERIFY(f.frame());
            QVERIFY(f.pts() >= 0);

            QAVVideoFrame vf = f;
            QVERIFY(vf);
            QVERIFY(vf.frame());
            QCOMPARE(vf.pts(), f.pts());
            QVERIFY(vf.size().isValid());
        }
    }

    QCOMPARE(d.eof(), true);
}

void tst_QAVDemuxer::qrcIO()
{
    QAVDemuxer d;

    QFile file(":/test.wav");
    if (!file.open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }

    QAVIODevice dev(file);
    QVERIFY(d.load(QUrl("test.wav"), &dev) >= 0);
    QVERIFY(d.currentVideoStreamIndex() < 0);
    QVERIFY(d.currentAudioStreamIndex() >= 0);
    QVERIFY(d.currentSubtitleStreamIndex() < 0);
    QVERIFY(d.duration() > 0);
    QCOMPARE(d.seekable(), true);
    QCOMPARE(d.eof(), false);

    QAVPacket p;
    QAVPacket p2;
    QAVFrame f2;
    while ((p = d.read())) {
        QVERIFY(p);
        QVERIFY(p.packet());
        QVERIFY(p.duration() > 0);
        QVERIFY(p.pts() >= 0);
        QVERIFY(p.bytes() > 0);
        QCOMPARE(p.streamIndex(), d.currentAudioStreamIndex());

        p2 = p;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        QAVPacket p3 = p2;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        auto f = p.decode();
        QVERIFY(f);
        QVERIFY(f.frame());
        QVERIFY(f.pts() >= 0);

        f2 = f;
        QVERIFY(f2);
        QVERIFY(f2.frame());
        QCOMPARE(f2.pts(), f.pts());

        QAVFrame f3 = f2;
        QVERIFY(f3);
        QVERIFY(f3.frame());
        QCOMPARE(f3.pts(), f.pts());

        QAVAudioFrame af = f;
        QVERIFY(af);
        QVERIFY(af.frame());
        QCOMPARE(af.pts(), f.pts());
        QVERIFY(af.codec()->codec() != nullptr);

        auto format = af.format();
        QCOMPARE(format.sampleFormat(), QAVAudioFormat::Int32);
        auto data = af.data();
        QVERIFY(!data.isEmpty());

        QCOMPARE(d.eof(), false);
    }

    QVERIFY(p2);
    QVERIFY(p2.packet());
    QVERIFY(p2.duration() > 0);
    QVERIFY(p2.bytes() > 0);
    QCOMPARE(p2.streamIndex(), d.currentAudioStreamIndex());

    QVERIFY(f2);
    QVERIFY(f2.frame());
    QVERIFY(f2.pts() > 0);

    QCOMPARE(d.eof(), true);
    QVERIFY(d.seek(0) >= 0);
}

QTEST_MAIN(tst_QAVDemuxer)
#include "tst_qavdemuxer.moc"
