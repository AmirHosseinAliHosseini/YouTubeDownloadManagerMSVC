#include "DownloadItemWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

DownloadItemWidget::DownloadItemWidget(const DownloadItem &item, QWidget *parent)
    : QWidget(parent), downloadItem(item)
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

    lblTitle = new QLabel(item.title);
    lblTitle->setWordWrap(true);
    lblTitle->setStyleSheet("font-weight: bold;");
    topBar->addWidget(lblTitle);

    btnRemove = new QPushButton("✕", this);
    btnRemove->setFixedSize(20, 20);
    btnRemove->setCursor(Qt::PointingHandCursor);
    btnRemove->setToolTip("Remove from queue");
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

    lblStatus = new QLabel("Waiting");
    right->addWidget(lblStatus);

    QHBoxLayout *download = new QHBoxLayout();
    right->addLayout(download);

    progress = new QProgressBar();
    progress->setRange(0, 0);
    progress->setValue(0);
    progress->setTextVisible(false);
    progress->setFixedHeight(6);
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

    btnStart = new QPushButton("Start");
    btnStart->setText(item.canResume ? "Resume" : "Start");
    download->addWidget(btnStart, 0, Qt::AlignRight);

    connect(btnStart, &QPushButton::clicked, this, [=]() {
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
}

void DownloadItemWidget::setFinished()
{
    lblStatus->setText("Completed");
    btnStart->setEnabled(false);
    btnStart->hide();
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
