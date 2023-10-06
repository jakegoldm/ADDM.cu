#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <map>
#include <ctime>
#include <time.h>
#include <cstdlib>
#include <random> 
#include "ddm.h"
#include "util.h"
#include "addm.h"
#include "stats.h"


FixationData::FixationData(float probFixLeftFirst, std::vector<int> latencies, 
    std::vector<int> transitions, fixDists fixations) {

    this->probFixLeftFirst = probFixLeftFirst;
    this->latencies = latencies;
    this->transitions = transitions;
    this->fixations = fixations;
}


aDDMTrial::aDDMTrial(
    unsigned int RT, int choice, int valueLeft, int valueRight, 
    std::vector<int> fixItem, std::vector<int> fixTime, 
    std::vector<float> fixRDV, float uninterruptedLastFixTime) :
    DDMTrial(RT, choice, valueLeft, valueRight) {
        this->fixItem = fixItem;
        this->fixTime = fixTime;
        this->fixRDV = fixRDV;
        this->uninterruptedLastFixTime = uninterruptedLastFixTime;
}


aDDM::aDDM(float d, float sigma, float theta, float k, float barrier, 
    unsigned int nonDecisionTime, float bias, float decay) : 
    DDM(d, sigma, barrier, nonDecisionTime, bias, decay) {
        this->theta = theta;
        this->k = k; 
}

void aDDM::exportTrial(aDDMTrial adt, std::string filename) {
    std::ofstream o(filename);
    json j;
    j["d"] = d;
    j["sigma"] = sigma;
    j["theta"] = theta;
    j["k"] = k; 
    j["barrier"] = barrier;
    j["NDT"] = nonDecisionTime;
    j["bias"] = bias;
    j["RT"] = adt.RT;
    j["choice"] = adt.choice;
    j["vl"] = adt.valueLeft;
    j["vr"] = adt.valueRight;
    j["RDVs"] = adt.RDVs;
    j["fixItem"] = adt.fixItem;
    j["fixTime"] = adt.fixTime;
    j["timeStep"] = adt.timeStep;
    o << std::setw(4) << j << std::endl;        
}


