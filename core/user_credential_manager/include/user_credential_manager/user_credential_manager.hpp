#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <array>
#include <vector>
#include <optional>
#include "user.hpp"

namespace UserCredential
{

class UserCredentialManager
{
    public:

        static constexpr int MAX_USERS = 5;

        UserCredentialManager & GetInstance();

        std::vector<std::string> GetUserNames() const;

        enum class Error{
            USER_NAME_TOO_LONG,
            PASSWORD_TOO_LONG,
            USER_DOES_NOT_EXIST,
            USER_ALREADY_EXISTING,
            MAX_USERS_HIT,
            ERROR
        };

        std::optional<Error> ChangeUserPassword(std::string_view userName, std::string_view newPassword); 

        std::optional<Error> ChangeUserLevel(std::string_view userName, int level); 

        std::optional<Error> VerifyUserPassword(std::string_view userName, std::string_view password, bool & passwordMatch); 

        std::optional<Error> DeleteUser(std::string_view userName);

        std::optional<Error> CreateNewUser(std::string_view userName, std::string_view userPassword, int level = 0);

    private:

        UserCredentialManager();

        std::unordered_map<std::string, User> m_Users;

        /// @brief Default users
        static constexpr std::string_view adminUserName{"admin"};
        static constexpr std::string_view adminUserDefaultPassword{"admin"};
        static constexpr int adminDefaultLevel = 9;

        static constexpr std::string_view readerUserName{"user"};
        static constexpr std::string_view readerUserDefaultPassword{"user"};
        static constexpr int userDefaultLevel = 0;

        static constexpr std::string_view userNamesNvs{"UcmNames"};

        using UsersList = std::array<std::array<char, MAX_USERNAME_SIZE>, MAX_USERS>;

        int m_NoUsers;
        UsersList m_UsersDb;
};

}