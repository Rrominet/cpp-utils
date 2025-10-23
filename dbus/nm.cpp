#include "./nm.h"

namespace dbus
{
    namespace nm
    {
        bool _initialized = false;
        void init()
        {
            if (_initialized)
                return;

            Glib::init();
            Gio::init();
            _initialized = true;
        }

        std::string DeviceBase::prop_s(const std::string& prop) const 
        {
            Glib::VariantBase prop_variant;
            _proxy->get_cached_property(prop_variant, prop);
            return Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(prop_variant).get();
        }

        unsigned int DeviceBase::prop_ui(const std::string& prop) const
        {
            Glib::VariantBase prop_variant;
            _proxy->get_cached_property(prop_variant, prop);
            return Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(prop_variant).get();
        }

        bool DeviceBase::prop_bool(const std::string& prop) const
        {
            Glib::VariantBase prop_variant;
            _proxy->get_cached_property(prop_variant, prop);
            return Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(prop_variant).get();
        }

        guint8 DeviceBase::prop_byte(const std::string& prop) const
        {
            Glib::VariantBase prop_variant;
            _proxy->get_cached_property(prop_variant, prop);
            return Glib::VariantBase::cast_dynamic<Glib::Variant<guint8>>(prop_variant).get();
        }


        WifiNetwork::WifiNetwork(const std::string& path)
        {

            _path = path;

            //like said after, this is sync, this is not ideal but to make it asynck would really mess up the whole thing
            _proxy = Gio::DBus::Proxy::create_for_bus_sync(
                Gio::DBus::BusType::SYSTEM,
                "org.freedesktop.NetworkManager",
                _path,
                "org.freedesktop.NetworkManager.AccessPoint"
            );
        }

        std::string WifiNetwork::ssid() const
        {

            Glib::VariantBase ssid_var;
            _proxy->get_cached_property(ssid_var, "Ssid");
            auto ssid_bytes = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<guint8>>>(ssid_var).get();
            return std::string(ssid_bytes.begin(), ssid_bytes.end());
        }

        std::string WifiNetwork::readableFrequency() const
        {
            auto freq = this->frequency();            
            if (freq < 3000)
                return "2.4Ghz";
            else if (freq < 6000)
                return "5Ghz";
            else
                return "6Ghz";
        }

        Device::Device(const std::string& path)
        {
            _path = path;

            //like said after, this is sync, this is not ideal but to make it asynck would really mess up the whole thing
            _proxy = Gio::DBus::Proxy::create_for_bus_sync(
                Gio::DBus::BusType::SYSTEM,
                "org.freedesktop.NetworkManager",
                _path,
                "org.freedesktop.NetworkManager.Device"
            );
        }

        DeviceType Device::type() const
        {
            auto dt = this->deviceType();
            if (dt == 1)
                return DeviceType::ETH;
            else if (dt == 2)
                return DeviceType::WIFI;
            else
                return DeviceType::OTHER;
        }
        void Device::scanNetworks(const std::function<void(ml::Vec<WifiNetwork>&)>& cb)
        {
            if (this->type() != DeviceType::WIFI)
                throw std::runtime_error("This device " + this->interface() + " is not a wifi device.\nIts type is " + this->readableType());

            auto onres = [cb](Glib::RefPtr<Gio::AsyncResult>& result)
            {
                auto proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
                auto oncalled = [cb, proxy](Glib::RefPtr<Gio::AsyncResult>& result)
                {
                    auto res = proxy->call_finish(result);
                    auto outer = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(res);
                    auto aps_variant = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::DBusObjectPathString>>>(outer.get_child(0));

                    std::vector<Glib::DBusObjectPathString> ap_paths = aps_variant.get();
                    ml::Vec<WifiNetwork> networks;
                    for (auto& path : ap_paths)
                    {
                        auto ap = WifiNetwork(path);
                        networks.push_back(ap);
                    }
                    cb(networks);
                };

                proxy->call("GetAllAccessPoints", oncalled);
            };

            Gio::DBus::Proxy::create_for_bus(
                Gio::DBus::BusType::SYSTEM,
                "org.freedesktop.NetworkManager",
                this->path(),
                "org.freedesktop.NetworkManager.Device.Wireless",
                onres
            );
        }

        std::string Device::readableState() const
        {
            auto st = this->state();            
            switch (st)
            {
                case UNKNOWN: 
                    return "Unknown";
                case UNMANAGED: 
                    return "Connection Unmanaged";
                case UNAVAILABLE: 
                    return "Connection Unavailable";
                case DISCONNECTED: 
                    return "The device is disconected.";
                case PREPARE: 
                    return "Preparing the connection...";
                case CONFIG: 
                    return "Configuring the connection...";
                case NEED_AUTH: 
                    return "Need Authentication";
                case IP_CONFIG: 
                    return "Configuring IP...";
                case ACTIVATED: 
                    return "Active and connected.";
                case DEACTIVATING:
                    return "Deactivating...";
                case FAILED:
                    return "Connection failed.";
                default:
                    return "Unknown";
            }
        }

        std::string Device::readableType() const
        {
            auto tp = this->type();
            if (tp == DeviceType::ETH)
                return "Ethernet";
            else if (tp == DeviceType::WIFI)
                return "Wifi";
            else
                return "Unknown";
        }


        void all_devices(const std::function<void(ml::Vec<Device>&)>& cb, bool filterOnlyWifiEth)
        {
            init();
            auto onres = [cb, filterOnlyWifiEth](Glib::RefPtr<Gio::AsyncResult>& result)
            {
                auto proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
                auto oncalled = [cb, filterOnlyWifiEth, proxy](Glib::RefPtr<Gio::AsyncResult>& result)
                {
                    ml::Vec<Device> devices;

                    auto tuple = proxy->call_finish(result);
                    auto outer = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(tuple);
                    auto res = outer.get_child(0);
                    auto arr = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::DBusObjectPathString>>>(res);
                    for (auto& path : arr.get())
                    {
                        //the device proxy creation is sync here, not ideal but async would make it truly a nightmare.
                        auto dev = Device(path);
                        if (filterOnlyWifiEth)
                        {
                            if (dev.type() == DeviceType::ETH || dev.type() == DeviceType::WIFI)
                                devices.push_back(dev);
                        }
                        else 
                            devices.push_back(dev);
                    }
                    cb(devices);
                };

                proxy->call("GetDevices", oncalled);
            };

            Gio::DBus::Proxy::create_for_bus(
                    Gio::DBus::BusType::SYSTEM,
                    "org.freedesktop.NetworkManager",
                    "/org/freedesktop/NetworkManager",
                    "org.freedesktop.NetworkManager",
                    onres
                    );
        }
    }
}
