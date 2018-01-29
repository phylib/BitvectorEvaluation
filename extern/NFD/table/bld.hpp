#ifndef BLD_H
#define BLD_H

#include <boost/dynamic_bitset.hpp>

namespace bld {

class BitmaskLoopDetection {
 public:
  BitmaskLoopDetection(unsigned int size) {
    m_size = size;
    boost::dynamic_bitset<> bitset(m_size);
    m_bits = bitset;
    m_highestSequenceNumber = 0;
  }

  int size() const { return m_size; }

  void Add(const unsigned long sequenceNumber) {

    if (m_size <= 1) {
      return;
    }

    if (sequenceNumber < m_highestSequenceNumber) { // Received packet has not the highest seq no
      long pos = m_size - (m_highestSequenceNumber - sequenceNumber);
      if (pos < 0) {
        return;
      }
      m_bits[pos] = 1;


    } else { // Received packet with higher sequence number
      unsigned long positions = sequenceNumber - m_highestSequenceNumber;
      m_bits >>= positions; // Shift bitset
      m_bits[m_size - 1] = 1;
      m_highestSequenceNumber = sequenceNumber;

    }
  }

  bool Contains(const unsigned long sequenceNumber) const {

    if (m_size <= 1) { // No Bitvector used
      return false;
    }

    if (sequenceNumber < (m_highestSequenceNumber - m_size + 1)) { // Already forgot old sequence number
      return false; // Default allow
    }

    if (sequenceNumber > m_highestSequenceNumber) { // New Packet
      return false;
    } else {

      unsigned long pos = m_size - (m_highestSequenceNumber - sequenceNumber);
      if (m_bits[pos - 1]) { // Known packet --> Duplicate
        return true;
      }

    }

    return false; // Old but unkown packet
  }

 public:
  friend std::ostream& operator<< (std::ostream & out, BitmaskLoopDetection const& bld) {
      out << bld.m_bits;
      out << " (highestNum=" << bld.m_highestSequenceNumber << ")";
      return out ;
  }

 private:
  boost::dynamic_bitset<> m_bits;
  unsigned int m_size;
  unsigned long m_highestSequenceNumber;
};

}  // namespace bld

#endif /* BLD_H */
