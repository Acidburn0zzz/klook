#ifndef LISTITEM_H
#define LISTITEM_H

#include "file.h"

#include <QtCore/QObject>

class ListItemContent;

class ListItem : public QObject
{
    Q_OBJECT
public:
    enum
    {
        FilePathRole = Qt::UserRole + 1,
        // this role will always return url no matter if file is downloaded or not
        UrlRole,
        LoadedRole,
        FileNameRole,
        TypeRole,
        LastModifiedRole,
        ContentSizeRole,
        CountRole,       // object count for folders
        MimeRole,
        LocalFileRole, // is file local??
        DownloadInProgress,
        StatCompleteRole, // for fallback content
        DirSizeCompleteRole, // for dir size job
        ErrorRole
    };

    ListItem(File *file, QObject *parent);

    static QHash<int, QByteArray> roleNames();

    QVariant data(int role) const;

    QString path() const;
    void setPath(QString path)
    {
        m_file->setUrl(QUrl(path));
    }

    QString mime() const
    {
        return m_file->mime();
    }

    int type() const
    {
        return m_file->type();
    }

    void setType(int type)
    {
        m_file->setType(static_cast<File::FileType>(type));
    }

    File *file()
    {
        return m_file;
    }

signals:
    void dataChanged();

private:
    File *m_file;
    mutable ListItemContent *m_content;
};

#endif // LISTITEM_H
