#include "DownloadItem.h"

QString DownloadItem::getFullPath()
{
    QString outputName = QString("%1 [%2]").arg(sanitizeFileName(Title)).arg(Resolution);
    FileName = generateUniqueFileName(SaveFolder, outputName);

    QString outputPath = QString("%1/%2").arg(SaveFolder).arg(FileName);
    FullPath = outputPath + ".mp4";

    return FullPath;
}

void DownloadItem::hasPartialFile()
{
    if (DownloadState == 2)
        return;

    QDir dir(SaveFolder);
    QStringList files = dir.entryList(QDir::Files);
    DownloadState = 0;

    QRegularExpression re("^" +
                          QRegularExpression::escape(FileName) +
                          R"(\.f)" +  QRegularExpression::escape(VideoFId) +
                          R"(\.\w+\.part$)"
                          );

    for (const QString &file : files) {
        if (re.match(file).hasMatch())
            DownloadState = 1;
    }

    re = QRegularExpression("^" +
                            QRegularExpression::escape(FileName) +
                            R"(\.f)" +  QRegularExpression::escape(VideoFId) +
                            R"(\.\w+$)"
                            );

    for (const QString &file : files) {
        if (re.match(file).hasMatch())
            DownloadState = 1;
    }
}

QJsonObject DownloadItem::toJson() const {
    QJsonObject o;
    o["Url"] = Url;
    o["Title"] = Title;
    o["VideoFId"] = VideoFId;
    o["AudioFId"] = AudioFId;
    o["Resolution"] = Resolution;
    o["FileName"] = FileName;
    o["SaveFolder"] = SaveFolder;
    o["FullPath"] = FullPath;
    o["ThumbPath"] = ThumbPath;
    o["Status"] = Status;
    o["DownloadState"] = DownloadState;
    return o;
}

DownloadItem DownloadItem::fromJson(const QJsonObject &o) {
    DownloadItem item;
    item.Url = o["Url"].toString();
    item.Title = o["Title"].toString();
    item.VideoFId = o["VideoFId"].toString();
    item.AudioFId = o["AudioFId"].toString();
    item.Resolution = o["Resolution"].toString();
    item.FileName = o["FileName"].toString();
    item.SaveFolder = o["SaveFolder"].toString();
    item.FullPath = o["FullPath"].toString();
    item.ThumbPath = o["ThumbPath"].toString();
    item.Status = o["Status"].toString();
    item.DownloadState = o["DownloadState"].toInt();
    return item;
}

QString DownloadItem::sanitizeFileName(const QString &name) {
    QString clean = name;
    clean.replace(QRegularExpression(R"([<>:"/\\|?*])"), "_");
    return clean;
}

QString DownloadItem::generateUniqueFileName(const QString &folder, const QString &baseName)
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
