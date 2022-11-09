#include <vector>
#include <deque>
#include <cmath>
#include <set>
#include <algorithm>
#include <array>
#include <cassert>
#include <complex>
#include <iostream>
#include <math.h>
#include <cstdint>
#include <fstream>
#include <string>
#include <iomanip>
#include <chrono>
#include <random>
#include <filesystem>

const long double PIL = 3.14159265358979323846264338327950288419716939937510l;
const double  PI = PIL;
const float PIF = PIL;

/*****************************************************************************/
// .wav file reader ///////////////////////////////////////////////////////////
/*****************************************************************************/

const uint16_t EXTENSIBLE = 0xfffe;
const uint16_t PCM = 0x0001;
const uint16_t IEEE = 0x0003;
const char EXTENSIBLE_GUID[15] = "\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9b\x71";

namespace wave_dialog {

const uint32_t PCM_ID =     0x0001;
const uint32_t IEEE_ID =    0x0003;

const uint32_t INT8_ID =    0x0008 | PCM_ID<<16;
const uint32_t INT16_ID =   0x0010 | PCM_ID<<16;
const uint32_t INT24_ID =   0x0018 | PCM_ID<<16;
const uint32_t INT32_ID =   0x0020 | PCM_ID<<16;
const uint32_t FLOAT32_ID = 0x0020 | IEEE_ID<<16;

inline uint16_t listen_uint16(const char *r){
    const uint8_t *c = (uint8_t*)r;
    return (uint16_t)c[0] | c[1]<<8;
}

inline uint32_t listen_uint32(const char *r){
    const uint8_t *c = (uint8_t*)r;
    return (uint32_t)c[0] | c[1]<<8 | c[2]<<16 | c[3]<<24;
}



inline float listen_int8_as_float(const char *r){
    const uint8_t *c = (uint8_t*)r;
    int16_t x = (int16_t)c[0] - 128;
    return (float)x/(1<<7);
}

inline float listen_int16_as_float(const char *r){
    const uint8_t *c = (uint8_t*)r;
    int16_t x = (int16_t)c[0] | c[1]<<8;
    return (float)x/(1<<15);
}

inline float listen_int24_as_float(const char *r){
    const uint8_t *c = (uint8_t*)r;
    int32_t x = ((int32_t)c[0]<<8 | c[1]<<16 | c[2]<<24)/(1<<8);
    return (float)x/(1<<23);
}

inline float listen_int32_as_float(const char *r){
    const uint8_t *c = (uint8_t*)r;
    int32_t x = (int32_t)c[0] | c[1]<<8 | c[2]<<16 | c[3]<<24;
    return (float)x/(1ll<<31);
}

inline float listen_float32(const char *r){
    const uint8_t *c = (uint8_t*)r;
    const int32_t x = (int32_t)c[0] | c[1]<<8 | c[2]<<16 | c[3]<<24;
    const char *t = (char*)&x;
    return *((float*)t);
}



inline void say_uint16(const uint16_t x, char *c){
    
    const uint8_t *cx = (uint8_t*)&x;
    const uint16_t b = (uint16_t)cx[0] | cx[1]<<8;
    const char *cb = (char*)&b;

    c[0] = cb[0];
    c[1] = cb[1];
}

inline void say_uint32(const uint32_t x, char *c){
    
    const uint8_t *cx = (uint8_t*)&x;
    const uint32_t b = (uint32_t)cx[0] | cx[1]<<8 | cx[2]<<16 | cx[3]<<24;
    const char *cb = (char*)&b;

    c[0] = cb[0];
    c[1] = cb[1];
    c[2] = cb[2];
    c[3] = cb[3];
}



inline void say_float_as_int8(const float x, char *c){
    const uint8_t t = (uint8_t)std::min(255.0f, std::max(0.0f, (x+1)*128));
    c[0] = t;
}

inline void say_float_as_int16(const float x, char *c){
    const int16_t t = (int16_t)std::min((float)((1<<15)-1),
            std::max(-(float)(1<<15), x*(1<<15)));
    c[0] = t&0x00ff;    
    c[1] = (t&0xff00)>>8;   
}

inline void say_float_as_int24(const float x, char *c){
    const int32_t t = (int32_t)std::min((float)((1<<23)-1),
            std::max(-(float)(1<<23), x*(1<<23)));
    c[0] = t&0x0000ff;
    c[1] = (t&0x00ff00)>>8;
    c[2] = (t&0xff0000)>>16 | ((t>>31)&1)<<7;
}


inline void say_float_as_int32(const float x, char *c){
    const int32_t t = (int32_t)std::min((float)((1ll<<31)-1),
            std::max(-(float)(1ll<<31), x*(1ll<<31)));
    c[0] = t&0x000000ff;
    c[1] = (t&0x0000ff00)>>8;
    c[2] = (t&0x00ff0000)>>16;
    c[3] = (t&0xff000000)>>24;
}

inline void say_float32(const float x, char *c){
    
    const uint8_t *cx = (uint8_t*)&x;
    const uint32_t b = (uint32_t)cx[0] | cx[1]<<8 | cx[2]<<16 | cx[3]<<24;
    const char *cb = (char*)&b;

    c[0] = cb[0];
    c[1] = cb[1];
    c[2] = cb[2];
    c[3] = cb[3];
}

inline uint32_t resolve_dialog(uint16_t format, uint16_t bits){
    uint32_t id = (uint32_t)format<<16 | bits;
    switch(id){
        case INT8_ID:
            return id;
        case INT16_ID:
            return id;
        case INT24_ID:
            return id;
        case INT32_ID:
            return id;
        case FLOAT32_ID:
            return id;
    }
    return 0;
}

}   // namespace wave_dialog

class waveconfig{

protected:

    char GUID[16];  // the first 2 bytes are the subformat.
    std::vector<std::string> log;
    void add_log(std::string message);

    /*
        I'm following the documentation on the wave format found on this site:
        http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    */

    // standard info
    uint16_t
        format = 0,         // 0x0001 (PCM), 0x0003 (IEEE) or 0xfffe (EXTENSIBLE)
        channels = 0,       // 1 - 18 of channels
        frameSize = 0,      // 1 frame = (1 sample from each channel)
        sampleBits = 0,     // number of (used?) bits in a sample. Only multiples of 8 are supported.
        sampleSize = 0,     // sample size in bytes.
        extensionSize = 0;  // Useless, formatSize already has this info


    uint32_t
        fileSize = 0,       // (size of the whole file in bytes) - 8 
        formatSize = 0,     // format chunk size in bytes
        frameRate = 0,      // sampling rate = frames per second
        byteRate = 0,       // bytes per second
        dataSize = 0;       // data chunk size in bytes



    // info for extensible format:

    uint16_t 
        validSampleBits = 0,    // same as sampleBits, as only multiples of 8 are supported.
        subformat = 0;          // actual format of the wave format extensible.

