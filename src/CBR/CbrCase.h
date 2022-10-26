#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "Metadata.h"
#include <array>

class CbrCase {
private:
    friend class boost::serialization::access;
    std::shared_ptr<Metadata>metaData;
    int inputStartingIndex;
    int inputEndIndex;

public:
    template<class Archive>
    void serialize(Archive& a, const unsigned version) {
        a& metaData& inputStartingIndex & inputEndIndex;
    }
    CbrCase();
    CbrCase(std::shared_ptr<Metadata>, int, int);

    Metadata* getMetadata();
    int getStartingIndex();
    int getEndIndex();
    void SetEndIndex(int index);
    void SetStartIndex(int index);

    
};
