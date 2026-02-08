#include "tabmanager.h"
#include "browser.h"
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEnginePermission>
#include <QToolButton>
#include <QPropertyAnimation>
#include <QWebChannel>
#include <QMessageBox>

TabManager::TabManager(QWidget *parent) : QTabWidget(parent) {
    setTabsClosable(true);
    setMovable(true);
    
    QToolButton *btn = new QToolButton(this);
    btn->setText("+");
    btn->setCursor(Qt::PointingHandCursor);
    setCornerWidget(btn, Qt::TopRightCorner);

    connect(btn, &QToolButton::clicked, this, &TabManager::handleNewTabButtonClick);
    connect(this, &QTabWidget::tabCloseRequested, [this](int idx) { if(count() > 1) removeTab(idx); });

    connect(this, &QTabWidget::currentChanged, [this](int index) {
        if (index != -1) {
            if (auto *view = qobject_cast<QWebEngineView*>(widget(index))) {
                if (auto *mainWindow = qobject_cast<Browser*>(window())) {
                    mainWindow->addressBar->setText(view->url().toString());
                    mainWindow->updateSslIcon(view->url());
                }
            }
        }
    });
}

void TabManager::handleNewTabButtonClick() {
    createNewTab(QUrl("https://www.google.com"));
}

QWebEngineView* TabManager::createNewTab(const QUrl &url) {
    QWebEngineView *view = new QWebEngineView;
    view->page()->setBackgroundColor(QColor("#0a0a0a"));
    
    connect(view->page(), &QWebEnginePage::permissionRequested, [this](QWebEnginePermission permission) {
        QString type = "Unknown Device";
        auto pType = permission.permissionType();

        if (pType == QWebEnginePermission::PermissionType::MediaAudioCapture) 
            type = "Microphone";
        else if (pType == QWebEnginePermission::PermissionType::MediaVideoCapture) 
            type = "Camera";
        else if (pType == QWebEnginePermission::PermissionType::MediaAudioVideoCapture)
            type = "Camera & Microphone";
        else if (pType == QWebEnginePermission::PermissionType::Geolocation)
            type = "Location";
        else if (pType == QWebEnginePermission::PermissionType::Notifications)
            type = "Notifications";
        else if (pType == QWebEnginePermission::PermissionType::DesktopVideoCapture || 
                pType == QWebEnginePermission::PermissionType::DesktopAudioVideoCapture)
            type = "Screen Sharing";

        QMessageBox::StandardButton res = QMessageBox::question(this, "Permission Request",
            QString("The website wants to access your %1. Allow?").arg(type));
        
        if (res == QMessageBox::Yes) {
            permission.grant();
        } else {
            permission.deny();
        }
    });

    connect(view, &QWebEngineView::urlChanged, [this, view](const QUrl &u) {
        if (this->currentWidget() == view) {
            if (auto *mainWindow = qobject_cast<Browser*>(window())) {
                mainWindow->addressBar->setText(u.toString());

                if (u.scheme() == "https") {
                    mainWindow->sslLabel->setPixmap(QIcon::fromTheme("security-high").pixmap(16, 16));
                    mainWindow->sslLabel->setStyleSheet("color: green;");
                } else {
                    mainWindow->sslLabel->setPixmap(QIcon::fromTheme("security-low").pixmap(16, 16));
                    mainWindow->sslLabel->setStyleSheet("color: red;");
                }
            }
        }
    });
    connect(view, &QWebEngineView::loadProgress, [this, view](int progress) {
        if (this->currentWidget() == view) {
            if (auto *mainWindow = qobject_cast<Browser*>(window())) {
                mainWindow->updateUI(progress);
            }
        }
    });

    connect(view->page(), &QWebEnginePage::certificateError, [this](QWebEngineCertificateError error) {
        if (auto *mainWindow = qobject_cast<Browser*>(window())) {
            mainWindow->handleSslErrors(error);
        }
    });

    connect(view, &QWebEngineView::loadFinished, [this, view](bool ok) {
        if (ok) {
            if (auto *mainWindow = qobject_cast<Browser*>(window())) {
                mainWindow->addHistoryEntry(view->url()); 
            }
        }
    });

    if (url.toString() == "capture://settings") {
        QString html = R"html(
            <html><head>
            <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
            <script>
                var backend;
                new QWebChannel(qt.webChannelTransport, function (channel) {
                    backend = channel.objects.handler;
                });
            </script>
            <style>
                body { background: #0a0a0a; color: #eee; font-family: 'Segoe UI', sans-serif; padding: 40px; }
                .container { max-width: 800px; margin: auto; }
                .card { background: #161616; border: 1px solid #333; padding: 20px; border-radius: 12px; margin-bottom: 15px; display: flex; justify-content: space-between; align-items: center; transition: 0.3s; }
                .card:hover { border-color: #0078d4; background: #1a1a1a; }
                select, button { background: #252525; color: white; border: 1px solid #444; padding: 10px; border-radius: 8px; cursor: pointer; min-width: 140px; }
                h1 { color: #0078d4; font-size: 28px; }
                .danger { background: #c0392b; border: none; font-weight: bold; }
                span { color: #888; font-size: 13px; display: block; }
            </style>
            </head>
            <body>
                <div class="container">
                    <h1>Browser Settings</h1>
                    <div class='card'><div><b>Appearance</b><span>UI Theme Palette</span></div>
                        <select onchange="backend.changeTheme(this.value)">
                            <option value="Dark">Dark Mode</option>
                            <option value="White">Light Mode</option>
                            <option value="Private">Private (Neon)</option>
                        </select>
                    </div>
                    <div class='card'><div><b>Privacy Level</b><span>Security & Tracking</span></div>
                        <select onchange="backend.setPrivacyLevel(this.value)">
                            <option value="None">None</option>
                            <option value="Easy" selected>Easy</option>
                            <option value="Hardest">Hardest (Secure)</option>
                        </select>
                    </div>
                    
                    <div class='card'><div><b>Browsing Data</b><span>Clear all persistent files</span></div>
                        <button class='danger' onclick="backend.clearData()">Clear Everything</button>
                    </div>
                </div>
            </body></html>
        )html";
        
        view->setHtml(html);
        QWebChannel *channel = new QWebChannel(view->page());
        view->page()->setWebChannel(channel);
        channel->registerObject(QStringLiteral("handler"), window());
    } else {
        view->load(url);
    }

    int idx = addTab(view, "Loading...");
    setCurrentIndex(idx);
    
    connect(view, &QWebEngineView::titleChanged, [this, view](const QString &t) {
        int i = indexOf(view);
        if (i != -1) setTabText(i, t.isEmpty() ? "New Tab" : t.left(18));
    });

    animateTab(idx);
    return view;
}

void TabManager::animateTab(int index) {
    QWidget *tab = widget(index);
    if (!tab) return;
    QPropertyAnimation *a = new QPropertyAnimation(tab, "windowOpacity");
    a->setDuration(300);
    a->setStartValue(0.0);
    a->setEndValue(1.0);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}