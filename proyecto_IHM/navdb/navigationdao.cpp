#include "navigationdao.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QLocale>
#include <QRegularExpression>
#include <QSqlDriver>
#include <QUuid>
#include <algorithm>

NavigationDAO::NavigationDAO(const QString &dbFilePath)
    : m_dbFilePath(dbFilePath),
      m_connectionName(QStringLiteral("navdb-%1").arg(QUuid::createUuid().toString()))
{
    open();
    createTablesIfNeeded();
}

NavigationDAO::~NavigationDAO()
{
    close();
}

void NavigationDAO::open()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        throw NavDAOException(QStringLiteral("Driver QSQLITE no disponible"));
    }

    if (QSqlDatabase::contains(m_connectionName)) {
        m_db = QSqlDatabase::database(m_connectionName);
    } else {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    }

    m_db.setDatabaseName(m_dbFilePath);
    if (!m_db.open()) {
        throwSqlError(QStringLiteral("open"), m_db.lastError());
    }
}

void NavigationDAO::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

void NavigationDAO::createTablesIfNeeded()
{
    createUserTable();
    createSessionTable();
    createProblemTable();
}

void NavigationDAO::createUserTable()
{
    QSqlQuery q(m_db);
    const auto ok = q.exec(
        "CREATE TABLE IF NOT EXISTS \"user\" ("
        " \"nickName\"  TEXT,"
        " \"password\"  TEXT,"
        " \"email\"     TEXT,"
        " \"birthDate\" TEXT,"
        " \"avatar\"    BLOB,"
        " PRIMARY KEY(\"nickName\")"
        ") WITHOUT ROWID");
    if (!ok) {
        throwSqlError(QStringLiteral("create user"), q.lastError());
    }
}

void NavigationDAO::createSessionTable()
{
    QSqlQuery q(m_db);
    const auto ok = q.exec(
        "CREATE TABLE IF NOT EXISTS \"session\" ("
        " \"userNickName\" TEXT NOT NULL,"
        " \"timeStamp\"    TEXT NOT NULL,"
        " \"hits\"         INTEGER,"
        " \"faults\"       INTEGER,"
        " FOREIGN KEY(\"userNickName\") REFERENCES \"user\"(\"nickName\")"
        ")");
    if (!ok) {
        throwSqlError(QStringLiteral("create session"), q.lastError());
    }
}

void NavigationDAO::createProblemTable()
{
    QSqlQuery q(m_db);
    const auto ok = q.exec(
        "CREATE TABLE IF NOT EXISTS \"problem\" ("
        " \"text\"   TEXT,"
        " \"answer1\" TEXT, \"val1\" TEXT,"
        " \"answer2\" TEXT, \"val2\" TEXT,"
        " \"answer3\" TEXT, \"val3\" TEXT,"
        " \"answer4\" TEXT, \"val4\" TEXT"
        ")");
    if (!ok) {
        throwSqlError(QStringLiteral("create problem"), q.lastError());
    }
}

QMap<QString, User> NavigationDAO::loadUsers()
{
    QMap<QString, User> users;
    QSqlQuery q(m_db);

    if (!q.exec("SELECT nickName, password, email, birthDate, avatar FROM user")) {
        throwSqlError(QStringLiteral("load users"), q.lastError());
    }

    while (q.next()) {
        User user = buildUserFromQuery(q);
        user.setSessions(loadSessionsFor(user.nickName()));
        user.setInsertedInDb(true);
        users.insert(user.nickName(), user);
    }
    return users;
}

QVector<Problem> NavigationDAO::loadProblems()
{
    QVector<Problem> problems;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT text, answer1, val1, answer2, val2, answer3, val3, answer4, val4 FROM problem")) {
        throwSqlError(QStringLiteral("load problems"), q.lastError());
    }

    while (q.next()) {
        problems.push_back(buildProblemFromQuery(q));
    }
    return problems;
}

void NavigationDAO::saveUser(User &user)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO user (nickName, password, email, birthDate, avatar)"
              " VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(user.nickName());
    q.addBindValue(user.password());
    q.addBindValue(user.email());
    q.addBindValue(dateToDb(user.birthdate()));
    q.addBindValue(imageToPng(user.avatar()));

    if (!q.exec()) {
        throwSqlError(QStringLiteral("insert user"), q.lastError());
    }
    user.setInsertedInDb(true);
}

