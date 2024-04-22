#include "projectforgejo.h"
#include "chumpackage.h"
#include "main.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocale>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSharedPointer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

QMap<QString, QString> ProjectForgejo::s_sites;

//////////////////////////////////////////////////////
/// helper functions

static QString getName(const QVariant &v) {
  QVariantMap m = v.toMap();
  QString login = m[QLatin1String("username")].toString();
  QString name = m[QLatin1String("name")].toString();
  if (login.isEmpty() || name==login) return name;
  if (name.isEmpty()) return login;
  return QStringLiteral("%1 (%2)").arg(name, login);
}

static void parseUrl(const QString &u, QString &h, QString &path) {
  QUrl url(u);
  h = url.host();
  QString p = url.path();
  if (p.startsWith('/')) p = p.mid(1);
  if (p.endsWith('/')) p.chop(1);
  path = p;
}

//////////////////////////////////////////////////////
/// ProjectForgejo

ProjectForgejo::ProjectForgejo(const QString &url, ChumPackage *package) : ProjectAbstract(package) {
  initSites();
  parseUrl(url, m_host, m_path);
  if (!s_sites.contains(m_host)) {
    qWarning() << "Shouldn't happen: ProjectForgejo initialized with incorrect service" << url;
    return;
  }

  m_token = s_sites.value(m_host, QString{});

  // url is not set as it can be different homepage that is retrieved from query
  m_package->setUrlIssues(QStringLiteral("https://%1/%2/issues").arg(m_host, m_path));

  // fetch information from Forgejo
  fetchRepoInfo();
}

// static
void ProjectForgejo::initSites() {
  if (!s_sites.isEmpty()) return;
  for (const QString &sitetoken: QStringLiteral(FORGEJO_TOKEN).split(QChar('|'))) {
      QStringList st = sitetoken.split(QChar(':'));
      if (st.size() != 2) {
          qWarning() << "Error parsing provided Forgejo site-token pairs";
          return;
      }
      s_sites[st[0]] = st[1];
      qDebug() << "Forgejo support added for" << st[0];
  }
}

// static
bool ProjectForgejo::isProject(const QString &url) {
  initSites();
  QString h, p;
  parseUrl(url, h, p);
  return s_sites.contains(h);
}

QNetworkReply* ProjectForgejo::sendQuery(const QString &type, const QString &path, const QVariant &payload)
  QString reqAuth = QStringLiteral("token %1").arg(m_token);
  QString reqUrl = QStringLiteral("https://%1/api/v1%2").arg(m_host).arg(path);
  QNetworkRequest request;
  request.setUrl(reqUrl);
  request.setRawHeader("Content-Type", "application/json");
  request.setRawHeader("Authorization", reqAuth.toLocal8Bit());
  if (type == QStringLiteral("GET")) {
      return nMng->get(request);
  } else  if (type == QStringLiteral("POST")) {
      return nMng->post(request, payload.toLocal8Bit());
  } else if (type == QStringLiteral("PUT")) {
      return nMng->put(request, payload.toLocal8Bit());
  } else {
      qWarning() << "Forgejo: unhandled network request type:" << type;
  }
}

void ProjectForgejo::fetchRepoInfo() {
  QNetworkReply *reply = sendQuery(
                  QStringLiteral("GET"),
                  QStringLiteral("/repos/%1/%2").arg(m_path)
                  );
  connect(reply, &QNetworkReply::finished, this, [this, reply](){
    if (reply->error() != QNetworkReply::NoError) {
      qWarning() << "Forgejo: Failed to fetch repository data for Forgejo" << this->m_path;
      qWarning() << "Forgejo: Error: " << reply->errorString();
    }

    QByteArray data = reply->readAll();
    QJsonObject r{QJsonDocument::fromJson(data).object()};

    int vi;

    vi = r.value("stars_count").toInt(-1);
    if (vi>=0) m_package->setStarsCount(vi);

    vi = r.value("forks_count").toInt(-1);
    if (vi>=0) m_package->setForksCount(vi);

    vi = r.value("open_issues_count").toInt(-1);
    if (vi>=0) m_package->setIssuesCount(vi);

    vi = r.value("release_counter").toInt(-1);
    if (vi>=0) m_package->setReleasesCount(vi);

    reply->deleteLater();
  });
}


