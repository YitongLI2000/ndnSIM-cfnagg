ndnSIM for CFNAgg
==================
Note:

This is a customized project, for any detail problem regarding ndnSIM,
please refer to official documentation (https://ndnsim.net/current/)
---

## How to build this project

- Make sure you've followed all ***prerequisites (only need to installed required libraries in this section)*** and configured the environment for ndnSIM (please refer to https://ndnsim.net/current/getting-started.html)
- This project needs installation of ***python 3.10***, and currently only tested successfully on ***ubuntu 22.04*** and ***Mac OS***
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
  git clone https://github.com/YitongLI2000/ndnSIM-cfnagg.git ns-3
  git clone https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
  ```
- build ndnSIM (If python3.10 can't be found, then you should specify python3.10 executable manually.
  E.g. "--with-python=/usr/bin/python3.10")
  ```shell 
  cd ns-3
  ./waf configure --with-python=python3.10 --enable-examples -d debug
  ./waf
  ```
- Start simulation (make sure you're under "ndnSIM/ns-3" directory)
  ```shell
  ./cfnagg_run.sh
  ```
- The result graphs (log files/graphs of consumer/aggregators) will be generated under directory
  ```shell
  ndnSIM/ns-3/src/ndnSIM/results 
  ```