void NavigationDAO::updateUser(const User &user)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE user SET password = ?, email = ?, birthDate = ?, avatar = ?"
              " WHERE nickName = ?");
    q.addBindValue(user.password());
    q.addBindValue(user.email());
    q.addBindValue(dateToDb(user.birthdate()));
    q.addBindValue(imageToPng(user.avatar()));
    q.addBindValue(user.nickName());

    if (!q.exec()) {
        throwSqlError(QStringLiteral("update user"), q.lastError());
    }
}

void NavigationDAO::deleteUser(const QString &nickName)
{
    QSqlQuery qs(m_db);
    qs.prepare("DELETE FROM session WHERE userNickName = ?");
    qs.addBindValue(nickName);
    if (!qs.exec()) {
        throwSqlError(QStringLiteral("delete sessions"), qs.lastError());
    }

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM user WHERE nickName = ?");
    q.addBindValue(nickName);
    if (!q.exec()) {
        throwSqlError(QStringLiteral("delete user"), q.lastError());
    }
}

QVector<Session> NavigationDAO::loadSessionsFor(const QString &nickName)
{
    QVector<Session> sessions;
    QSqlQuery q(m_db);
    q.prepare("SELECT timeStamp, hits, faults FROM session WHERE userNickName = ?");
    q.addBindValue(nickName);

    if (!q.exec()) {
        throwSqlError(QStringLiteral("load sessions"), q.lastError());
    }

    while (q.next()) {
        sessions.push_back(buildSessionFromQuery(q));
    }
    std::sort(sessions.begin(), sessions.end(), [](const Session &a, const Session &b) {
        return a.timeStamp() < b.timeStamp();
    });
    return sessions;
}

void NavigationDAO::addSession(const QString &nickName, const Session &session)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO session (userNickName, timeStamp, hits, faults)"
              " VALUES (?, ?, ?, ?)");
    q.addBindValue(nickName);
    q.addBindValue(dateTimeToDb(session.timeStamp()));
    q.addBindValue(session.hits());
    q.addBindValue(session.faults());

    if (!q.exec()) {
        throwSqlError(QStringLiteral("insert session"), q.lastError());
    }
}

void NavigationDAO::replaceAllProblems(const QVector<Problem> &problems)
{
    QSqlQuery clear(m_db);
    if (!clear.exec("DELETE FROM problem")) {
        throwSqlError(QStringLiteral("clear problems"), clear.lastError());
    }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO problem (text, answer1, val1, answer2, val2, answer3, val3, answer4, val4)"
              " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

    for (const auto &p : problems) {
        const auto &answers = p.answers();
        if (answers.size() != 4) {
            continue;
        }
        q.addBindValue(p.text());
        q.addBindValue(answers[0].text());
        q.addBindValue(boolToDb(answers[0].validity()));
        q.addBindValue(answers[1].text());
        q.addBindValue(boolToDb(answers[1].validity()));
        q.addBindValue(answers[2].text());
        q.addBindValue(boolToDb(answers[2].validity()));
        q.addBindValue(answers[3].text());
        q.addBindValue(boolToDb(answers[3].validity()));

        if (!q.exec()) {
            throwSqlError(QStringLiteral("insert problem"), q.lastError());
        }
        q.finish();
    }
}

User NavigationDAO::buildUserFromQuery(QSqlQuery &q)
{
    const auto nick      = q.value(0).toString();
    const auto password  = q.value(1).toString();
    const auto email     = q.value(2).toString();
    const auto birthDate = dateFromDb(q.value(3).toString());
    const auto avatar    = imageFromPng(q.value(4).toByteArray());

    return User(nick, email, password, avatar, birthDate);
}

Session NavigationDAO::buildSessionFromQuery(QSqlQuery &q)
{
    const auto ts     = dateTimeFromDb(q.value(0).toString());
    const auto hits   = q.value(1).toInt();
    const auto faults = q.value(2).toInt();
    return Session(ts, hits, faults);
}

