#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H
#include <QString>
#include <QDir>
#include <QRegularExpression>
#include <QJsonObject>

class DownloadItem {
public:
    QString Url;
    QString Title;
    QString VideoFId;
    QString AudioFId;
    QString Resolution;
    QString FileName;
    QString SaveFolder;
    QString FullPath;
    QString ThumbPath;
    QString Status = "Queued";
    int DownloadState = 0;

    void hasPartialFile()
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

    QJsonObject toJson() const {
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

    static DownloadItem fromJson(const QJsonObject &o) {
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
};

#endif // DOWNLOADITEM_H
