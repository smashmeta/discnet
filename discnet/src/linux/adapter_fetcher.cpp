/*
 *
 */

#include "discnet/typedefs.hpp"
#include <discnet/linux/adapter_fetcher.hpp>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <linux/if.h>


namespace discnet
{
    linux_adapter_fetcher::linux_adapter_fetcher()
    {
        // nothing for now
    }
    
    /*
        guid: {154EA313-6D41-415A-B007-BBB7AD740F1F}
        mac_address: 3C:A9:F4:3C:1F:00
        index: 4
        name: Ethernet
        description: Intel(R) Centrino(R) Ultimate-N 6300 AGN
        enabled: true
        address_list: { [192.169.10.10, 255.255.255.0], [192.200.10.10, 255.255.255.0] }
        gateway: 192.200.10.1
     */
    
    std::vector<adapter_t> linux_adapter_fetcher::get_adapters()
    {
        std::vector<adapter_t> result;

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) 
        {
            return result;
        }

        char buf[1024];
        struct ifconf ifc;
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;

        if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) 
        {
            close(sock);
            return result;
        }

        struct ifreq* ifr = ifc.ifc_req;
        int num_interfaces = ifc.ifc_len / sizeof(struct ifreq);


        for (int i = 0; i < num_interfaces; ++i) 
        {
            std::string name = ifr[i].ifr_name;
            std::string mac_addr = "";
            bool loopback = false;
            bool enabled = false;
            // bool multicast_enabled = false;
            int index = -1;
            uint32_t ipv4_addr = 0; // inet_ntoa(addr->sin_addr) => to string
            uint32_t ipv4_mask = 0;
            int mtu = 0;
            // int metric = 0;

            // Get enabled and mc enabled
            int flags_result = ioctl(sock, SIOCGIFFLAGS, &ifr[i]);
            if (flags_result >= 0)
            {
                loopback = (ifr->ifr_flags & IFF_LOOPBACK);
                bool up = (ifr->ifr_flags & IFF_UP);
                bool running = (ifr->ifr_flags & IFF_RUNNING);
                enabled = up && running;
                
            }

            // Get MTU
            if (ioctl(sock, SIOCGIFMTU, &ifr[i]) >= 0)
            {
                mtu = ifr->ifr_mtu;
            }

            if (mtu < 500)
            {
                mtu = 1500;
            }

            // Get metric
            // if (ioctl(sock, SIOCGIFMETRIC, &ifr[i]) >= 0)
            // {
            //     metric = ifr->ifr_metric;
            // }
            
            // Get NIC Index            
            if (ioctl(sock, SIOCGIFINDEX, &ifr[i]) == 0) {
                 index = ifr[i].ifr_ifindex;
            }

            // Get MAC Address (Hardware Address)
            if (ioctl(sock, SIOCGIFHWADDR, &ifr[i]) == 0) {
                unsigned char* mac = reinterpret_cast<unsigned char*>(ifr[i].ifr_hwaddr.sa_data);
                mac_addr = std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }
            
            // Get IP Address
            if (ioctl(sock, SIOCGIFADDR, &ifr[i]) == 0) 
            {
                struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(&ifr[i].ifr_addr);
                ipv4_addr = ntohl(addr->sin_addr.s_addr);
            }

            // Get IP Mask
            if (ioctl(sock, SIOCGIFNETMASK, &ifr[i]) == 0) 
            {
                struct sockaddr_in* netmask = (struct sockaddr_in*)&ifr[i].ifr_netmask;
                ipv4_mask = ntohl(netmask->sin_addr.s_addr);
            }

            adapter_t adapter;
            adapter.m_name = name;
            adapter.m_index = index;
            adapter.m_mac_address = mac_addr;
            adapter.m_address_list.push_back(std::make_pair(address_t(ipv4_addr), address_t(ipv4_mask)));
            adapter.m_loopback = loopback;
            adapter.m_enabled = enabled || true;
            adapter.m_mtu = mtu;
            result.push_back(adapter);
        }

        close(sock);

        return result;
    }
} // !namespace discnet