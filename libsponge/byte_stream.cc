#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
//初始化ByteStream
ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity)
    , bstrlen(0)
    , startidx(0)
    , eofidx(0)
    , bswritten(0)
    , bsread(0)
    , bstream(capacity, 0)
    , input_end_flag(false) {
    // bstream.resize(capacity);           //重新设置bstream大小
}
//{DUMMY_CODE(capacity);}

size_t ByteStream::write(const string &data) {
    size_t wrlen = data.size();
    if (bstrlen == _capacity)  // stream full
        return 0;
    if (wrlen > remaining_capacity())  // can't write all the given data
        wrlen = remaining_capacity();

    if ((eofidx + wrlen) > _capacity) {  // eofidx+wrlen超过了_capacity
        int c1 = _capacity - eofidx;
        int c2 = wrlen - c1;
        bstream.replace(eofidx, c1, data.substr(0, c1));
        bstream.replace(0, c2, data.substr(c1, c2));
    } else {
        bstream.replace(eofidx, wrlen, data.substr(0, wrlen));
    }
    bswritten += wrlen;                     // update bswritten
    eofidx = (eofidx + wrlen) % _capacity;  // update eofidx
    bstrlen += wrlen;                       // update bytestream length
    // DUMMY_CODE(data);
    return wrlen;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    std::string res;
    if (eofidx < (startidx + len) % _capacity) {  //
        if (startidx >= eofidx)                   // startidx on the right of the eofidx
        {
            res.append(bstream.substr(startidx));
            res.append(bstream.substr(0, startidx));
        } else  // startidx on the left normal sequence
        {
            res.append(bstream.substr(startidx, bstrlen));
        }
    } else {
        if (startidx >= eofidx) {
            if ((startidx + len) > _capacity) {
                res.append(bstream.substr(startidx));
                res.append(bstream.substr(0, len - (_capacity - startidx)));
            } else
                res.append(bstream.substr(startidx, len));
        } else
            res.append(bstream.substr(startidx, len));
    }
    // DUMMY_CODE(len);
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {  // DUMMY_CODE(len); }
    if (len > bstrlen) {
        startidx = 0;
        eofidx = 0;
        bstrlen = 0;
        bsread += bstrlen;
    } else {
        startidx = (startidx + len) % _capacity;
        bstrlen -= len;
        bsread += len;
    }
    // if(eofidx < (startidx+len)%_capacity) {      //len exceeds the total length of the bytestream, pop all
    //    startidx = 0;
    //    eofidx = 0;
    //    bstrlen = 0;
    //}
    // else {
    //    startidx = (startidx+len)%_capacity;
    //    bstrlen -= len;
    //}
    // bsread += len;
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string res = peek_output(len);
    // std::cout << res << "\n";
    size_t length = res.size();

    pop_output(length);
    // DUMMY_CODE(len);
    return res;
}

void ByteStream::end_input() { input_end_flag = 1; }

bool ByteStream::input_ended() const { return input_end_flag; }

size_t ByteStream::buffer_size() const { return bstrlen; }

bool ByteStream::buffer_empty() const {
    if (bstrlen == 0)
        return true;
    else
        return false;
}

bool ByteStream::eof() const {
    if ((bstrlen == 0) && input_end_flag)
        return true;
    else
        return false;
}

size_t ByteStream::bytes_written() const { return bswritten; }

size_t ByteStream::bytes_read() const { return bsread; }

size_t ByteStream::remaining_capacity() const { return _capacity - bstrlen; }
