#include "user_credential_manager/user_credential_manager.hpp"
#include <data_storer_if/data_storer.hpp>
#include <algorithm>

namespace UserCredential
{

UserCredentialManager::UserCredentialManager():
    m_NoUsers(0),
    m_UsersDb{}
{
    auto adminInNvs = User::IsUserExisting(adminUserName);
    auto userInNvs = User::IsUserExisting(readerUserName);

    if (!adminInNvs)
    {
        m_Users.emplace(std::make_pair(adminUserName, User(adminUserName, adminUserDefaultPassword, adminDefaultLevel)));
    }
    else
    {
        m_Users.emplace(std::make_pair(adminUserName, User(adminUserName)));
    }

    if (!userInNvs)
    {
        m_Users.emplace(std::make_pair(readerUserName, User(readerUserName, readerUserDefaultPassword, userDefaultLevel)));
    }
    else
    {
        m_Users.emplace(std::make_pair(readerUserName, User(readerUserName)));
    }

    auto & dataStorer = DataStorage::DataStorer::GetInstance();
    auto dataEntry = dataStorer.GetDataEntry<UsersList>(userNamesNvs);
    if (dataEntry.GetData(m_UsersDb) == DataStorage::DataRawStorerIf::ReadStatus::NOT_FOUND)
    {
        dataEntry.SetData(m_UsersDb);
    }
    else
    {
        for (auto & userEntry : m_UsersDb)
        {
            static constexpr const char * EMPTY_NAME = "";
            if (strncmp(userEntry.data(), EMPTY_NAME, MAX_USERNAME_SIZE) == 0) break;
            m_Users.emplace(std::make_pair(userEntry.data(), User(userEntry.data())));
            m_NoUsers++;
        }
    }

}

UserCredentialManager & UserCredentialManager::GetInstance()
{
    static UserCredentialManager ucm;
    return ucm;
}

std::vector<std::string> UserCredentialManager::GetUserNames() const
{
    std::vector<std::string> v{};

    for (auto & [userName, d] : m_Users)
    {
        v.push_back(userName);
    }
    return v;
}

std::optional<UserCredentialManager::Error> UserCredentialManager::ChangeUserPassword(std::string_view userName, std::string_view newPassword)
{
    if (newPassword.size() > MAX_PASSWORD_SIZE) return UserCredentialManager::Error::PASSWORD_TOO_LONG;

    std::string userstring(userName);
    auto it = m_Users.find(userstring);
    if (it == m_Users.end()) return UserCredentialManager::Error::USER_DOES_NOT_EXIST;

    if (it->second.ChangePassword(newPassword) == false) return UserCredentialManager::Error::ERROR;

    return std::nullopt;
}

std::optional<UserCredentialManager::Error> UserCredentialManager::ChangeUserLevel(std::string_view userName, int level)
{
    std::string userstring(userName);
    auto it = m_Users.find(userstring);
    if (it == m_Users.end()) return UserCredentialManager::Error::USER_DOES_NOT_EXIST;

    if (it->second.ChangeLevel(level) == false) return UserCredentialManager::Error::ERROR;

    return std::nullopt;
}

std::optional<UserCredentialManager::Error> UserCredentialManager::VerifyUserPassword(std::string_view userName, std::string_view password, bool & passwordMatch)
{
    std::string userstring(userName);
    auto it = m_Users.find(userstring);
    if (it == m_Users.end()) return UserCredentialManager::Error::USER_DOES_NOT_EXIST;

    if (it->second.VerifyPassword(password))
    {
        passwordMatch = true;
        return std::nullopt;
    }

    return UserCredentialManager::Error::ERROR;
}

std::optional<UserCredentialManager::Error> UserCredentialManager::DeleteUser(std::string_view userName)
{
    std::string userstring(userName);
    auto it = m_Users.find(userstring);
    if (it == m_Users.end()) return UserCredentialManager::Error::USER_DOES_NOT_EXIST;

    m_Users.erase(userstring);
    auto & dataStorer = DataStorage::DataStorer::GetInstance();
    dataStorer.RemoveDataForKey(userName);

    auto dataEntry = dataStorer.GetDataEntry<UsersList>(userNamesNvs);

    for (int i = 0; i < m_UsersDb.size(); i++)
    {
        if (strncmp(m_UsersDb[i].data(), userName.data(), MAX_USERNAME_SIZE) == 0)
        {
            std::memset(m_UsersDb[i].data(), 0, MAX_USERNAME_SIZE);
        }
    }

    dataEntry.SetData(m_UsersDb);

    m_NoUsers--;
    return std::nullopt;
}

std::optional<UserCredentialManager::Error> UserCredentialManager::CreateNewUser(std::string_view userName, std::string_view userPassword, int level)
{
    if (m_NoUsers >= MAX_USERS) return UserCredentialManager::Error::MAX_USERS_HIT;

    std::string userstring(userName);
    auto it = m_Users.find(userstring);
    if (it != m_Users.end()) return UserCredentialManager::Error::USER_ALREADY_EXISTING;

    if (userName.size() > MAX_USERNAME_SIZE) return UserCredentialManager::Error::USER_NAME_TOO_LONG;
    if (userPassword.size() > MAX_PASSWORD_SIZE) return UserCredentialManager::Error::PASSWORD_TOO_LONG;
    
    m_Users.emplace(std::make_pair(userName, User(userName, userPassword, level)));
    auto & dataStorer = DataStorage::DataStorer::GetInstance();
    auto dataEntry = dataStorer.GetDataEntry<UsersList>(userNamesNvs);

    for (int i = 0; i < m_UsersDb.size(); i++)
    {
        static constexpr const char * EMPTY_NAME = "";
        if (strncmp(m_UsersDb[i].data(), EMPTY_NAME, MAX_USERNAME_SIZE) == 0)
        {
            size_t size = strnlen(userName.data(), MAX_USERNAME_SIZE);
            std::copy_n(userName.begin(), size, m_UsersDb[i].begin());
            m_UsersDb[i][size] = '\0';
            m_NoUsers++;
            break;
        }
    }

    dataEntry.SetData(m_UsersDb);

    return std::nullopt;
}


}