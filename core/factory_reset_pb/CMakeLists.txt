cmake_minimum_required(VERSION 3.16)

idf_component_register(
                        SRCS "./src/factory_reset_pb.cpp"
                        INCLUDE_DIRS "./include"
                        REQUIRES network_status wifi_nat_router_app
                    )