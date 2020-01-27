# Automatically generated by SST
import sst

KB = 1024
MB = 1024*KB
GB = 1024*MB

# Define SST Program Options:
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Define the SST Components:
comp_cpu0 = sst.Component("cpu0", "macsimComponent.macsimComponent")
comp_cpu0.addParams({
     "trace_file" : "trace_file_list",
     "mem_size" : 4*GB,
     "command_line" :"--clock_cpu=4.0 --clock_cpu=4.0 --clock_llc=4.0 --clock_noc=4.0 --clock_mc=4.0 " + \
                     "--use_memhierarchy=1 --perfect_icache=1 " + \
                     "--num_sim_cores=1 --num_sim_large_cores=1 --num_sim_small_cores=0 " + \
                     "--core_type=x86",
     "debug_level" : "0",
     "num_link" : "1",
     "frequency" : "4 Ghz",
     "output_dir" : "results",
     "debug" : "0",
     "param_file" : "params.in"
})


comp_cpu0Membus = sst.Component("cpuMembus", "memHierarchy.Bus")
comp_cpu0Membus.addParams({
     "debug" : "0",
     "bus_frequency" : "4 Ghz"
})

link_c0_icache = sst.Link("link_c0_icachec0_icache")
link_c0_icache.connect( (comp_cpu0, "core0-icache", "1000ps"), (comp_cpu0Membus, "high_network_0", "1000ps") )
link_c0_dcache = sst.Link("link_c0_dcachec0_dcache")
link_c0_dcache.connect( (comp_cpu0, "core0-dcache", "1000ps"), (comp_cpu0Membus, "high_network_1", "1000ps") )


comp_memory0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memory0.addParams({
     "debug" : "0",
     "coherence_protocol" : "MESI",
     "backend.mem_size" : 4096,
     "clock" : "1 GHz",
     "access_time" : "97 ns",
     "rangeStart" : "0"
})

link_bus_cpu0mem = sst.Link("link_bus_cpu0l2cachebus_cpu0l2cache")
link_bus_cpu0mem.connect( (comp_cpu0Membus, "low_network_0", "10000ps"), (comp_memory0, "direct_link", "10000ps") )


# Define the SST Component Statistics Information
# Define SST Statistics Options:
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statoutputcsv", {
  "separator" : ",",
  "filepath" : "sst.stat.csv",
  "outputtopheader" : 1,
  "outputsimtime" : 1,
  "outputrank" : 1,
})

# Define Component Statistics Information:
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")
