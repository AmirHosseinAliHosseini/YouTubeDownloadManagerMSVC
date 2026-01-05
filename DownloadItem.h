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

    QString getFullPath();
    void hasPartialFile();
    QJsonObject toJson() const;
    static DownloadItem fromJson(const QJsonObject &o);

private:
    QString sanitizeFileName(const QString &name);
    QString generateUniqueFileName(const QString &folder, const QString &baseName);
};

#endif // DOWNLOADITEM_H
