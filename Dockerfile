FROM nvidia/cuda:12.2.0-devel-ubuntu22.04

# Utils
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y wget && \ 
    apt-get install -y git && \
    apt-get install -y vim &&\
    apt install -y python3-pip

# aDDM toolbox dependencies
RUN wget -O /usr/include/c++/11/BS_thread_pool.hpp https://raw.githubusercontent.com/bshoshany/thread-pool/master/include/BS_thread_pool.hpp && \
    mkdir /usr/include/c++/11/nlohmann/ && \
    wget -O /usr/include/c++/11/nlohmann/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp && \
    apt-get install -y libboost-math-dev libboost-math1.74-dev catch2

WORKDIR /home

# aDDM toolbox installations
RUN git clone https://github.com/aDDM-Toolbox/ADDM.cu.git && \
    cd ADDM.cu/ && \
    make install

WORKDIR /home/ADDM.cu

# Python dependencies
# Make sure you're in correct directory with access to file
RUN pip install --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt

CMD ["/bin/bash"]