    uint32_t
        channelMask = 0;    // bitmask of the speaker positions for the channels
    /*

        the positions in the mask are:

        0.   Front Left - FL,
        1.   Front Right - FR,
        2.   Front Center - FC
        3.   Low Frequency - LF
        4.   Back Left - BL
        5.   Back Right - BR
        6.   Front Left of Center - FLC
        7.   Front Right of Center - FRC
        8.   Back Center - BC
        9.   Side Left - SL
        10.  Side Right - SR
        11.  Top Center - TC
        12.  Top Front Left - TFL
        13.  Top Front Center - TFC
        14.  Top Front Right - TFR
        15.  Top Back Left - TBL
        16.  Top Back Center - TBC
        17.  Top Back Right - TBR

        mono uses FL                            000000000000000001
        stereo uses FL, FR                      000000000000000011
        5.1 uses FL, FR, FC, LF, BL, BR         000000000000111111 (probably)
        7.1 uses FL, FR, FC, LF, BL, BR, SL, SR 000000011000111111 (I guess)
        
    */

public:

    waveconfig();
    waveconfig(
            uint16_t format,
            uint16_t channel_amount,
            uint16_t sample_size,
            uint32_t frame_rate,
            uint16_t subformat = 0,
            uint32_t mask = 0);

    waveconfig(waveconfig *other);

    std::vector<std::string> get_log();
    bool logging = 0;

    // all functions returning a boolean will tell wether the function call was succesful.
    // If an error or something worth of notice happened, a message is added to the log.

    bool set_format(uint16_t);
    bool set_channel_amount(uint16_t);
    bool set_sample_bitsize(uint16_t);
    bool set_frame_rate(uint32_t);      // frames per second
    bool set_subformat(uint16_t);
    bool set_channel_mask(uint32_t);

    bool config(
            uint16_t format,
            uint16_t channel_amount,
            uint16_t sample_size,
            uint32_t frame_rate,
            uint16_t subformat = 0,
            uint32_t mask = 0);

    bool copy_config(waveconfig*);

    uint32_t get_sample_amount();   // data size in samples.
    uint32_t get_frame_amount();    // data size in frames.

    uint16_t get_format();
    uint16_t get_channel_amount();
    uint16_t get_sample_bitsize();
    uint32_t get_frame_rate();  
    uint16_t get_subformat();
    uint32_t get_channel_mask();

};

class iwstream : public waveconfig{

protected:
    
    std::string source;
    std::ifstream wavFile;
    uint32_t dataBegin;

    uint16_t read_uint16();
    uint32_t read_uint32();

    bool handle_unexpected_chunk();
    bool compare_id(char*, std::string);

    uint32_t datatype = 0;

public:
    
    iwstream();

    // also opens & initializes the the stream
    iwstream(std::string source_);

    // reads the wave file configuration.
    bool initialize();

    // opens & initializes the wave file.
    // iwstream must be opened before anything can be read.
    bool open(std::string source_);
    bool close();

    // tell & seek reading position.
    uint32_t tell();
    bool seek(uint32_t beginSample);

    // continue reading amount samples from the current position.
    // if end of file is reached, the rest of the values are assigned to 0.
    // for the vector overload, values are appended to the end of the vector.
    // returns the amount of samples read.
    uint32_t read_move(std::vector<float> &waves, uint32_t amount);
    uint32_t read_move(float *waves, uint32_t amount);
    std::vector<float> read_move(uint32_t amount);
    
    // read samples but dont move the file pointer forward
    uint32_t read_silent(std::vector<float> &waves, uint32_t amount);
    uint32_t read_silent(float *waves, uint32_t amount);
    std::vector<float> read_silent(uint32_t amount);

    // navigate to beginFrame & read from that point.
    uint32_t read_move(std::vector<float> &waves, uint32_t beginSample, uint32_t amount);
    uint32_t read_move(float *waves, uint32_t beginSample, uint32_t amount);
    std::vector<float> read_move(uint32_t beginSample, uint32_t amount);
    

    uint32_t read_silent(std::vector<float> &waves, uint32_t beginSample, uint32_t amount);
    uint32_t read_silent(float *waves, uint32_t beginSample, uint32_t amount);
    std::vector<float> read_silent(uint32_t beginSample, uint32_t amount);
    
    // navigate to begin of file and read all frames.
    uint32_t read_file(std::vector<float> &waves);
    uint32_t read_file(float *waves);
    std::vector<float> read_file();
        
};

///////////////////////////////////////////////////////////////////////////////
// config /////////////////////////////////////////////////////////////////////

waveconfig::waveconfig(){}

waveconfig::waveconfig(uint16_t format, uint16_t channel_amount, uint16_t sample_size,
        uint32_t frame_rate, uint16_t subformat, uint32_t mask){    
    config(format, channel_amount, sample_size, frame_rate, subformat, mask);
}

waveconfig::waveconfig(waveconfig *other){
    copy_config(other);
}

void waveconfig::add_log(std::string message){
    log.push_back(message);
    if(log.size() >= 200){
        for(int i=0; i<100; i++) log[i] = log[i+100];
        log.resize(100);
    }
}

std::vector<std::string> waveconfig::get_log(){
    return log;
}



bool waveconfig::set_format(uint16_t format_){
    switch(format_){
        case PCM:
            formatSize = 16;
            extensionSize = 0;
            format = format_;
            return 1;
        case IEEE:
            formatSize = 18;
            extensionSize = 0;
            format = format_;
            return 1;
        case EXTENSIBLE:
            formatSize = 40;
            extensionSize = 22;
            format = format_;
            return 1;
    }
    
    if(logging){
        add_log("format "+std::to_string(format_)
            +" not recognized. Known formats are 0x0001, 0x0003 and 0xfffe");
    }
    return 0;
}

bool waveconfig::set_channel_amount(uint16_t channels_){
    
    if(channels_ > 18){
        if(logging) add_log(std::to_string(channels_)+" is too many channels. Max 18.");
        return 0;
    }

    channels = channels_;
    frameSize = channels*sampleSize;
    byteRate = frameRate*frameSize;
    
    return 1;
}

bool waveconfig::set_sample_bitsize(uint16_t sampleBits_){
    
    if(sampleBits_%8){
        add_log("sample "+std::to_string(sampleBits_)
                +" size not supported, must be a multiple of 8");
        return 0;
    }

    sampleBits = sampleBits_;
    sampleSize = sampleBits/8;
    frameSize = channels*sampleSize;
    byteRate = frameRate*frameSize;

    return 1;
}

bool waveconfig::set_frame_rate(uint32_t frameRate_){
    
    frameRate = frameRate_;
    byteRate = frameRate*frameSize;

    return 1;
}

bool waveconfig::set_subformat(uint16_t subformat_){

    if(format == EXTENSIBLE){
        
        switch(subformat_){
            case PCM:
                subformat = subformat_;
                return 1;
            case IEEE:
                subformat = subformat_;
                return 1;
        }
    
        if(logging){
            add_log("Subormat "+std::to_string(subformat_)
                +" not recognized. Supported formats are 0x0001, 0x0003");
        }

        return 0;

    }
        
    if(logging) add_log("there is no subformat, as format is not WAVE_FORMAT_EXTENSIBLE");    
    return 0;
    
}

bool waveconfig::set_channel_mask(uint32_t mask){
    channelMask = mask;
    return 1;
}

