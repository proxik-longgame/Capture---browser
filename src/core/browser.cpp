#include "browser.h"
#include <QToolBar>
#include <QPushButton>
#include <QApplication>
#include <QDir>
#include <QWebEngineSettings>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>
#include <QNetworkProxy>
#include <QMenu>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QFileInfo>
#include <QProgressBar>
#include <QDesktopServices>
#include <QWebEngineCertificateError>

class VideoWindow : public QMainWindow {
public:
    VideoWindow(const QUrl &url, QWidget *parent = nullptr) : QMainWindow(parent) {
        auto *view = new QWebEngineView(this);
        setCentralWidget(view);
        view->load(url);
        setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
        resize(640, 360);
    }
};

Browser::Browser(QWidget *parent) : QMainWindow(parent) {
    setupProfile();
    setupProxy();
    setupUI();

    connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested,
            this, &Browser::handleDownload);
            
    applyTheme("Dark");
    tabs->createNewTab(QUrl("https://www.google.com"));
}
void Browser::handleSslErrors(QWebEngineCertificateError error) {
    QMessageBox::StandardButton res = QMessageBox::warning(this, "SSL Error",
        QString("The certificate for %1 is invalid: %2. Proceed?")
        .arg(error.url().host())
        .arg(error.description()), 
        QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes) {
        error.acceptCertificate();
    } else {
        error.rejectCertificate();
    }
}

class DownloadNotification : public QWidget {
    Q_OBJECT
    QWebEngineDownloadRequest *m_download;
    QProgressBar *m_bar;
public:
    DownloadNotification(QWebEngineDownloadRequest *download, QWidget *parent = nullptr) 
        : QWidget(parent), m_download(download) {
        setFixedSize(320, 110);
        setStyleSheet("background: #1a1a1a; color: white; border: 1px solid #333; border-radius: 10px;");
        
        auto *layout = new QVBoxLayout(this);
        auto *label = new QLabel("Downloading: " + download->suggestedFileName());
        label->setWordWrap(true);
        
        m_bar = new QProgressBar;
        m_bar->setStyleSheet("QProgressBar { height: 10px; border-radius: 5px; background: #333; } "
                           "QProgressBar::chunk { background: #0078d4; }");

        auto *btnLayout = new QHBoxLayout();
        auto *openBtn = new QPushButton("Open");
        auto *cancelBtn = new QPushButton("Cancel");
        openBtn->setEnabled(false);
        
        btnLayout->addWidget(openBtn);
        btnLayout->addWidget(cancelBtn);

        layout->addWidget(label);
        layout->addWidget(m_bar);
        layout->addLayout(btnLayout);

        connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, this, [this, download]() {
            int prog = (download->receivedBytes() * 100) / download->totalBytes();
            m_bar->setValue(prog);
        });

        connect(download, &QWebEngineDownloadRequest::stateChanged, this, [this, openBtn, download](int state) {
            if (state == 2) { 
                openBtn->setEnabled(true);
                m_bar->setValue(100);
            }
        });

        connect(openBtn, &QPushButton::clicked, [this, download]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(download->downloadDirectory()));
            this->close();
        });

        connect(cancelBtn, &QPushButton::clicked, [this, download]() {
            download->cancel();
            this->close();
        });

        move(parent->width() - 340, 40);
        show();
    }
};

void Browser::handleDownload(QWebEngineDownloadRequest *download) {
    QString path = QFileDialog::getSaveFileName(this, "Save File", 
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + download->suggestedFileName());

    if (path.isEmpty()) { download->cancel(); return; }

    download->setDownloadDirectory(QFileInfo(path).path());
    download->setDownloadFileName(QFileInfo(path).fileName());
    download->accept();

    new DownloadNotification(download, this);
}

void Browser::setupProfile() {
    QWebEngineProfile *p = QWebEngineProfile::defaultProfile();

    QString ua = p->httpUserAgent();
    ua.replace("QtWebEngine/6.8.0 ", ""); 
    p->setHttpUserAgent(ua);

    QWebEngineSettings *s = p->settings();
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    s->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

    p->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
}
void Browser::addHistoryEntry(const QUrl &url) {
    QString urlStr = url.toString();
    if (urlStr.startsWith("capture://") || urlStr.isEmpty()) return;
    
    historyList.removeAll(urlStr);
    historyList.prepend(urlStr);
    if (historyList.size() > 1000) historyList.removeLast();
}

