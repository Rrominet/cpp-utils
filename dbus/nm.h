#pragma once

#include <string>
#include <functional>
#include <giomm.h>
#include <glibmm.h>
#include "../vec.h"
//wrapper for the NetworkManager Dbus Interface.
//all the fucntions are async except for the properties

//One thing important toknow is that everyone of the callbacks are will be ALWAYS called from the MAIN thread, so you CAN callGUI stuff in it without race condition risks.
namespace dbus
{
    namespace nm
    {
        enum DeviceType
        {
            ETH, 
            WIFI, 
            OTHER
        };

        enum DeviceState
        {
            UNKNOWN       = 0  ,// Unknown state
            UNMANAGED     = 10 ,// Device not managed by NM
            UNAVAILABLE   = 20 ,// Device unavailable (no carrier, rfkill, etc.)
            DISCONNECTED  = 30 ,// Device ready but not connected
            PREPARE       = 40 ,// Preparing connection
            CONFIG        = 50 ,// Configuring connection (getting IP, etc.)
            NEED_AUTH     = 60 ,// Needs authentication (WiFi password, etc.)
            IP_CONFIG     = 70 ,// Requesting/configuring IP addresses
            IP_CHECK      = 80 ,// Checking IP connectivity
            SECONDARIES   = 90 ,// Waiting for secondary connections (VPN, etc.)
            ACTIVATED     = 100,// Device is active and connected
            DEACTIVATING  = 110,// Disconnecting/deactivating
            FAILED        = 120,// Connection failed
        };

        void init();

        class DeviceBase
        {
            protected : 
                Glib::RefPtr<Gio::DBus::Proxy> _proxy = nullptr;
                std::string _path;

            public : 
                std::string prop_s(const std::string& prop) const; 
                unsigned int prop_ui(const std::string& prop) const;
                bool prop_bool(const std::string& prop) const;
                guint8 prop_byte(const std::string& prop) const;
                std::string path() const {return _path;}
        };

        class WifiNetwork : public DeviceBase
        {
            private: 
                std::string _dbuspath;
            public : 
                WifiNetwork() {}

                //dbuspath, could be org.freedesktop.NetworkManager.AccessPoint or org.freedesktop.NetworkManager.Connection.Active
                WifiNetwork(const std::string& path, const std::string& dbusPath="org.freedesktop.NetworkManager.AccessPoint");

                //if the ssid is empty, it means that the network is hidden and that to connect to it, you need to know and enter the ssid by your self.
                std::string ssid() const;

                unsigned int strength() const {return (unsigned int)this->prop_byte("Strength");}
                float strengthRatio() const {return (float)this->strength() / 100.0f;}

                unsigned int frequency() const {return this->prop_ui("Frequency");}
                std::string readableFrequency() const;

                bool needPassword() const;
        };

        class Device: public DeviceBase
        {
            public : 
                Device() {}
                Device(const std::string& path);
                std::string interface() const {return this->prop_s("Interface");}
                unsigned int deviceType() const {return this->prop_ui("DeviceType");}

                DeviceType type() const;
                std::string readableType() const;

                bool plugged() const {return this->prop_bool("Carrier");}
                DeviceState state() const {return (DeviceState)this->prop_ui("State");}

                std::string readableState() const;

                //carefull here, if the device is not wifi type, it won't work !
                //the device is the device itself, useful if you need to use it to connect it to a network directly in the function for example.
                void scanNetworks(const std::function<void(Device, ml::Vec<WifiNetwork>&)>& cb);
                void networks(const std::function<void(Device, ml::Vec<WifiNetwork>&)>& cb);
                void connect(const WifiNetwork& network, const std::string& password="", const std::function<void()>& cb=nullptr);

                // will disconnect from the current network but let the device enabled
                void disconnect(const std::function<void(const std::string&)>& cb=nullptr);

                void enable(const std::function<void()>& cb=nullptr);
                void disable(const std::function<void()>& cb=nullptr);

                //it's a the path of the device string, not a wifi network because it will work for a wire connection too
                std::string currentConnected() const;
        };

        //There is quite a mess of divices types that are not useful for the vast Majority of users.
        //filterOnlyWifiEth filters out all devices that are not wifi or ethernet
        void all_devices(const std::function<void(ml::Vec<Device>&)>& cb, bool filterOnlyWifiEth=true);
        void enable_wifi(const std::function<void()>& cb);
        void disable_wifi(const std::function<void()>& cb);
        void wifi_enabled(const std::function<void(bool)>& cb);
    }
}