bool waveconfig::config(
        uint16_t format_,
        uint16_t channel_amount_,
        uint16_t sample_size_,
        uint32_t frame_rate_,
        uint16_t subformat_,
        uint32_t mask_){
    
    bool ret = 1;

    ret &= set_format(format_);
    ret &= set_channel_amount(channel_amount_);
    ret &= set_sample_bitsize(sample_size_);
    ret &= set_frame_rate(frame_rate_);

    if(format == EXTENSIBLE){
        ret &= set_subformat(subformat_);
        ret &= set_channel_mask(mask_);
        validSampleBits = sampleBits;
    }

    return ret;
}

bool waveconfig::copy_config(waveconfig *other){

    format = other->format;
    channels = other->channels;
    frameSize = other->frameSize;
    sampleBits = other->sampleBits;
    sampleSize = other->sampleSize;
    extensionSize = other->extensionSize;

    formatSize = other->formatSize;
    frameRate = other->frameRate;
    byteRate = other->byteRate;

    validSampleBits = other->validSampleBits;
    subformat = other->subformat;

    channelMask = other->channelMask;

    return 1;
}

uint32_t waveconfig::get_sample_amount(){
    return dataSize/sampleSize;
}

uint32_t waveconfig::get_frame_amount(){
    return dataSize/frameSize;
}

uint16_t waveconfig::get_format(){ return format; }
uint16_t waveconfig::get_channel_amount(){ return channels; }
uint16_t waveconfig::get_sample_bitsize(){ return sampleBits; }
uint32_t waveconfig::get_frame_rate(){ return frameRate; }
uint16_t waveconfig::get_subformat(){ return subformat; }
uint32_t waveconfig::get_channel_mask(){ return channelMask; }

///////////////////////////////////////////////////////////////////////////////
// iwstream ///////////////////////////////////////////////////////////////////

uint16_t iwstream::read_uint16(){
    char buff[2];
    wavFile.read(buff, 2);    
    return wave_dialog::listen_uint16(buff);
}

uint32_t iwstream::read_uint32(){
    char buff[4];
    wavFile.read(buff, 4);    
    return wave_dialog::listen_uint32(buff);
}

bool iwstream::handle_unexpected_chunk(){
    
    uint32_t chunkSize = read_uint32();
    char *buff = new char[chunkSize];
    wavFile.read(buff, chunkSize);

    if(!wavFile.good()){
        if(logging) add_log("error reading unexpected chunk of size "+std::to_string(chunkSize));
        return 0;
    }

    delete[] buff;

    return 1;
}

bool iwstream::compare_id(char *buff, std::string pattern){
    for(uint32_t i=0; i<pattern.size(); i++) if(pattern[i] != buff[i]) return 0;
    return 1;
}

iwstream::iwstream(){
    source = "";
}

iwstream::iwstream(std::string source_){
    open(source_);
}

bool iwstream::open(std::string source_){
    source = source_;
    wavFile.open(source_);
    return initialize();
}

bool iwstream::close(){
    wavFile.close();
    return 1;
}

bool iwstream::initialize(){

    if(!wavFile.good()){
        if(logging) add_log("error reading file");
        return 0;
    }

    char buff4[4];
    
    wavFile.read(buff4, 4);
    
    if(!compare_id(buff4, "RIFF")){
        if(logging) add_log("file is not RIFF format");
        return 0;
    }

    fileSize = read_uint32();

    wavFile.read(buff4, 4);

    if(!compare_id(buff4, "WAVE")){
        if(logging) add_log("file is not WAVE format");
        return 0;
    }

    wavFile.read(buff4, 4);

    while(!compare_id(buff4, "fmt ")){
        if(logging) add_log("unexpexted chunk, expected \"fmt \"");
        bool ok = handle_unexpected_chunk();
        if(!ok) return 0;
        wavFile.read(buff4, 4);
    }
    
    formatSize = read_uint32();
    format = read_uint16();
    channels = read_uint16();
    frameRate = read_uint32();
    byteRate = read_uint32();
    frameSize = read_uint16();
    sampleBits = read_uint16();
    sampleSize = sampleBits/8;

    if(formatSize >= 18){
        extensionSize = read_uint16();
    }

    if(formatSize == 40){
        
        validSampleBits = read_uint16();
        channelMask = read_uint32();

        wavFile.read(GUID, 16);
        subformat = wave_dialog::listen_uint16(GUID);
    }

    if(format == EXTENSIBLE){ 
        datatype = wave_dialog::resolve_dialog(subformat, validSampleBits);
        if(!datatype){
            if(logging){
                add_log("format 0xfffe with subformat "+
                    std::to_string(subformat)+" "+std::to_string(validSampleBits)
                    +" is not supported.");
            }
            return 0;
        }
    } else {

        subformat = format;             // this should enable handling all files
        validSampleBits = sampleBits;   // as 0xfffe WAVE_FORMAT_EXTENSIBLE

        datatype = wave_dialog::resolve_dialog(format, sampleBits);
        if(!datatype){
            if(logging){
                add_log("format "+std::to_string(format)
                    +" "+std::to_string(sampleBits)+" is not supported.");
            }
            return 0;
        }
    }
    
    wavFile.read(buff4, 4);

    while(!compare_id(buff4, "data")){
        if(!compare_id(buff4, "fact") && logging) add_log("unexpected chunk, expected \"data\"");
        bool ok = handle_unexpected_chunk();
        if(!ok) return 0;
        wavFile.read(buff4, 4);
    }

    dataSize = read_uint32();

    dataBegin = wavFile.tellg();

    if(logging){
        add_log(
            "file initialized with:\nformat: "+std::to_string(format)
            +"\nsubformat (if format is 0xfffe): "+std::to_string(subformat)
            +"\nchannels: "+std::to_string(channels)
            +"\nsample size: "+std::to_string(validSampleBits)
            +"\nframe rate: "+std::to_string(frameRate)
            +"\ndata amount: "+std::to_string(dataSize));
    }

    return 1;
}

uint32_t iwstream::tell(){
    return ((uint32_t)wavFile.tellg() - dataBegin) / sampleSize;
}

bool iwstream::seek(uint32_t beginSample){

    if((int64_t)beginSample*sampleSize > (int64_t)dataSize){
        if(logging) add_log("couldn't move to position, sample is out of bounds.");
        return 0;
    }

    wavFile.seekg(dataBegin+beginSample*sampleSize);

    return 1;
}

uint32_t iwstream::read_move(std::vector<float> &waves, uint32_t amount){

    if(!wavFile.good()){
        if(logging) add_log("error reading file");
        return 0;
    }

    int64_t bsize = waves.size();
    waves.resize(bsize+amount, 0);

    return read_move(waves.data()+bsize, amount);
}

