/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

/*!
 * DO NOT appear qDebug, qWanring etc in Logger.cpp! They are undefined and redefined to QtAV:Internal::Logger.xxx
 */
// we need LogLevel so must include QtAV_Global.h
#include <QtCore/QString>
#include "QtAV/QtAV_Global.h"
#include "Logger.h"

#ifdef HACK_QT_LOG

void ffmpeg_version_print();
namespace QtAV {
namespace Internal {
static QString gQtAVLogTag("");

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
typedef Logger::Context QMessageLogger;
#endif

static void log_helper(QtMsgType msgType, const QMessageLogger *qlog, const char* msg, va_list ap) {
    QString qmsg(gQtAVLogTag);
    if (msg)
        qmsg += QString().vsprintf(msg, ap);
    // qt_message_output is a public api
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    qt_message_output(msgType, qmsg.toUtf8().constData());
    return;
#else
    if (!qlog) {
        QMessageLogContext ctx;
        qt_message_output(msgType, ctx, qmsg);
        return;
    }
#endif
#ifndef QT_NO_DEBUG_STREAM
    if (msgType == QtWarningMsg)
        qlog->warning() << qmsg;
    else if (msgType == QtCriticalMsg)
        qlog->critical() << qmsg;
    else if (msgType == QtFatalMsg)
        qlog->fatal("%s", qmsg.toUtf8().constData());
    else
        qlog->debug() << qmsg;
#else
    if (msgType == QtFatalMsg)
        qlog->fatal("%s", qmsg.toUtf8().constData());
#endif //
}

// macro does not support A::##X

void Logger::debug(const char *msg, ...) const
{
    QtAVDebug d; // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return;
    if (v > (int)LogDebug && v < (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    // can not use ctx.debug() <<... because QT_NO_DEBUG_STREAM maybe defined
    log_helper(QtDebugMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::warning(const char *msg, ...) const
{
    QtAVDebug d; // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return;
    if (v > (int)LogWarning && v < (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    log_helper(QtWarningMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::critical(const char *msg, ...) const
{
    QtAVDebug d; // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return;
    if (v > (int)LogCritical && v < (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    log_helper(QtCriticalMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::fatal(const char *msg, ...) const
{
    QtAVDebug d; // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    /*
    if (v <= (int)LogOff)
        abort();
    if (v > (int)LogFatal && v < (int)LogAll)
        abort();
    */
    if (v > (int)LogOff) {
        va_list ap;
        va_start(ap, msg);
        log_helper(QtFatalMsg, &ctx, msg, ap);
        va_end(ap);
    }
    abort();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifndef QT_NO_DEBUG_STREAM
// internal used by Logger::fatal(const char*,...) with log level checked, so always do things here
void Logger::Context::fatal(const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    QString qmsg;
    if (msg)
        qmsg += QString().vsprintf(msg, ap);
    qt_message_output(QtFatalMsg, qmsg.toUtf8().constData());
    va_end(ap);
    abort();
}
#endif //QT_NO_DEBUG_STREAM
#endif //QT_VERSION

#ifndef QT_NO_DEBUG_STREAM
// will print message in ~QDebug()
// can not use QDebug on stack. It must lives in QtAVDebug
QtAVDebug Logger::debug() const
{
    QtAVDebug d(QtDebugMsg); //// initialize something. e.g. environment check
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return d;
    if (v <= (int)LogDebug || v >= (int)LogAll)
        d.setQDebug(new QDebug(ctx.debug()));
    return d;
}

QtAVDebug Logger::warning() const
{
    QtAVDebug d(QtWarningMsg);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return d;
    if (v <= (int)LogWarning || v >= (int)LogAll)
        d.setQDebug(new QDebug(ctx.warning()));
    return d;
}

QtAVDebug Logger::critical() const
{
    QtAVDebug d(QtCriticalMsg);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return d;
    if (v <= (int)LogCritical || v >= (int)LogAll)
        d.setQDebug(new QDebug(ctx.critical()));
    return d;
}
// no QMessageLogger::fatal()
#endif //QT_NO_DEBUG_STREAM

bool isLogLevelSet();

QtAVDebug::QtAVDebug(QtMsgType t, QDebug *d)
    : type(t)
    , dbg(0)
{
    setQDebug(d); // call *dbg << gQtAVLogTag
    static bool sFirstRun = true;
    if (!sFirstRun)
        return;
    sFirstRun = false;
    //printf("Qt Logging first run........\n");
    // check environment var and call other functions at first Qt logging call
    // always override setLogLevel()
    QByteArray env = qgetenv("QTAV_LOG_LEVEL");
    if (!env.isEmpty()) {
        bool ok = false;
        const int level = env.toInt(&ok);
        if (ok) {
            if (level < (int)LogOff)
                setLogLevel(LogOff);
            else if (level > (int)LogAll)
                setLogLevel(LogAll);
            else
                setLogLevel((LogLevel)level);
        } else {
            env = env.toLower();
            if (env.endsWith("off"))
                setLogLevel(LogOff);
            else if (env.endsWith("debug"))
                setLogLevel(LogDebug);
            else if (env.endsWith("warning"))
                setLogLevel(LogWarning);
            else if (env.endsWith("critical"))
                setLogLevel(LogCritical);
            else if (env.endsWith("fatal"))
                setLogLevel(LogFatal);
            else if (env.endsWith("all") || env.endsWith("default"))
                setLogLevel(LogAll);
        }
    }
    env = qgetenv("QTAV_LOG_TAG");
    if (!env.isEmpty()) {
        gQtAVLogTag = env;
    }

    if ((int)logLevel() > (int)LogOff) {
        ffmpeg_version_print();
        printf("%s\n", aboutQtAV_PlainText().toUtf8().constData());
        fflush(0);
    }
}

QtAVDebug::~QtAVDebug()
{
    setQDebug(0);
}

void QtAVDebug::setQDebug(QDebug *d)
{
    if (dbg) {
        delete dbg;
    }
    dbg = d;
    if (dbg && !gQtAVLogTag.isEmpty()) {
        *dbg << gQtAVLogTag;
    }
}

#if 0
QtAVDebug debug(const char *msg, ...)
{
    if ((int)logLevel() > (int)Debug && logLevel() != All) {
        return QtAVDebug();
    }
    va_list ap;
    va_start(ap, msg); // use variable arg list
    QMessageLogContext ctx;
    log_helper(QtDebugMsg, &ctx, msg, ap);
    va_end(ap);
    return QtAVDebug();
}
#endif

} //namespace Internal
} // namespace QtAV

#endif //HACK_QT_LOG
