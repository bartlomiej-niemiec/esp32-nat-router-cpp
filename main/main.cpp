#include "wifi_nat_router_app.hpp"

#include "data_storer_if/data_storer.hpp"

extern "C" void app_main(void)
{
    DataStorage::DataStorer::Init();
}