uint32_t iwstream::read_move(float *waves, uint32_t amount){

    if(!wavFile.good()){
        if(logging) add_log("error reading file");
        return 0;
    }

    uint32_t readAmount = amount;
    
    int64_t probeSize = (int64_t)amount*sampleSize+wavFile.tellg()-dataBegin;

    if(probeSize > (int64_t)dataSize){
        readAmount = amount - (probeSize-dataSize)/sampleSize;
        if(logging){
            add_log(
                "could only read "+std::to_string(readAmount)
                +" frames as end end of file was reached.");
        }
    }

    int64_t buffz = readAmount*sampleSize;

    char *buff = new char[buffz];
    
    wavFile.read(buff, buffz);

    switch(datatype){
        case wave_dialog::INT8_ID:
            for(uint32_t i=0; i<readAmount; i++){
                waves[i] = wave_dialog::listen_int8_as_float(buff+i*sampleSize);
            }
            break;
        case wave_dialog::INT16_ID:
            for(uint32_t i=0; i<readAmount; i++){
                waves[i] = wave_dialog::listen_int16_as_float(buff+i*sampleSize);
            }
            break;
        case wave_dialog::INT24_ID:
            for(uint32_t i=0; i<readAmount; i++){
                waves[i] = wave_dialog::listen_int24_as_float(buff+i*sampleSize);
            }
            break;
        case wave_dialog::INT32_ID:
            for(uint32_t i=0; i<readAmount; i++){
                waves[i] = wave_dialog::listen_int32_as_float(buff+i*sampleSize);
            }
            break;
        case wave_dialog::FLOAT32_ID:
            for(uint32_t i=0; i<readAmount; i++){
                waves[i] = wave_dialog::listen_int32_as_float(buff+i*sampleSize);
            }
            break;
        default:
            if(logging){
                add_log("file was initialized with unrecognized datatype\nand no data was read");
            }
    }
    
    if(!wavFile){
        if(logging) add_log("error reading file");
        return 0;
    }

    if(wavFile.eof() || (uint32_t)wavFile.tellg()-dataBegin >= dataSize){
        if(logging) add_log("end of file reached");
        return readAmount;
    }

    delete[] buff;

    return readAmount;
}

std::vector<float> iwstream::read_move(uint32_t amount){
    std::vector<float> waves;
    read_move(waves, amount);
    return waves;
}

uint32_t iwstream::read_silent(std::vector<float> &waves, uint32_t amount){
    uint32_t previous = tell();
    uint32_t num = read_move(waves, amount);
    seek(previous);
    return num;
}

uint32_t iwstream::read_silent(float *waves, uint32_t amount){
    uint32_t previous = tell();
    uint32_t num = read_move(waves, amount);
    seek(previous);
    return num;
}

std::vector<float> iwstream::read_silent(uint32_t amount){
    std::vector<float> waves;
    read_silent(waves, amount);
    return waves;
}

uint32_t iwstream::read_move(std::vector<float> &waves, uint32_t beginSample, uint32_t amount){
    if(!seek(beginSample)) return 0;
    return read_move(waves, amount);
}

uint32_t iwstream::read_move(float *waves, uint32_t beginSample, uint32_t amount){
    if(!seek(beginSample)) return 0;
    return read_move(waves, amount);
}

std::vector<float> iwstream::read_move(uint32_t beginSample, uint32_t amount){
    std::vector<float> waves;
    read_move(waves, beginSample, amount);
    return waves;
}

uint32_t iwstream::read_silent(std::vector<float> &waves, uint32_t beginSample, uint32_t amount){
    uint32_t previous = tell();
    uint32_t num = read_move(waves, beginSample, amount);
    seek(previous);
    return num;
}

uint32_t iwstream::read_silent(float *waves, uint32_t beginSample, uint32_t amount){
    uint32_t previous = tell();
    uint32_t num = read_move(waves, beginSample, amount);
    seek(previous);
    return num;
}

std::vector<float> iwstream::read_silent(uint32_t beginSample, uint32_t amount){
    std::vector<float> waves;
    read_silent(waves, beginSample, amount);
    return waves;
}

uint32_t iwstream::read_file(std::vector<float> &waves){
    wavFile.seekg(dataBegin);
    return read_move(waves, dataSize/sampleSize);
}

uint32_t iwstream::read_file(float *waves){
    wavFile.seekg(dataBegin);
    return read_move(waves, dataSize/sampleSize);
}

std::vector<float> iwstream::read_file(){
    std::vector<float> waves;
    read_file(waves);
    return waves;
}
/*****************************************************************************/
// fft ////////////////////////////////////////////////////////////////////////
/*****************************************************************************/

namespace math {

// calculate discrete fourier transform of v. works fast
// for sizes that are powers of 2. otherwise falls back to bluestein -> 6x slowdown.

void in_place_fft(std::complex<float> *v, unsigned n, bool inv = 0);
void in_place_fft(std::vector<std::complex<float> > &v, bool inv = 0);

std::vector<std::complex<float> > fft(const float *v, unsigned n);
std::vector<std::complex<float> > fft(const std::vector<float> &v);

std::vector<float> inverse_fft(const std::complex<float> *v, unsigned n);
std::vector<float> inverse_fft(const std::vector<std::complex<float> > &v);

std::vector<std::complex<float> > fft(std::vector<std::complex<float> > v, bool inv = 0);
std::vector<std::complex<float> > fft(std::complex<float> *v, unsigned n, bool inv = 0);

std::vector<float> convolution(std::vector<float> &a, std::vector<float> &b, unsigned size = 0);
std::vector<std::complex<float> > convolution(
        std::vector<std::complex<float> > a,
        std::vector<std::complex<float> > b,
        unsigned size = 0);

std::vector<float> correlation(std::vector<float> a, std::vector<float> b, unsigned size = 0);

// implementation that utilizes fft's bandwidth of 2 vectors. The second vector is optional.
std::array<std::vector<float>, 2> autocorrelation(std::vector<float> a, std::vector<float> b = {});

// these are ~6 times slower on average than radix 2, but support all sizes.

std::vector<std::complex<float> > bluestein(std::vector<std::complex<float> > v, bool inv = 0); 
std::vector<std::complex<float> > bluestein(std::vector<float> &v);
std::vector<float> inverse_bluestein(std::vector<std::complex<float> > &v); 

// default precalc table size is 18 -> vectors of size 2^18 max can be processed

struct FFTPrecalc {

    FFTPrecalc(unsigned);
    void resize(unsigned);

    unsigned B;

    std::vector<std::vector<std::complex<float> > > w;
    std::vector<unsigned> invbit;

};

extern FFTPrecalc fftPrecalc;

} // namespace math



