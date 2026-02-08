#ifndef BROWSER_H
#define BROWSER_H

#include <QMainWindow>
#include <QLineEdit>
#include <QProgressBar>
#include <QStringList>
#include <QWebEngineDownloadRequest>
#include <QWebEngineCertificateError> 
#include "tabmanager.h"
#include <QHBoxLayout>
#include <QLabel>

class Browser : public QMainWindow {
    Q_OBJECT

public:
    explicit Browser(QWidget *parent = nullptr); 
    
    QLineEdit *addressBar;
    QLabel *sslLabel;
    QProgressBar *progressBar;
    void updateSslIcon(const QUrl &url);
    void updateUI(int progress);
public slots:
    void clearData();
    void changeTheme(const QString &theme);
    void setPrivacyLevel(const QString &level);
    void showHistory();
    void handleSslErrors(QWebEngineCertificateError error); 
    void handleDownload(QWebEngineDownloadRequest *download);
    void addHistoryEntry(const QUrl &url);

private:
    TabManager *tabs;
    QStringList historyList;
    QHBoxLayout *addressLayout;
    void setupUI();
    void setupProfile();
    void setupProxy();
    void applyTheme(const QString &mode);
    void onReturnPressed();
    void showContextMenu(const QPoint &pos);
};

#endif