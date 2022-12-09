#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}
int t = 1;
using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(eof == 1)
        _eof = true;
    if(data == "" && _eof && empty())
        _output.end_input();
    if(index + data.size() <= next_send)
        return;
    if(index >= _output.remaining_capacity() + next_send)
        return;
    struct byte_segment seg = {};
    if(index < next_send){
        seg.head = next_send;
        seg.data = data.substr(next_send - index);
    } else {
        seg.head = index;
        seg.data = data;
    }

    for(auto iter = wait_for_process.begin(); iter != wait_for_process.end(); ) {
        if((seg.head >= iter->head) && (seg.head + seg.data.size() <= iter->head + iter->data.size()))
            return;
        if((seg.head <= iter->head) && (seg.head + seg.data.size() >= iter->head + iter->data.size())) {
            not_assembled -= iter->data.size();
            iter = wait_for_process.erase(iter);
            continue;
        }
        if((seg.head + seg.data.size() >= iter->head + iter->data.size()) && (seg.head >= iter->head) && (seg.head <= iter->head + iter->data.size())){
            seg.data = iter->data + seg.data.substr(iter->head + iter->data.size() - seg.head);
            seg.head = iter->head;
            not_assembled -= iter->data.size();
            iter = wait_for_process.erase(iter);
            continue;
        }
        if((seg.head <= iter->head) && (seg.head + seg.data.size() >= iter->head) && (seg.head + seg.data.size() <= iter->head + iter->data.size())){
            seg.data = seg.data + iter->data.substr(seg.head + seg.data.size() - iter->head);
            not_assembled -= iter->data.size();
            iter = wait_for_process.erase(iter);
            continue;
        }
        iter++;
    }
    wait_for_process.insert(seg);
    not_assembled += seg.data.size();
    for(auto iter = wait_for_process.begin(); iter != wait_for_process.end(); ++iter) {
        if(iter->head == next_send) {
            if(iter->data.size() <= _output.remaining_capacity()){
                _output.write(iter->data);
                not_assembled -= iter->data.size();
                next_send += iter->data.size();
                wait_for_process.erase(iter);
            } else {
                struct byte_segment s = {};
                size_t cap = _output.remaining_capacity();
                _output.write(iter->data);
                not_assembled -= cap;
                next_send += cap;
                s.head = iter->head + cap;
                s.data = iter->data.substr(cap);
                wait_for_process.insert(s);
                wait_for_process.erase(iter);
            }
            break;
        }
    }
    if(empty() && _eof){
        _output.end_input();
    }
        
    
}
size_t StreamReassembler::unassembled_bytes() const { return not_assembled; }

bool StreamReassembler::empty() const { 
    if(unassembled_bytes() == 0)
        return true;
    return false;
 }