namespace math {

using std::vector;
using std::complex;

FFTPrecalc::FFTPrecalc(unsigned B_){
    resize(B_);
}

void FFTPrecalc::resize(unsigned B_){
    
    B = B_;
    w.resize(B);
    invbit.resize(1u<<B, 0);

    w[B-1].resize(1u<<(B-1));
    for(unsigned i=0; i<(1u<<(B-1)); i++) w[B-1][i] = std::polar(1.0f, -PIF*i/(1<<(B-1)));
    if(B>1) invbit[1] = 1;

    for(unsigned b=B-1; b-->0;){
        unsigned n = 1<<b, nn = 1<<(B-b-1);
        w[b].resize(n);
        for(unsigned i=0; i<nn; i++){
            invbit[i] <<= 1;
            invbit[i+nn] = invbit[i]|1;
        }
        for(unsigned i=0; i<n; i++) w[b][i] = w[b+1][2*i];
    }
}

FFTPrecalc fftPrecalc(18);

void in_place_fft(complex<float> *v, unsigned n, bool inv){
    
    unsigned bits = 0;
    while(1u<<bits < n) bits++;

    if(bits > fftPrecalc.B) return;
    if(1u<<bits != n){
        vector<complex<float> > w(n);
        for(unsigned i=0; i<n; i++) w[i] = v[i];
        w = bluestein(w, inv);
        for(unsigned i=0; i<n; i++) v[i] = w[i];
        return;
    }
    
    unsigned shift = fftPrecalc.B-bits;

    for(unsigned i=0; i<n; i++){
        if(i < fftPrecalc.invbit[i]>>shift) std::swap(v[i], v[fftPrecalc.invbit[i]>>shift]);
    }

    for(unsigned r=0; r<bits; r++){
        unsigned rd = 1u<<r;
        for(unsigned i=0; i<n; i+=2*rd){
            for(unsigned j=i; j<i+rd; j++){
                complex<float> tmp = fftPrecalc.w[r][j-i]*v[j+rd];
                v[j+rd] = v[j]-tmp;
                v[j] = v[j]+tmp;
            }
        }
    }

    if(inv){
        std::reverse(v+1, v+n);
        for(unsigned i=0; i<n; i++) v[i] /= n;
    }
}

void in_place_fft(vector<complex<float> > &v, bool inv){
    in_place_fft(v.data(), v.size(), inv);
}

vector<complex<float> > fft(const float *v, unsigned n){
    vector<complex<float> > f(n);
    for(unsigned i=0; i<n; i++) f[i] = v[i];
    in_place_fft(f.data(), n);
    return f;
}

vector<complex<float> > fft(const vector<float> &v){
    return fft(v.data(), v.size());
}

vector<float> inverse_fft(const complex<float> *v, unsigned n){
    vector<float> r(n);
    vector<complex<float> > f(n);
    for(unsigned i=0; i<n; i++) f[i] = v[i];
    in_place_fft(f, 1);
    for(unsigned i=0; i<n; i++) r[i] = f[i].real();
    return r;
}

vector<float> inverse_fft(const vector<complex<float> > &v){
    return inverse_fft(v.data(), v.size());
}

vector<complex<float> > fft(vector<complex<float> > v, bool inv){
    in_place_fft(v.data(), v.size(), inv);
    return v;
}

vector<complex<float> > fft(complex<float> *v, unsigned n, bool inv){
    vector<complex<float> > f(n);
    for(unsigned i=0; i<n; i++) f[i] = v[i];
    in_place_fft(f, inv);
    return f;
}

vector<float> convolution(vector<float> &a, vector<float> &b, unsigned size){
    
    unsigned za = a.size(), zb = b.size();
    unsigned mx = std::max(za, zb);
    unsigned n = size;
    if(!n) n = za + zb - 1;

    unsigned cz = 1;
    while(cz < n) cz *= 2;

    vector<complex<float> > c(cz, {0, 0});
    
    for(unsigned i=0; i<mx; i++){
        if(i < za) c[i] = {a[i], 0};
        if(i < zb) c[i] = {c[i].real(), b[i]};
    }

    in_place_fft(c);

    for(unsigned i=0; 2*i<=cz; i++){
        unsigned j = i == 0 ? 0 : cz-i;
        c[i] = -(c[i]-conj(c[j]))*(c[i]+conj(c[j]))*complex<float>(0, 0.25f);
        c[j] = conj(c[i]);
    }

    in_place_fft(c, 1);

    vector<float> r(n, 0);
    for(unsigned i=0; i<n; i++) r[i] = c[i].real();
    
    return r;
}

vector<complex<float> > convolution(
        vector<complex<float> > a,
        vector<complex<float> > b,
        unsigned size){

    unsigned n = size;
    if(!n) n = a.size() + b.size() - 1;

    unsigned z = 1;
    while(z < n) z *= 2;

    a.resize(z, {0, 0});
    b.resize(z, {0, 0});

    in_place_fft(a);
    in_place_fft(b);

    for(unsigned i=0; i<z; i++) a[i] *= b[i];

    in_place_fft(a, 1);

    a.resize(n);

    return a;
}

vector<float> correlation(vector<float> a, vector<float> b, unsigned size){

    unsigned n = size;
    if(!n) n = a.size() + b.size() - 1;

    unsigned z = 1;
    while(z < n) z *= 2;

    a.resize(z, 0.0f);
    b.resize(z, 0.0f);

    std::reverse(b.begin() + 1, b.end());

    return convolution(a, b, z);
}

std::array<vector<float>, 2> autocorrelation(vector<float> a, vector<float> b){
    
    unsigned n = a.size(), m = b.size(), z = 1;
    while(z < std::max(n, m)) z *= 2;

    vector<complex<float> > c(2*z, 0.0f);
    for(unsigned i=0; i<n; i++) c[i] = {a[i], 0.0f};
    for(unsigned i=0; i<m; i++) c[i] = {c[i].real(), b[i]};

    in_place_fft(c);

    for(unsigned i=0; i<=z; i++){
        
        unsigned j = i == 0 ? 0 : 2*z-i;
        
        float xr = c[i].real()+c[j].real(), xi = c[i].imag()-c[j].imag();
        float yr = c[i].real()-c[j].real(), yi = c[i].imag()+c[j].imag();
        c[i] = c[j] = {(xr*xr + xi*xi) * 0.25f, (yr*yr + yi*yi) * 0.25f};
        
        /*
        auto l = c[i]+c[j];
        auto r = c[i]-c[j];
        complex<float> x{l.real()/2, r.imag()/2};
        complex<float> y{r.real()/2, l.imag()/2};
        c[i] = c[j] = x*conj(x) - y*complex<float>(-y.real(), y.imag()) * complex<float>(0, 1);
        */
    }
    
    in_place_fft(c, 1);

    for(unsigned i=0; i<n; i++) a[i] = c[i].real();
    for(unsigned i=0; i<m; i++) b[i] = c[i].imag();

    return {a, b};
}

vector<complex<float> > bluestein(vector<complex<float> > v, bool inv){
    
    if(v.empty()) return {};

    unsigned z = v.size(), n = 1;
    while(n < 2*z-1) n *= 2;

    v.resize(n, {0, 0});
    vector<complex<float> > w(n, {0, 0});

    for(unsigned i=0; i<z; i++){
        w[i] = w[(n-i)%n] = std::polar(1.0f, PIF*i*i/z);
        v[i] *= conj(w[i]);
    }

    vector<complex<float> > c = convolution(v, w, n);
 
    for(unsigned i=0; i<z; i++) c[i] *= conj(w[i]);

    c.resize(z);

    if(inv){
        for(auto &i : c) i /= z;
        std::reverse(c.begin()+1, c.end());
    }

    return c;
}

vector<complex<float> > bluestein(vector<float> &v){
    vector<complex<float> > w(v.size());
    for(unsigned i=0; i<v.size(); i++) w[i] = v[i];
    return bluestein(w, 0);
}

vector<float> inverse_bluestein(vector<complex<float> > &v){
    auto w = bluestein(v, 1);
    vector<float> r(w.size());
    for(unsigned i=0; i<v.size(); i++) r[i] = w[i].real();
    return r;
}

}   // namespace math


/*****************************************************************************/
// ft /////////////////////////////////////////////////////////////////////////
/*****************************************************************************/

