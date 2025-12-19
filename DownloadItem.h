#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H
#include <QString>
#include <QDir>
#include <QRegularExpression>

class DownloadItem {
public:
    QString url;
    QString title;
    QString videoFormat;
    QString audioFormat;
    QString resolution;
    QString baseName;
    QString saveFolder;
    bool canResume = false;

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
};

#endif // DOWNLOADITEM_H
