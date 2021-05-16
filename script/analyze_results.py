#!/usr/bin/env python3

import sys
import glob
import os
import subprocess
import json
import numpy
from functools import cmp_to_key
from pymongo import MongoClient

def analyze_results(timeToRun):

    # Establish mongo connection
    client = MongoClient()
    derrick = client.derrick
    results = derrick.results

    start_string = """{
  "pstate": 0,
  "num_hosts": 0,
  "exec_time": 0,
  "energy_cost": 0,
  "energy_co2": 0
}
"""

    last_item = {}
    json_string = json.loads(start_string)
    all_results = results.find({})

    for result in all_results:
        if ((result["exec_time"] < timeToRun) and (json_string["exec_time"] < result["exec_time"])):
            json_string["pstate"] = result["pstate"]
            json_string["num_hosts"] = result["num_hosts"]
            json_string["exec_time"] = result["exec_time"]
            json_string["energy_cost"] = result["energy_cost"]
            json_string["energy_co2"] = result["energy_co2"]
        last_item = result
    
    t_1 = results.find_one({"pstate": last_item["pstate"], "num_hosts": 1}, 
          {"_id": 0, "pstate": 1, "num_hosts": 1, "exec_time": 1, "energy_cost": 1, "energy_co2": 1})
    t_n = results.find_one({"pstate": last_item["pstate"], "num_hosts": last_item["num_hosts"]},
          {"_id": 0, "pstate": 1, "num_hosts": 1, "exec_time": 1, "energy_cost": 1, "energy_co2": 1})
    print(t_1)
    print(t_n)
    print()

    speedup = t_1["exec_time"] / t_n["exec_time"]
    print("Question #1 Answer: ")
    print("Speedup = " + str(speedup))
    par_eff = speedup / last_item["num_hosts"]
    print("Parallel Efficiency = " + str(par_eff))
    print()

    print("Question #2 Answer: ")
    print(json_string)
    print()

if __name__ == '__main__':

    if (len(sys.argv) != 2):
        sys.stderr.write("Usage: " + sys.argv[0] + " <run time in secs for Q#2>\n")
        sys.exit(1)
    
    timeToRun = int(sys.argv[1])
    
    analyze_results(timeToRun)


