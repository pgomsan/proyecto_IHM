#include "navigation.h"

#include <QCoreApplication>

Navigation &Navigation::instance()
{
    static Navigation nav;
    return nav;
}

Navigation::Navigation()
    : m_dao(QCoreApplication::applicationDirPath() + "/navdb.sqlite")
{
    loadFromDb();
}

User *Navigation::findUser(const QString &nick)
{
    auto it = m_users.find(nick);
    return it == m_users.end() ? nullptr : &it.value();
}

const User *Navigation::findUser(const QString &nick) const
{
    auto it = m_users.constFind(nick);
    return it == m_users.cend() ? nullptr : &it.value();
}

User *Navigation::authenticate(const QString &nick, const QString &password)
{
    auto *user = findUser(nick);
    if (user && user->password() == password) {
        return user;
    }
    return nullptr;
}

void Navigation::addUser(User &user)
{
    m_dao.saveUser(user);
    m_users.insert(user.nickName(), user);
}

void Navigation::updateUser(const User &user)
{
    if (!m_users.contains(user.nickName())) {
        return;
    }
    m_dao.updateUser(user);
    m_users[user.nickName()] = user;
}

void Navigation::removeUser(const QString &nickName)
{
    m_dao.deleteUser(nickName);
    m_users.remove(nickName);
}

void Navigation::addSession(const QString &nickName, const Session &session)
{
    auto it = m_users.find(nickName);
    if (it == m_users.end()) {
        return;
    }
    it->addSession(session);
    m_dao.addSession(nickName, session);
}

void Navigation::reload()
{
    loadFromDb();
}

void Navigation::loadFromDb()
{
    m_users    = m_dao.loadUsers();
    m_problems = m_dao.loadProblems();
}
