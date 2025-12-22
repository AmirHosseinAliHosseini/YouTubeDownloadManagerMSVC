#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QStandardPaths>
#include <QComboBox>
#include <QJsonArray>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPixmap>
#include <QMessageBox>
#include <QListWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include "ProgressRing.h"
#include "DownloadItem.h"
#include "DownloadItemWidget.h"

QString sanitizeFileName(const QString &name) {
    QString clean = name;
    clean.replace(QRegularExpression(R"([<>:"/\\|?*])"), "_");
    return clean;
}

QString generateUniqueFileName(const QString &folder, const QString &baseName)
{
    QDir dir(folder);

    QString fileName = baseName + ".mp4";
    int counter = 1;

    while (dir.exists(fileName)) {
        fileName = QString("%1 (%2).mp4")
        .arg(baseName)
            .arg(counter);
        counter++;
    }

    if (counter > 1)
        return QString("%1 (%2)").arg(baseName).arg(counter - 1);

    return baseName;
}

QString queueFilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/queue.json";
}

void saveQueue(QListWidget *queue) {
    QJsonArray arr;

    for (int i = 0; i < queue->count(); ++i) {
        auto *w = qobject_cast<DownloadItemWidget*>(
            queue->itemWidget(queue->item(i))
            );
        if (!w) continue;

        arr.append(w->downloadItem.toJson());
    }

    QFile f(queueFilePath());
    QDir().mkpath(QFileInfo(f).absolutePath());

    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    }
}

void connectDownloadItem(DownloadItemWidget *widget, QListWidget *queue, QString saveFolder) {
    QObject::connect(widget, &DownloadItemWidget::removeRequested,
                     [=](DownloadItemWidget *wItem) {
                         auto reply = QMessageBox::question(wItem, "Remove item",
                                                            "Are you sure you want to remove this item?", QMessageBox::Yes | QMessageBox::No);
                         if (reply != QMessageBox::Yes) return;

                         auto *opacity = new QGraphicsOpacityEffect(wItem);
                         wItem->setGraphicsEffect(opacity);

                         auto *fade = new QPropertyAnimation(opacity, "opacity");
                         fade->setDuration(200);
                         fade->setStartValue(1.0);
                         fade->setEndValue(0.0);

                         QObject::connect(fade, &QPropertyAnimation::finished, [=]() {
                             if (wItem->proc && wItem->proc->state() == QProcess::Running)
                                 wItem->proc->kill();

                             for (int i = 0; i < queue->count(); ++i) {
                                 if (queue->itemWidget(queue->item(i)) == wItem) {
                                     delete queue->takeItem(i);
                                     break;
                                 }
                             }
                         });

                         fade->start(QAbstractAnimation::DeleteWhenStopped);
                     });

    QObject::connect(widget, &DownloadItemWidget::startRequested, [=](DownloadItemWidget *wItem) {
        if (!wItem->isDownloading) {
            wItem->lblStatus->setText("Starting download...");
            wItem->progress->setRange(0, 0);
            wItem->progress->show();
            wItem->currentType = "Video";
            wItem->proc = new QProcess(wItem);
            wItem->proc->setProcessChannelMode(QProcess::MergedChannels);

            QObject::connect(wItem->proc, &QProcess::readyReadStandardOutput, [wItem]() {
                if (!wItem->proc) return;

                QByteArray output = wItem->proc->readAllStandardOutput();
                QStringList lines = QString(output).split("\n", Qt::SkipEmptyParts);

                for (QString &line : lines) {
                    QJsonParseError err;
                    QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);

                    if (err.error == QJsonParseError::NoError && doc.isObject()) {
                        QJsonObject obj = doc.object();
                        QString status = obj.value("status").toString();
                        if (!status.isEmpty())
                            wItem->lblStatus->setText(status + " - " + obj.value("downloaded_bytes").toVariant().toString());

                    } else {
                        if (line.contains("download") && line.contains("ETA"))
                            wItem->parseYtDlpStatus(line);
                        else if (line.contains("Destination") && line.contains(".mp4"))
                            wItem->currentType = "Video";
                        else if (line.contains("Destination"))
                            wItem->currentType = "Audio";
                        else if (line.contains("Merger"))
                        {
                            wItem->progress->setRange(0, 0);
                            wItem->btnStart->setEnabled(false);
                            wItem->lblStatus->setText("Merging Video & Audio ...");
                        }
                    }
                }
            });

            QObject::connect(wItem->proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                             [wItem, queue](int exitCode, QProcess::ExitStatus status) {
                                 if (exitCode == 0) {
                                     wItem->progress->setRange(0, 100);
                                     wItem->progress->setValue(100);
                                     wItem->setFinished();
                                     wItem->btnStart->setEnabled(false);
                                 }
                                 else
                                     wItem->lblStatus->setText("Stopped");

                                 wItem->proc->deleteLater();
                                 wItem->proc = nullptr;
                                 saveQueue(queue);
                             });

            QString outputName = QString("%1 [%2]").arg(sanitizeFileName(wItem->downloadItem.title)).arg(wItem->downloadItem.resolution);
            QString outputPath = QString("%1/%2").arg(saveFolder).arg(generateUniqueFileName(wItem->downloadItem.saveFolder, outputName));
            wItem->downloadItem.finalFilePath = outputPath + ".mp4";

            QString program = QCoreApplication::applicationDirPath() + "/yt-dlp.exe";
            QStringList args;
            args << "--ffmpeg-location" << QCoreApplication::applicationDirPath()
                << "--newline"
                << "-c"
                << "-f"  << QString("%1+%2/%1").arg(wItem->downloadItem.videoFormat, wItem->downloadItem.audioFormat)
                << "--merge-output-format" << "mp4"
                << "-o" << outputPath
                << wItem->downloadItem.url;

            wItem->proc->start(program, args);
            wItem->isDownloading = true;
            wItem->btnStart->setText("■");
            wItem->btnStart->setToolTip("Stop download");
        }
        else
        {
            if (wItem->proc && wItem->proc->state() == QProcess::Running)
                wItem->proc->kill();

            wItem->progress->hide();
            wItem->isDownloading = false;
            wItem->btnStart->setText("▶");
            wItem->btnStart->setToolTip("Resume download");
        }
    });
}