namespace math {

// Laplace transform... I guess
std::complex<float> lt(const float *waves, unsigned size, float frequency);
std::complex<float> lt(const std::vector<float> &waves, float frequency);

// Poor man's DFT. can't handle vectors larger than 2^15
std::vector<std::complex<float> > ft(
        const float *waves, unsigned size, unsigned n, bool haszero = 0);
std::vector<std::complex<float> > ft(
        const std::vector<float> &waves, unsigned n, bool haszero = 0);

// multiplies the waves by a cosine window (convolution kernel [0.25, 0.5, 0.25] on frequency side)
// and then takes dft of every other frequency
std::vector<std::complex<float> > cos_window_ft(
        float *waves, unsigned size, unsigned n, bool haszero = 0);
std::vector<std::complex<float> > cos_window_ft(
        std::vector<float> waves, unsigned n, bool haszero = 0);

std::vector<std::complex<float> > precise_ft(
        const std::vector<float> &waves, unsigned n, bool haszero = 0, float speed = 1.0f);

std::vector<float> ift(
        const std::vector<std::complex<float> > &frequencies,
        unsigned size, bool haszero = 0);

std::vector<float> sized_ift(
        const std::vector<std::complex<float> > &frequencies,
        unsigned period, unsigned size);

struct FTPrecalc {
    FTPrecalc();
    static const unsigned N = 1<<18;
    static constexpr double dN = N;
    std::complex<float> exp[N+1];
};

extern FTPrecalc ftPrecalc;
std::complex<float> cexp(double radians);

}   // namespace math



namespace math {

using std::vector;
using std::complex;

complex<float> lt(const float *waves, unsigned size, float frequency){
    
    complex<float> sinusoid = {0, 0};

    for(unsigned i=0; i<size; i++){
        sinusoid += waves[i]*std::polar(1.0f, -2 * PIF / size * i * (float)frequency);
    }

    return std::conj(sinusoid);
}

complex<float> lt(const vector<float> &waves, float frequency){
    return lt(waves.data(), waves.size(), frequency);
}

vector<complex<float> > ft(const float *waves, unsigned size, unsigned n, bool haszero){
    
    vector<complex<float> > frequencies(n, 0.0f);
    
    vector<complex<float> > exp(size);
    for(unsigned i=0; i<size; i++) exp[i] = cexp((double)(size - i) / size);

    unsigned offset = !haszero;

    for(unsigned i=0; i<size; i++){
        for(unsigned j=0; j<n; j++){
            frequencies[j] += exp[i * (j+offset) % size] * waves[i];
        }
    }

    for(auto &i : frequencies) i = 2.0f * i / (float)size;

    return frequencies;
}

vector<complex<float> > ft(const vector<float> &waves, unsigned n, bool haszero){
    return ft(waves.data(), waves.size(), n, haszero);
}

vector<complex<float> > cos_window_ft(float *waves, unsigned size, unsigned n, bool haszero){
    
    vector<complex<float> > frequencies(n, 0.0f);
    
    vector<complex<float> > exp(size);
    for(unsigned i=0; i<size; i++) exp[i] = cexp((double)-i / size);

    for(unsigned i=0; i<size; i++) waves[i] *= 0.5f - 0.5f * exp[i].real();

    unsigned offset = !haszero;

    for(unsigned i=0; i<size; i++){
        for(unsigned j=0; j<n; j++){
            frequencies[j] += exp[i * (2*(j+offset)) % size] * waves[i];
        }
    }

    for(auto &i : frequencies) i = 4.0f * i / (float)size;

    return frequencies;
}

vector<complex<float> > cos_window_ft(vector<float> waves, unsigned n, bool haszero){
    return cos_window_ft(waves.data(), waves.size(), n, haszero);
}

vector<complex<float> > precise_ft(const vector<float> &waves,
        unsigned n, bool haszero, float speed){
    
    vector<complex<float> > frequencies(n, 0.0f);

    unsigned size = waves.size();
    unsigned offset = !haszero;

    for(unsigned i=0; i<size; i++){
        for(unsigned j=0; j<n; j++){
            frequencies[j] += waves[i] * std::polar(1.0f, -2 * PIF / size * i * (j + offset) * speed);
        }
    }

    for(auto &i : frequencies) i = 2.0f * speed * i / (float)size;

    return frequencies;
}

vector<float> ift(const vector<complex<float> > &frequencies,
        unsigned size,
        bool haszero){
    
    std::vector<float> waves(size, 0);
    
    vector<complex<float> > exp(size);
    for(unsigned i=0; i<size; i++) exp[i] = cexp((double)i / size);

    unsigned n = frequencies.size();
    
    unsigned offset = !haszero;

    for(unsigned j=0; j<n; j++){
        for(unsigned i=0; i<size; i++){
            waves[i] += (frequencies[j]*exp[i * (j+offset) % size]).real();
        }
    }

    return waves;
}

vector<float> sized_ift(const vector<complex<float> > &frequencies,
        unsigned period,
        unsigned size){
    
    std::vector<float> waves(size, 0);
    
    vector<complex<float> > exp(period);
    for(unsigned i=0; i<period; i++) exp[i] = cexp((double)i / period);

    unsigned n = frequencies.size();
    
    for(unsigned j=0; j<n; j++){
        for(unsigned i=0; i<size; i++){
            waves[i] += (frequencies[j]*exp[i * (j+1) % period]).real();
        }
    }

    return waves;
}

FTPrecalc::FTPrecalc(){
    for(unsigned i=0; i<N; i++){
        exp[i] = std::polar(1.0, 2.0*PI*i/N);
    }
    exp[N] = exp[0];
}

FTPrecalc ftPrecalc;

complex<float> cexp(double radians){
    double dummy;
    double fractional = std::modf(radians, &dummy);
    fractional = fractional < 0.0 ? fractional + 1.0 : fractional;
    return ftPrecalc.exp[(unsigned)(fractional*ftPrecalc.dN)];
}

}   // namespace math

/*****************************************************************************/
// detector ///////////////////////////////////////////////////////////////////
/*****************************************************************************/

namespace change {

class Detector {

    // pitch detector. Designed to detect the pitch of a single source
    // in good conditions.

public:
    
    Detector(
            unsigned frameRate = 44100, // input sampling rate, Hz
            float lower = 60,           // pitch search range lower bound, Hz
            float upper = 900);         // pitch search range upper bound, Hz

    // size fits 2 periods of lower bound frequency. Access only.
    unsigned size;

    // the variables below should be used as access-only.
    // they are updated after each feed.

    // note that it doesn't neccesarily hold true that rate / pitch = period
    // since the pitch is interpolated and the period is not.
    // real_period returns the real valued period i.e. rate / pitch.

    unsigned period = 0;
    float pitch = 0;
    float confidence = 0;
    bool voiced = 0;
    bool quiet = 1;

    float real_period();

    // tweakable variables. Look up the default values in the initializer.
    // changing these to some funny values might result in segfaults.
    
    unsigned peakWindowMax;
    unsigned trustLimit;

    float minCutoff;
    float voicedThreshold;
    float quietThreshold;
    float momentumDecay;

    // the pitch is updated only if the buffer has been fed at least <size> samples.
    // time complexity of feed is O(size * log(size)).
    void feed(const std::vector<float> &data);
    
    // clear previous information.
    void reset();

