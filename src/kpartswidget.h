#ifndef KPARTSWIDGET_H
#define KPARTSWIDGET_H

#include <kparts/mainwindow.h>

#include <QPointer>
#include <kservice.h>

class KPartsWidget : public KParts::MainWindow
{
    Q_OBJECT
public:
    static KPartsWidget *instance();
    QString url() const { return m_part->url().url(); }
    void setUrl(const QString &url);

    QStringList supportedMimeTypes() const;

private:
    KPartsWidget(QWidget *parent = 0);
    ~KPartsWidget();

    static KPartsWidget *m_instance;
    QPointer<KParts::ReadOnlyPart> m_part;
    KService::Ptr m_service;
};

#endif // KPARTSWIDGET_H
