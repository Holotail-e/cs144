#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
//    DUMMY_CODE(n, isn);
    uint64_t sequence_number = n+isn.raw_value();
    if(sequence_number > 0xffffffff) {
        sequence_number <<= 32;
        sequence_number >>= 32;
    }
    return WrappingInt32{static_cast<uint32_t>(sequence_number)};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    //DUMMY_CODE(n, isn, checkpoint);
    const uint64_t _INT32 = 1l << 32;
    uint32_t nofs = n.raw_value() - isn.raw_value();
    if(checkpoint > nofs) {
        uint64_t rcheckpoint = (checkpoint - nofs) + (_INT32 >> 1);
        uint64_t wrap_num = rcheckpoint / _INT32;
        return wrap_num * _INT32 + nofs;
    }
    else return nofs;
    //uint64_t abs_sequence1 = nraw + (1ul << 32) - isnraw;
    //uint64_t abs_sequence2 = nraw - isnraw;
    //isn64 + n64 > checkpoint

    //if(dis1 > dis2) return abs_sequence2;
    //else return abs_sequence1;
}
