#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H
#include <QString>
#include <QDir>
#include <QRegularExpression>
#include <QJsonObject>

class DownloadItem {
public:
    QString url;
    QString title;
    QString videoFormat;
    QString audioFormat;
    QString resolution;
    QString baseName;
    QString saveFolder;
    QString finalFilePath;
    bool canResume = false;
    bool finished = false;

    QString thumbPath;

    void hasPartialFile()
    {
        QDir dir(saveFolder);
        QStringList files = dir.entryList(QDir::Files);
        canResume = false;

        QRegularExpression re("^" +
                              QRegularExpression::escape(baseName) +
                              R"(\.f)" +  QRegularExpression::escape(videoFormat) +
                              R"(\.\w+\.part$)"
                              );

        for (const QString &file : files) {
            if (re.match(file).hasMatch())
                canResume = true;
        }

        re = QRegularExpression("^" +
                                QRegularExpression::escape(baseName) +
                                R"(\.f)" +  QRegularExpression::escape(videoFormat) +
                                R"(\.\w+$)"
                                );

        for (const QString &file : files) {
            if (re.match(file).hasMatch())
                canResume = true;
        }
    }

    QJsonObject toJson() const {
        QJsonObject o;
        o["url"] = url;
        o["title"] = title;
        o["videoFormat"] = videoFormat;
        o["audioFormat"] = audioFormat;
        o["resolution"] = resolution;
        o["baseName"] = baseName;
        o["saveFolder"] = saveFolder;
        o["finalFilePath"] = finalFilePath;
        o["canResume"] = canResume;
        o["finished"] = finished;
        o["thumbPath"] = thumbPath;
        return o;
    }

    static DownloadItem fromJson(const QJsonObject &o) {
        DownloadItem item;
        item.url = o["url"].toString();
        item.title = o["title"].toString();
        item.videoFormat = o["videoFormat"].toString();
        item.audioFormat = o["audioFormat"].toString();
        item.resolution = o["resolution"].toString();
        item.baseName = o["baseName"].toString();
        item.saveFolder = o["saveFolder"].toString();
        item.finalFilePath = o["finalFilePath"].toString();
        item.canResume = o["canResume"].toBool();
        item.finished = o["finished"].toBool();
        item.thumbPath = o["thumbPath"].toString();
        return item;
    }
};

#endif // DOWNLOADITEM_H
