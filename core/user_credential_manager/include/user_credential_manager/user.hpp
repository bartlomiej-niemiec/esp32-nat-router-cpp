#pragma once

#include <string_view>
#include <stdint.h>
#include <data_storer_if/data_entry.hpp>

#include <algorithm>
#include <cstring>
#include <array>

namespace UserCredential
{

static constexpr uint32_t MAX_USERNAME_SIZE = 32;
static constexpr uint32_t MAX_PASSWORD_SIZE = 64;

class User{

    public:

        User(std::string_view name, std::string_view password, int level = 0);

        User(std::string_view name);

        User & operator=(const User & u) = default;

        ~User();

        bool ChangePassword(std::string_view newPassword);

        bool ChangeLevel(const int newLevel);

        bool VerifyPassword(std::string_view password);

        static bool IsUserExisting(std::string_view name);

    private:

        struct Data{
            private:
                std::array<char, MAX_USERNAME_SIZE> name;
                std::array<char, MAX_PASSWORD_SIZE> password;
                int level;
            public:
                Data() = default;
                Data & SetPassword(std::string_view newPassword);
                Data & SetLevel(int level);
                Data & SetName(std::string_view newName);
                Data & operator=(const Data & d) = default;
                std::string_view GetName() const;
                std::string_view GetPassword() const;
        }; 

        
        Data m_CachedData;

};

}

