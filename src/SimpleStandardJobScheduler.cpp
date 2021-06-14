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
    num_vm_instances = num_vm_instances;
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
    cloud_tasks_set = cloud_tasks_set;
}

/**
 * @brief Method to update the number of cores available
 *
 * @param increment: positive or negative increment
 */
void SimpleStandardJobScheduler::updateNumCoresAvailable(long increment) {
    this->numCoresAvailable += increment;
}

/**
 * @brief Method to set the number of cores available
 * @param num_cores: a number of cores
 */
void SimpleStandardJobScheduler::setNumCoresAvailable(unsigned long num_cores) {
    this->numCoresAvailable = num_cores;
}

/**
 * @brief Method to get the number of cores available
 * @return a number of cores
 */
unsigned long SimpleStandardJobScheduler::getNumCoresAvailable() {
    return this->numCoresAvailable;
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

    std::vector<std::shared_ptr<wrench::ComputeService>> cs_vector(compute_services.begin(), compute_services.end());

    // Check that the right compute_services is passed
    if (compute_services.size() < 1) {
        throw std::runtime_error("This example Simple Scheduler requires at least one compute service");
    }

    auto compute_service = *compute_services.begin();
    // check for bare metal service
    std::shared_ptr<wrench::BareMetalComputeService> baremetal_service;
    if (not(baremetal_service = std::dynamic_pointer_cast<wrench::BareMetalComputeService>(compute_service))) {
        throw std::runtime_error("This Scheduler can only handle bare metal services");
    }

    if (SimpleStandardJobScheduler::getNumVmInstances() > 0) {
        // check for the vm created bare metal services
        for (int k = 1; k = compute_services.size() + 1; k++) {
            auto compute_service = cs_vector.at(k);
            std::shared_ptr<wrench::BareMetalComputeService> baremetal_service;
            if (not(baremetal_service = std::dynamic_pointer_cast<wrench::BareMetalComputeService>(compute_service))) {
                throw std::runtime_error("This Scheduler can only handle bare metal services");
            }
        }
    }

    // TODO: Update the "keeping track of available cores" to work for the local BMService
    //  and all the remote BMServices (This Scheduler never knows about the CloudComputeService)

    auto storage_service = this->default_storage_service;

    WRENCH_INFO("About to submit jobs for %ld ready tasks", tasks.size());

    for (auto task: tasks) {

        // if not enough cores available (oversubscribing), go on to next task
        if (this->getNumCoresAvailable() < task->getMinNumCores()) {
            continue;
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
        unsigned long num_cores = 1;
        std::map<std::string, std::string> service_specific_argument;
        service_specific_argument[task->getID()] = std::to_string(num_cores);
        if (SimpleStandardJobScheduler::getNumVmInstances() > 0) {
            // check if task is a cloud task
            if (isCloudTask(task->getID())) {
                // submit job to one of cloud vm bms
                for (int j = 1; j < num_vm_instances + 1; j++) {
                    try {
                        StandardJobScheduler::getJobManager()->submitJob(
                                standard_job, cs_vector.at(j), service_specific_argument);
                        break;
                    } catch (wrench::WorkflowExecutionException &e) {
                        continue;
                    }
                }
            } else {
                StandardJobScheduler::getJobManager()->submitJob(
                        standard_job, compute_service, service_specific_argument);
            }
        } else {
            StandardJobScheduler::getJobManager()->submitJob(
                    standard_job, compute_service, service_specific_argument);
        }
        // decrement num cores needed for task from numCoresAvailable
        updateNumCoresAvailable(-1 * (signed long)num_cores);

    }
    WRENCH_INFO("Done with scheduling tasks as standard jobs");
}

