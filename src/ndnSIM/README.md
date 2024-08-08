How to configure this customized ndnSIM project 
=============
---


- Clone from github repository
    ``` shell
    mkdir ndnSIM
    cd ndnSIM
    git clone https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
    git clone https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
    git clone --recursive https://github.com/CasterYT/ndnSIM-complete.git ns-3/src/ndnSIM
    ```
  - Build ndnSIM
      ``` shell
    cd ns-3
    ./waf configure --enable-examples -d debug
    ./waf
      ```
    - Start simulation (make sure you're under ndnSIM/ns-3 directory)
        ```shell
        ./cfnagg_run.sh
        ```
      The shell will ask for some input parameters, I show an example in the following:
    ```shell
      Enter the number of producers: 50
      Enter the number of aggregators: 20
      Enter the number of producers connected with one edge forwarder: 5
      Enter the bit rate: 5Mbps
    ```



