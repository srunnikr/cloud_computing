

Simulating 3-Tier and PortLand Architectures using NS-3 and ntu-dsi-dcn
------------------------------------------------------------------------

CSE 291-G Cloud Computing, Fall 2016
Assignment 1

Team Members:

* Dhruv Sharma 	(A53101868, dhsharma@cs.ucsd.edu)
* Amit Borase 		(A53095391, aborase@eng.ucsd.edu)
* Sreejith Unnikrishnan	(A53097010, srunnikr@eng.ucsd.edu)
* Khalid Alqinyah	(A53087580, kalqinya@eng.ucsd.edu)
* Praneeth Sanapathi	(A53096247, psanapat@eng.ucsd.edu)
* Debjit Roy		(A53094436, deroy@eng.ucsd.edu)

Location of code and Data
--------------------------

All code related to Portland Switching, fabric manager and internal frameworks is located in the folder /ntu-dsi-dcn/src/portland

Some modifications are made to the flow monitor module for data analysis. The file location is /ntu-dsi-dcn/src/flow-monitor

Topologies are present in the /scratch folder
Portland-Setup.cc and Three-tier.cc are the simulation toplogies for Portland and Three-tier architectures

Throughput and delay values are stored in the folder /ntu-dsi-dcn/throughput-data/

Running the simulations
--------------------------------------
The value of k (radix) can be changed in the source files at scratch/ and then can be run as follows:

- To simulate the Fat tree architecture,

```
./waf --run scratch/Fat-tree-AlFares > throughput-data/Fattree_kvalue
```

- To simulate the BCube architecture, 

```
./waf --run scratch/BCube > throughput-data/BCube_kvalue
```

- To simulate the Portland architecture, 

```
./waf --run scratch/Portland-Setup > throughput-data/Portland_kvalue
```

- To simulate the 3-tier architecture, 

```
./waf --run scratch/Three-tier > throughput-data/Three-tier_kvalue
```

- The performance statistics outputs are generated in the "/statistics" folder in XML format. Statistics output information such as the average throughput, number of packets transmitted and packet delay can be found by opening the XML file in a text editor


License
--------------------------------------
Licensed under the [GNU General Public License](https://github.com/ntu-dsi-dcn/ntu-dsi-dcn/blob/master/LICENSE).
