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
}


size_t ByteStream::write(const string &data) {
    if(input_end_flag || !(bstrlen ^ _capacity)) return 0;
    uint64_t wrlen = data.size();
    if (!(bstrlen ^ _capacity)) return 0;  // stream full
    wrlen = min(remaining_capacity(), wrlen);// can't write all the given data

    if(eofidx > startidx) {                //eofidx on the right side of the startdix
        if(_capacity - eofidx > wrlen) bstream.replace(eofidx, wrlen, data);        //the right side has adquate space for data being stored
        else {
            uint64_t c1 = _capacity - eofidx;
            uint64_t c2 = wrlen - c1;
            bstream.replace(eofidx, c1, data.substr(0, c1));
            bstream.replace(0, c2, data.substr(c1, c2));
        }
    }   
    else {                                  //eofidx on the left
        bstream.replace(eofidx, wrlen, data.substr(0, wrlen));
    }
    bswritten += wrlen;                     // update bswritten
    bstrlen += wrlen;                       // update bytestream length
    eofidx = (eofidx + wrlen) % _capacity;  // update eofidx
    return wrlen;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    if(len == 0) return "";
    uint64_t rlen = min(len, bstrlen);         
    if(eofidx > startidx) return bstream.substr(startidx, rlen);
    else if((eofidx == startidx && bstrlen) || (eofidx < startidx)) {
        if(startidx + rlen < _capacity) return bstream.substr(startidx, rlen);
        else return bstream.substr(startidx).append(bstream.substr(0,startidx+rlen-_capacity));
    }
    else return "";
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {  // DUMMY_CODE(len); }
    if (len >= bstrlen) {
        startidx = 0;
        eofidx = 0;
        bsread += bstrlen;
        bstrlen = 0;
    } else {
        startidx = (startidx + len) % _capacity;
        bstrlen -= len;
        bsread += len;
    }
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    uint64_t rlen = min(len, bstrlen);
    std::string res(this->peek_output(rlen));
    this->pop_output(rlen);
    return res;
}

void ByteStream::end_input() { input_end_flag = 1; return; }

bool ByteStream::input_ended() const { return input_end_flag; }

size_t ByteStream::buffer_size() const { return bstrlen; }

bool ByteStream::buffer_empty() const { return bstrlen == 0; }

bool ByteStream::eof() const { return bstrlen == 0 && input_end_flag; }

size_t ByteStream::bytes_written() const { return bswritten; }

size_t ByteStream::bytes_read() const { return bsread; }

size_t ByteStream::remaining_capacity() const { return _capacity - bstrlen; }