aDDMTrial aDDM::simulateTrial(
    int valueLeft, int valueRight, FixationData fixationData, int timeStep, 
    int numFixDists, fixDists fixationDist, vector<int> timeBins, int seed) {

    std::vector<int> fixItem;
    std::vector<int> fixTime;
    std::vector<float> fixRDV;

    // If no seed is provided (default -1), use a random generator. Otherwise, use the seed
    std::random_device rd;
    std::mt19937 gen(seed == -1 ? rd() : seed); 

    float RDV = this->bias;
    int time = 0;
    int choice; 
    int uninterruptedLastFixTime;
    int RT;

    std::vector<float>RDVs = {RDV};

    std::uniform_int_distribution<std::size_t> ludist(0, fixationData.latencies.size() - 1);
    int rIDX = ludist(gen);
    int latency = fixationData.latencies.at(rIDX);
    int remainingNDT = this->nonDecisionTime - latency;

    // Iterate over latency
    for (int t = 0; t < latency / timeStep; t++) {
        std::normal_distribution<float> ndist(0, this->sigma);
        float inc = ndist(gen);
        RDV += inc;
        RDVs.push_back(RDV);

        if(RDV >= this->barrier || RDV <= -this->barrier) {
            if (RDV >= this->barrier) {
                choice = -1;
            } else {
                choice = 1; 
            }
            fixRDV.push_back(RDV);
            fixItem.push_back(0);
            int dt = (t + 1) * timeStep;
            fixTime.push_back(dt);
            time += dt;
            RT = time;
            uninterruptedLastFixTime = latency;
            return aDDMTrial(
                RT, choice, valueLeft, valueRight, 
                fixItem, fixTime, fixRDV, uninterruptedLastFixTime);
        }
    }

    // Add latency to this trial's data
    fixRDV.push_back(RDV);
    RDVs.push_back(RDV);
    fixItem.push_back(0);
    int dt = latency - (latency % timeStep);
    fixTime.push_back(dt);
    time += dt;

    int fixNumber = 1;
    int prevFixatedItem = -1;
    int currFixLocation = 0;
    bool decisionReached = false;
    float currFixTime;

    while (true) {
        if (currFixLocation == 0) {
            // Sample based off of item location
            if (prevFixatedItem == -1) {
                std::discrete_distribution<> ddist({fixationData.probFixLeftFirst, 1 - fixationData.probFixLeftFirst});
                currFixLocation = ddist(gen) + 1;
            } else if (prevFixatedItem == 1) {
                currFixLocation = 2;
            } else if (prevFixatedItem == 2) {
                currFixLocation = 1;
            }
            prevFixatedItem = currFixLocation;
            if (fixationDist.empty()) {
                vector<float> fixTimes = fixationData.fixations.at(fixNumber);
                std::uniform_int_distribution<std::size_t> fudist(0, fixTimes.size() - 1);
                rIDX = fudist(gen);
                currFixTime = fixTimes.at(rIDX);
            }
            if (fixNumber < numFixDists) {
                fixNumber++;
            }
        }
        // Transition
        else {
            currFixLocation = 0;
            rIDX = rand() % fixationData.transitions.size();
            currFixTime = fixationData.transitions.at(rIDX);
        }
        // Iterate over any remaining non-decision time 
        if (remainingNDT > 0)  {
            for (int t = 0; t < remainingNDT / timeStep; t++) {
                std::normal_distribution<float> ndist(0, this->sigma);
                float inc = ndist(gen);
                RDV += inc;
                RDVs.push_back(RDV);

                if(RDV >= this->barrier || RDV <= -this->barrier) {
                    if (RDV >= this->barrier) {
                        choice = -1;
                    } else {
                        choice = 1; 
                    }
                    fixRDV.push_back(RDV);
                    fixItem.push_back(currFixLocation);
                    int dt = (t + 1) * timeStep;
                    fixTime.push_back(dt);
                    time += dt;
                    RT = time;
                    uninterruptedLastFixTime = currFixTime;
                    decisionReached = true;
                    break;
                }
            }
        }
        if (decisionReached) {
            break;
        }
        float remainingFixTime = max(0.0f, currFixTime - max(0, remainingNDT));
        remainingNDT -= currFixTime;

        for (int t = 0; t < round(remainingFixTime / timeStep); t++) {
            float mean;
            if (currFixLocation == 0) {
                mean = 0;
            } else if (currFixLocation == 1) {
                mean = this->d * ((valueLeft + this->k) - (this->theta * valueRight));
            } else if (currFixLocation == 2) {
                mean = this->d * ((this->theta * valueLeft) - (valueRight + this->k));
            }
            std::normal_distribution<float> ndist(mean, this->sigma);
            float inc = ndist(gen);
            RDV += inc;
            RDVs.push_back(RDV); 

            if(RDV >= this->barrier || RDV <= -this->barrier) {
                if (RDV >= this->barrier) {
                    choice = -1;
                } else {
                    choice = 1; 
                }
                fixRDV.push_back(RDV);
                fixItem.push_back(currFixLocation);
                int dt = (t + 1) * timeStep;
                fixTime.push_back(dt);
                time += dt;
                RT = time;
                uninterruptedLastFixTime = currFixTime;
                decisionReached = true;
                break;
            }                
        }

        if (decisionReached) {
            break;
        }

        fixRDV.push_back(RDV);
        fixItem.push_back(currFixLocation);
        int cft = round(currFixTime);
        int dt = cft - (cft % timeStep);
        fixTime.push_back(dt);
        time += dt;
    } 

    aDDMTrial trial = aDDMTrial(RT, choice, valueLeft, valueRight, fixItem, fixTime, fixRDV, uninterruptedLastFixTime);
    trial.RDVs = RDVs;
    trial.timeStep = timeStep;
    return trial;
}


void aDDMTrial::writeTrialsToCSV(std::vector<aDDMTrial> trials, string filename) {
    std::ofstream fp;
    fp.open(filename);
    fp << "trial,choice,rt,valueLeft,valueRight,fixItem,fixTime\n";
    int id = 0; 

    for (aDDMTrial adt : trials) {
        assert(adt.fixItem.size() == adt.fixTime.size());
        for (int i = 0; i < adt.fixItem.size(); i++) {
            fp << id << "," << adt.choice << "," << adt.RT << "," << 
                adt.valueLeft << "," << adt.valueRight << "," <<
                adt.fixItem[i] << "," << adt.fixTime[i] << "\n";
        }
        id++;
    }
    fp.close();    
}


