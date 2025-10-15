#include "Payoff.h"

#include <cstdlib>
#include <iostream>

namespace ipd {
    Payoff::Payoff(double temptation, double reward, double punishment, double sucker)
        : T(temptation), R(reward), P(punishment), S(sucker) {
        const bool valid = (T > R) && (R > P) && (P > S) && (2.0 * R > T + S);
        if (!valid) {
            std::cerr << "Error: invalid payoffs. Must satisfy T>R>P>S and 2R>T+S." << '\n';
            std::exit(1);
        }
    }
}