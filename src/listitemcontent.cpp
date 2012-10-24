#include "listitemcontent.h"
#include "listitem.h"
#include "file.h"

#include <kio/directorysizejob.h>
#include <KIO/StatJob>
#include <KLocale>
#include <QFileInfo>

ListItemContent::ListItemContent(File *file, QObject *parent)
    : QObject(parent)
    , m_file(file)
{
}

File *ListItemContent::file() const
{
    return m_file;
}

ListItemFallbackContent::ListItemFallbackContent(File *file, QObject *parent)
    : ListItemContent(file, parent), m_size(0), m_statComplete(false)
{
    KIO::StatJob *job = KIO::stat(file->url(), KIO::HideProgressInfo);
    connect(job, SIGNAL(result(KJob*)), SLOT(handleStatJob(KJob*)));
}

QVariant ListItemFallbackContent::data(int role) const
{
    if ( role == ListItem::LastModifiedRole )
    {
        return KGlobal::locale()->formatDate(m_modificationTime.date());
    }
    else if ( role == ListItem::ContentSizeRole )
    {
        return KGlobal::locale()->formatByteSize( m_size );
    }
    else if(role == ListItem::StatCompleteRole) {
        return m_statComplete;
    }

    return QVariant();
}

void ListItemFallbackContent::handleStatJob(KJob *job)
{
    if(job && !job->error()) {
        const KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
        m_size = entry.numberValue(KIO::UDSEntry::UDS_SIZE);
        m_modificationTime = QDateTime::fromTime_t(entry.numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME));
        m_statComplete = true;
        emit dataChanged();
    }
}

ListItemDirectoryContent::ListItemDirectoryContent(File *file, QObject *parent)
    : ListItemFallbackContent(file, parent)
    , m_isScanned(false)
    , m_totalFiles(0)
    , m_totalSize(0)
{
    KIO::DirectorySizeJob *job = KIO::directorySize(file->url());
    connect(job, SIGNAL(result(KJob *)), SLOT(handleDirSizeJob(KJob *)));

    KIO::StatJob *statJob = KIO::stat(file->url(), KIO::HideProgressInfo);
    connect(statJob, SIGNAL(result(KJob *)), SLOT(handleStatJob(KJob*)));
}

QVariant ListItemDirectoryContent::data(int role) const
{
    switch (role)
    {
    case ListItem::DirSizeCompleteRole:
        return m_isScanned;

    case ListItem::ContentSizeRole:
        return KGlobal::locale()->formatByteSize(m_totalSize);

    case ListItem::CountRole:
        return QString::number(m_totalFiles);
    }

    return ListItemFallbackContent::data(role);
}

void ListItemDirectoryContent::handleDirSizeJob(KJob *job)
{
    KIO::DirectorySizeJob *dirSizeJob = dynamic_cast<KIO::DirectorySizeJob *>(job);
    if(dirSizeJob && !dirSizeJob->error()) {
        m_totalFiles = dirSizeJob->totalFiles();
        m_totalSize = dirSizeJob->totalSize();
        m_isScanned = true;
        emit dataChanged();
    }
}
