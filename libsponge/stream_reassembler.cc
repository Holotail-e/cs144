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
                    _map_block(),
                    _first_unaccepted(capacity) {}

void StreamReassembler::block_insert(uint64_t startindex, uint64_t endindex, uint64_t datasize, const std::string &data){
    Segnode nt {startindex, endindex, datasize, data};
    _map_block.insert({startindex, nt});

    _unassembled_count += datasize;
    return;
}



//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //eof data come set eof_flag true
    if(eof) {
        eof_flag = true;
        _eof_index = data.size() + index;
    }
    if(!data.size() && !eof) return;
    //if(!data.size()) goto eof_empty;
    uint64_t data_last_index = index+data.size();
    //data too new / data too old -- drop
    if((index >= _first_unaccepted || data_last_index <= _first_unassembled) && data.size()) return;
    
    //uint64_t buf_data_last_index = min(_first_unaccepted, data_last_index);
    uint64_t iterindex = index;             
    //uint64_t dataoffset {0};
    
    //insert data into _map_block

    //_map_block empty insert data
    if(_map_block.empty()){
        //index on the _first_unassembled right side

        // uint64_t opz = ((index>=_first_unassembled) << 1) + (index+data.size()<=_first_unaccepted);
        // switch(opz){
        //     case 0:
        //         block_insert(_first_unassembled, _first_unaccepted, _first_unaccepted-_first_unassembled, data.substr(_first_unassembled-index,_first_unaccepted-_first_unassembled));
        //         iterindex = _first_unassembled;
        //         break;
        //     case 1:
        //         block_insert(_first_unassembled, index+data.size(), data_last_index-_first_unassembled, data.substr(_first_unassembled-index));
        //         iterindex = _first_unassembled;
        //         break;
        //     case 2:
        //         block_insert(index, _first_unaccepted, _first_unaccepted-index, data.substr(0,_first_unaccepted-index));
        //         break;
        //     case 3:
        //         block_insert(index, data_last_index, data.size(), data);
        //         break;
        //     default:
        //         break;
        // }

        if(index>=_first_unassembled){
            //data last byte on the left side of the _first_unaccepted. data is within the range of the window accept all data bytes
            if(index+data.size()<=_first_unaccepted) block_insert(index, data_last_index, data.size(), data);
            //data last byte index on the right side of the _first_unaccepted
             else block_insert(index, _first_unaccepted, _first_unaccepted-index, data.substr(0,_first_unaccepted-index));
            
        }
        //index on the left side
        else{
            //data last byte index on the left side of the _first_unaccepted
            if(index+data.size()<=_first_unaccepted){
                block_insert(_first_unassembled, index+data.size(), data_last_index-_first_unassembled, data.substr(_first_unassembled-index));
                iterindex = _first_unassembled;
            //data last byte index on the right side of the _first_unaccepted drop exceeding part
            //insert byte length == _first_unaccepted-_first_unassembled
            } else {
                block_insert(_first_unassembled, _first_unaccepted, _first_unaccepted-_first_unassembled, data.substr(_first_unassembled-index,_first_unaccepted-_first_unassembled));
                iterindex = _first_unassembled;
            }
        }

    }
    auto mbend = _map_block.end();
    //_map_block not empty
    for(auto i = _map_block.begin(); i != mbend; i++){
        
        if(iterindex >= data_last_index) break;
        //string already contained continue
        if(i->first == iterindex) { iterindex = i->second.last_index; continue; }//dataoffset += i->second.slength;continue; }
        uint64_t opt = ((i->first >= data_last_index) << 2)+((_first_unassembled >= iterindex) << 1)+(i->first > iterindex);
        //certain block on the right of the iterindex
        switch(opt){
            // 000 i->first < data_last_index _first_unassembled >= iterindex i->first <= iterindex
            // case 0:
            // break;
            // 001 i->first <  data_last_index _first_unassembled < iterindex i->first > iterindex
            case 1:
                block_insert(iterindex, i->first, i->first-iterindex, data.substr(iterindex-index, i->first-iterindex));
                iterindex = i->second.last_index;
                break;
            // 010 i->first < data_last_index _first_unassembled >= iterindex i->first <= iterindex
            // case 2:
            // break;
            // 011 i->first < data_last_index _first_unassembled >= iterindex i->first > iterindex
            case 3:
                block_insert(_first_unassembled, i->first, i->first-_first_unassembled, data.substr(_first_unassembled-index, i->first-_first_unassembled));
                iterindex = i->second.last_index;
                break;
            // 100 i->first >= data_last_index _first_unassembled < iterindex i->first <= iterindex
            // case 4:
            // break;
            // 101 i->first >= data_last_index _first_unassembled < iterindex i->first > iterindex
            case 5:
                block_insert(iterindex, data_last_index, data_last_index-iterindex, data.substr(iterindex-index));
                goto bp;
            // 110 i->first >= data_last_index _first_unassembled >= iterindex i->first <= iterindex
            // case 6:
            // break;
            // 111 i->first >= data_last_index _first_unassembled >= iterindex i->first > iterindex
            case 7:
                block_insert(_first_unassembled, data_last_index, data_last_index-_first_unassembled, data.substr(_first_unassembled-index));
                goto bp;
            default:
                if(i->second.last_index >= iterindex) { iterindex = i->second.last_index; }
                break;

        }

        // if(iterindex >= data_last_index) break;
        // //string already contained continue
        // if(i->first == iterindex) { iterindex = i->second.last_index; continue; }//dataoffset += i->second.slength;continue; }
        // //certain block on the right of the iterindex
        // if(i->first >= iterindex){
        //     //the rest part of data can't overlap the block part insert the rest
        //     if(i->first >= data_last_index){
        //         if(_first_unassembled >= iterindex){
        //             //_first_unassembled, 
        //             block_insert(_first_unassembled, data_last_index, data_last_index-_first_unassembled, data.substr(_first_unassembled-index));
        //         } else {
        //             block_insert(iterindex, data_last_index, data_last_index-iterindex, data.substr(iterindex-index));
        //         }
        //         break;
        //     }
        //     //the rest part of data overlap the block part
        //     //clip the rest data and update iterindex dataoffset
        //     else{
        //         if(_first_unassembled >= iterindex){
        //             //_first_unassembled, 
        //             block_insert(_first_unassembled, i->first, i->first-_first_unassembled, data.substr(_first_unassembled-index, i->first-_first_unassembled));
        //             //dataoffset += i->second.last_index-iterindex;
        //             iterindex = i->second.last_index;               //update iterindex
        //         } else {
        //             //clip data iterindex~i->first
        //             block_insert(iterindex, i->first, i->first-iterindex, data.substr(iterindex-index, i->first-iterindex));
        //             //dataoffset += i->second.last_index-iterindex;
        //             iterindex = i->second.last_index;               //update iterindex
        //         }
        //     }
        // } else {
        //     if(i->second.last_index >= iterindex) { iterindex = i->second.last_index; }
        // }
        //block i on the left of the iterindex
    } 
    bp:
    auto end_p = _map_block.end();
    //reach the map_block.end()
    if((iterindex >= ((--end_p)->second).last_index) && (iterindex < data_last_index)){
        //data last byte index within the range
        if(data_last_index <= _first_unaccepted){
            block_insert(iterindex, data_last_index, data_last_index-iterindex, data.substr(iterindex-index));
        }
        //data last byte out range
        else{
            block_insert(iterindex, _first_unaccepted, _first_unaccepted-iterindex, data.substr(iterindex-index, _first_unaccepted-iterindex));
        }
    }
    //the most number that _output can be written in.

    //check assembled bytes iterate _map_block from the start 
    auto search = _map_block.begin();
    while(search != _map_block.end()){
        //_map_block first 
        uint64_t remain_cap {_output.remaining_capacity()};
        if(search->second.start_index == _first_unassembled){
            if(search->second.slength <= remain_cap){                   //can write all the block string
                _output.write(search->second.subsegment);
                _first_unassembled += search->second.slength;           //move first_unassembled
                _first_unaccepted += search->second.slength;            //move first_unaccepted
                _unassembled_count -= search->second.slength;
                auto era_search = search;
                search++;
                _map_block.erase(era_search);                               //delete block
            } else {                                                       //can't write all the string in the block
                _output.write(search->second.subsegment.substr(0, remain_cap));
                search->second.subsegment = search->second.subsegment.substr(remain_cap);
                search->second.slength -= remain_cap;
                search->second.start_index += remain_cap;
                _first_unassembled += remain_cap;
                _first_unaccepted += remain_cap;
                _unassembled_count -= remain_cap;
                break;
            }
        }
        else break;
    }

    //eof_flag set and no ubassembeld bytes
    //eof_empty:
    if(eof_flag && empty()) _output.end_input();
    return;
    // if(eof && index + data.size() <= first_wait_byte_index + _output.remaining_capacity()){
    //     eof_index = data.size()+index;
    //     eof_flag = true;
    // }
    // if(!data.size() && !eof) return;                  //not at eof and datasize == 0
    // if(index >= _capacity+first_wait_byte_index) return;                  //outbound first_wait_byte_index first_wait_byte_index+_capacity
    // if(index + data.size() > first_wait_byte_index){
    // uint64_t sindex = max(first_wait_byte_index, index);
    // uint64_t jj = sindex - index;
    
    // unassembled_bytes_cnt += data.size()+index-sindex;

    // for(uint64_t j = sindex; j < data.size()+index; j++){
    //     uint64_t idx = j%_capacity;                         //relative index in the assembled_segment
    //     //uint64_t idx2 = (j+1)%_capacity;
    //     if(j-first_wait_byte_index >= _capacity) break;                //_capacity loop once
    //     if(!vflag[idx]){                                    //not write vflga[idx] == false
    //         vflag[idx] = true;
    //         assembled_segment[idx] = data[jj];
    //     } else unassembled_bytes_cnt--;
    //     jj++;

    // }

    // //update vflag and write 
    // uint64_t write_bytes{0};
    // uint64_t output_remaincap {_output.remaining_capacity()};
    // uint64_t tmp {first_wait_byte_index_relative+_capacity};
    // for(uint64_t i = first_wait_byte_index_relative; i < tmp; i++){
    //     if(vflag[i%_capacity] && write_bytes < output_remaincap) {            //vflag[] have been written                        //vflag written update
    //         write_bytes++;
    //         vflag[i%_capacity] = false;
    //         first_wait_byte_index++;
    //         assembled_bytes++;
    //         unassembled_bytes_cnt--;
    //     }
    //     else break;                                                     //not written
    // }
    
    // if(first_wait_byte_index_relative+write_bytes<_capacity) 
    //     _output.write(assembled_segment.substr(first_wait_byte_index_relative, write_bytes));
    // else {
    //     _output.write(assembled_segment.substr(first_wait_byte_index_relative));
    //     _output.write(assembled_segment.substr(0, write_bytes+first_wait_byte_index_relative-_capacity));
    // }
    // //update first_wait_byte_index_relative    
    // first_wait_byte_index_relative = first_wait_byte_index % _capacity;
    // }
    // if(eof_flag && empty()) _output.end_input();
    // return;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_count; }

bool StreamReassembler::empty() const { return _unassembled_count == 0; }