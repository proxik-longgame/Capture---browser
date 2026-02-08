#include <QApplication>
#include <QWebEngineProfile>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include "core/browser.h"

int main(int argc, char *argv[]) {
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9222");
    
    QApplication app(argc, argv);

    QWebEngineProfile *p = QWebEngineProfile::defaultProfile();
    
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    
    // Using a fixed folder name instead of random ensures "Normal Save" persists across restarts
    QString storagePath = appData + "/CaptureData/storage";

    QDir().mkpath(storagePath);

    p->setPersistentStoragePath(storagePath);
    p->setCachePath(appData + "/CaptureData/cache");
    p->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    Browser w;
    w.show();
    return app.exec();
}