#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    //DUMMY_CODE(seg);
    
    const TCPHeader &tcp_header = seg.header();
    if(tcp_header.syn) {
        seq_set = true;
        initial_seq_num = tcp_header.seqno;
    }
    if(!seq_set) return;                    //SYN not set discard segment

    uint64_t abs_ackno = _reassembler.stream_out().bytes_written() + 1;
    uint64_t current_absseq = unwrap(tcp_header.seqno, initial_seq_num, abs_ackno);
    uint64_t index = current_absseq - 1 + (tcp_header.syn);
    _reassembler.push_substring(seg.payload().copy(), index, tcp_header.fin);
    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!seq_set)                //SYN not set
        return nullopt;
    
    uint64_t abs_ack = _reassembler.stream_out().bytes_written() + 1;
    if(_reassembler.stream_out().input_ended())
        ++abs_ack;
    return WrappingInt32(initial_seq_num) + abs_ack;
}
size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
