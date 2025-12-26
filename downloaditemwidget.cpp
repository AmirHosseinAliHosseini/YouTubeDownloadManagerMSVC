#include "DownloadItemWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QMessageBox>

DownloadItemWidget::DownloadItemWidget(const DownloadItem &item, QWidget *parent)
    : QWidget(parent), dItem(item)
{
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);

    QHBoxLayout *main = new QHBoxLayout(this);

    lblThumb = new QLabel();
    lblThumb->setFixedSize(120, 70);
    lblThumb->setScaledContents(true);
    main->addWidget(lblThumb);

    QVBoxLayout *right = new QVBoxLayout();
    main->addLayout(right);

    QHBoxLayout *topBar = new QHBoxLayout;
    right->addLayout(topBar);

    lblTitle = new QLabel("");
    lblTitle->setStyleSheet("font-weight: bold;");
    QFontMetrics fm(lblTitle->font());
    QString elided = fm.elidedText(dItem.Title, Qt::ElideRight, lblTitle->width());
    lblTitle->setText(elided);
    lblTitle->setToolTip(dItem.Title);
    topBar->addWidget(lblTitle);

    btnRemove = new QPushButton("✕", this);
    btnRemove->setFixedSize(20, 20);
    btnRemove->setCursor(Qt::PointingHandCursor);
    btnRemove->setToolTip("Remove");
    btnRemove->hide();
    btnRemove->setStyleSheet(R"(
                                QPushButton {
                                    border: none;
                                    background: transparent;
                                    color: #999;
                                    font-size: 14px;
                                }
                                QPushButton:hover {
                                    color: red;
                                }
                                )");
    topBar->addWidget(btnRemove);

    lblStatus = new QLabel(dItem.Status);
    dItem.Status = lblStatus->text();
    right->addWidget(lblStatus);

    QHBoxLayout *download = new QHBoxLayout();
    right->addLayout(download);

    progress = new QProgressBar();
    progress->setRange(0, 0);
    progress->setValue(0);
    progress->setTextVisible(false);
    progress->setFixedHeight(6);
    progress->setFixedWidth(300);
    progress->hide();
    progress->setStyleSheet(R"(
                                QProgressBar {
                                    background: #2c2c2c;
                                    border-radius: 3px;
                                }
                                QProgressBar::chunk {
                                    background: #3daee9;
                                    border-radius: 3px;
                                }
                                )");
    download->addWidget(progress);
    download->addStretch();

    btnStart = new QPushButton(dItem.DownloadState == 1 ? "Resume" : "Start");
    dItem.hasPartialFile();
    btnStart->setToolTip(dItem.DownloadState == 1 ? "Resume download" : "Start download");
    download->addWidget(btnStart, 0, Qt::AlignRight);

    btnPlay = new QPushButton("▶");
    btnPlay->setToolTip("Play video");
    btnPlay->setVisible(false);
    download->addWidget(btnPlay);

    btnOpenFolder = new QPushButton("📂");
    btnOpenFolder->setToolTip("Show in folder");
    btnOpenFolder->setVisible(false);
    download->addWidget(btnOpenFolder);

    if (dItem.DownloadState == 2)
        setFinished();

    connect(btnPlay, &QPushButton::clicked, this, [this]() {
        if (dItem.FullPath.isEmpty()) return;

        QDesktopServices::openUrl(QUrl::fromLocalFile(dItem.FullPath));
    });

    connect(btnOpenFolder, &QPushButton::clicked, this, [this]() {
        QString path = dItem.FullPath;
        if (path.isEmpty() || !QFile::exists(path)) {
            QMessageBox::warning(this, "File not found", "The video file could not be found.");
            return;
        }

#ifdef Q_OS_WIN
        QString nativePath = QDir::toNativeSeparators(path);
        QProcess::startDetached("explorer.exe", QStringList{"/select,", nativePath});
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
    });

    connect(btnStart, &QPushButton::clicked, this, [this]() {
        emit startRequested(this);
    });

    connect(btnRemove, &QPushButton::clicked, this, [this]() {
        emit removeRequested(this);
    });
}

void DownloadItemWidget::parseYtDlpStatus(QString &line)
{
    double percent;
    if (!line.startsWith("[download]"))
        return;

    QRegularExpression re(
        R"(\[download\]\s+([\d\.]+)%\s+of\s+~?\s*([\d\.]+\w+)\s+at\s+([\d\.]+\w+/s)\s+ETA\s+(\S+)(?:\s+\(frag\s+(\d+/\d+)\))?)"
        );
    auto match = re.match(line);

    if (!match.hasMatch())
        return;

    percent = match.captured(1).toDouble() / 2;

    if (currentType == "Audio")
        percent += 50;

    if (progress->maximum() == 0)
        progress->setRange(0, 100);

    progress->setValue(static_cast<int>(percent));
    lblStatus->setText(QString("Downloaded(%4): %1 % \tSpeed: %2 \tETA: %3")
                           .arg(match.captured(1))
                           .arg(match.captured(3).replace("MiB/s"," MB/sec").replace("KiB/s"," KB/sec"))
                           .arg(match.captured(4))
                           .arg(currentType));

    dItem.Status = lblStatus->text();
}

void DownloadItemWidget::setFinished()
{
    lblStatus->setText("Completed");
    dItem.Status = lblStatus->text();
    btnStart->hide();
    btnStart->hide();
    btnPlay->show();
    btnOpenFolder->show();
    progress->hide();
    dItem.DownloadState = 2;
}

void DownloadItemWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    btnRemove->show();
}

void DownloadItemWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    btnRemove->hide();
}
