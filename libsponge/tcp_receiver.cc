#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    if (_syn_flag == false){
        if (header.syn == false)
            return ;
        _syn_flag = true;
        _isn = header.seqno;
        // 这里不能直接 return，它的测试数据中 syn 为 true 时是可以带有数据的
        //return;
    }
    string data = seg.payload().copy();
    _check_point = unwrap(header.seqno, _isn, _check_point);
    // 这里当 header.syn 为 true 时也要占据一个 seqno，而 push_substring 中的 index 是 stream index 而不是 absolute seqno
    // 因此，当 syn 为 true 时不需要减一，而当 syn 为 false 时需要减一
    size_t index = header.syn ? _check_point : _check_point - 1;
    _reassembler.push_substring(data, index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn_flag == false)
        return nullopt;
    uint64_t abs_ackno = _reassembler.stream_out().bytes_written() + 1;
    // 这里当 header.fin 为 true 时也要占据一个 seqno ，因此有下面的判断，并且要加一
    if (_reassembler.stream_out().input_ended() == true)
        abs_ackno++;
    return _isn + abs_ackno; // add the syn
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
