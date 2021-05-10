#!/usr/bin/env python3

import sys
import glob
import os
import subprocess
import json
import numpy
from functools import cmp_to_key
from pymongo import MongoClient

def analyze_results():

    # Establish mongo connection
    client = MongoClient()
    derrick = client.derrick
    results = derrick.results

    # seconds
    timeToRun = 600
    json_string = {}
    
    all_results = results.find({})

    all_pstate = []
    all_num_hosts = []
    all_exec_time = []
    all_total_cost = []
    all_total_co2 = []

    for result in all_results:
        all_pstate.append(result["pstate"])
        all_num_hosts.append(result["num_hosts"])
        all_exec_time.append(result["num_hosts"])
        all_total_cost.append(result["num_hosts"])
        all_total_co2.append(result["num_hosts"])

    index = -1
    i = 0
    while i < len(all_exec_time):
        if i == 0:
            index = 0
        if ((all_exec_time[i] < timeToRun) and (all_exec_time[index] < all_exec_time[i])):
            json_string["pstate"] = all_pstate.pop(i)
            all_pstate.insert(i, json_string["pstate"])
            json_string["num_hosts"] = all_num_hosts.pop(i)
            all_num_hosts.insert(i, json_string["num_hosts"])
            json_string["exec_time"] = all_exec_time.pop(i)
            all_exec_time.insert(i, json_string["exec_time"])
            json_string["total_cost"] = all_total_cost.pop(i)
            all_total_cost.insert(i, json_string["total_cost"])
            json_string["total_co2"] = all_total_co2.pop(i)
            all_total_co2.insert(i, json_string["total_co2"]) 
        i+=1 

    print(json_string)

if __name__ == '__main__':
    
    analyze_results()


