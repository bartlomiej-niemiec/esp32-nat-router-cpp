cmake_minimum_required(VERSION 3.16)

idf_component_register(
                        SRCS "./src/user.cpp"
                             "./src/user_credential_manager_impl.cpp"
                        INCLUDE_DIRS "./include"
                        PRIV_REQUIRES data_storer_if
                    )