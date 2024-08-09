ndnSIM for CFNAgg
==================
Note:

This is a customized project, for any detail problem regarding ndnSIM,
please refer to official documentation (https://ndnsim.net/current/)
---

## How to build this project

- Make sure you've installed all prerequisites for ndnSIM (please refer to https://ndnsim.net/current/getting-started.html)
- Clone the repository
    ```shell
  mkdir ndnSIM
  cd ndnSIM
  git clone https://github.com/CasterYT/agg-ndnSIM-8.8.git ns-3
  git clone https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
    ```
- build ndnSIM
    ```shell
  cd ns-3
  ./waf configure --enable-examples -d debug
  ./waf
    ```
- Start simulation (make sure you're under ndnSIM/ns-3)
    ```shell
  ./cfnagg_run.sh
    ```
    The shell will ask for some input parameters, I show an example in the following:
    ```shell
      Enter the number of producers: 50
      Enter the number of aggregators: 20
      Enter the number of producers connected with one edge forwarder: 5
      Enter the bit rate: 30Mbps
    ```
- The result graphs (consumer_rtt_aggregationTime.png, consumer_window_rto.png) will be generated under directory
    ```shell
  cd ndnSIM/ns-3/src/ndnSIM/experiments/result
    ```


