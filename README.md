# ADDM.cu

CUDA implementation of the aDDM-Toolbox. 

## Table of Contents ## 

<!-- START doctoc -->
<!-- END doctoc -->

## Getting Started ## 

The aDDM Toolbox library for CUDA can be cloned on the user's machine or run in a Docker container. __We recommend using the Docker image unless you are familiar with installing and compiling c++ packages__. *Note that although this is a CUDA library, the installation and usage process is almost identical to the C++ instructions - don't worry if you don't have any CUDA experience.* For requirements for a local build of the ADDM.cpp, see the __Local Installation__. For instructions on the Docker installation, continue to the __Docker Image__ section. 


## Docker Image ## 

To pull a Docker Image with ADDM.cu installed, follow the steps below: 

* Install [Docker Desktop](https://www.docker.com/products/docker-desktop/).
* Start Docker Desktop. 
* Pull the Docker image, currently linked at `rnlcaltech/addm.cu`. This can be done via terminal using 

```shell
docker pull rnlcaltech/addm.cu:latest
```
* You can use the Docker image either through Docker Desktop and selecting __run__ on `rnlcaltech/addm.cu` in the list of your local Docker or using a CLI with the following command: 

```shell
docker run -it --rm \
-v $(pwd):/home \
rnlcaltech/addm.cu:latest
```

* If you not on an architecture that is currently supported by the images on Docker Hub you can build the image appropriate for your system using the [Dockerfile](https://github.com/aDDM-Toolbox/ADDM.cu/blob/main/Dockerfile) provided in this repo. To do so navigate to the directory you cloned this repo to and run: 

``` shell
docker build -t {USER_NAME}/addm.cu:{YOUR_TAG} -f ./Dockerfile .
```

## Local Installation ##

### Requirements ###

The standard build of the ADDM.cpp assumes the Linux Ubuntu Distribution 22.04. This library requires g++ version 11.3.0, as well as three third-party C++ packages for thread pools, JSON processing, and statistical distributions:

* [BS::thread_pool](https://github.com/bshoshany/thread-pool)
* [JSON for Modern C++](https://github.com/nlohmann/json)
* [Boost Math/Statistical Distributions](https://www.boost.org/doc/libs/?view=category_math)

These dependencies can be installed using the following commands. Note that they assume the paths `/usr/include/c++/11/` and `apt-get` exist on your system.

```shell
$ wget -O /usr/include/c++/11/BS_thread_pool.hpp https://raw.githubusercontent.com/bshoshany/thread-pool/master/include/BS_thread_pool.hpp
$ mkdir -p /usr/include/c++/11/nlohmann
$ wget -O /usr/include/c++/11/nlohmann/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
$ apt-get install libboost-math-dev libboost-math1.74-dev
```

Be sure to clone the [ADDM.cu](https://github.com/aDDM-Toolbox/ADDM.cu) library from Github if you haven't done so already. 

*Note that the installation directory /usr/include/c++/11 may be modified to support newer versions of C++. In the event of a __Permission Denied__ error, precede the above commands with __sudo__.*

The latest version of NVIDIA CUDA is also requried to be installed locally. This includes NVCC and CUDA versions 12.2, whose installation guidelines are described in the [NVIDIA Documentation](https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html).

### Installation ### 

TThe ADDM.cu library can then be built and installed in one step: 

```shell
$ make install
```

*In the event of a __Permission Denied__ error, precede the above command with __sudo__.*

## Basic Usage ##

Both of the above methods will install the libaddm.so shared library as well as the corresponding header files. Although there are multiple header files corresponding to the aDDM and DDM programs, simply adding `#include <addm/cuda_toolbox.h>` to a C++ program will include all necessary headers. A simple usage example is described below: 

`main.cpp`:
```C++
#include <addm/cuda_toolbox.h>
#include <iostream>

int main() {
    aDDM addm = aDDM(0.005, 0.07, 0.5);
    std::cout << "d: " << addm.d << std::endl; 
    std::cout << "sigma: " << addm.sigma << std::endl; 
    std::cout << "theta: " << addm.theta << std::endl; 
}
```

When compiling any code using the toolbox, include the `-laddm` flag to link with the installed shared object library.

```
$ nvcc -o main main.cpp -laddm
$ ./main
d: 0.005
sigma: 0.07
theta: 0.5
```

*Note that the default installation of `nvcc` may not be the most updated version. In that case, users should compile with the full path i.e. `/usr/local/cuda-12.2/bin/nvcc`.*

## Tutorial ##

In the [data](data/) directory, we have included two test files to demonstrate how to use the toolbox. [expdata.csv](data/expdata.csv) contains experimental data and [fixations.csv](data/fixations.csv) contains the corresponding fixation data. A description of how to fit models corresponding to these subjects is located in [tutorial.cpp](sample/tutorial.cpp). An executable version of this script can be build using the `make run` target. The contents of the tutorial are listed below, but can be found __verbatim__ in [tutorial.cpp](sample/tutorial.cpp).

`sample/tutorial.cpp`
```cpp
#include <addm/cuda_toolbox.h>
#include <iostream> 

int main() {
    // Load trial and fixation data
    std::map<int, std::vector<aDDMTrial>> data = loadDataFromCSV("data/expdata.csv", "data/fixations.csv");
    // Iterate through each SubjectID and its corresponding vector of trials. 
    for (const auto& [subjectID, trials] : data) {
        std::cout << subjectID << ": "; 
        // Compute the most optimal parameters to generate 
        MLEinfo info = aDDM::fitModelMLE(trials, {0.001, 0.002, 0.003}, {0.0875, 0.09, 0.0925}, {0.1, 0.3, 0.5}, {0}, "thread");
        std::cout << "d: " << info.optimal.d << " "; 
        std::cout << "sigma: " << info.optimal.sigma << " "; 
        std::cout << "theta: " << info.optimal.theta << " "; 
        std::cout << "k: " << info.optimal.k << std::endl; 
    }
}
```

Let's break this down piece by piece: 

```cpp
#include <addm/cuda_toolbox.h>
#include <iostream> 
```

This tells the C++ pre-processor to find the `addm` library and the main header file `ccuda_toolbox.h`. The main header file includes all sub-headers for the `DDM` and `aDDM` classes and utility methods, so there is no need to include any other files. If you haven't already, run `make install` to install the `addm` library on your machine. This also tells the pre-processor to compile with the `<iostream>` library, which provides functionality for printing to the console. 

```cpp
// Load trial and fixation data
std::map<int, std::vector<aDDMTrial>> data = loadDataFromCSV("data/expdata.csv", "data/fixations.csv");
```

This uses the `loadDataFromCSV` function included in `util.h` to load the experimental data and fixation data from CSV files. Both files need to be structured in a specific format to be properly loaded. These formats are described below, with sample experimental data in `data/expdata.csv` and fixation data in `data/fixations.csv`. 

__Experimental Data__

| parcode | trial | rt | choice | valueLeft | valueRight |  
| :-:     | :-:   |:-: | :-:    | :-:       | :-:        | 
| 0       | 0     |1962| -1     | 15        | 0          | 
| 0       | 1     |873 | 1      | -15       | 5          |  
| 0       | 2     |1345| 1      | 10        | -5         |  

__Fixation Data__

| parcode | trial | fixItem | fixTime |
| :-:     | :-:   | :-:      | :-:      |
| 0       | 0     | 3        | 176      | 
| 0       | 0     | 0        | 42       | 
| 0       | 0     | 1        | 188      | 

Note that data can also be loaded from a single CSV as well. To do this, use the `loadDataFromSingleCSV` function. This format is described below: 

__Single CSV__

|  trial 	|choice |   rt	|  valueLeft 	|  valueRight 	|  fixItem 	|  fixTime 	|
|:-:	|:-:	|:-:	|:-:	        |:-:	        |:-:	    |:-:	    |
|   0	|  1 	|  350 	|   3	        |   0           |   0	    |   200	    |
|   0	|   1	|  350 	|   3	        |   0	        |   1	    |   150	    |
|   1	|   -1	|  400 	|   4	        |  5            |   0	    |   300	    |
|   1	|   -1	|  400 	|   4	        |   5	        |   2	    |   100	    |

The `loadDataFromCSV` function returns a `std::map<int, std::vector<aDDMTrial>>`. This is a mapping from subjectIDs to their corresponding list of trials. A single trial (choice, response time, fixations) is represented in the `aDDMTrial` object. 

```cpp
for (const auto& [subjectID, trials] : data) ...
```

Iterate through each individual subjectID and its list of aDDMTrials. 

```cpp
std::cout << subjectID << ": "; 
// Compute the most optimal parameters to generate 
MLEinfo info = aDDM::fitModelMLE(trials, {0.001, 0.002, 0.003}, {0.0875, 0.09, 0.0925}, {0.1, 0.3, 0.5}, {0});
std::cout << "d: " << info.optimal.d << " "; 
std::cout << "sigma: " << info.optimal.sigma << " "; 
std::cout << "theta: " << info.optimal.theta << std::endl; 
```

Perform model fitting via Maximum Likelihood Estimation (MLE) to find the optimal parameters for each subject. The arguments for this function are as follows: 

* `trials` - The list of trials for the given subjectID. 
* `{0.001, 0.002, 0.003}` - Range to test for the drift rate (d).
* `{0.0875, 0.09, 0.0925}` - Range to test for noise (sigma).
* `{0.1, 0.3, 0.5}` - Range to test for the fixation discount (theta).
* `{0}` - Range to test for additive fixation factor (k). The default aDDM model assumes no additive scalar for fixations. 

When building the tutorial with `make run`, an executable will be created at `bin/tutorial`. Running this executable should print the model parameters for each subject. At first, it may seem like most subjecs report similar parameters. This is to be expected given the small parameter space the grid search is testing; however, there should be a slight variance among parameters for some subjects. The expected output is described below: 

```
0: d: 0.001 sigma: 0.0925 theta: 0.1
1: d: 0.001 sigma: 0.09 theta: 0.1
2: d: 0.001 sigma: 0.0875 theta: 0.1
3: d: 0.001 sigma: 0.09 theta: 0.1
⋮
```

## Modifying the Toolbox ## 

Some users may want to modify this codebase for their own purposes. Below are some examples of what users may want to do and some tips on altering the code. 

### Alternative Optimization Algorithms ###

For some use-cases, MLE may not be optimal for model fitting. So, some users may want to change how the model fitting algorithm identifies the optimal parameters. The portion of the `fitModelMLE` function in the aDDM class where MLE is actually performed is highlighted in the `addm.cpp` file, but is also described below. 

```cpp
double minNLL = __DBL_MAX__; 
aDDM optimal = aDDM(); 
for (aDDM addm : potentialModels) {
    ProbabilityData aux = addm.computeGPUNLL(trials, trialsPerThread, timeStep, approxStateStep);
    if (normalizePosteriors) {
        allTrialLikelihoods.insert({addm, aux});
        posteriors.insert({addm, 1 / numModels});
    } else {
        posteriors.insert({addm, aux.NLL});
    }
    if (aux.NLL < minNLL) {
        minNLL = aux.NLL; 
        optimal = addm; 
    }
}
```

Key Variables: 
* `potentialModels`: Vector of all possible aDDM models and is created by iterating through the entire parameter grid-space. 
* `posteriors`: Mapping from individual aDDM models to their NLL or marginalized posteriors, depending on input conditions. If the marginal posteriors are to be computed, these calculations are performed at the end of computations. 
* `allTrialLikelihoods`: Mapping from individual aDDM models to their computed `ProbabilityData` objects. For reference, this object contains information regarding the computed proabilities for a vector of `aDDMTrial` objects. It is comprised of the sum of Negative Log Likelihoods, sum of likelihoods, and a list of all likelihoods for each trial. 
* `optimal`: The most optimal model to fit the given trials. 

When redesigning this segment of code, the minimum requirement is that some `aDDM` object is selected as the __optimal__ model. All other computational features can be determined by the user. The decision to compute the marginal posteriors or include code to add to the mappings can also be determined by the user if they inted on using that feature. The code should still compile and run if the `posteriors` and `allTrialLikelihoods` maps are left empty. 

## Python Bindings ## 

Python bindings are also provided for users who prefer to work with a Python codebase over C++. The provided bindings are located in [lib/bindings.cpp](lib/bindings.cpp). Note that [pybind11](https://github.com/pybind/pybind11) and Python version 3.10 (at a minimum) are __strict__ prerequisites for installation and usage of the Python code. These are installed with the Docker image. For local isntallation on a LInux OS they can be installed with 

```shell
apt-get install python3.10
pip3 install pybind11
```

Once `pybind11` and Python 3.10 are installed, the module can be built with:

```
make pybind
```

This will create a shared library object in the repository's root directory containing the `addm_toolbox_cuda` module. Although function calls remain largely analogous with the original C++ code, an example is described below that can be used to ensure the code is working properly: 

`main.py`: 

```Python
import addm_toolbox_cuda

ddm = addm_toolbox_cuda.DDM(0.005, 0.07)
print(f"d = {ddm.d}")
print(f"sigma = {ddm.sigma}")

trial = ddm.simulateTrial(3, 7, 10, 540)
print(f"RT = {trial.RT}")
print(f"choice = {trial.choice}")
```
To run the code: 
```
$ python3 main.py
d = 0.005
sigma = 0.07
RT = 850
choice = 1
```

### Optional: Python Syntax Highlighting ###

For users working in a user interface, such as Visual Studio Code, a Python stub is provided to facilitate features including syntax highlighting, type-hinting, auto-complete. Although the `addm_toolbox_cuda.pyi` stub is built-in, the file can be dynamically generated using the [mypy stubgen](https://mypy.readthedocs.io/en/stable/stubgen.html) tool. The `mypy` module can be installed using: 

```shell
pip install mypy
```

For users who plan to modify the library for their own use and want the provided features, the stub file can be built as follows: 

```shell
stubgen -m addm_toolbox_cuda -o .
```
*Note that the `pybind11` shared library file should be built before running `stubgen`.*

## Data Analysis Scripts ##

A set of data analysis and visualization tools are provided in the [analysis](analysis/) directory. Current provided scripts include: 

* DDM Heatmaps for MLE. 
* aDDM Heatmap for MLE. 
* Posterior Pair Plots.
* Time vs RDV for individual trial.
* Value Differences against Response Time Frequencies. 

See the individual file documentation for usage instructions. 

## Authors ## 

* Jake Goldman - jgoldman@caltech.edu, [jakegoldm](https://github.com/jakegoldm)
* Zeynep Enkavi - zenkavi@caltech.edu, [zenkavi](https://github.com/zenkavi)

## Acknowledgements ##

This toolbox was developed as part of a resarch project in the [Rangel Neuroeconomics Lab](http://www.rnl.caltech.edu/) at the California Institute of Technology. Special thanks to Antonio Rangel and Zeynep Enkavi for your help with this project. 