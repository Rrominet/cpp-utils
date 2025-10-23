#pragma once 

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

//all this functions will just return cmds but it will not actually process it
//ffmpeg must include " " if needed
namespace ffmpeg 
{
    std::string concatenate(std::vector<std::string> files, const std::string& output, const std::string &ffmpegPath = "ffmpeg");
    std::string concat(std::vector<std::string> files, const std::string& output, const std::string &ffmpegPath = "ffmpeg");
    std::string proxy(const std::string& file, std::string output="", const std::string &ffmpegPath = "ffmpeg");
    std::string raw(const std::string& file, const std::string& ffmpegPath = "ffmpeg");
    std::string raw(const std::string& file, const std::string& start, const std::string& duration, const std::string& ffmpegPath = "ffmpeg");

    //  return cmd for getting audio as raw pcm 32bit floting point (big-endian)
    std::string pcm(const std::string& file, const std::string& ffmpegPath = "ffmpeg");

    // cmd to convert a file as wav file
    // if output is empty, it will take the basename of the source with the extension .wav
    std::string wav(const std::string& file, const std::string output="", const std::string& ffmpegPath = "ffmpeg");

    std::string tmpDir();
    std::string web(const std::string& file, const std::string output="", const std::string& ffmpegPath = "ffmpeg");

    json encoders(const std::string& ffmpegPath = "ffmpeg");

    //careful here, cb is executed on another thread, if you use a GUI, queue your fuctions in your main thread, DO NOT EXECUTE GUI FUNCTION DIRECTLY IN THE cb. (with mlgui.2 you can use : App::queue(your_gui_func) to queue your function on the mainthread)
    void encoders_async(const std::function<void(const json&)> cb, const std::string& ffmpegPath = "ffmpeg");

    //careful here, cb is executed on another thread, if you use a GUI, queue your fuctions in your main thread, DO NOT EXECUTE GUI FUNCTION DIRECTLY IN THE cb. (with mlgui.2 you can use : App::queue(your_gui_func) to queue your function on the mainthread)
    //the frame is in the JPG format : TODO add other images format
    void firstFrame(const std::string& filepath, const std::function<void(const std::vector<unsigned char>&)> cb, const std::string& ffmpegPath="ffmpeg");
}

namespace ffprobe
{
    std::vector<std::string> data_cmd(const std::string& filename, std::string ffprobe="");
    json data(const std::string& filename, const std::string& ffprobe="");

    std::string format(const json& data);
    std::string format(const std::string& filename, const std::string& ffprobe="");

    double duration(const json& data);
    double duration(const std::string& filename, const std::string& ffprobe="");

    double bitRate(const json& data);
    double bitRate(const std::string& filename, const std::string& ffprobe="");

    std::string videoCodec(const json& data);
    std::string videoCodec(const std::string& filename, const std::string& ffprobe="");

    std::string audioCodec(const json& data);
    std::string audioCodec(const std::string& filename, const std::string& ffprobe="");

    json videoStream(const json& data);
    json videoStream(const std::string& filename, const std::string& ffprobe="");

    json audioStream(const json& data);
    json audioStream(const std::string& filename, const std::string& ffprobe="");

    int width(const json& data);
    int width(const std::string& filename, const std::string& ffprobe="");

    int height(const json& data);
    int height(const std::string& filename, const std::string& ffprobe="");

    double fps(const json& data);
    double fps(const std::string& filename, const std::string& ffprobe="");

    //return the number of miliseconds you have to wait to next frame (from fps)
    //example 24.00 fps is 41.666, 60fps is 16.666
    //
    double mTimer(const json& data);
    double mTimer(const std::string& filename, const std::string& ffprobe="");

    long microTimer(const json& data);
    long microTimer(const std::string& filename, const std::string& ffprobe="");

    int framesNumber(const json& data);
    int framesNumber(const std::string& filename, const std::string& ffprobe="");

    int audioSampleRate(const json& data);
    int audioSampleRate(const std::string& filename, const std::string& ffprobe="");

    int audioMaxBitRate(const json& data);
    int audioMaxBitRate(const std::string& filename, const std::string& ffprobe="");
}
