#include "../include/gnuradio-4.0/dashboard_blocks/imGUI_management.hpp"
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::ordered_json;

namespace gr::dashboard_blocks {

    void imGUI_DashboardRegistry::config_fileGenerator() {
        
        //Config type
        json root_config;
        root_config["msg_type"] = "config";

        //Storage for all the panels in the flowgraph
        std::map<std::string, json> panel_groups;

        //Loop through each callback to get the string containng the data about the blocks
        for (auto& callback : ptrs_to_imGUIblocks_callbacks) {
            
            // Get the string from each block using the lamdas
            std::string element_str = callback(); 
            
            try {
                // Parse the raw string into a JSON object
                json element_json = json::parse(element_str);
                
                // Find the panel name 
                std::string panel_key = element_json.value("panel_name", "default_panel");

                //If the panel name already exists we simple add this dashboard element object into it
                //If it doesnt a new json object is created and the element is added to it
                panel_groups[panel_key].push_back(element_json); 
                
            } catch (const json::parse_error& e) {
                std::cerr << "[Registry] JSON Parse Error from block callback: " << e.what() << std::endl;
            }
        }


        //json array for the actual config file generation
        json panels_array = json::array();


        //For each panel_group
        //place all the elements in their associated panel section of the config file
        for (auto& [p_name, elements] : panel_groups) {
            json panel_obj;
            panel_obj["panel_name"] = p_name;
            panel_obj["dashboardElement"] = elements;
            panels_array.push_back(panel_obj);
        }


        //attach the panel array 
        root_config["panels"] = panels_array;
        //Serialize 
        std::string final_json_str = root_config.dump(2);
        
        //TO DO: Remove, for debugging
        std::cout << final_json_str << std::endl;

        //Write into config file
        std::ofstream out_file(config_file_path);
        if (out_file.is_open()) {
            out_file << final_json_str;
            out_file.close();
            std::cout << "[Registry] Successfully wrote configuration to " << config_file_path << std::endl;
        } else {
            std::cerr << "[Registry] ERROR: Could not open " << config_file_path << " for writing!" << std::endl;
        }
    }

    void imGUI_DashboardRegistry::boot_dashboardServer_Once() {
        
        //Mutex as this function will be called in blocks start function
        //Note: Lock removed on return of function 
        std::lock_guard<std::mutex> lock(registry_mutex);

        // Check if the server has already been booted, return if yes
        if (is_server_booted) {
            return; 
        }

        //TO DO: Remove, for debugging
        std::cout << "[Registry] Generating config.json from lambdas..." << std::endl;
        
        // Because blocks registered in their constructors, it is guaranteed
        // that all elements are ready to be written to the config file safely here
        config_fileGenerator();

        //Starting up dashboard_server, this will occur after every block has been registerd
        //TO DO: Remove for debugging
        std::cout << "[Registry] Booting dashboard daemon..." << std::endl;
    
        // Build the command: e.g., "./dashBoard_daemon &"
        std::string command = dashBoard_daemon_path + " &";

        // Execute the program
        int result = system(command.c_str());
        if (result != 0) {
            std::cerr << "[Registry] WARNING: Failed to start daemon!" << std::endl;
        }

        start_signal_watcher();
        
        is_server_booted = true;
    }

    void imGUI_DashboardRegistry::register_imGUI_block(std::function<std::string()> callback) {
        
        //TO DO: This is a container, so i think i should pass callback using move function
        //Should probably check this
        ptrs_to_imGUIblocks_callbacks.push_back(std::move(callback));
        count_imGUI_blocks++;

        //TO DO: Comment out Debug Message
        std::cout << " [Registering] Number of imGUIBlocks registered: " << count_imGUI_blocks << "\n";
    }

    void imGUI_DashboardRegistry::unregisterBlockAndTeardown(){

        //Mutex as this function will be called in blocks stop function
        //Note: Lock removed on return of function 
        std::lock_guard<std::mutex> lock(registry_mutex);

        //Decrement the count of imGUI blocks
        count_imGUI_blocks--;
        std::cout << " [Unregistering] Number of imGUIBlocks registered: " << count_imGUI_blocks << "\n";

        //Once there are not blocks left kill the server process & clear the vector block info
        if (count_imGUI_blocks == 0) {
            
            //TO DO: Remove for  debugging
            std::cout << "[Registry] Last block destroyed. Clearing registry." << std::endl;
            
            //Kill dashBoard server process
            system("pkill -f dashboard_daemon"); 
            
            //Clear vector of block info
            ptrs_to_imGUIblocks_callbacks.clear();
            
            is_server_booted = false;
        }
    }

    // Handling closing of the dashboard server
    std::atomic<bool> imGUI_DashboardRegistry::sigint_received{false};

    void imGUI_DashboardRegistry::sigint_handler(int) {
        sigint_received.store(true, std::memory_order_relaxed);
    }

    void imGUI_DashboardRegistry::set_stop_callback(std::function<void()> cb) {
        stop_callback = std::move(cb);
    }

    void imGUI_DashboardRegistry::start_signal_watcher() {
        std::signal(SIGINT, &imGUI_DashboardRegistry::sigint_handler);
        std::signal(SIGTERM, &imGUI_DashboardRegistry::sigint_handler);

        std::thread([this]() {
            while (!sigint_received.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            std::cout << "[Registry] SIGINT/SIGTERM received, requesting scheduler stop...\n";
            if (stop_callback) {
                stop_callback();
            }
        }).detach();
    }

} // namespace gr::dashboard_blocks