    // get one period from the buffer. The buffer has a lag of size samples.
    std::vector<float> get(unsigned amount = 0);
    
    // get two periods from the buffer. One extending after the lag and one before.
    std::vector<float> get2(unsigned amount = 0);

    // get the momentum mse graph. for debugging purposes.
    std::vector<float> get_mse();

private:

    const unsigned rate;
    unsigned min, max, trust;
    float power;

    std::deque<float> buffer;

    struct Info {
        unsigned move, top;
        std::vector<float> mse;
        float voiced, value;
    };

    Info momentum;
    std::vector<float> nonorm;

};

}   // namespace change



namespace change {

Detector::Detector(unsigned frameRate, float lower, float upper) :
    peakWindowMax(5),
    trustLimit(5),
    minCutoff(0.25f),
    voicedThreshold(0.3f),
    quietThreshold(5e-5f),
    momentumDecay(0.35f),
    rate(frameRate)
{
    if(lower > upper) std::swap(lower, upper);

    min = std::floor(rate / upper);
    max = std::ceil(rate / lower);

    size = 2 * max;
    for(unsigned i=0; i < 2*size; i++) buffer.push_back(0.0f);

    trust = 0;

    momentum.mse.resize(size, 0.0f);
    nonorm.resize(size, 0.0f);
    momentum.top = 0;
}

float Detector::real_period(){
    if(pitch == 0.0f) return 0.0f;
    return rate / pitch;
}

void Detector::feed(const std::vector<float> &data){
    
    for(float i : data) buffer.push_back(i);

    while(buffer.size() > 2 * size) buffer.pop_front();

    // check if the signal is quiet

    {
        float avg = 0.0f, sum = 0.0f;

        for(unsigned i=0; i<max; i++) avg += buffer[size + i];
        avg /= max;

        for(unsigned i=0; i<max; i++) sum += (buffer[size+i] - avg) * (buffer[size+i] - avg);
        sum /= max;

        power = sum;

        quiet = sum < quietThreshold;
    }

    if(quiet){

        voiced = 0;
        confidence = 1;
        trust = 0;
        
        float oldWeight = std::pow(momentumDecay, (float)data.size() / size);

        for(float &i : nonorm) i *= oldWeight;

        return;
    }

    Info one, two;

    // calculate mean square errors and normalize them by dividing
    // by the cumulative average. I think the YIN algorithm does
    // a similar thing. Not sure though.

    // autocorrelation returns ac[j] = sum(i = 0..n, audio[i]*audio[j+i]).
    // while it is in of itself ok for pitch detection, mse seems
    // to work better. We can easily convert autocorrelation to mse.
   
    // if the previous feeds have been voiced, we use cross-correlation between
    // the left and right side of the current point insted of autocorrelation.
    // While autocorrelation performs better on initially picking up the
    // pitch, cross-correlation is more robust during longer voiced segments.

    // The approaches should be roughly equally fast as autocorrelation
    // only requires 1 fft operation per vector insted of 2 but cross-correlation
    // operates on vectors that are half the size.
    
    if(trust > trustLimit){
   
        one.move = data.size() / 2;
        two.move = data.size() - one.move;
        
        // pitch detected. Follow it with correlation.

        std::vector<float> left1(max), right1(max);
        std::vector<float> left2(max), right2(max);

        for(unsigned i=0; i<max; i++){
            left1[i] = buffer[size + i - two.move - max];
            right1[i] = buffer[size + i - two.move];
            left2[i] = buffer[size + i - max];
            right2[i] = buffer[size + i];
        }

        auto process_mse = [&](
                std::vector<float> &left,
                std::vector<float> &right,
                Info &x) -> void
        {

            // calculate correlation

            x.mse = math::correlation(left, right);
            
            // convert to mse

            float sum = 0.0f;
            for(float i : left) sum += i*i;
            for(float i : right) sum += i*i;
            
            for(unsigned i=0; i<max; i++){
                
                if(i+min <= max && sum != 0.0f)
                    x.voiced = std::max(x.voiced, x.mse[i] / sum);
                
                x.mse[i] = (sum - 2 * x.mse[i]) / (max - i);
                sum -= left[i]*left[i] + right[max-i-1]*right[max-i-1];
            }

            x.mse.resize(max+1);
            std::reverse(x.mse.begin(), x.mse.end());

            // normalize

            x.mse[0] = 2.0f;
            sum = 0.0f;
            for(unsigned i=1; i<=max; i++){
                sum += x.mse[i];
                if(sum != 0.0f) x.mse[i] *= i / sum;
            }

            x.mse.resize(size, 2.0f);
        };

        process_mse(left1, right1, one);
        process_mse(left2, right2, two);
    } 
    else {

        // trying to pick up the pitch. initial probing with autocorrelation.
        
        one.move = data.size() / 2;
        two.move = data.size() - one.move;

        std::vector<float> onev(size), twov(size);
        for(unsigned i=0; i<size; i++) onev[i] = buffer[size + i - two.move];
        for(unsigned i=0; i<size; i++) twov[i] = buffer[size + i];

        {
            auto [omse, tmse] = math::autocorrelation(onev, twov);
            one.mse.swap(omse);
            one.mse.resize(size);
            two.mse.swap(tmse);
            two.mse.resize(size);
        }

        auto process_mse = [&](std::vector<float> &time, Info &x) -> void {
           
            // convert to mse

            float sum = 0.0f;
            for(float i : time) sum += i*i;
            sum *= 2;

            x.mse[0] = 2.0f;
            x.voiced = 0.0f;

            for(unsigned i=1; i<size; i++){
                sum -= time[i-1]*time[i-1] + time[size-i]*time[size-i];
                if(i >= min && i <= max && sum != 0.0f) x.voiced = std::max(x.voiced, x.mse[i] / sum);
                x.mse[i] = (sum - 2 * x.mse[i]) / (size - i);
            }

            // normalize

            sum = 0.0f;
            for(unsigned i=1; i<size; i++){
                sum += x.mse[i];
                if(sum != 0.0f) x.mse[i] *= i / sum;
            }
        };
        
        process_mse(onev, one);
        process_mse(twov, two);
    }

    auto normalize = [&](Info &x) -> void {

        float avg = 0.0f;
        for(unsigned i=min; i<max; i++) avg += x.mse[i];
        avg /= (max-min);

        if(avg > 1e-18){
            float iavg = 1.0f / avg;
            for(float &i : x.mse) i *= iavg;
        } else {
            for(float &i : x.mse) i = 2.0f;
        }
    };
    
    auto find_peak = [&](Info &x) -> void {

        unsigned window = std::min(min/2, peakWindowMax);
        
        x.top = 0;

        // keep track of surrounding area using a multiset
        std::multiset<float> s;
        for(unsigned i=0; i<=2*window && i<size; i++) s.insert(x.mse[i]);
        
        x.value = 1.0f;
        for(unsigned i=window; i<=max && i+window+1<size; i++){

            // index i is a minimum in the surrounding range.
            if(x.mse[i] == *s.begin() && i >= min && x.mse[i] < x.value){
                x.value = x.mse[i];
                x.top = i;
                if(x.value < minCutoff) break;
            }

            // update range
            s.erase(s.find(x.mse[i-window]));
            s.insert(x.mse[i+window+1]);
        }
    };

    normalize(one);
    normalize(two);
    
    auto apply_momentum = [&](Info &x){

        float newWeight = x.voiced * power;
        float oldWeight = std::pow(momentumDecay, (float)x.move / size);

        for(unsigned i=0; i<size; i++){
            momentum.mse[i] = newWeight * x.mse[i] + oldWeight * nonorm[i];
        }

        nonorm = momentum.mse;
   
        momentum.voiced = x.voiced;
    };

    apply_momentum(one);
    apply_momentum(two);
    normalize(momentum);

    find_peak(momentum);

    Info &best = momentum;

    // classify.

    if(best.voiced > voicedThreshold){
        
        confidence = 1.0f - best.value;

        voiced = 1;
        period = best.top;

        // quadratic interpolation is useful for high pitches

        pitch = (float)rate / period;
        if(period > 1 && period + 1 <= max){
            
            std::array<float, 3> val = {
                best.mse[period-1],
                best.mse[period],
                best.mse[period+1]
            };

            // construct a parabola using the three points around
            // the peak. set the peak as origo and find the
            // bottom of the parabola. val[0] and val[2] are at distance
            // 1 from val[1] on the x -axis.

            // y = ax^2 + bx

            float y0 = val[0] - val[1], y1 = val[2] - val[1];
            float a = (y1 + y0) / 2;
            float b = (y1 - y0) / 2;

            float bottom = -b / (2*a);

            if(bottom > -1.0f && bottom < 1.0f) pitch = (float)rate / (period + bottom);
        }

        trust++;

    } else {

        confidence = 1.0f - best.voiced;
        voiced = 0;
        trust = 0;

    }
}

void Detector::reset(){
    
    buffer.clear();
    buffer.resize(2*size, 0.0f);
    
    period = 0;
    pitch = 0;
    confidence = 0;
    voiced = 0;
    quiet = 1;
    trust = 0;
    
    momentum.top = 0;
    
    for(float &i : nonorm) i = 0.0f;
}

std::vector<float> Detector::get(unsigned amount){

    if(amount == 0) amount = period;
    amount = std::min(amount, max);

    std::vector<float> samples(amount);
    for(unsigned i=0; i<amount; i++) samples[i] = buffer[size + i];

    return samples;
}

std::vector<float> Detector::get2(unsigned amount){

    if(amount == 0) amount = period;
    amount = std::min(amount, max);
    
    std::vector<float> samples(2*amount);
    for(unsigned i=0; i<2*amount; i++) samples[i] = buffer[size - amount + i];

    return samples;
}

std::vector<float> Detector::get_mse(){
    return momentum.mse;
}

}   // namespace change

