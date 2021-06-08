/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <iostream>

#include "SimpleWMS.h"
#include "SimpleStandardJobScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms, "Log category for Simple WMS");

/**
 * @brief Create a Simple WMS with a workflow instance, a scheduler implementation, and a list of compute services
 */
SimpleWMS::SimpleWMS(std::unique_ptr<wrench::StandardJobScheduler> standard_job_scheduler,
                     std::unique_ptr<wrench::PilotJobScheduler> pilot_job_scheduler,
                     const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                     const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                     const std::string &hostname) : wrench::WMS(
        std::move(standard_job_scheduler),
        std::move(pilot_job_scheduler),
        compute_services,
        storage_services,
        {}, nullptr,
        hostname,
        "simple") {}

/**
 * @brief main method of the SimpleWMS daemon
 */
int SimpleWMS::main() {

    wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_GREEN);

    // Check whether the WMS has a deferred start time
    checkDeferredStart();

    WRENCH_INFO("About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

    // Create a job manager
    this->job_manager = this->createJobManager();

    // Create a data movement manager
    std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

    // Get the available compute services
    auto compute_services = this->getAvailableComputeServices<wrench::ComputeService>();
    if (compute_services.empty()) {
        throw std::runtime_error("WMS needs at least one compute service to run!");
    }

    auto compute_service = *(this->getAvailableComputeServices<wrench::ComputeService>().begin());
    auto cloud_service = *(this->getAvailableComputeServices<wrench::CloudComputeService>().rbegin());

    // Set the scheduler's available num cores
    // WARNING: This is only done for the first compute service (perhaps ugly?)
    ((SimpleStandardJobScheduler *)(this->getStandardJobScheduler()))->setNumCoresAvailable((*compute_services.begin())->getTotalNumCores());

    // Get the available storage services
    auto storage_services = this->getAvailableStorageServices();
    if (storage_services.empty()) {
        throw std::runtime_error("WMS needs at least one storage service to run!");
    }

    // possible to have zero size arrays?
    std::string cloud_vm[num_vm_instances];

    if (SimpleWMS::getNumVmInstances() > 0) {
        for (int i = 0; i < num_vm_instances; i++) {
            cloud_vm[i] = cloud_service->createVM(4, 500000);
        }
        ((SimpleStandardJobScheduler *)this->getStandardJobScheduler())->setNumVmInstances(SimpleWMS::getNumVmInstances());
        ((SimpleStandardJobScheduler *)this->getStandardJobScheduler())->setCloudTasks(cloud_tasks);
    }

    while (true) {
        // Get the ready tasks
        std::vector<wrench::WorkflowTask *> ready_tasks = this->getWorkflow()->getReadyTasks();

        this->getStandardJobScheduler()->scheduleTasks(compute_services, ready_tasks);

        // Wait for a workflow execution event, and process it
        try {
            WRENCH_INFO("Waiting for some execution event (job completion or failure)");
            this->waitForAndProcessNextEvent();
//            WRENCH_INFO("Got the execution event");
        } catch (wrench::WorkflowExecutionException &e) {
            WRENCH_INFO("Error while getting next execution event (%s)... ignoring and trying again",
                        (e.getCause()->toString().c_str()));
            continue;
        }

        if (this->getWorkflow()->isDone()) {
            break;
        }
    }

    this->job_manager.reset();

    return 0;
}

/**
 * @brief Process a standard job failure event
 *
 * @param event: the event
 */
void SimpleWMS::processEventStandardJobFailure(std::shared_ptr<wrench::StandardJobFailedEvent> event) {
    /* Retrieve the job that this event is for */
    auto job = event->standard_job;
    WRENCH_INFO("Notified that a standard job has failed (failure cause: %s)",
                event->failure_cause->toString().c_str());
    /* Retrieve the job's tasks */
    WRENCH_INFO("As a result, the following tasks have failed:");
    for (auto const &task : job->getTasks()) {
        WRENCH_INFO(" - %s", task->getID().c_str());
        ((SimpleStandardJobScheduler *) this->getStandardJobScheduler())
                ->updateNumCoresAvailable(task->getNumCoresAllocated());
    }
    throw std::runtime_error("A job failure has occurred... this should never happen!");
}

/**
 * @brief Process a standard job completion event
 *
 * @param event: the event
 */
void SimpleWMS::processEventStandardJobCompletion(std::shared_ptr<wrench::StandardJobCompletedEvent> event) {
    /* Retrieve the job that this event is for */
    auto job = event->standard_job;
    WRENCH_INFO("Notified that a standard job has successfully completed");
    /* Retrieve the job's tasks */
    WRENCH_INFO("As a result, the following tasks have completed:");
    for (auto const &task : job->getTasks()) {
        WRENCH_INFO(" - %s", task->getID().c_str());
        ((SimpleStandardJobScheduler *) this->getStandardJobScheduler())
        ->updateNumCoresAvailable(task->getNumCoresAllocated());
    }
}

/**
 * @brief Method to get the number of vm instances
 * @return the number of vm instances
 */
int SimpleWMS::getNumVmInstances() {
    return num_vm_instances;
}

/**
 * @brief Method to set the number of vm instances
 *
 * @param num_vm_instances: number of vm instances to set
 */
void SimpleWMS::setNumVmInstances(int num_vm_instances) {
    num_vm_instances = num_vm_instances;
}

/**
 * @brief Method to get the string containing the tasks for the vm
 * @return number of cloud vm tasks
 */
std::string SimpleWMS::getCloudTasks() {
    return cloud_tasks;
}

/**
 * @brief Method to set the string containing the cloud tasks
 *
 * @param tasks: string of cloud vm tasks
 */
void SimpleWMS::setCloudTasks(std::string tasks) {
    cloud_tasks = tasks;
}