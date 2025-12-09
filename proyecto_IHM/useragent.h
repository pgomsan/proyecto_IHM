#ifndef USERAGENT_H
#define USERAGENT_H

#include "navdb/lib/include/navigation.h"
#include <QString>

class UserAgent
{
public:
    explicit UserAgent(Navigation &nav);

    bool login(const QString &username, const QString &password, QString *error);
    void logout();

    bool isLoggedIn() const;
    const User *currentUser() const;

private:
    Navigation &navigation;
    QString currentNick;
};

#endif // USERAGENT_H
