#ifndef DOWNLOADITEMWIDGET_H
#define DOWNLOADITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
#include <QProgressBar>
#include "DownloadItem.h"

class DownloadItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DownloadItemWidget(const DownloadItem &item, QWidget *parent = nullptr);

    DownloadItem dItem;

    QLabel *lblThumb;
    QLabel *lblTitle;
    QLabel *lblStatus;
    QPushButton *btnStart;
    QPushButton *btnRemove;
    QPushButton *btnPlay;
    QPushButton *btnOpenFolder;
    QProgressBar *progress;
    QProcess *proc = nullptr;
    bool isDownloading = false;
    QString currentType;

    void parseYtDlpStatus(QString &line);
    void setFinished();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void startRequested(DownloadItemWidget *self);
    void removeRequested(DownloadItemWidget *item);
};

#endif // DOWNLOADITEMWIDGET_H
