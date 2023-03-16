#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

// 一个很重要的点在于，我在 _sender 的 fill_window 函数中已经处理了 什么时候该发送 fin 或者 syn，因此这里无需再去进行处理
void TCPConnection::segment_received(const TCPSegment &seg) {
    // 如果没有建立连接，却收到了 syn 为 false 的包，则直接忽视，这样写是因为测试样例
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::LISTEN &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED &&
        seg.header().syn == false) {
        return;
    }
    _time_since_last_segment_received = 0;
    if (seg.header().rst){
        set_error_state();
        return ;
    }

    _receiver.segment_received(seg);

    if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0)
        and seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
        add_ackno_and_window_size();
        return ;
    }

    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    _sender.fill_window();
    bool sent_empty_segment = false;
    if (_sender.segments_out().empty()) // 说明不能捎带确认，需要我们再发一个空的 ack 包
        sent_empty_segment = true;
    if (sent_empty_segment == true && seg.length_in_sequence_space() > 0) {
        _sender.send_empty_segment();
    }
    add_ackno_and_window_size();


    if (!_sender.stream_in().eof() && _receiver.stream_out().input_ended())
        _linger_after_streams_finish = false;
    // 前三个条件都满足，即处于 TIME_WAIT 状态
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED &&
        !_linger_after_streams_finish) {
        _active = false;
        return;
    }
}

bool TCPConnection::active() const { return _active; }

//! \brief Write data to the outbound byte stream, and send it over TCP if possible
//! \returns the number of bytes from `data` that were actually written.
size_t TCPConnection::write(const string &data) {
    size_t byte = _sender.stream_in().write(data);
    _sender.fill_window();
    add_ackno_and_window_size();
    return byte;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    // 在 _sender 调用了 tick 函数后，可能会重新发送一些包，因此必须进行处理
    add_ackno_and_window_size();
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        send_rst_segment();
        set_error_state();
        return;
    }

    // 对于处于 TIME_WAIT 状态，应该关闭
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED &&
        _linger_after_streams_finish &&
        _time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
        _active = false;
        _linger_after_streams_finish = false;
    }
}

//! \brief Shut down the outbound byte stream (still allows reading incoming data)
void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    // 这里非常重要，我们需要在结束 outbound ByteStream 后再发送 FIN
    _sender.fill_window();
    add_ackno_and_window_size();
}

//! \brief Initiate a connection by sending a SYN segment
void TCPConnection::connect() {
    _active = true;
    // 直接调用 fill_window() 函数就可以了，它会在连接还没有建立的时候会发送 syn
    _sender.fill_window();
    add_ackno_and_window_size();
}

// 将 sender 发送队列的包取出，加上 ack 和 window size 并且放在 connection 的队列中
void TCPConnection::add_ackno_and_window_size() {
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()){
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        seg.header().win = _receiver.window_size();
        _segments_out.push(seg);
    }
}

void TCPConnection::send_rst_segment() {
    // 确保前面没有在 sender 队列中的包
    // 不直接调用 add_ackno_and_window_size() 解决是因为已经发送 rst 了，因此前面的包也比较无关紧要了
    // 而且，直接调用 add_ackno_and_window_size() 会出错
    while (_segments_out.empty() == false)
        _segments_out.pop();
    _sender.send_empty_segment();
    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    seg.header().rst = true;
    _segments_out.push(seg);

}

void TCPConnection::set_error_state() {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _linger_after_streams_finish = false;
    _active = false;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            send_rst_segment();
            set_error_state();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