Problem NavigationDAO::buildProblemFromQuery(QSqlQuery &q)
{
    QVector<Answer> answers;
    answers.reserve(4);
    answers.push_back(Answer(q.value(1).toString(), boolFromDb(q.value(2).toString())));
    answers.push_back(Answer(q.value(3).toString(), boolFromDb(q.value(4).toString())));
    answers.push_back(Answer(q.value(5).toString(), boolFromDb(q.value(6).toString())));
    answers.push_back(Answer(q.value(7).toString(), boolFromDb(q.value(8).toString())));

    return Problem(q.value(0).toString(), answers);
}

QByteArray NavigationDAO::imageToPng(const QImage &img)
{
    QByteArray data;
    QBuffer    buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    return data;
}

QImage NavigationDAO::imageFromPng(const QByteArray &bytes)
{
    QImage img;
    img.loadFromData(bytes, "PNG");
    return img;
}

QString NavigationDAO::dateToDb(const QDate &date) const
{
    return date.toString(Qt::ISODate);
}

QDate NavigationDAO::dateFromDb(const QString &s) const
{
    return QDate::fromString(s, Qt::ISODate);
}

QString NavigationDAO::dateTimeToDb(const QDateTime &dt) const
{
    return dt.toString(Qt::ISODate);
}

QDateTime NavigationDAO::dateTimeFromDb(const QString &s) const
{
    if (s.isEmpty()) {
        return {};
    }

    QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
    if (dt.isValid()) {
        return dt;
    }

    dt = QDateTime::fromString(s, Qt::ISODateWithMs);
    if (dt.isValid()) {
        return dt;
    }

    const QString normalized = QString(s)
                                   .replace(QChar(0x202F), QLatin1Char(' '))
                                   .replace(QChar(0x00A0), QLatin1Char(' '))
                                   .simplified();

    {
        const QRegularExpression rx(
            QStringLiteral(R"(^(\d{1,2})/(\d{1,2})/(\d{2,4}),\s*(\d{1,2}):(\d{2})\s*([AP]M)$)"),
            QRegularExpression::CaseInsensitiveOption);
        const auto m = rx.match(normalized);
        if (m.hasMatch()) {
            const int month = m.captured(1).toInt();
            const int day = m.captured(2).toInt();
            int year = m.captured(3).toInt();
            int hour = m.captured(4).toInt();
            const int minute = m.captured(5).toInt();
            const QString ampm = m.captured(6).toUpper();

            if (year < 100) {
                year = (year < 70) ? (2000 + year) : (1900 + year);
            }

            if (ampm == QLatin1String("PM") && hour < 12) {
                hour += 12;
            } else if (ampm == QLatin1String("AM") && hour == 12) {
                hour = 0;
            }

            const QDate date(year, month, day);
            const QTime time(hour, minute, 0);
            if (date.isValid() && time.isValid()) {
                return QDateTime(date, time);
            }
        }
    }

    const QLocale systemLocale = QLocale::system();
    dt = systemLocale.toDateTime(normalized, QLocale::ShortFormat);
    if (dt.isValid()) {
        return dt;
    }
    dt = systemLocale.toDateTime(normalized, QLocale::LongFormat);
    if (dt.isValid()) {
        return dt;
    }

    const QLocale enUs(QLocale::English, QLocale::UnitedStates);
    dt = enUs.toDateTime(normalized, QStringLiteral("M/d/yy, h:mm AP"));
    if (dt.isValid()) {
        return dt;
    }
    dt = enUs.toDateTime(normalized, QStringLiteral("M/d/yyyy, h:mm AP"));
    if (dt.isValid()) {
        return dt;
    }

    return {};
}

QString NavigationDAO::boolToDb(bool v) const
{
    return v ? QStringLiteral("1") : QStringLiteral("0");
}

bool NavigationDAO::boolFromDb(const QString &s) const
{
    return s == QStringLiteral("1") || s.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
}

[[noreturn]] void NavigationDAO::throwSqlError(const QString &where, const QSqlError &err) const
{
    throw NavDAOException(QStringLiteral("%1: %2").arg(where, err.text()));
}
