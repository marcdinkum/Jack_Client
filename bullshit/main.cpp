//
// Created by Dean on 30/10/2022.
//

namespace AdditionTools {
    template <typename T>
    T addOneToInput(T value){
        return value + static_cast<T>(1);
    }
};

#include <vector>
#include <span>

struct SiblingCounter {
    void numSiblings (std::span<float> siblings) {
        nrosiblings = 0;
        for (auto i = 0; i < siblings.size(); ++i){
            nrosiblings = AdditionTools::addOneToInput<int>(nrosiblings);
        }
    }
    int getNumber(){
        return nrosiblings;
    }

private:
    int nrosiblings { 0 };
};


int main(){
    SiblingCounter test;

    std::vector<float> Stuff;
    Stuff.resize(40, 0);
    test.numSiblings(Stuff);

    assert(test.getNumber() == 39);

    return 0;
}