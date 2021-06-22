/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef MY_SIMPLESCHEDULER_H
#define MY_SIMPLESCHEDULER_H

#include <wrench-dev.h>
#include "SimpleWMS.h"

class SimpleStandardJobScheduler : public wrench::StandardJobScheduler {
public:
  SimpleStandardJobScheduler(std::shared_ptr<wrench::StorageService> default_storage_service) :
          default_storage_service(default_storage_service) {}

  void scheduleTasks(const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                     const std::vector<wrench::WorkflowTask *> &tasks);

  void updateNumCoresAvailable(std::shared_ptr<wrench::BareMetalComputeService> &cs, long increment);
  void createCoresTracker(std::set<std::shared_ptr<wrench::ComputeService>> &compute_services);
  unsigned long getNumCoresAvailable(std::shared_ptr<wrench::BareMetalComputeService> &cs);
  int getNumVmInstances();
  void setNumVmInstances(int num_vm_instances);
  bool isCloudTask(std::string task_id);
  void setCloudTasks(std::set<std::string> cloud_tasks_set);
    std::map<wrench::WorkflowTask *, std::shared_ptr<wrench::BareMetalComputeService>> tasks_run_on;
private:
  std::shared_ptr<wrench::StorageService> default_storage_service;
  // number of cores available on the local cluster
  // unsigned long numCoresAvailable;

  // TODO: Move to a table of core availabilities (one entry per compute service)
  std::map<std::shared_ptr<wrench::BareMetalComputeService>, unsigned long> numCoresAvailable;
  std::set<std::string> cloud_tasks_set;
  int num_vm_instances;
};

#endif //MY_SIMPLESCHEDULER_H

