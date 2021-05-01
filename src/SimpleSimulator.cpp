/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <wrench.h>
#include "SimpleStandardJobScheduler.h"
#include "SimpleWMS.h"
#include <nlohmann/json.hpp>
#include <fstream>

static bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

int main(int argc, char **argv) {

    // Declaration of the top-level WRENCH simulation object
    wrench::Simulation simulation;

    // Initialization of the simulation
    simulation.init(&argc, argv);

    // Parsing of the command-line arguments for this WRENCH simulation
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <json file>" << std::endl;
        exit(1);
    }

    std::ifstream i(argv[1]);
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(i);
    } catch (std::invalid_argument &e) {
        std::cerr << "Problem parsing JSON input file: " + std::string(e.what()) + "\n";
    }

    // number of compute nodes
    int num_hosts = j.at("num_hosts").get<int>();

    // the number of cores per compute node
    int cores = j.at("cores").get<int>();

    // compute host speed
    std::string speed = j.at("speed").get<std::string>();
    // pstate spec
    int pstate = j.at("pstate").get<int>();

    // pstate value
    std::string pstate_value = j.at("value").get<std::string>();

    // energy cost per MWh ($/MWh)
    double cost = j.at("energy_cost_per_mwh").get<double>();

    // energy CO2 per MWh (CO2/MWh)
    double co2 = j.at("energy_co2_per_mwh").get<double>();

    // platform description file, written in XML following the SimGrid-defined DTD
    std::string xml = "<?xml version='1.0'?>\n"
                      "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">\n"
                      "<platform version=\"4.1\">\n"
                      "   <zone id=\"AS0\" routing=\"Full\">\n\n"
                      "       <host id=\"WMSHost\" speed=\"1Gf\" pstate=\"0\" core=\"1\">\n"
                      "           <prop id=\"wattage_per_state\" value=\"0.0:0.0\"/>\n"
                      "           <prop id=\"wattage_off\" value=\"0\"/>\n"
                      "       </host>\n\n"
                      "       <host id=\"storage_host\" speed=\"1Gf\" pstate=\"0\" core=\"1\">\n"
                      "           <disk id=\"hard_drive\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                      "               <prop id=\"size\" value=\"5000GiB\"/>\n"
                      "               <prop id=\"mount\" value=\"/\"/>\n"
                      "           </disk>\n"
                      "           <prop id=\"wattage_per_state\" value=\"10.00:100.00\"/>\n"
                      "           <prop id=\"wattage_off\" value=\"0\"/>\n"
                      "       </host>\n";

    xml.append("\n");
    for (int i = 1; i < num_hosts + 1; i++) {
        xml.append("       <host id=\"compute_host_" + std::to_string(i)
                   + "\" speed=\"" + speed + "\" pstate=\"" + std::to_string(pstate) + "\" core=\""
                   + std::to_string(cores) + "\">\n" +
                   "           <prop id=\"wattage_per_state\" value=\"" + pstate_value + "\"/>\n" +
                   "           <prop id=\"wattage_off\" value=\"0\"/>\n" +
                   "       </host>\n");
    }
    xml.append("\n");

    // links between each compute host and storage host (1 to hosts)
    for (int i = 1; i < num_hosts + 1; i++) {
        xml.append("       <link id=\"" + std::to_string(i)
                   + "\" bandwidth=\"5000GBps\" latency=\"0us\"/>\n");
    }

    // links between WMS Host and Storage host
    xml.append("       <link id=\"" + std::to_string(num_hosts + 1)
               + "\" bandwidth=\"5000GBps\" latency=\"0us\"/>\n");
    xml.append("\n");

    // routes between each compute host and storage host (1 to hosts)
    for (int i = 1; i < num_hosts + 1; i++) {
        xml.append("       <route src=\"compute_host_" + std::to_string(i) +
                   "\" dst=\"storage_host\"> <link_ctn id=\"" + std::to_string(i) + "\"/> </route>\n");
    }
    // routes between WMS Host and Storage host
    xml.append("       <route src=\"WMSHost\" dst=\"storage_host\"> "
               "<link_ctn id=\"" + std::to_string(num_hosts + 1) + "\"/> </route>\n");
    xml.append("\n");

    xml.append(
            "   </zone>\n"
            "</platform>\n");

    std::string platform_file = "/tmp/hosts.xml";
    auto xml_file = fopen(platform_file.c_str(), "w");
    fprintf(xml_file, "%s", xml.c_str());
    fclose(xml_file);

    // workflow description file, written in XML using the DAX DTD
    std::string s = j.at("workflow_file").get<std::string>();
    char *workflow_file = &s[0];

    // Reading and parsing the workflow description file to create a wrench::Workflow object
    std::cerr << "Loading workflow..." << std::endl;
    wrench::Workflow *workflow;

    // min number of cores per task
    int min_cores = j.at("min_cores_per_task").get<int>();

    // max number of cores per task
    int max_cores = j.at("max_cores_per_task").get<int>();

    if (ends_with(workflow_file, "dax")) {
        workflow = wrench::PegasusWorkflowParser::createWorkflowFromDAX(workflow_file, "1f", false, min_cores,
                                                                        max_cores);
    } else if (ends_with(workflow_file, "json")) {
        workflow = wrench::PegasusWorkflowParser::createWorkflowFromJSON(workflow_file, "1f", false, min_cores,
                                                                         max_cores);
    } else {
        std::cerr << "Workflow file name must end with '.dax' or '.json'" << std::endl;
        exit(1);
    }
    std::cerr << "The workflow has " << workflow->getNumberOfTasks() << " tasks " << std::endl;

    // Reading and parsing the platform description file to instantiate a simulated platform
    std::cerr << "Instantiating SimGrid platform..." << std::endl;
    simulation.instantiatePlatform(platform_file);

    // Get a vector of all the hosts in the simulated platform
    std::vector<std::string> hostname_list = simulation.getHostnameList();

    // Instantiate a storage service
    std::string storage_host = "storage_host";
    // in xml file, need storage_host w/ disk
    std::cerr << "Instantiating a SimpleStorageService on " << storage_host << "..." << std::endl;
    auto storage_service = simulation.add(new wrench::SimpleStorageService(storage_host, {"/"}));

    // Create a list of storage services that will be used by the WMS
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;
    storage_services.insert(storage_service);

    // wms host
    std::string wms_host = storage_host;

    // Create a list of compute services that will be used by the WMS
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;
    try {
        std::vector<std::string> execution_hosts;
        for (int i = 1; i < num_hosts + 1; i++) {
            execution_hosts.push_back("compute_host_" + std::to_string(i));
        }
        auto baremetal_service = new wrench::BareMetalComputeService(
                wms_host, execution_hosts, "", {}, {});
        compute_services.insert(simulation.add(baremetal_service));
    } catch (std::invalid_argument &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);
    }

    // Instantiate a WMS
    auto wms = simulation.add(
            new SimpleWMS(std::unique_ptr<SimpleStandardJobScheduler>(
                    new SimpleStandardJobScheduler(storage_service)),
                          nullptr, compute_services, storage_services, wms_host));
    wms->addWorkflow(workflow);

    // Instantiate a file registry service
    std::string file_registry_service_host = hostname_list[(hostname_list.size() > 2) ? 1 : 0];
    std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
    auto file_registry_service =
            new wrench::FileRegistryService(file_registry_service_host);
    simulation.add(file_registry_service);

    // It is necessary to store, or "stage", input files
    std::cerr << "Staging input files..." << std::endl;
    auto input_files = workflow->getInputFiles();
    try {
        for (auto const &f : input_files) {
            simulation.stageFile(f, storage_service);
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }

    // Launch the simulation
    std::cerr << "Launching the Simulation..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    try {
        simulation.launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = duration_cast<std::chrono::microseconds>(end - start);
    std::cerr << "Simulation done!" << std::endl;

    auto exit_tasks = workflow->getExitTaskMap();
    double workflow_finish_time = 0.0;
    for (auto const &t : exit_tasks) {
        workflow_finish_time = std::max<double>(t.second->getEndDate(), workflow_finish_time);
    }

    auto total_energy = 0;
    total_energy += simulation.getEnergyConsumed("WMSHost");
    total_energy += simulation.getEnergyConsumed("storage_host");
    for (int i = 1; i < num_hosts + 1; i++) {
        total_energy += simulation.getEnergyConsumed("compute_host_" + std::to_string(i));
    }

    // 1 MWh = 3,600 MJ = 3,600,000,000 J
    auto total_cost = (cost / 3600000000) * total_energy;
    auto total_co2 = (co2 / 3600000000) * total_energy;

    char cost_buf[25];
    char co2_buf[25];
    sprintf(cost_buf, "%.2f", total_cost);
    sprintf(co2_buf, "%.2f", total_co2);

    std::cerr << "Total Energy Consumption: " << total_energy << " joules" << std::endl;
    std::cerr << "Total Energy Monetary Cost: $" << cost_buf << std::endl;
    std::cerr << "Total Energy CO2 Cost: " << co2_buf << " CO2" << std::endl;

    std::cerr << "Simulated workflow execution time: " << workflow_finish_time << " seconds" << std::endl;
    std::cerr << "(Simulation time: " << duration.count() << " microseconds)" << std::endl;

    nlohmann::json output_json =
            {
                {"energy_consumption", total_energy},
                {"energy_cost", total_cost},
                {"energy_co2", total_co2},
                {"exec_time", workflow_finish_time}
            };

    std::cout << output_json.dump() << std::endl;

    return 0;
}