vector<aDDMTrial> aDDMTrial::loadTrialsFromCSV(string filename) {
    std::vector<aDDMTrial> trials; 
    std::vector<aDDM> addms;
    std::ifstream file(filename);
    std::string line;
    std::getline(file, line);

    int ID;
    int choice; 
    int RT; 
    int valueLeft;
    int valueRight;
    int prevID;
    int fItem;
    int fTime; 
    bool firstIter = true; 
    std::vector<int> fixItem;
    std::vector<int> fixTime; 

    aDDMTrial adt = aDDMTrial();
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        std::getline(ss, field, ',');
        ID = std::stoi(field);
        std::getline(ss, field, ',');
        choice = std::stoi(field);
        std::getline(ss, field, ',');
        RT = std::stoi(field);
        std::getline(ss, field, ',');
        valueLeft = std::stoi(field);
        std::getline(ss, field, ',');
        valueRight = std::stoi(field);
        std::getline(ss, field, ',');
        fItem = std::stoi(field);
        std::getline(ss, field, ',');
        fTime = std::stoi(field);
        if (ID == prevID && !firstIter) {
            adt.fixItem.push_back(fItem);
            adt.fixTime.push_back(fTime);
        } else {
            if (firstIter) {
                firstIter = false; 
            } else {
                trials.push_back(adt);
            }
            adt = aDDMTrial(RT, choice, valueLeft, valueRight);
            adt.fixItem.push_back(fItem);
            adt.fixTime.push_back(fTime);

        }
        prevID = ID;
    }
    trials.push_back(adt);
    file.close();
    return trials;
}


MLEinfo<aDDM> aDDM::fitModelMLE(
    std::vector<aDDMTrial> trials, 
    std::vector<float> rangeD, 
    std::vector<float> rangeSigma, 
    std::vector<float> rangeTheta, 
    std::vector<float> rangeK,
    bool normalizePosteriors,
    float barrier,
    unsigned int nonDecisionTime,
    std::vector<float> bias, 
    std::vector<float> decay) {

    sort(rangeD.begin(), rangeD.end());
    sort(rangeSigma.begin(), rangeSigma.end());
    sort(rangeTheta.begin(), rangeTheta.end()); 
    sort(rangeK.begin(), rangeK.end());
    sort(bias.begin(), bias.end());
    sort(decay.begin(), decay.end());

    std::vector<aDDM> potentialModels; 
    for (float d : rangeD) {
        for (float sigma : rangeSigma) {
            for (float theta : rangeTheta) {
                for (float k : rangeK) {
                    for (float b : bias) {
                        for (float dec : decay) {
                            aDDM addm = aDDM(d, sigma, theta, k, barrier, nonDecisionTime, b, dec);
                            potentialModels.push_back(addm);
                        }
                    }
                }
            }
        }
    }
    
    double minNLL = __DBL_MAX__; 
    std::map<aDDM, ProbabilityData> allTrialLikelihoods; 
    std::map<aDDM, float> posteriors; 
    double numModels = rangeD.size() * rangeSigma.size() * rangeTheta.size() * bias.size() * decay.size();

    aDDM optimal = aDDM(); 
    for (aDDM addm : potentialModels) {
        ProbabilityData aux = addm.computeGPUNLL(trials);
        if (normalizePosteriors) {
            allTrialLikelihoods.insert({addm, aux});
            posteriors.insert({addm, 1 / numModels});
        } else {
            posteriors.insert({addm, aux.NLL});
        }

        std::cout << "testing d=" << addm.d << " sigma=" << addm.sigma << " theta=" << addm.theta; 
        if (rangeK.size() > 1) {
            std::cout << " k=" << addm.k; 
        }
        if (bias.size() > 1) {
            std::cout << " bias=" << addm.bias; 
        } 
        if (decay.size() > 1) {
            std::cout << " decay=" << addm.decay; 
        }
        std::cout << " NLL=" << aux.NLL << std::endl; 
        if (aux.NLL < minNLL) {
            minNLL = aux.NLL; 
            optimal = addm; 
        }
    }
    if (normalizePosteriors) {
        for (int tn = 0; tn < trials.size(); tn++) {
            double denominator = 0; 
            for (const auto &addmPD : allTrialLikelihoods) {
                aDDM curr = addmPD.first; 
                ProbabilityData data = addmPD.second; 
                double likelihood = data.trialLikelihoods[tn];
                denominator += posteriors[curr] * likelihood; 
            }
            double sum = 0; 
            for (const auto &addmPD : allTrialLikelihoods) {
                aDDM curr = addmPD.first; 
                ProbabilityData data = addmPD.second; 
                double prior = posteriors[curr];
                double newLikelihoood = data.trialLikelihoods[tn] * prior / denominator; 
                posteriors[curr] = newLikelihoood; 
                sum += newLikelihoood;
            }
            if (sum != 1) {
                double normalizer = 1 / sum; 
                for (auto &p : posteriors) {
                    p.second *= normalizer; 
                }
            }
        }
    }
    MLEinfo<aDDM> info;
    info.optimal = optimal; 
    info.likelihoods = posteriors; 
    return info;   
}