void loadQueue(QListWidget *queue) {
    QFile f(queueFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return;

    for (auto v : doc.array()) {
        DownloadItem item = DownloadItem::fromJson(v.toObject());

        auto *widget = new DownloadItemWidget(item);
        connectDownloadItem(widget, queue, item.saveFolder);

        QListWidgetItem *li = new QListWidgetItem(queue);
        li->setSizeHint(QSize(500, 90));
        queue->setItemWidget(li, widget);

        if (!item.thumbPath.isEmpty() && QFile::exists(item.thumbPath)) {
            QPixmap pix(item.thumbPath);
            widget->lblThumb->setPixmap(
                pix.scaled(widget->lblThumb->size(),
                           Qt::KeepAspectRatio,
                           Qt::SmoothTransformation)
                );
        } else {
            //widget->lblThumb->setPixmap(QPixmap(":/icons/no_thumb.png"));
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/appicon.png"));
    QWidget w;
    w.setWindowTitle("YouTube Download Manager V1.5");
    w.setFixedSize(1050, 600);

    //--------------------Parameters--------------------

    QString saveFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString realExt, realTitle;
    QPixmap currentThumb;
    QString currentThumbPath;
    QString outputPath;
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(&w);

    //--------------------Main Ui--------------------

    QHBoxLayout *mainLayout = new QHBoxLayout(&w);

    QWidget *leftWidget = new QWidget(&w);
    mainLayout->addWidget(leftWidget);

    QListWidget *queue = new QListWidget();
    loadQueue(queue);
    mainLayout->addWidget(queue, 1);

    leftWidget->setFixedSize(475,600);
    QVBoxLayout *left = new QVBoxLayout(leftWidget);

    //--------------------Left Menu--------------------

    QLineEdit *urlEdit = new QLineEdit();
    left->addWidget(urlEdit);

    QPushButton *btnCheck = new QPushButton("Check Video Info");
    left->addWidget(btnCheck);

    QLabel *lblTitle = new QLabel("Title will appear here");
    left->addWidget(lblTitle);

    //--------------------Story Board--------------------

    QLabel *lblStoryboard = new QLabel("Storyboard will appear here");
    lblStoryboard->setAlignment(Qt::AlignCenter);
    lblStoryboard->setFixedSize(448, 252);
    left->addWidget(lblStoryboard);

    ProgressRing *ring = new ProgressRing(lblStoryboard);
    ring->move((lblStoryboard->width() - ring->width()) / 2,
               (lblStoryboard->height() - ring->height()) / 2);
    ring->hide();

    //--------------------Video Quality--------------------

    QHBoxLayout *layVideo = new QHBoxLayout();

    QWidget *videoWidget = new QWidget();
    videoWidget->setLayout(layVideo);
    left->addWidget(videoWidget);

    QLabel *lblVideo = new QLabel("Video Quality:");
    lblVideo->setMaximumWidth(100);
    layVideo->addWidget(lblVideo);

    QComboBox *videoCombo = new QComboBox();
    videoCombo->setEnabled(false);
    layVideo->addWidget(videoCombo);

    //--------------------Audio Quality--------------------

    QHBoxLayout *layAudio = new QHBoxLayout();

    QWidget *audioWidget = new QWidget();
    audioWidget->setLayout(layAudio);
    left->addWidget(audioWidget);

    QLabel *lblAudio = new QLabel("Audio Quality:");
    lblAudio->setMaximumWidth(100);
    layAudio->addWidget(lblAudio);

    QComboBox *audioCombo = new QComboBox();
    audioCombo->setEnabled(false);
    layAudio->addWidget(audioCombo);

    //--------------------Save Folder--------------------

    QHBoxLayout *laypath = new QHBoxLayout();

    QWidget *pathWidget = new QWidget();
    pathWidget->setLayout(laypath);
    left->addWidget(pathWidget);

    QLabel *lblPath = new QLabel("Path: " + saveFolder);
    lblPath->setMaximumWidth(400);
    lblPath->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    lblPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lblPath->setWordWrap(false);
    laypath->addWidget(lblPath);

    QPushButton *btnFolder = new QPushButton("Choose folder");
    btnFolder->setMaximumWidth(100);
    btnFolder->setEnabled(false);
    laypath->addWidget(btnFolder);

    //--------------------Left Menu--------------------

    QPushButton *btnAdd = new QPushButton("Add to List");
    btnAdd->setEnabled(false);
    left->addWidget(btnAdd);

    QLabel *lblStatus = new QLabel("Status...");
    left->addWidget(lblStatus);

    //--------------------Slot Method--------------------

    auto updatePathLabel = [&](const QString &fullPath) {
        int avail = lblPath->width();
        if (avail <= 0) avail = lblPath->maximumWidth();

        QFontMetrics fm(lblPath->font());
        QString elided = fm.elidedText(fullPath, Qt::TextElideMode::ElideMiddle, avail);
        lblPath->setText(elided);
        lblPath->setToolTip(fullPath);
    };

    QObject::connect(btnFolder, &QPushButton::clicked, [&]() {
        saveFolder = QFileDialog::getExistingDirectory(&w, "Select folder");
        updatePathLabel(saveFolder);
    });

    QObject::connect(btnCheck, &QPushButton::clicked, [&]() {
        if(urlEdit->text().isEmpty()) {
            lblStatus->setText("Status: Enter URL first!");
            QMessageBox::warning(&w, "Warning", "Enter URL first!");
            return;
        }

        urlEdit->setEnabled(false);
        btnCheck->setEnabled(false);
        ring->startIndeterminate();
        lblStoryboard->clear();
        lblStatus->setText("Status: Fetching video info...");

        QProcess *proc = new QProcess(&w);
        proc->setProcessChannelMode(QProcess::MergedChannels);

        QObject::connect(proc, &QProcess::readyReadStandardOutput, [=, &realTitle, &realExt, &currentThumb, &currentThumbPath]() {
            QByteArray output = proc->readAllStandardOutput();
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(output, &err);

            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject obj = doc.object();
                realTitle = obj.value("title").toString();
                realExt = obj.value("ext").toString();
                lblTitle->setText("Title: " + realTitle);

                //--------------------Add Video & Audio Quality--------------------
                QSet<QString> videoQualities, audioQualities;
                videoCombo->clear();
                audioCombo->clear();

                QJsonArray formats = obj.value("formats").toArray();
                for (auto f : formats) {
                    QJsonObject fmt = f.toObject();
                    qint64 filesize = fmt.value("filesize").toVariant().toLongLong();

                    QString sizeText;
                    if (filesize > (1024*1024*1024))
                        sizeText = QString(" ~ %1 GB").arg(filesize / (1024*1024*1024));
                    else if (filesize > (1024*1024))
                        sizeText = QString(" ~ %1 MB").arg(filesize / (1024*1024));
                    else if (filesize > 1024)
                        sizeText = QString(" ~ %1 KB").arg(filesize / 1024);
                    else if (filesize > 0)
                        sizeText = QString(" ~ %1 B").arg(filesize);

                    QString acodec = fmt.value("acodec").toString();
                    QString vcodec = fmt.value("vcodec").toString();
                    QString format_id = fmt.value("format_id").toString();
                    if (!sizeText.isEmpty()) {
                        if (vcodec != "none") {
                            QString resolution = fmt.value("resolution").toString();
                            if (!videoQualities.contains(resolution)) {
                                videoQualities.insert(resolution);

                                videoCombo->addItem(resolution + sizeText, format_id);
                            }
                        }
                        else if (vcodec == "none" && acodec != "none")
                        {
                            double abr = fmt.value("abr").toDouble(0.0);

                            if (abr > 0.1) {
                                QString key = QString("%1-%2").arg(acodec).arg(abr);
                                if (audioQualities.contains(key)) continue;
                                audioQualities.insert(key);

                                QString display = QString("%1 kbps (%2)%3")
                                                      .arg(abr)
                                                      .arg(acodec)
                                                      .arg(sizeText);

                                audioCombo->addItem(display, format_id);
                            }
                        }
                    }
                }

                if (audioCombo->count() == 0)
                    audioCombo->addItem("Best audio", "ba");

                //--------------------Thumbnails Load--------------------

                QJsonArray thumbnails = obj.value("thumbnails").toArray();
                QString bestUrlThumbnails;
                int maxWidth = 0;

                for (auto t : thumbnails) {
                    QJsonObject thumb = t.toObject();
                    QString url = thumb.value("url").toString();
                    int width = thumb.value("width").toInt(0);
                    QString fileName = QFileInfo(QUrl(url).fileName()).completeBaseName();

                    if (fileName.isEmpty()) continue;

                    if (width > maxWidth) {
                        maxWidth = width;
                        bestUrlThumbnails = url;
                    }
                }

                if (!bestUrlThumbnails.isEmpty()) {
                    QNetworkRequest request((QUrl(bestUrlThumbnails)));
                    QString fileName = QFileInfo(QUrl(bestUrlThumbnails).fileName()).completeBaseName();
                    QNetworkReply *reply = networkManager->get(request);

                    QObject::connect(reply, &QNetworkReply::finished, [reply, lblStoryboard, realTitle,
                                                                       &currentThumb, &currentThumbPath]() {
                        QByteArray data = reply->readAll();
                        QPixmap pix;
                        if (pix.loadFromData(data)) {
                            currentThumb = pix;
                            lblStoryboard->setPixmap(pix.scaled(lblStoryboard->size(),
                                                                Qt::KeepAspectRatio, Qt::SmoothTransformation));

                            QString thumbDir = QStandardPaths::writableLocation(
                                                   QStandardPaths::AppDataLocation
                                                   ) + "/thumbs";

                            QDir().mkpath(thumbDir);

                            QString thumbFile = thumbDir + "/" +
                                                sanitizeFileName(realTitle) + ".jpg";

                            pix.save(thumbFile, "JPG");
                            currentThumbPath = thumbFile;
                        }
                        reply->deleteLater();
                    });
                }

                videoCombo->setEnabled(true);
                audioCombo->setEnabled(true);
                btnFolder->setEnabled(true);
                btnAdd->setEnabled(true);

                if (videoCombo->count() > 0) {
                    int index = videoCombo->currentIndex();
                    emit videoCombo->currentIndexChanged(index);
                }
            }
        });

        QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         [=](int exitCode, QProcess::ExitStatus status) {
                             Q_UNUSED(exitCode);
                             Q_UNUSED(status);
                             ring->stop();

                             if (videoCombo->count() == 0)
                                 lblStatus->setText("Error: Not Found Url");
                             else
                                 lblStatus->setText("Status: Info fetch finished.");
                             proc->deleteLater();

                             urlEdit->setEnabled(true);
                             btnCheck->setEnabled(true);
                         });

        QString program = QCoreApplication::applicationDirPath() + "/yt-dlp.exe";
        QStringList args;
        args << "--dump-json"
             << urlEdit->text();

        proc->start(program, args);
    });

    QObject::connect(btnAdd, &QPushButton::clicked, [&]() {
        if (urlEdit->text().isEmpty()) return;

        QString resolution = videoCombo->currentText().split(" ").first();
        QString baseName = QString("%1 [%2]").arg(sanitizeFileName(realTitle)).arg(resolution);
        baseName = generateUniqueFileName(saveFolder, baseName);

        DownloadItem item;
        item.url = urlEdit->text();
        item.title = realTitle;
        item.resolution = resolution;
        item.videoFormat = videoCombo->currentData().toString();
        item.audioFormat = audioCombo->currentData().toString();
        item.baseName = baseName;
        item.saveFolder = saveFolder;
        item.thumbPath = currentThumbPath;
        item.hasPartialFile();

        resolution = videoCombo->currentText();
        if (resolution.isEmpty())
            resolution = "unknown";
        else
            resolution = resolution.split(" ").first();

        auto *widget = new DownloadItemWidget(item);
        connectDownloadItem(widget, queue, saveFolder);
        widget->lblThumb->setPixmap(currentThumb.scaled(widget->lblThumb->size(),
                                                        Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QListWidgetItem *li = new QListWidgetItem(queue);
        li->setSizeHint(QSize(500, 90));
        queue->setItemWidget(li, widget);
        urlEdit->setText("");
        saveQueue(queue);
    });

    QObject::connect(urlEdit, &QLineEdit::textChanged, [&]() {
        btnCheck->setEnabled(true);
        lblTitle->setText("Title will appear here");
        lblStoryboard->clear();
        videoCombo->clear();
        videoCombo->setEnabled(false);
        audioCombo->clear();
        audioCombo->setEnabled(false);
        btnFolder->setEnabled(false);
        btnAdd->setEnabled(false);
        lblStatus->setText("Status: Enter URL and check info...");
    });

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        saveQueue(queue);
    });

    w.show();
    return app.exec();
}