void Browser::showHistory() {
    QDialog dlg(this);
    dlg.setWindowTitle("History");
    dlg.resize(500, 400);
    dlg.setStyleSheet("background: #1a1a1a; color: white;");

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QListWidget *list = new QListWidget;
    list->addItems(historyList);
    list->setStyleSheet("QListWidget { border: none; } QListWidget::item { padding: 8px; border-bottom: 1px solid #333; }");

    connect(list, &QListWidget::itemDoubleClicked, [this, &dlg](QListWidgetItem *item) {
        if (auto *v = qobject_cast<QWebEngineView*>(tabs->currentWidget())) {
            v->load(QUrl(item->text()));
            dlg.close();
        }
    });

    layout->addWidget(new QLabel("Double-click to open:"));
    layout->addWidget(list);
    dlg.exec();
}

void Browser::clearData() {
    QWebEngineProfile::defaultProfile()->clearHttpCache();
    if (auto *store = QWebEngineProfile::defaultProfile()->cookieStore()) {
        store->deleteAllCookies();
    }
    historyList.clear();
    QMessageBox::information(this, "Privacy", "All browsing data cleared.");
}
void Browser::updateSslIcon(const QUrl &url) {
    if (url.scheme() == "https") {
        sslLabel->setPixmap(QIcon::fromTheme("security-high").pixmap(16, 16));
        sslLabel->setToolTip("Connection is Secure");
    } else {
        sslLabel->setPixmap(QIcon::fromTheme("security-low").pixmap(16, 16));
        sslLabel->setToolTip("Connection is NOT Secure");
    }
}
void Browser::setupUI() {
    tabs = new TabManager(this);
    setCentralWidget(tabs);
    tabs->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabs, &QWidget::customContextMenuRequested, this, &Browser::showContextMenu);

    QToolBar *nav = addToolBar("Navigation");
    nav->setMovable(false);

    QPushButton *backBtn = new QPushButton("◁");
    QPushButton *fwdBtn = new QPushButton("▷");
    QPushButton *reloadBtn = new QPushButton("↻");

    QWidget *addressContainer = new QWidget(this);
    addressLayout = new QHBoxLayout(addressContainer);
    addressLayout->setContentsMargins(5, 0, 5, 0);
    addressLayout->setSpacing(5);

    sslLabel = new QLabel(this);
    sslLabel->setPixmap(QIcon::fromTheme("security-high").pixmap(16, 16));
    
    addressBar = new QLineEdit(this);
    
    addressLayout->addWidget(sslLabel);
    addressLayout->addWidget(addressBar);

    progressBar = new QProgressBar(this);
    progressBar->setMaximumWidth(80);
    progressBar->setFixedHeight(4);
    progressBar->setTextVisible(false);
    progressBar->hide();

    QPushButton *histBtn = new QPushButton("H");
    QPushButton *settings = new QPushButton("⚙");

    nav->addWidget(backBtn);
    nav->addWidget(fwdBtn);
    nav->addWidget(reloadBtn);
    nav->addWidget(addressContainer); 
    nav->addWidget(progressBar);
    nav->addWidget(histBtn);
    nav->addWidget(settings);

    connect(backBtn, &QPushButton::clicked, [this](){ if(auto *v = qobject_cast<QWebEngineView*>(tabs->currentWidget())) v->back(); });
    connect(fwdBtn, &QPushButton::clicked, [this](){ if(auto *v = qobject_cast<QWebEngineView*>(tabs->currentWidget())) v->forward(); });
    connect(reloadBtn, &QPushButton::clicked, [this](){ if(auto *v = qobject_cast<QWebEngineView*>(tabs->currentWidget())) v->reload(); });
    connect(addressBar, &QLineEdit::returnPressed, this, &Browser::onReturnPressed);
    connect(histBtn, &QPushButton::clicked, this, &Browser::showHistory);
    connect(settings, &QPushButton::clicked, [this]() { tabs->createNewTab(QUrl("capture://settings")); });
}

