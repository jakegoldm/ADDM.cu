#include <addm/cuda_toolbox.h>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#define CHECK_MESSAGE(cond, msg) do { INFO(msg); CHECK(cond); } while((void)0, 0)

const std::string EXP_DATA = "data/expdata.csv";
const std::string FIX_DATA = "data/fixations.csv"; 
const float ERROR_BOUND = 1.0E-6; 

inline bool within_abs(float f1, float f2, float error) {
    return abs(f1 - f2) < error; 
}


/**
 * @brief Check that input seeds yield consistent results for aDDM::simulateTrial and the 
 * function runs without error. 
 * 
 */
TEST_CASE("aDDM::simulateTrial gives expected Results") {
    aDDM a1 = aDDM(0.005, 0.07, 0.5);
    std::map<int, std::vector<aDDMTrial>> data = loadDataFromCSV(EXP_DATA, FIX_DATA);
    FixationData fixationData = getEmpiricalDistributions(data);
    aDDMTrial t1 = a1.simulateTrial(0, 3, fixationData, 10, 3, {}, {}, 540);

    std::vector<int> expectedItem = {
        0, 2, 0, 1, 0, 2
    };
    std::vector<int> expectedTime = {
        370, 280, 70, 340, 80, 760
    };
    std::vector<float> expectedRDV = {
        0.208652,
        -0.313011,
        -0.131733,
        0.262609,
        0.270378,
        -1.06167
    };

    REQUIRE(t1.RT == 1900);
    REQUIRE(t1.choice == 1);
    REQUIRE(t1.fixItem.size() == t1.fixTime.size());
    REQUIRE(t1.fixTime.size() == t1.fixRDV.size());

    REQUIRE(t1.fixItem == expectedItem);
    REQUIRE(t1.fixTime == expectedTime);
    for (int i = 0; i < t1.fixRDV.size(); i++) {
        REQUIRE(within_abs(t1.fixRDV[i], expectedRDV[i], ERROR_BOUND));
    }
}

/**
 * @brief Confirm that aDDM::fitModelMLE predicts exact parameters for small grid sizes. 
 * 
 */
TEST_CASE("aDDM::fitModelMLE gives expected results") {
    std::map<int, std::vector<aDDMTrial>> data = loadDataFromCSV(EXP_DATA, FIX_DATA);
    FixationData fixationData = getEmpiricalDistributions(data);

    std::vector<float> rangeD = {0.005, 0.009};
    std::vector<float> rangeSigma = {0.05, 0.09};
    std::vector<float> rangeTheta = {0.5, 0.9};

    // Map the correct model to its corresponding trials
    std::map<aDDM, std::vector<aDDMTrial>> datasets; 
    for (float d : rangeD) {
        for (float sigma : rangeSigma) {
            for (float theta : rangeTheta) {
                aDDM addm = aDDM(d, sigma, theta);
                std::vector<aDDMTrial> trials;
                for (int i = 0; i < 1000; i++) {
                    if (i % 2 == 0) {
                        trials.push_back(
                            addm.simulateTrial(5, 2, fixationData, 10, 3, {}, {}, i + 1));
                    } else {
                        trials.push_back(
                            addm.simulateTrial(2, 5, fixationData, 10, 3, {}, {}, i + 1));
                    }
                }
                datasets.insert({
                    addm, 
                    trials
                });
            }
        }
    }

    // Check every dataset of trials and check that the aDDM can recover its paramters
    for (const auto i : datasets) {
        aDDM correct = i.first; 
        MLEinfo<aDDM> info = aDDM::fitModelMLE(
            i.second, rangeD, rangeSigma, rangeTheta, {0});

        CHECK_MESSAGE(info.optimal == correct, 
            "MLE Checks FAILED for {" << correct.d << ", " << 
                correct.sigma << ", " << correct.theta << "}\n" <<
            "info optimal -> {" << info.optimal.d << ", " << 
                info.optimal.sigma << ", " << info.optimal.theta << "}");

        if (info.optimal == correct) {
            std::cout << "MLE Checks passed for {" << correct.d << ", " << 
                correct.sigma << ", " << correct.theta << "}" << std::endl; 
        } 
    }
}
