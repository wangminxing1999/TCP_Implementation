#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):store(),max(capacity),readsize(0),writesize(0),endinput(false),_error(false) { 
    
 }

size_t ByteStream::write(const string &data) {
    if(endinput == true)
        return 0;
    size_t num = min(data.size(),max-store.size());
    writesize += num;
    for(size_t i=0; i<num; i++) {
        store.push_back(data[i]);
    }
    return num;
    
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t num = min(len,store.size());
    return string(store.begin(),store.begin()+num);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t num = min(len,store.size());
    for(size_t i=0; i<num; i++) {
        store.pop_front();
    }
    readsize += num;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string data = this->peek_output(len);
    this->pop_output(len);
    return data;
}

void ByteStream::end_input() {
    endinput = true;
}

bool ByteStream::input_ended() const {
    return endinput;
}

size_t ByteStream::buffer_size() const { 
    return store.size();
 }

bool ByteStream::buffer_empty() const {
    return store.empty();
}

bool ByteStream::eof() const {
    return store.empty() && endinput;
 }

size_t ByteStream::bytes_written() const {
    return writesize;
}

size_t ByteStream::bytes_read() const {
    return readsize;
}

size_t ByteStream::remaining_capacity() const {
    return max-store.size();
}