void Browser::showContextMenu(const QPoint &pos) {
    if (auto *view = qobject_cast<QWebEngineView*>(tabs->currentWidget())) {
        QMenu menu(this);
        menu.addAction(view->pageAction(QWebEnginePage::Back));
        menu.addAction(view->pageAction(QWebEnginePage::Forward));
        menu.addAction(view->pageAction(QWebEnginePage::Reload));
        menu.addSeparator();
        menu.addAction("Pop-out Video", [this, view]() {
            auto *pip = new VideoWindow(view->url());
            pip->show();
        });
        menu.addAction("Inspect Element", [this, pos]() {
            if(auto *v = qobject_cast<QWebEngineView*>(tabs->currentWidget())) {
                v->page()->setInspectedPage(v->page());
                QDesktopServices::openUrl(QUrl("http://localhost:9222"));
            }
        });
        menu.exec(tabs->mapToGlobal(pos));
    }
}

void Browser::onReturnPressed() {
    QString val = addressBar->text();
    if (auto *v = qobject_cast<QWebEngineView*>(tabs->currentWidget())) {
        if (!val.contains(".") && !val.contains("://")) {
            v->load(QUrl("https://www.google.com/search?q=" + val));
        } else {
            v->load(QUrl::fromUserInput(val));
        }
    }
}

void Browser::applyTheme(const QString &mode) {
    if (mode == "White") {
        qApp->setStyleSheet(
            "QMainWindow { background: #ffffff; }"
            "QToolBar { background: #f8f9fa; border-bottom: 1px solid #ddd; padding: 8px; spacing: 10px; }"
            "QLineEdit { background: #f1f3f4; color: #202124; border-radius: 20px; padding: 6px 15px; border: 1px solid #dfe1e5; }"
            "QPushButton { background: transparent; color: #5f6368; border-radius: 10px; padding: 5px 12px; }"
            "QPushButton:hover { background: #e8eaed; color: #1a73e8; }"
            "QTabWidget::pane { border: none; background: #ffffff; }"
            "QTabBar::tab { background: #f1f3f4; color: #5f6368; padding: 10px 20px; border-top-left-radius: 8px; border-top-right-radius: 8px; }"
            "QTabBar::tab:selected { background: #ffffff; color: #1a73e8; border-bottom: 2px solid #1a73e8; }"
        );
    } else if (mode == "Private") {
        qApp->setStyleSheet(
            "QMainWindow { background: #0d0216; }"
            "QToolBar { background: #1a0b2e; border-bottom: 2px solid #bc13fe; padding: 8px; }"
            "QLineEdit { background: #0d0216; color: #bc13fe; border: 1px solid #4b0082; border-radius: 20px; padding: 6px; }"
            "QPushButton { color: #bc13fe; background: #1a0b2e; border: 1px solid #4b0082; border-radius: 10px; }"
            "QPushButton:hover { background: #bc13fe; color: #000; }"
            "QTabBar::tab:selected { background: #0d0216; color: #bc13fe; border-bottom: 2px solid #bc13fe; }"
        );
    } else {
        qApp->setStyleSheet(
            "QMainWindow { background: #0f0f0f; }"
            "QToolBar { background: #1a1a1a; border-bottom: 1px solid #2d2d2d; padding: 8px; spacing: 10px; }"
            "QLineEdit { background: #2b2b2b; color: #e8eaed; border-radius: 20px; padding: 6px 15px; border: 1px solid #3c4043; }"
            "QPushButton { background: #2b2b2b; color: #bdc1c6; border: 1px solid #3c4043; border-radius: 10px; padding: 5px 12px; }"
            "QPushButton:hover { background: #3c4043; color: #ffffff; border-color: #8ab4f8; }"
            "QTabWidget::pane { border: none; background: #0f0f0f; }"
            "QTabBar::tab { background: #1a1a1a; color: #9aa0a6; padding: 10px 20px; }"
            "QTabBar::tab:selected { background: #2b2b2b; color: #8ab4f8; border-bottom: 2px solid #8ab4f8; }"
        );
    }
}


void Browser::updateUI(int progress) {
    progressBar->setValue(progress);
    progressBar->setVisible(progress < 100);
}

void Browser::changeTheme(const QString &theme) { applyTheme(theme); }
void Browser::setupProxy() { QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy); }
void Browser::setPrivacyLevel(const QString &level) {
    QWebEngineSettings *s = QWebEngineProfile::defaultProfile()->settings();
    QWebEngineProfile *p = QWebEngineProfile::defaultProfile();

    if (level == "Hardest") {
        s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
        s->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
        p->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
        p->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    } else {
        s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
        p->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
        p->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    }
}
#include "browser.moc"