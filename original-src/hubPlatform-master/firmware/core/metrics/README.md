# Metrics
The Metrics project aims to obtain hub and service information and send this information to the cloud in encrypted protobuf format.

# Structure
The Metrics service requests system and service information from the Watcher service. It receives this information in 
Json format and creates protobuf data by parsing this information. This created data is serialized and encrypted with 
specified encryption protocols. The encrypted data is sent to the target address via UDP protocol. 

For more information, you can check out the ha-metrics section on 
[confluence](https://confluence.mios.com/display/ELFD/Proactive+Reporting#ProactiveReporting-ha-metrics).

