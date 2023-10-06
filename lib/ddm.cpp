#include <stdexcept>
#include <functional> 
#include <iostream>
#include <chrono> 
#include <cstddef>
#include <string> 
#include <random>
#include <fstream>
#include <iomanip> 
#include <BS_thread_pool.hpp>
#include "util.h"
#include "ddm.h"
#include "stats.h"

DDMTrial::DDMTrial(unsigned int RT, int choice, int valueLeft, int valueRight) {
    this->RT = RT;
    this->choice = choice;
    this->valueLeft = valueLeft;
    this->valueRight = valueRight;
}

DDM::DDM(float d, float sigma, float barrier, unsigned int nonDecisionTime, float bias, float decay) {
    if (barrier <= 0) {
        throw std::invalid_argument("barrier parameter must be larger than 0.");
    }
    if (bias >= barrier) {
        throw std::invalid_argument("bias parameter must be smaller than barrier parameter.");
    }
    this->d = d;
    this->sigma = sigma; 
    this->barrier = barrier; 
    this->nonDecisionTime = nonDecisionTime;
    this->bias = bias;    
    this->decay = decay; 
}

void DDM::exportTrial(DDMTrial dt, std::string filename) {
    std::ofstream o(filename);
    json j;
    j["d"] = d;
    j["sigma"] = sigma;
    j["barrier"] = barrier;
    j["NDT"] = nonDecisionTime;
    j["bias"] = bias;
    j["RT"] = dt.RT;
    j["choice"] = dt.choice;
    j["vl"] = dt.valueLeft;
    j["vr"] = dt.valueRight;
    j["RDVs"] = dt.RDVs;
    j["timeStep"] = dt.timeStep;
    o << std::setw(4) << j << std::endl;        
}

DDMTrial DDM::simulateTrial(int valueLeft, int valueRight, int timeStep, int seed) {
    float RDV = this->bias;
    int time = 0;
    int elapsedNDT = 0;
    int RT;
    int choice;
    float mean;
    std::vector<float>RDVs = {RDV};

    std::random_device rd;
    std::mt19937 gen(seed == -1 ? rd() : seed); 

    while (true) {
        if (RDV >= this->barrier || RDV <= -this->barrier) {
            RT = time * timeStep;
            if (RDV >= this->barrier) {
                choice = -1;
            } else {
                choice = 1;
            }
            break;
        }
        if (elapsedNDT < this->nonDecisionTime / timeStep) {
            mean = 0;
            elapsedNDT += 1;
        }
        else {
            mean = this->d * (valueLeft - valueRight);
        }
        std::normal_distribution<float> dist(mean, this->sigma);
        float inc = dist(gen);
        RDV += inc;
        RDVs.push_back(RDV);
        time += 1;
    }
    DDMTrial trial = DDMTrial(RT, choice, valueLeft, valueRight);
    trial.RDVs = RDVs;
    trial.timeStep = timeStep;
    return trial;
}

void DDMTrial::writeTrialsToCSV(std::vector<DDMTrial> trials, std::string filename) {
    std::ofstream fp;
    fp.open(filename);
    fp << "choice,rt,valueLeft,valueRight\n";
    for (DDMTrial t : trials) {
        fp << t.choice << "," << t.RT << "," << t.valueLeft << "," << t.valueRight << "\n";
    }
    fp.close();
}

std::vector<DDMTrial> DDMTrial::loadTrialsFromCSV(std::string filename) {
    std::vector<DDMTrial> trials; 
    std::ifstream file(filename);
    std::string line;
    std::getline(file, line);
    int choice; 
    int RT; 
    int valDiff;
    int valueLeft; 
    int valueRight; 

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        std::getline(ss, field, ',');
        choice = std::stoi(field);
        std::getline(ss, field, ',');
        RT = std::stoi(field);
        std::getline(ss, field, ',');
        valueLeft = std::stoi(field);
        std::getline(ss, field, ',');
        valueRight = std::stoi(field);
        DDMTrial dt = DDMTrial(RT, choice, valueLeft, valueRight);
        trials.push_back(dt);
    }
    file.close();

    return trials; 
}

MLEinfo<DDM> DDM::fitModelMLE(
    vector<DDMTrial> trials, 
    vector<float> rangeD, 
    vector<float> rangeSigma, 
    bool normalizePosteriors, 
    float barrier, 
    unsigned int nonDecisionTime, 
    vector<float> bias, 
    vector<float> decay) {

    sort(rangeD.begin(), rangeD.end());
    sort(rangeSigma.begin(), rangeSigma.end());
    sort(bias.begin(), bias.end());
    sort(decay.begin(), decay.end());

    std::vector<DDM> potentialModels; 
    for (float d : rangeD) {
        for (float sigma : rangeSigma) {
            for (float b : bias) {
                for (float dec : decay) {
                    DDM ddm = DDM(d, sigma, barrier, nonDecisionTime, b, dec);
                    potentialModels.push_back(ddm);
                } 
            }
        }
    }

    double minNLL = __DBL_MAX__;
    std::map<DDM, ProbabilityData> allTrialLikelihoods;
    std::map<DDM, float> posteriors; 
    double numModels = rangeD.size() * rangeSigma.size() * bias.size() * decay.size(); 

    DDM optimal = DDM(); 
    for (DDM ddm : potentialModels) {
        ProbabilityData aux = ddm.computeGPUNLL(trials);
        if (normalizePosteriors) {
            allTrialLikelihoods.insert({ddm, aux});
            posteriors.insert({ddm, 1 / numModels});
        } else {
            posteriors.insert({ddm, aux.NLL});
        }
        std::cout << "testing d=" << ddm.d << " sigma=" << ddm.sigma; 
        if (bias.size() > 1) {
            std::cout << " bias=" << ddm.bias; 
        } 
        if (decay.size() > 1) {
            std::cout << " decay=" << ddm.decay; 
        }
        std::cout << " NLL=" << aux.NLL << std::endl; 
        if (aux.NLL < minNLL) {
            minNLL = aux.NLL; 
            optimal = ddm; 
        }
    }
    if (normalizePosteriors) {
        for (int tn = 0; tn < trials.size(); tn++) {
            double denominator = 0; 
            for (const auto &ddmPD : allTrialLikelihoods) {
                DDM curr = ddmPD.first; 
                ProbabilityData data = ddmPD.second; 
                double likelihood = data.trialLikelihoods[tn];
                denominator += posteriors[curr] * likelihood; 
            }
            double sum = 0; 
            for (const auto &ddmPD : allTrialLikelihoods) {
                DDM curr = ddmPD.first; 
                ProbabilityData data = ddmPD.second; 
                double prior = posteriors[curr];
                double newLikelihood = data.trialLikelihoods[tn] * prior / denominator; 
                posteriors[curr] = newLikelihood; 
                sum += newLikelihood; 
            }
            if (sum != 1) {
                double normalizer = 1 / sum; 
                for (auto &p : posteriors) {
                    p.second *= normalizer; 
                } 
            }
        }
    }
    MLEinfo<DDM> info;
    info.optimal = optimal; 
    info.likelihoods = posteriors; 
    return info;   
}
