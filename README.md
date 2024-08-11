ndnSIM for CFNAgg
==================
Note:

This is a customized project, for any detail problem regarding ndnSIM,
please refer to official documentation (https://ndnsim.net/current/)
---

## How to build this project

- Make sure you've followed all ***prerequisites (only need to installed required libraries in this section)*** and configured the environment for ndnSIM (please refer to https://ndnsim.net/current/getting-started.html)
- This project needs installation of ***python 3.10***, and currently only tested to work successfully on ***ubuntu 22.04*** and ***Mac OS***
  - Check python 3.10 installation
  ```shell
  python3.10 --version 
  ```
  - Install ***pandas*** and ***matplotlib*** for python 3.10 
  ```shell
  python3.10 -m pip install matplotlib pandas
  ```
- Clone the repository
    ```shell
  mkdir ndnSIM
  cd ndnSIM
  git clone https://github.com/CasterYT/agg-ndnSIM-8.8.git ns-3
  git clone https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
    ```
- Check and specify python3.10 installation path
  - Check python 3.10 installation path (my python3.10 installation is "/usr/bin/python3.10", 
  you should change it to your own path)
  ```shell
  which python 3.10
  ```
- build ndnSIM (change "/usr/bin/python3.10" with your own python3.10 executable path)
    ```shell
  cd ns-3
  ./waf configure --with-python=/usr/bin/python3.10 --enable-examples -d debug
  ./waf
    ```
- Start simulation (make sure you're under ndnSIM/ns-3, and change "/usr/bin/python3.10" with your own python3.10 executable path)
    ```shell
  ./cfnagg_python3.10_run.sh /usr/bin/python3.10
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


