#include "useragent.h"

UserAgent::UserAgent(Navigation &nav)
    : navigation(nav)
{
}

bool UserAgent::login(const QString &username, const QString &password, QString *error)
{
    User *user = navigation.authenticate(username, password);
    if (!user) {
        if (error) {
            *error = QObject::tr("Usuario o contraseña incorrectos.");
        }
        return false;
    }
    currentNick = user->nickName();
    return true;
}

void UserAgent::logout()
{
    currentNick.clear();
}

bool UserAgent::isLoggedIn() const
{
    return !currentNick.isEmpty();
}

const User *UserAgent::currentUser() const
{
    if (currentNick.isEmpty()) {
        return nullptr;
    }
    return navigation.findUser(currentNick);
}
