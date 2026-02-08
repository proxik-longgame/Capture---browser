#include <QApplication>
#include <QWebEngineProfile>
#include <QStandardPaths>
#include <QDir>
#include "core/browser.h"

int main(int argc, char *argv[]) {
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9222");
    
    QApplication app(argc, argv);

    QWebEngineProfile *p = QWebEngineProfile::defaultProfile();
    
    QString path = QApplication::applicationDirPath() + "/user_data";
    QDir().mkpath(path);

    p->setPersistentStoragePath(path + "/storage");
    p->setCachePath(path + "/cache");
    p->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    Browser w;
    w.show();
    return app.exec();
}