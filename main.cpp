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
#include "ProgressRing.h"

static QString typeFile;
static double percent;

QString parseYtDlpStatus(QString &line)
{
    if (!line.startsWith("[download]"))
        return line;

    QRegularExpression re(
        R"(\[download\]\s+([\d\.]+)%\s+of\s+~?\s*([\d\.]+\w+)\s+at\s+([\d\.]+\w+/s)\s+ETA\s+(\S+)(?:\s+\(frag\s+(\d+/\d+)\))?)"
        );
    QRegularExpressionMatch match = re.match(line);

    if (!match.hasMatch())
        return line;

    percent = match.captured(1).toDouble() / 2;

    if (typeFile == "Audio")
        percent += 50;

    return QString("Downloaded(%4): %1 % \tTransfer rate: %2 \tTime left: %3")
                         .arg(match.captured(1))
                         .arg(match.captured(3).replace("MiB/s"," MB/sec").replace("KiB/s"," KB/sec"))
                         .arg(match.captured(4))
                         .arg(typeFile);
}

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

bool hasPartialFile(const QString &folder, const QString &baseName, const QString &VideoformatId, const QString &AudioformatId)
{
    QDir dir(folder);
    QStringList files = dir.entryList(QDir::Files);

    QRegularExpression re(
        "^" +
        QRegularExpression::escape(baseName) +
        R"(\.f)" +  QRegularExpression::escape(VideoformatId) +
        R"(\.\w+\.part$)"
        );

    for (const QString &file : files) {
        if (re.match(file).hasMatch())
            return true;
    }

    re = QRegularExpression(
        "^" +
        QRegularExpression::escape(baseName) +
        R"(\.f)" +  QRegularExpression::escape(VideoformatId) +
        R"(\.\w+$)"
        );

    bool video = false;
    for (const QString &file : files) {
        if (re.match(file).hasMatch())
            video = true;
    }

    if (!video)
        return false;

    QRegularExpression reA(
        "^" +
        QRegularExpression::escape(baseName) +
        R"(\.f)" +  QRegularExpression::escape(AudioformatId) +
        R"(\.\w+\.part$)"
        );

    for (const QString &file : files) {
        if (reA.match(file).hasMatch())
            return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/appicon.png"));

    // ---------------- UI ----------------
    QWidget w;
    w.setWindowTitle("YouTube Download Manager V1.3");
    w.setMinimumSize(475, 600);
    w.setMaximumSize(475, 600);
    QVBoxLayout *lay = new QVBoxLayout(&w);
    QHBoxLayout *laypath = new QHBoxLayout();
    QHBoxLayout *layVideo = new QHBoxLayout();
    QHBoxLayout *layAudio = new QHBoxLayout();
    QWidget *pathWidget = new QWidget();
    pathWidget->setLayout(laypath);
    QWidget *videoWidget = new QWidget();
    videoWidget->setLayout(layVideo);
    QWidget *audioWidget = new QWidget();
    audioWidget->setLayout(layAudio);

    QString saveFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    QLineEdit *urlEdit = new QLineEdit();

    QPushButton *btnCheck = new QPushButton("Check Video Info");
    QLabel *lblTitle = new QLabel("Title will appear here");
    QString realExt;
    QString realTitle;
    QLabel *lblVideo = new QLabel("Video Quality:");
    QComboBox *videoCombo = new QComboBox();
    QLabel *lblAudio = new QLabel("Audio Quality:");
    QComboBox *audioCombo = new QComboBox();
    QPushButton *btnFolder = new QPushButton("Choose folder");
    QLabel *lblPath = new QLabel("Path: " + saveFolder);
    QPushButton *btnStart = new QPushButton("Start");
    QLabel *lblStatus = new QLabel("Status...");
    QLabel *lblStoryboard = new QLabel("Storyboard will appear here");

    btnFolder->setMaximumWidth(100);

    lblStoryboard->setAlignment(Qt::AlignCenter);
    lblStoryboard->setFixedSize(448, 252);

    ProgressRing *ring = new ProgressRing(lblStoryboard);

    ring->move(
        (lblStoryboard->width() - ring->width()) / 2,
        (lblStoryboard->height() - ring->height()) / 2
        );

    ring->hide();

    lblPath->setMaximumWidth(400);
    lblPath->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    lblPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lblPath->setWordWrap(false);

    lblVideo->setMaximumWidth(100);
    lblAudio->setMaximumWidth(100);

    btnStart->setEnabled(false);
    btnFolder->setEnabled(false);
    videoCombo->setEnabled(false);
    audioCombo->setEnabled(false);

    lay->addWidget(urlEdit);
    lay->addWidget(btnCheck);
    lay->addWidget(lblTitle);
    lay->addWidget(lblStoryboard);

    lay->addWidget(videoWidget);
    layVideo->addWidget(lblVideo);
    layVideo->addWidget(videoCombo);
    lay->addWidget(audioWidget);
    layAudio->addWidget(lblAudio);
    layAudio->addWidget(audioCombo);


    lay->addWidget(pathWidget);
    laypath->addWidget(lblPath);
    laypath->addWidget(btnFolder);
    lay->addWidget(btnStart);
    lay->addWidget(lblStatus);


    auto updatePathLabel = [&](const QString &fullPath){
        int avail = lblPath->width();
        if (avail <= 0) avail = lblPath->maximumWidth();

        QFontMetrics fm(lblPath->font());
        QString elided = fm.elidedText(fullPath, Qt::TextElideMode::ElideMiddle, avail);
        lblPath->setText(elided);
        lblPath->setToolTip(fullPath);
    };

    QObject::connect(btnFolder, &QPushButton::clicked, [&](){
        saveFolder = QFileDialog::getExistingDirectory(&w, "Select folder");
        updatePathLabel(saveFolder);
    });

    QNetworkAccessManager *networkManager = new QNetworkAccessManager(&w);

    QObject::connect(videoCombo, &QComboBox::currentIndexChanged, [&](){
        if (realTitle.isEmpty() || realExt.isEmpty()) return;

        QString resolution = videoCombo->currentText().split(" ").first();
        QString baseName = QString("%1 [%2]").arg(sanitizeFileName(realTitle)).arg(resolution);
        baseName = generateUniqueFileName(saveFolder, baseName);

        QString selectedVideo = videoCombo->currentData().toString();
        QString selectedAudio = audioCombo->currentData().toString();

        if (hasPartialFile(saveFolder, baseName, selectedVideo, selectedAudio)) {
            btnStart->setText("Resume");
        } else {
            btnStart->setText("Start");
        }
    });

    QObject::connect(audioCombo, &QComboBox::currentIndexChanged, [&](){
        if (realTitle.isEmpty() || realExt.isEmpty()) return;

        QString resolution = videoCombo->currentText().split(" ").first();
        QString baseName = QString("%1 [%2]").arg(sanitizeFileName(realTitle)).arg(resolution);
        baseName = generateUniqueFileName(saveFolder, baseName);

        QString selectedVideo = videoCombo->currentData().toString();
        QString selectedAudio = audioCombo->currentData().toString();

        qDebug() << selectedVideo << selectedAudio;

        if (hasPartialFile(saveFolder, baseName, selectedVideo, selectedAudio)) {
            btnStart->setText("Resume");
        } else {
            btnStart->setText("Start");
        }
    });

    QObject::connect(btnCheck, &QPushButton::clicked, [&](){
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

        QObject::connect(proc, &QProcess::readyReadStandardOutput, [=, &realTitle, &realExt](){
            QByteArray output = proc->readAllStandardOutput();
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(output, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject obj = doc.object();
                QString title = obj.value("title").toString();
                realExt = obj.value("ext").toString();
                realTitle = title;
                lblTitle->setText("Title: " + title);

                // ---------------- Video Quality ----------------
                QSet<QString> addedQualities;
                videoCombo->clear();

                QJsonArray formats = obj.value("formats").toArray();
                for (auto f : formats) {
                    QJsonObject fmt = f.toObject();
                    QString format_id = fmt.value("format_id").toString();
                    QString resolution = fmt.value("resolution").toString();
                    QString acodec = fmt.value("acodec").toString();
                    QString vcodec = fmt.value("vcodec").toString();
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

                    if (sizeText.isEmpty()) continue;

                    if (vcodec == "none") continue;

                    if (addedQualities.contains(resolution)) continue;
                    addedQualities.insert(resolution);

                    QString display = resolution + sizeText;
                    videoCombo->addItem(display, format_id);
                }

                // ---------------- Audio Quality ----------------
                QSet<QString> addedAudio;
                audioCombo->clear();

                for (auto f : formats) {
                    QJsonObject fmt = f.toObject();

                    QString format_id = fmt.value("format_id").toString();
                    QString acodec = fmt.value("acodec").toString();
                    QString vcodec = fmt.value("vcodec").toString();
                    double abr = fmt.value("abr").toDouble(0.0);
                    qint64 filesize = fmt.value("filesize").toVariant().toLongLong();

                    if (vcodec != "none" || acodec == "none")
                        continue;

                    if (abr <= 0.1) continue;

                    QString key = QString("%1-%2").arg(acodec).arg(abr);
                    if (addedAudio.contains(key)) continue;
                    addedAudio.insert(key);

                    QString sizeText;
                    if (filesize > 1024*1024)
                        sizeText = QString(" ~ %1 MB").arg(filesize / (1024*1024));

                    QString display = QString("%1 kbps (%2)%3")
                                          .arg(abr)
                                          .arg(acodec)
                                          .arg(sizeText);

                    audioCombo->addItem(display, format_id);
                }

                if (audioCombo->count() == 0)
                    audioCombo->addItem("Best audio", "ba");

                // ---------------- Thumbnails ----------------
                QJsonArray thumbnails = obj.value("thumbnails").toArray();

                QString bestUrl;
                int maxWidth = 0;

                for (auto t : thumbnails) {
                    QJsonObject thumb = t.toObject();
                    QString url = thumb.value("url").toString();
                    int width = thumb.value("width").toInt(0);
                    QString fileName = QFileInfo(QUrl(url).fileName()).completeBaseName();

                    if (fileName.isEmpty()) continue;

                    if (width > maxWidth) {
                        maxWidth = width;
                        bestUrl = url;
                    }
                }

                if (!bestUrl.isEmpty()) {
                    QNetworkRequest request((QUrl(bestUrl)));
                    QNetworkReply *reply = networkManager->get(request);
                    QString fileName = QFileInfo(QUrl(bestUrl).fileName()).completeBaseName();

                    QObject::connect(reply, &QNetworkReply::finished, [reply, lblStoryboard]() {
                        QByteArray data = reply->readAll();
                        QPixmap pix;
                        if(pix.loadFromData(data))
                            lblStoryboard->setPixmap(pix.scaled(lblStoryboard->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                        reply->deleteLater();
                    });
                }

                btnStart->setEnabled(true);
                btnFolder->setEnabled(true);
                videoCombo->setEnabled(true);
                audioCombo->setEnabled(true);

                if (videoCombo->count() > 0) {
                    int index = videoCombo->currentIndex();
                    emit videoCombo->currentIndexChanged(index);
                }
            }
        });

        QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         [=](int exitCode, QProcess::ExitStatus status){
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

        QString selectedVideo = videoCombo->currentData().toString();
        QStringList args;
        args << "-m" << "yt_dlp"
             << "--dump-json"
             << urlEdit->text();

        proc->start("python", args);
    });

    QProcess *proc;
    bool isDownloading = false;
    QString outputPath;

    QObject::connect(btnStart, &QPushButton::clicked, [&](){
        if (!isDownloading) {
            if (urlEdit->text().isEmpty()) {
                lblStatus->setText("Status: URL empty!");
                QMessageBox::critical(&w, "Error", "URL empty!");
                return;
            }
            if (saveFolder.isEmpty()) {
                lblStatus->setText("Status: Folder not selected");
                QMessageBox::critical(&w, "Error", "Folder not selected!");
                return;
            }

            urlEdit->setEnabled(false);
            videoCombo->setEnabled(false);
            audioCombo->setEnabled(false);
            btnFolder->setEnabled(false);
            btnCheck->setEnabled(false);
            ring->startDeterminate();
            percent = 0;
            ring->setProgress(percent);

            lblStatus->setText("Status: Starting download...");

            // ---------------- QProcess for yt-dlp ----------------
            proc = new QProcess(&w);

            // redirect stdout/stderr
            proc->setProcessChannelMode(QProcess::MergedChannels);

            QObject::connect(proc, &QProcess::readyReadStandardOutput, [=]() {
                QByteArray output = proc->readAllStandardOutput();
                QStringList lines = QString(output).split("\n", Qt::SkipEmptyParts);

                for (QString &line : lines) {
                    QJsonParseError err;
                    QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
                    if (err.error == QJsonParseError::NoError && doc.isObject()) {
                        QJsonObject obj = doc.object();
                        QString status = obj.value("status").toString();
                        if (!status.isEmpty())
                            lblStatus->setText(status + " - " + obj.value("downloaded_bytes").toVariant().toString());

                    } else {
                        if (line.contains("download") && line.contains("ETA"))
                        {
                            lblStatus->setText(parseYtDlpStatus(line));
                            ring->setProgress(percent);
                        }
                        else if (line.contains("Destination") && line.contains(".mp4"))
                            typeFile = "Video";
                        else if (line.contains("Destination"))
                            typeFile = "Audio";
                        else if (line.contains("Merger"))
                            lblStatus->setText("Merging Video & Audio ...");
                    }
                }
            });

            QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                             [&](int exitCode, QProcess::ExitStatus status){
                                 Q_UNUSED(status);
                                 ring->stop();

                                 if (exitCode == 0){
                                     lblStatus->setText("Status: Download finished Successful.");
                                     QMessageBox::information(&w, "Info", "Download finished Successful.");
                                 }
                                 else if (!isDownloading){
                                     lblStatus->setText("Download stopped manually.");
                                     QMessageBox::information(&w, "Info", "Download stopped manually.");
                                 }
                                 else{
                                     lblStatus->setText("Download finished by Error.");
                                     QMessageBox::critical(&w, "Info", "Download finished by Error.");
                                 }
                                 proc->deleteLater();
                                 isDownloading = false;
                                 btnStart->setText("Start");
                                 urlEdit->setEnabled(true);
                                 videoCombo->setEnabled(true);
                                 audioCombo->setEnabled(true);
                                 btnFolder->setEnabled(true);
                                 btnCheck->setEnabled(false);

                                 int index = videoCombo->currentIndex();
                                 emit videoCombo->currentIndexChanged(index);
                             });

            QString program = "python";
            QStringList args;
            QString selectedVideo = videoCombo->currentData().toString();
            QString resolution = videoCombo->currentText();
            QString selectedAudio = audioCombo->currentData().toString();
            if (resolution.isEmpty())
                resolution = "unknown";
            else
                resolution = resolution.split(" ").first();

            QString outputName = QString("%1 [%2]").arg(sanitizeFileName(realTitle)).arg(resolution);
            outputPath = QString("%1/%2").arg(saveFolder).arg(generateUniqueFileName(saveFolder, outputName));

            args << "-m" << "yt_dlp"
                 << "--ffmpeg-location" << QCoreApplication::applicationDirPath()
                 << "--newline"
                 << "-c"
                 << "-f"  << QString("%1+%2/%1").arg(selectedVideo, selectedAudio)
                 << "--merge-output-format" << "mp4"
                 << "-o" << outputPath
                 << urlEdit->text();

            proc->start(program, args);

            isDownloading = true;
            btnStart->setText("Stop");
        }
        else
        {
            if (proc && proc->state() == QProcess::Running) {
                proc->kill();
            }

            isDownloading = false;
        }
    });

    QObject::connect(urlEdit, &QLineEdit::textChanged, [&](){
        lblTitle->setText("Title will appear here");
        videoCombo->clear();
        audioCombo->clear();
        lblStoryboard->clear();
        lblStatus->setText("Status: Enter URL and check info...");
        btnStart->setEnabled(false);
        btnFolder->setEnabled(false);
        videoCombo->setEnabled(false);
        audioCombo->setEnabled(false);
        btnCheck->setEnabled(true);
    });

    w.show();
    return app.exec();
}
