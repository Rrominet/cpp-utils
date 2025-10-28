#include "./nm.h"
#include "../thread.h"
#include <glibmm-2.68/glibmm/variant.h>
#include <glibmm-2.68/glibmm/variantdbusstring.h>
#include "../str.h"

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


        WifiNetwork::WifiNetwork(const std::string& path, const std::string& dbusPath)
        {
            lg("a");
            _path = path;
            _dbuspath = dbusPath;
            lg("a");

            //like said after, this is sync, this is not ideal but to make it asynck would really mess up the whole thing
            _proxy = Gio::DBus::Proxy::create_for_bus_sync(
                    Gio::DBus::BusType::SYSTEM,
                    "org.freedesktop.NetworkManager",
                    _path,
                    _dbuspath
                    );
            lg("a");
            lg(_proxy.get());
        }

        std::string WifiNetwork::ssid() const
        {
            lg("a");
            std::string propname = "Ssid";
            lg("a");
            if (str::contains(_dbuspath, "Active"))
            {
            lg("a");
                propname = "Id";
            lg("a");
                return this->prop_s(propname);
            }
            else 
            {
            lg("a");
                Glib::VariantBase ssid_var;
            lg("a");
                _proxy->get_cached_property(ssid_var, propname);
            lg("a");
                auto ssid_bytes = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<guint8>>>(ssid_var).get();
            lg("a");
                return std::string(ssid_bytes.begin(), ssid_bytes.end());
            }
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

        bool WifiNetwork::needPassword() const
        {
            auto rsn = this->prop_ui("RsnFlags");
            auto wpa = this->prop_ui("WpaFlags");

            return (rsn != 0 || wpa != 0);
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

        void Device::scanNetworks(const std::function<void(Device, ml::Vec<WifiNetwork>&)>& cb)
        {
            if (this->type() != DeviceType::WIFI)
                throw std::runtime_error("This device " + this->interface() + " is not a wifi device.\nIts type is " + this->readableType());
            auto this_cp = *this;
            auto onres = [this_cp, cb](Glib::RefPtr<Gio::AsyncResult>& result)
            {

                assert(threads::is_main() && "proxy::create_for_bus(...) callback : This should be on the main thread, if not there is something wrong with glib async system.");

                auto proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
                auto oncalled = [this_cp, cb, proxy](Glib::RefPtr<Gio::AsyncResult>& result)
                {
                    assert(threads::is_main() && "Proxy->call(...) callback : This should be on the main thread, if not there is something wrong with glib async system.");

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
                    cb(this_cp, networks);
                };

                auto onscan_called = [](Glib::RefPtr<Gio::AsyncResult>& result){};

                proxy->call("RequestScan", onscan_called, Glib::VariantContainerBase::create_tuple({
                    Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create({})
                }));
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

        void Device::networks(const std::function<void(Device, ml::Vec<WifiNetwork>&)>& cb)
        {
            if (this->type() != DeviceType::WIFI)
                throw std::runtime_error("This device " + this->interface() + " is not a wifi device.\nIts type is " + this->readableType());
            auto this_cp = *this;
            auto onres = [this_cp, cb](Glib::RefPtr<Gio::AsyncResult>& result)
            {

                assert(threads::is_main() && "proxy::create_for_bus(...) callback : This should be on the main thread, if not there is something wrong with glib async system.");

                auto proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
                auto oncalled = [this_cp, cb, proxy](Glib::RefPtr<Gio::AsyncResult>& result)
                {
                    assert(threads::is_main() && "Proxy->call(...) callback : This should be on the main thread, if not there is something wrong with glib async system.");

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
                    cb(this_cp, networks);
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
                    return "The device is disconnected.";
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

        Glib::VariantBase build_connection_settings(
                const std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>>& settings) 
        {
            // Create the properly nested type: map<string, map<string, variant>>
            using InnerDict = std::map<Glib::ustring, Glib::VariantBase>;
            using OuterDict = std::map<Glib::ustring, InnerDict>;

            return Glib::Variant<OuterDict>::create(settings);
        }

        void Device::connect(const WifiNetwork& network, const std::string& password, const std::function<void()>& cb)
        {
            // Build connection settings
            Glib::VariantContainerBase connection_settings;

            std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>> settings;

            // Connection section
            settings["connection"]["type"] = Glib::Variant<Glib::ustring>::create("802-11-wireless");
            settings["connection"]["id"] = Glib::Variant<Glib::ustring>::create(network.ssid());
            settings["connection"]["uuid"] = Glib::Variant<Glib::ustring>::create(Glib::ustring(g_uuid_string_random()));

            // WiFi section
            std::vector<guint8> ssid_bytes;
            for (char c : network.ssid())
                ssid_bytes.push_back(c);

            settings["802-11-wireless"]["ssid"] = Glib::Variant<std::vector<guint8>>::create(ssid_bytes);
            settings["802-11-wireless"]["mode"] = Glib::Variant<Glib::ustring>::create("infrastructure");

            // If secured, add security section
            if(network.needPassword()) {
                settings["802-11-wireless-security"]["key-mgmt"] = Glib::Variant<Glib::ustring>::create("wpa-psk");
                settings["802-11-wireless-security"]["psk"] = Glib::Variant<Glib::ustring>::create(password);
            }

            // IP settings
            settings["ipv4"]["method"] = Glib::Variant<Glib::ustring>::create("auto");
            settings["ipv6"]["method"] = Glib::Variant<Glib::ustring>::create("auto");

            // Now convert this clusterfuck to the right variant type (a{sa{sv}})
            // This is the annoying part with glibmm...


            auto& pth = _path;
            auto onproxy_res = [pth, network, settings, cb](Glib::RefPtr<Gio::AsyncResult>& result)
            {
                auto nm_proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
                auto onconnec_res = [nm_proxy, cb](Glib::RefPtr<Gio::AsyncResult>& result)
                {
                    auto res = nm_proxy->call_finish(result);
                    cb();
                };

                // Call AddAndActivateConnection
                std::vector<Glib::VariantBase> tpl;
                tpl.push_back(build_connection_settings(settings));
                tpl.push_back(Glib::Variant<Glib::DBusObjectPathString>::create(pth));
                tpl.push_back(Glib::Variant<Glib::DBusObjectPathString>::create(network.path()));
                auto params = Glib::VariantContainerBase::create_tuple(tpl);
                nm_proxy->call(
                        "AddAndActivateConnection",
                        onconnec_res,
                        params);
            };

            Gio::DBus::Proxy::create_for_bus(
                    Gio::DBus::BusType::SYSTEM,
                    "org.freedesktop.NetworkManager",
                    "/org/freedesktop/NetworkManager",
                    "org.freedesktop.NetworkManager",
                    onproxy_res
                    );
        }

        void Device::disconnect(const std::function<void(const std::string&)>& cb)
        {
            std::string path;
            try
            {
                path = this->currentConnected();
            }
            catch(const std::exception& e)
            {
                cb("Device " + this->interface() + " is already disconnected : \n" + std::string(e.what()));
                return;
            }
            
            auto onproxy_res = [path, cb](Glib::RefPtr<Gio::AsyncResult>& result)
            {
                auto nm_proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
                auto ondisc = [nm_proxy, cb, path](Glib::RefPtr<Gio::AsyncResult>& result)
                {
                    auto res = nm_proxy->call_finish(result);
                    cb("Disconnected from " + path);
                };

                // Call RemoveConnection
                nm_proxy->call(
                        "DeactivateConnection", //changed
                        ondisc,
                        Glib::VariantContainerBase::create_tuple({
                    Glib::Variant<Glib::DBusObjectPathString>::create(path)
                }));
            };

            Gio::DBus::Proxy::create_for_bus(
                    Gio::DBus::BusType::SYSTEM,
                    "org.freedesktop.NetworkManager",
                    "/org/freedesktop/NetworkManager",
                    "org.freedesktop.NetworkManager",
                    onproxy_res
                    );

        }

        std::string Device::currentConnected() const
        {
            if (this->state() != ACTIVATED)
                throw std::runtime_error("Device " + this->interface() + " is not activated (or not connected to any network)");

            if (this->type() != DeviceType::WIFI)
                throw std::runtime_error("Device " + this->interface() + " is not a wifi device");

            auto active_path = this->prop_s("ActiveConnection");
            if (active_path == "" || active_path == "/")
                throw std::runtime_error("Device " + this->interface() + " is not connected to a network");

            return active_path;
        }

        void all_devices(const std::function<void(ml::Vec<Device>&)>& cb, bool filterOnlyWifiEth)
        {
            init();
            auto onres = [cb, filterOnlyWifiEth](Glib::RefPtr<Gio::AsyncResult>& result)
            {
                assert(threads::is_main() && "proxy::create_for_bus(...) callback : This should be on the main thread, if not there is something wrong with glib async system.");
                auto proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
                auto oncalled = [cb, filterOnlyWifiEth, proxy](Glib::RefPtr<Gio::AsyncResult>& result)
                {
                    assert(threads::is_main() && "Proxy->call(GetDevices) callback : This should be on the main thread, if not there is something wrong with glib async system.");
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

        void setWifi(bool value, const std::function<void ()>& cb)
        {
            init();
            auto onres = [cb, value](Glib::RefPtr<Gio::AsyncResult>& result)
            {
                auto proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
                auto oncalled = [cb, proxy, value](Glib::RefPtr<Gio::AsyncResult>& result)
                {
                    proxy->call_finish(result);
                    cb();
                };

            // Create the variant properly - Properties.Set needs signature "ssv"
                auto params = Glib::VariantContainerBase::create_tuple({
                    Glib::Variant<Glib::ustring>::create("org.freedesktop.NetworkManager"),
                    Glib::Variant<Glib::ustring>::create("WirelessEnabled"),
                    Glib::Variant<Glib::VariantBase>::create(Glib::Variant<bool>::create(value))
                });
                
                proxy->call("org.freedesktop.DBus.Properties.Set",
                        oncalled,
                        params
                        );

            };


            Gio::DBus::Proxy::create_for_bus(
                    Gio::DBus::BusType::SYSTEM,
                    "org.freedesktop.NetworkManager",
                    "/org/freedesktop/NetworkManager",
                    "org.freedesktop.NetworkManager",
                    onres
                    );
        }

        void enable_wifi(const std::function<void()>& cb)
        {
            setWifi(true, cb);
        }

        void disable_wifi(const std::function<void()>& cb)
        {
            setWifi(false, cb);
        }

        void wifi_enabled(const std::function<void(bool)>& cb)
        {
            init();
            auto onres = [cb](Glib::RefPtr<Gio::AsyncResult>& result)
            {
                auto proxy = Gio::DBus::Proxy::create_for_bus_finish(result);

                Glib::VariantBase prop_variant;
                proxy->get_cached_property(prop_variant, "WirelessEnabled");
                cb(Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(prop_variant).get());
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
