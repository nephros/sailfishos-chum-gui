#ifndef PROJECTSOURCEFORGE_H
#define PROJECTSOURCEFORGE_H

#include <QObject>
#include <QString>

#include "projectabstract.h"

class ProjectSourceForge : public ProjectAbstract
{
    Q_OBJECT
public:
    explicit ProjectSourceForge(const QString &url, ChumPackage *package);

    static bool isProject(const QString &url);

    virtual void issue(const QString &id, LoadableObject *value) override;
    virtual void issues(LoadableObject *value) override;
    virtual void release(const QString &id, LoadableObject *value) override;
    virtual void releases(LoadableObject *value) override;

signals:

private:
    void fetchRepoInfo();

private:
    QString m_project;

};

#endif // PROJECTSOURCEFORGE_H
