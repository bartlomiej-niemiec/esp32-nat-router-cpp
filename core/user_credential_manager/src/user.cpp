#include "user_credential_manager/user.hpp"
#include <data_storer_if/data_storer.hpp>
#include <string.h>
#include <assert.h>

namespace UserCredential
{

using namespace DataStorage;


User::User(std::string_view name, std::string_view password, int level):
    m_CachedData{}
{
    m_CachedData.SetName(name).SetPassword(password).SetLevel(level);
    Data tempData{};
    auto & dataStorer = DataStorage::DataStorer::GetInstance();
    auto dataEntry = dataStorer.GetDataEntry<Data>(name);
    DataRawStorerIf::ReadStatus stat = dataEntry.GetData(tempData);

    Data tempData2;

    if (stat != DataRawStorerIf::ReadStatus::NOT_FOUND)
    {
        dataEntry.Remove();
        dataEntry = dataStorer.GetDataEntry<Data>(name);
    }

    dataEntry.SetData(tempData2);
}

User::User(std::string_view name)
{
    auto & dataStorer = DataStorage::DataStorer::GetInstance();
    auto dataEntry = dataStorer.GetDataEntry<Data>(name);
    DataRawStorerIf::ReadStatus stat = dataEntry.GetData(m_CachedData);
    
    assert(stat == DataRawStorerIf::ReadStatus::OK);
}

bool User::IsUserExisting(std::string_view name)
{
    auto & dataStorer = DataStorage::DataStorer::GetInstance();
    auto dataEntry = dataStorer.GetDataEntry<Data>(name);
    Data temp;
    DataRawStorerIf::ReadStatus stat = dataEntry.GetData(temp);
    return stat == DataRawStorerIf::ReadStatus::OK;
}


User::~User()
{

}

bool User::ChangePassword(std::string_view newPassword)
{
    Data tempData{};
    auto & dataStorer = DataStorage::DataStorer::GetInstance();
    auto dataEntry = dataStorer.GetDataEntry<Data>(m_CachedData.GetName());
    
    DataRawStorerIf::ReadStatus stat = dataEntry.GetData(tempData);
    if (stat != DataRawStorerIf::ReadStatus::OK) return false;

    tempData.SetPassword(newPassword);
    bool res = dataEntry.SetData(tempData);
    if (res)
    {
        m_CachedData.SetPassword(newPassword);
    }

    return res;
}

bool User::ChangeLevel(const int newLevel)
{
    Data tempData{};
    auto & dataStorer = DataStorage::DataStorer::GetInstance();
    auto dataEntry = dataStorer.GetDataEntry<Data>(m_CachedData.GetName());
    
    DataRawStorerIf::ReadStatus stat = dataEntry.GetData(tempData);
    if (stat != DataRawStorerIf::ReadStatus::OK) return false;

    tempData.SetLevel(newLevel);
    bool res = dataEntry.SetData(tempData);
    if (res)
    {
        m_CachedData.SetLevel(newLevel);
    }

    return res;
}

bool User::VerifyPassword(std::string_view password)
{
    return m_CachedData.GetPassword() == password;
}

User::Data & User::Data::SetName(std::string_view newName)
{
    memset(this->name.data(), 0, MAX_USERNAME_SIZE);
    size_t min = MAX_USERNAME_SIZE - 1 < strnlen(newName.data(), MAX_USERNAME_SIZE) ? strnlen(newName.data(), MAX_USERNAME_SIZE): MAX_USERNAME_SIZE - 1;
    strncpy(this->name.data(), newName.data(), min);
    this->name[min] = '\0';
    return *this;
}


User::Data & User::Data::SetPassword(std::string_view newPassword)
{
    memset(this->password.data(), 0, MAX_PASSWORD_SIZE);
    size_t min = MAX_PASSWORD_SIZE - 1 < strnlen(newPassword.data(), MAX_PASSWORD_SIZE) ? strnlen(newPassword.data(), MAX_PASSWORD_SIZE): MAX_PASSWORD_SIZE - 1;
    strncpy(this->password.data(), newPassword.data(), min);
    this->password[min] = '\0';
    return *this;
}


User::Data & User::Data::SetLevel(int level)
{
    this->level = level;
    return *this;
}

std::string_view User::Data::GetName() const
{
    return this->name.data();
}

std::string_view User::Data::GetPassword() const
{
    return this->password.data();
}

}


