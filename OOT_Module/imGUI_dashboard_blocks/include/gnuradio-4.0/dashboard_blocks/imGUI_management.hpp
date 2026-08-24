#ifndef GNURADIO_DASHBOARDBLOCKS_IMGUI_MANAGEMENT_HPP
#define GNURADIO_DASHBOARDBLOCKS_IMGUI_MANAGEMENT_HPP

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <functional>
#include <csignal>
#include <thread>

namespace gr::dashboard_blocks {

    class imGUI_DashboardRegistry {
    private:

        //Container for holding the dashboard Elements in the flowgraph
        // (Not static! The Singleton instance guarantees only one exists)
        std::vector<std::function<std::string()>> ptrs_to_imGUIblocks_callbacks;

        //Some flag for tracking if the dashboard has been started up
        bool is_server_booted = false;

        //Counter for number of blocks
        std::atomic<int> count_imGUI_blocks{0};
        
        // Mutex for thread-safe start()/stop() calls from the GR4 scheduler
        std::mutex registry_mutex;
        
        //Path to location where the dashBoard_daemon.cpp exists
        std::string dashBoard_daemon_path = "dashboard_daemon";

        //Path to location where config file should be stored
        std::string config_file_path = "/tmp/gr4_dashboard_config.json";

        //Private constructor as this is a singleton
        imGUI_DashboardRegistry() = default; 

        //Help function for building the config file
        //To Do: Check this is the best way to pass the config file parameters
        void config_fileGenerator();


        //Paremeters required for setting up proper SIGINT handling to close dashboard server
        //Lamda for handling calling stop funciton
        std::function<void()>     stop_callback;
        static std::atomic<bool>  sigint_received;
        static void               sigint_handler(int);
        void                      start_signal_watcher();

    public:
        //deleting the cpy & move constructors
        imGUI_DashboardRegistry(const imGUI_DashboardRegistry&) = delete;
        imGUI_DashboardRegistry& operator=(const imGUI_DashboardRegistry&) = delete;

        void set_stop_callback(std::function<void()> callback);


        //The function by which the instance is generated, every subsequent function call after the first will return the same instance
        static imGUI_DashboardRegistry& getInstance() {
            static imGUI_DashboardRegistry instance;
            return instance;
        }

        //Function called by a imGUI block to register its existence
        //This is passed a lamda function as i cant find a way to pass it the variables of the block pre-start function
        void register_imGUI_block(std::function<std::string()> ptrs_to_callback);
        
        //Function called by a imGUI block to register unregister (Call in stop function)
        void unregisterBlockAndTeardown();

        //Function for starting up dashboard, to be called in all imGUI blocks 
        //has internal check for if dashboard has already been booted
        void boot_dashboardServer_Once();
        
    };

} // namespace gr::dashboard_blocks

#endif