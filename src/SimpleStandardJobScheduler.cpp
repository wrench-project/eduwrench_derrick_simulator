/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "SimpleStandardJobScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_scheduler, "Log category for Simple Scheduler");

/**
 * @brief Method to get the number of vm instances
 * @return the number of vm instances
 */
int SimpleStandardJobScheduler::getNumVmInstances() {
    return num_vm_instances;
}

/**
 * @brief Method to set the number of vm instances
 *
 * @param num_vm_instances: number of vm instances to set
 */
void SimpleStandardJobScheduler::setNumVmInstances(int num_vm_instances) {
    this->num_vm_instances = num_vm_instances;
}

/**
 * @brief Method to check if a task if a cloud task
 * @param task_id: task name
 * @return true if task_id is a cloud task, false if not
 */
bool SimpleStandardJobScheduler::isCloudTask(std::string task_id) {
    return (this->cloud_tasks_set.find(task_id) != this->cloud_tasks_set.end());
}

/**
 * @brief Method to pass the set of cloud tasks
 *
 * @param cloud_tasks_set: set of cloud tasks
 */
void SimpleStandardJobScheduler::setCloudTasks(std::set<std::string> cloud_tasks_set) {
    this->cloud_tasks_set = cloud_tasks_set;
}

/**
 * @brief Method to update the number of cores available for a compute service
 * @param cs: the compute service to find
 * @param increment: positive or negative increment
 */
void SimpleStandardJobScheduler::updateNumCoresAvailable(std::shared_ptr<wrench::BareMetalComputeService> &cs, long increment) {
    this->numCoresAvailable.find(cs)->second += increment;
}

/**
 * @brief Method to get the number of cores available for a compute service
 * @param cs: the compute service to find
 * @return a number of cores
 */
unsigned long SimpleStandardJobScheduler::getNumCoresAvailable(std::shared_ptr<wrench::BareMetalComputeService> &cs) {
    return this->numCoresAvailable.find(cs)->second;
}

/**
 * @brief Method to create initial map of compute services and cores
 *
 * @param compute_services: the set of compute services
 */
void SimpleStandardJobScheduler::createCoresTracker(std::set<std::shared_ptr<wrench::ComputeService>> &compute_services) {
    for (auto const &cs : compute_services) {
        this->numCoresAvailable.insert({std::dynamic_pointer_cast<wrench::BareMetalComputeService>(cs),
                cs->getTotalNumCores()});
    }
}

/**
 * @brief Schedule and run a set of ready tasks on available bare metal resources
 *
 * @param compute_services: a set of compute services available to run jobs
 * @param tasks: a map of (ready) workflow tasks
 *
 * @throw std::runtime_error
 */
void SimpleStandardJobScheduler::scheduleTasks(const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                               const std::vector<wrench::WorkflowTask *> &tasks) {

    // Check that the at least one compute_services is passed
    if (compute_services.empty()) {
        throw std::runtime_error("This example Simple Scheduler requires at least one compute service");
    }

    // Check that all compute services are BareMetal
    for (auto const &cs : compute_services) {
        if (not(std::dynamic_pointer_cast<wrench::BareMetalComputeService>(cs))) {
            // throw std::runtime_error("This Scheduler can only handle bare metal services");
        }
    }

    // Keep track of the local_cluster_cs
    auto local_cluster_cs = std::dynamic_pointer_cast<wrench::BareMetalComputeService>(*compute_services.begin());
    WRENCH_INFO("1---> %s", local_cluster_cs->getName().c_str());
    // Keep track of the cloud VM cs-s
    std::set<std::shared_ptr<wrench::BareMetalComputeService>> vm_css;
    for (auto const &cs : compute_services) {
        auto vm_cs = std::dynamic_pointer_cast<wrench::BareMetalComputeService>(cs);
        if (not vm_cs) {
            continue;
        }
        vm_css.insert(vm_cs);
        WRENCH_INFO("2---> %s", vm_cs->getName().c_str());
    }
    vm_css.erase(local_cluster_cs);

//    if (SimpleStandardJobScheduler::getNumVmInstances() > 0) {
//        // check for the vm created bare metal services
//        for (int k = 1; k < compute_services.size(); k++) {
//            auto compute_service = cs_vector.at(k);
//            std::shared_ptr<wrench::BareMetalComputeService> baremetal_service;
//            if (not(baremetal_service = std::dynamic_pointer_cast<wrench::BareMetalComputeService>(compute_service))) {
//                throw std::runtime_error("This Scheduler can only handle bare metal services");
//            }
//        }
//    }

    // TODO: Update the "keeping track of available cores" to work for the local BMService
    //  and all the remote BMServices (This Scheduler never knows about the CloudComputeService)

    auto storage_service = this->default_storage_service;

    WRENCH_INFO("About to submit jobs for %ld ready tasks", tasks.size());

//    auto tasks_run = 0;

//    for(auto e : this->cloud_tasks_set)
//        std::cout << e << ' ';

    for (auto task: tasks) {

        std::shared_ptr<wrench::BareMetalComputeService> selected_cs = nullptr;
        WRENCH_INFO("Trying to schedule task %s (cloud=%d)", task->getID().c_str(), isCloudTask(task->getID()));

        // check if task is a cloud task
        // if it is, look for available cloud created bmcs
        // if one is found, set selected_cs to it and break; if not, continue to next task
        if (isCloudTask(task->getID())) {
            for (auto cs : vm_css) {
                WRENCH_INFO("CS %s: %ld cores (task min: %ld)",
                    cs->getName().c_str(),
                    this->getNumCoresAvailable(cs), task->getMinNumCores());
                if (this->getNumCoresAvailable(cs) >= task->getMinNumCores()) {
                    selected_cs = cs;
                    break;
                }
            }
            if (selected_cs == nullptr) {
                WRENCH_INFO("The task was a cloud task, but couldn't be scheduled [skipping it]");
                continue;
            }
        }
        else {
            // if not enough cores available (oversubscribing), go on to next task
            if (this->getNumCoresAvailable(local_cluster_cs) < task->getMinNumCores()) {
                WRENCH_INFO("The task was NOT a cloud task, but couldn't be scheduled [skipping it]");
                continue;
            }
            selected_cs = local_cluster_cs;
        }

        /* Create a standard job for the task */
        WRENCH_INFO("Creating a job for task %s", task->getID().c_str());

        /* First, we need to create a map of file locations, stating for each file
         * where is should be read/written */
        std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>> file_locations;
        for (auto const &f : task->getInputFiles()) {
            file_locations[f] = wrench::FileLocation::LOCATION(storage_service);
        }
        for (auto const &f : task->getOutputFiles()) {
            file_locations[f] = wrench::FileLocation::LOCATION(storage_service);
        }

        /* Create the job  */
        auto standard_job = StandardJobScheduler::getJobManager()->createStandardJob(task, file_locations);

        /* Submit the job to the compute service, using ONE core */
        WRENCH_INFO("Submitting the job to the compute service");
        // unsigned long num_cores = 1;
        std::map<std::string, std::string> service_specific_argument;
        service_specific_argument[task->getID()] = std::to_string(task->getMinNumCores());

        StandardJobScheduler::getJobManager()->submitJob(
                standard_job, selected_cs, service_specific_argument);

        tasks_run_on.insert({task, selected_cs});

        // decrement num cores needed for task from numCoresAvailable
        updateNumCoresAvailable(selected_cs, -1 * (signed long)task->getMinNumCores());

    }
    WRENCH_INFO("Done with scheduling tasks as standard jobs");
    // std::cout << tasks_run << std::endl;
}

