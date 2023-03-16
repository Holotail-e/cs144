#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timeout(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _abs_ackno; }

// 从 TCPReceiver 中可以看出，它的框架中，在一个 TCP Segment 中，如果 syn 为 true，这个 segment 也是可以带有数据的
// fill_window 的作用是将 ByteStream 中的数据发送出去
// 需要注意的是，fill_window() 也承担了发送 syn 和 fin 的任务，
// 因此 fill_window 函数也需要对当前的状况进行判断，而后决定是否将相应的标志位设置为 set
void TCPSender::fill_window() {
    if (_fin_flag == true)
        return ;
    uint16_t window_size = _window_size > 0 ? _window_size : 1;
    bool sent = false;
    // 这两个要拿出来而不是放在循环里面是因为：当 _stream.buffer_empty() 为 true 的时候，我们也可能需要发送 SYN 或者 FIN 的包
    if (_syn_flag == false){
        TCPSegment seg;
        seg.header().syn = true;
        _syn_flag = true;
        seg.header().seqno = next_seqno();
        _segment_sent.push(sender_in_queue(seg, _next_seqno));
        _next_seqno++;
        _segments_out.push(seg);
        sent = true;
    }
    // 第二个确认条件是为了保证还能放得下 FIN，毕竟 FIN 也是要占用一个 seqno 的
    if (_stream.eof() && _next_seqno - _abs_ackno < window_size){
        TCPSegment seg;
        seg.header().fin = true;
        _fin_flag = true;
        seg.header().seqno = next_seqno();
        _segment_sent.push(sender_in_queue(seg, _next_seqno));
        _next_seqno++;
        _segments_out.push(seg);
        sent = true;
    }
    // 第二个确认条件与上面同理
    while (!_stream.buffer_empty() && _abs_ackno + window_size > _next_seqno) {
        TCPSegment seg;
        seg.header().seqno = next_seqno();
        const size_t payload_size =
            min(TCPConfig::MAX_PAYLOAD_SIZE, window_size - (_next_seqno - _abs_ackno) - seg.header().syn);
        seg.payload() = _stream.read(min(payload_size, _stream.buffer_size()));
        if (_stream.eof() && seg.payload().size() + _next_seqno - _abs_ackno < window_size){
            _fin_flag = seg.header().fin = true;
        }
        _segment_sent.push(sender_in_queue(seg, _next_seqno));
        _segments_out.push(seg);
        _next_seqno += seg.length_in_sequence_space();
        sent = true;
        if (seg.header().fin)
            break;
    }
    // 如果发送了，并且原来计时器没有启动则启动计时器
    if (_timer_flag == false && sent == true){
        _timer_flag = true;
        _timer = 0;
    }
}

void TCPSender::send_segment(TCPSegment seg) {
    DUMMY_CODE(seg);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    // 判断是否在相应的范围内，如果不在的话说明要么收到了非法的，直接返回就好了
    // 需要注意的是，这里的第一个判断条件是大于等于，因为对方可能并没有收到新的数据，但重新发了一个包告知我方窗口更新了
    if (abs_ackno >= _abs_ackno && abs_ackno <= _next_seqno) {
        _window_size = window_size;
        _abs_ackno = abs_ackno;
    }
    else
        return ;
    while (!_segment_sent.empty()) {
        TCPSegment seg = _segment_sent.front().seg;
        if (abs_ackno < _segment_sent.front().abs_seqno + seg.length_in_sequence_space())
            break;
        _segment_sent.pop();
        _retransmission_timeout = _initial_retransmission_timeout;
        _timer = 0;
        _consecutive_retransmissions = 0;
    }
    fill_window();
    // 特别需要注意，这里需要判断计时器是否需要启动
    _timer_flag = !_segment_sent.empty();;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_syn_flag == false || _timer_flag == false)
        return ;
    _timer += ms_since_last_tick;
    if (_timer >= _retransmission_timeout){
        TCPSegment seg = _segment_sent.front().seg;
        _segments_out.push(seg);
        _timer = 0;
        // 我认为有可能是因为在接收端的 window size 为 0，因此没有接收，也没有收到 ack ，因此我们无需将 RTO * 2
        // 这也从某种程度上解释了一开始的 window_size 是不能设置成 0 的
        if (_window_size > 0) {
            _retransmission_timeout = _retransmission_timeout * 2;
        }
        _consecutive_retransmissions++;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
