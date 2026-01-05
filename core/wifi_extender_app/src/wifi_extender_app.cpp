#include "wifi_extender_app.hpp"
#include "model.hpp"

#include "data_storer_if/data_storer.hpp"

namespace WifiExtenderApp
{

void Init()
{
    DataStorage::DataStorer::Init();
}

void Startup()
{
    static Model m;
    m.Startup();
}


}