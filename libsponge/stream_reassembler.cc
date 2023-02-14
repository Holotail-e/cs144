#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
                    _output(capacity), 
                    _capacity(capacity), 
                    assembled_segment(capacity, 0), 
                    vflag(capacity, 0), 
                    eof_flag(false), 
                    eof_index(0),
                    unassembled_bytes_cnt(0),
                    assembled_bytes(0), 
                    first_wait_byte_index_relative(0), 
                    first_wait_byte_index(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(eof && index + data.size() <= first_wait_byte_index + _output.remaining_capacity()){
        eof_index = data.size()+index;
        eof_flag = true;
    }
    if(!data.size() && !eof) return;                  //not at eof and datasize == 0
    if(index >= _capacity+first_wait_byte_index) return;                  //outbound first_wait_byte_index first_wait_byte_index+_capacity
    if(index + data.size() > first_wait_byte_index){
        uint64_t sindex = max(first_wait_byte_index, index);
        uint64_t jj = sindex - index;
        unassembled_bytes_cnt += data.size()+index-sindex;
        for(uint64_t j = sindex; j < data.size()+index; j++){
            uint64_t idx = j%_capacity;                         //relative index in the assembled_segment
            if(j-first_wait_byte_index >= _capacity) break;                //_capacity loop once
            if(!vflag[idx]){                                    //not write vflga[idx] == false
                vflag[idx] = true;
                assembled_segment[idx] = data[jj];
            } else unassembled_bytes_cnt--;
            jj++;
        }
        //update vflag and write 
        uint64_t write_bytes{0};
        uint64_t output_remaincap {_output.remaining_capacity()};
        uint64_t tmp {first_wait_byte_index_relative+_capacity};
        for(uint64_t i = first_wait_byte_index_relative; i < tmp; i++){
            if(vflag[i%_capacity] && write_bytes < output_remaincap) {            //vflag[] have been written                        //vflag written update
                write_bytes++;
                vflag[i%_capacity] = false;
                first_wait_byte_index++;
                assembled_bytes++;
                unassembled_bytes_cnt--;
            }
            else break;                                                     //not written
        }
        if(first_wait_byte_index_relative+write_bytes<_capacity) 
            _output.write(std::string(assembled_segment.begin()+first_wait_byte_index_relative, 
                                    assembled_segment.begin()+first_wait_byte_index_relative+write_bytes));
        else
            _output.write(std::string(assembled_segment.begin()+first_wait_byte_index_relative, assembled_segment.end())+
                        std::string(assembled_segment.begin(), assembled_segment.begin()+write_bytes-_capacity+first_wait_byte_index_relative));
        //update first_wait_byte_index_relative    
        first_wait_byte_index_relative = (first_wait_byte_index) % _capacity;
    }
    if(eof_flag && empty()) _output.end_input();
    return;
}

size_t StreamReassembler::unassembled_bytes() const { return unassembled_bytes_cnt; }

bool StreamReassembler::empty() const { return unassembled_bytes_cnt == 0; }

