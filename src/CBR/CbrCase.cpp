#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "CbrCase.h"
#include "Metadata.h"


CbrCase::CbrCase()
{
}

//metadata, startindex, endindex
CbrCase::CbrCase(std::shared_ptr<Metadata> m, int start, int end) :inputStartingIndex(start), inputEndIndex(end), metaData(std::move(m))
{

}

int CbrCase::getStartingIndex() {
	return inputStartingIndex;
}

int CbrCase::getEndIndex() {
	return inputEndIndex;
}


Metadata* CbrCase::getMetadata() {
	return metaData.get();
}

void CbrCase::SetEndIndex(int index) {
	inputEndIndex = index;
}
void CbrCase::SetStartIndex(int index) {
	inputStartingIndex = index;
}

bool CbrCase::getInputBufferSequence() {
	return inputBufferSequence;
}
void CbrCase::setInputBufferSequence(bool b) {
	inputBufferSequence = b;
}
