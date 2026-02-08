#ifndef TABMANAGER_H
#define TABMANAGER_H

#include <QTabWidget>
#include <QWebEngineView>

class Browser; 

class TabManager : public QTabWidget {
    Q_OBJECT
public:
    TabManager(QWidget *parent = nullptr);
    QWebEngineView* createNewTab(const QUrl &url);
public slots:
    void handleNewTabButtonClick();
private:
    void animateTab(int index);
};

#endif