void ProjectForgejo::issue(const QString &issue_id, LoadableObject *value) {
  if (value->ready() && value->valueId()==issue_id)
    return; // value already corresponds to that release
  value->reset(issue_id);

  QNetworkReply *reply = sendQuery(
                  QStringLiteral("GET"),
                  QStringLiteral("/repos/%1/issues/%2").arg(m_path).arg(issue_id)
                  );
  connect(reply, &QNetworkReply::finished, this, [this, issue_id, reply, value](){
    if (reply->error() != QNetworkReply::NoError) {
      qWarning() << "Forgejo: Failed to fetch issue for" << this->m_path;
      qWarning() << "Forgejo: Error: " << reply->errorString();
    }

    QByteArray data = reply->readAll();
    QVariantMap r = QJsonDocument::fromJson(data).object();

    QVariantMap result;
    result["id"] = r.value("id");
    result["number"] = r.value("number");
    result["title"] = r.value("title");
    //QVariantList clist = r.value("notes").toMap().value("nodes").toList();
    //QVariantList result_list;
    //result["commentsCount"] = clist.size();
    result["commentsCount"] = r.value("commments").toString();
    QVariantMap m;
    m["author"] = getName(r.value("original_author"));
    m["created"] = parseDate(r.value("created_at").toString(), true);
    m["updated"] = parseDate(r.value("updated_at").toString(), true);
    m["body"] = r.value("body").toString();
    result_list.append(m);
    /*
    // iterate in reverse as gitlab returns notes in reverse order
    for (auto e=clist.rbegin(); e!=clist.rend(); ++e) {
      QVariantMap element = e->toMap();
      m.clear();
      m["author"] = getName(element.value("author"));
      m["created"] = parseDate(element.value("createdAt").toString(), true);
      m["updated"] = parseDate(element.value("updatedAt").toString(), true);
      m["body"] = element.value("bodyHtml").toString();
      result_list.append(m);
    }

    result["discussion"] = result_list;
    */
    value->setValue(issue_id, result);
    reply->deleteLater();
  });
}


void ProjectForgejo::issues(LoadableObject *value) {
  const QString issues_id{QStringLiteral("issues")}; // huh?
  value->reset(issues_id);

  QNetworkReply *reply = sendQuery(
                  QStringLiteral("GET"),
                  QStringLiteral("/repos/%1/%2").arg(m_path).arg(issue_id) // ok lets use that
                  );

  connect(reply, &QNetworkReply::finished, this, [this, issues_id, reply, value](){
    if (reply->error() != QNetworkReply::NoError) {
      qWarning() << "Forgejo: Failed to fetch issues for" << this->m_path;
      qWarning() << "Forgejo: Error: " << reply->errorString();
    }

    QByteArray data = reply->readAll();
    QVariantList r = QJsonDocument::fromJson(data).object().toArray().toVariantList();

    QVariantList rlist;
    for (const auto &e: r) {
      QVariantMap element = e.toMap();
      QVariantMap m;
      m["id"] = element.value("id");
      m["author"] = getName(element.value("author"));
      m["commentsCount"] = element.value("comments");
      m["number"] = element.value("number");
      m["title"] = element.value("title");
      m["created"] = parseDate(element.value("created_at").toString(), true);
      m["updated"] = parseDate(element.value("updated_at").toString(), true);
      rlist.append(m);
    }

    QVariantMap result;
    result["issues"] = rlist;
    value->setValue(issues_id, result);

    reply->deleteLater();
  });
}


void ProjectForgejo::release(const QString &release_id, LoadableObject *value) {
  if (value->ready() && value->valueId()==release_id)
    return; // value already corresponds to that release
  value->reset(release_id);

  QNetworkReply *reply = sendQuery(
                  QStringLiteral("GET"),
                  QStringLiteral("/repos/%1/release/%2").arg(m_path).arg(release_id)
                  );

  connect(reply, &QNetworkReply::finished, this, [this, release_id, reply, value](){
    if (reply->error() != QNetworkReply::NoError) {
      qWarning() << "Forgejo: Failed to fetch release for" << this->m_path;
      qWarning() << "Forgejo: Error: " << reply->errorString();
    }

    QByteArray data = reply->readAll();
    QVariantMap r = QJsonDocument::fromJson(data).object().toObject().toVariantMap();

    QVariantMap result;
    result["name"] = r.value("name");
    result["description"] = r.value("body");
    result["datetime"] = parseDate(r.value("created_at").toString());

    value->setValue(release_id, result);
    reply->deleteLater();
  });
}


void ProjectForgejo::releases(LoadableObject *value) {
  const QString releases_id{QStringLiteral("releases")}; // huh? II
  value->reset(releases_id);

  QNetworkReply *reply = sendQuery(
                  QStringLiteral("GET"),
                  QStringLiteral("/repos/%1/%2").arg(m_path).arg(release_id) // well lets use it II
                  );

  connect(reply, &QNetworkReply::finished, this, [this, releases_id, reply, value](){
    if (reply->error() != QNetworkReply::NoError) {
      qWarning() << "Forgejo: Failed to fetch releases for" << this->m_path;
      qWarning() << "Forgejo: Error: " << reply->errorString();
    }

    QByteArray data = reply->readAll();
    QVariantList r = QJsonDocument::fromJson(data).object().toArray().toVariantList();

    QVariantList rlist;
    for (const auto &e: r) {
      QVariantMap element = e.toMap();
      QVariantMap m;
      m["id"] = element.value("tag_name");
      m["name"] = element.value("name");
      m["datetime"] = parseDate(element.value("created_at").toString());
      rlist.append(m);
    }

    QVariantMap result;
    result["releases"] = rlist;
    value->setValue(releases_id, result);

    reply->deleteLater();
  });
}
