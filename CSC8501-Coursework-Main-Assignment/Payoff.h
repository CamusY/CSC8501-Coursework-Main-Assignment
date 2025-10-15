#pragma once

namespace ipd {
    struct Payoff {
        double T;
        double R;
        double P;
        double S;

        Payoff(double temptation = 5.0, double reward = 3.0, double punishment = 1.0, double sucker = 0.0);
    };
}