/*****************************************************************************/
// parser /////////////////////////////////////////////////////////////////////
/*****************************************************************************/

std::mt19937 rng32(std::chrono::steady_clock::now().time_since_epoch().count());

int parse_to_csv(std::string directory, std::string output, unsigned N){

    using std::vector;
    using std::complex;

    auto iswave = [](std::string &s) -> bool {
        if(s.size() < 4) return 0;
        return s.substr(s.size()-4) == ".wav";
    };

    std::vector<std::string> files;
    try {
        for(auto &file : std::filesystem::directory_iterator(directory)){
            std::string f = file.path();
            if(!iswave(f)) continue;
            files.push_back(f);
        }
    }
    catch(const std::filesystem::filesystem_error &e){
        return 1;
    }

    std::ofstream spectrums(output+"_input.csv"), labels(output+"_label.csv");
    if(spectrums.bad() || labels.bad()) return 1;

    spectrums << std::setprecision(6) << std::fixed;
    labels << std::setprecision(6) << std::fixed;

    unsigned step = 128;

    auto to_energy = [&](const vector<complex<float>> &in) -> vector<float> {

        vector<float> out;
        for(auto i : in) out.push_back(i.real()*i.real()+i.imag()*i.imag());

        return out;
    };

    auto sinc = [](float x) -> float {
        if(std::abs(x) < 1e-5) return 1.0f;
        return std::sin(x*M_PI) / (x*M_PI);
    };

    auto sinc2 = [&](float x) -> float {
        if(std::abs(x) > 3) return 0.0f;
        float y = sinc(x);
        return y*y;
    };
    
    auto interpolate_and_normalize = [&](vector<float> in, float pitch) -> vector<float> {

        const float p = 60.0f;
        vector<float> out(100, 0.0f);
   
        assert(in.size()*pitch >= 6000.0f);

        // amplitude

        for(float &i : in) i = std::sqrt(std::abs(i));

        for(int i=0, j=0; i<=100; i++){

            while((j+1)*pitch < (i+1)*p) j++;

            float lp = j-1 >= 0 ? in[j-1] : 0.0f;
            float rp = j < (int)in.size() ? in[j] : 0.0f;

            float l = ((i+1)*p - j*pitch)/pitch;
            float r = ((j+1)*pitch - (i+1)*p)/pitch;

            out[i] = lp * r + rp * l;

            assert(std::abs(l+r-1) < 1e-5);
            assert(out[i] >= 0.0f);
        }

        /*
        // energy

        for(int i=0; i<100; i++){
            for(int j=0; j<(int)in.size(); j++){
                out[i] += sinc2(((i+1)*p-j*pitch)/pitch) * in[j];
            }
        }
        */

        float sum = 0.0f;
        for(float i : out) sum += i;

        const float norm = 10.0f;
        sum = norm / sum;

        for(float &i : out) i *= sum;

        return out;
    };

    auto get_label = [](std::string s) -> std::string {
        int n = s.size();
        int i = n-1;
        for(; i>=0; i--){ if(s[i] == '/') break; }
        if(s[i] != '/') return "a";
        return s.substr(i+2, 1);
    };

    for(auto f : files){

        iwstream I;
        if(!I.open(f)) continue;

        change::Detector detector;
        vector<float> samples(step);

        vector<std::pair<vector<float>, float> > all;

        while(I.read_move(samples.data(), step) == step){
            
            detector.feed(samples);

            if(detector.voiced && detector.pitch > 80.0f && detector.pitch < 500.0f){
                
                unsigned num = (unsigned)std::ceil(6000.0f / detector.pitch);

                auto freq = math::cos_window_ft(detector.get2(), num);
                auto e = to_energy(freq);

                float sum = 0.0f;
                for(auto i : e) sum += i;

                if(sum > 1e-3) all.push_back({e, detector.pitch});
            }
        }

        if(all.size() < N) continue;

        std::shuffle(all.begin(), all.end(), rng32);

        auto label = get_label(f);

        for(unsigned i=0; i<N; i++){
            auto out = interpolate_and_normalize(all[i].first, all[i].second);
            for(float i : out) assert(!std::isnan(i) && !std::isinf(i));
            for(int j=0; j<99; j++) spectrums << out[j] << ',';
            spectrums << out[99] << '\n';
            labels << label << '\n';
        }

        std::cerr << f << ' ' << label << '\n';
        labels.flush();
        spectrums.flush();
    }

    return 0;
}

int main(){

    std::string directory = "../dataset", output = "../training1";
    unsigned N = 10;

    // std::cin >> directory >> output >> N;

    parse_to_csv(directory, output, N);

    return 0;
}
