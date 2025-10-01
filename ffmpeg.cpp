#include "./ffmpeg.h"
#include "./debug.h"
#include "./str.h"
#include "./thread.h"
#include "./mlprocess.h"
#include "./files.2/files.h"
#include "./NamedMutex.h"

namespace bp = boost::process;

std::string ffmpeg::concatenate(std::vector<std::string> files, const std::string &output, const std::string &ffmpegPath)
{
    std::string ls = ffmpeg::tmpDir() + files::sep() + "ffmpeg_concat_ls";
    while(files::exists(ls))
        ls = ffmpeg::tmpDir() + files::sep() + "ffmpeg_concat_ls_" + str::random(5);

    std::string lsContent;
    for (auto f : files)
        lsContent += "file '" + f + "'\n";
    files::write(ls, lsContent);

    lg2("cmd", ffmpegPath + " -f concat -safe 0 -i \"" + ls + "\" -c copy \"" + output + "\"");
    return ffmpegPath + " -loglevel 16 -f concat -safe 0 -i \"" + ls + "\" -c copy -progress - \"" + output + "\"";
}

std::string ffmpeg::tmpDir()
{
    std::string d(files::tmp() + files::sep() + "ffmpeg");
    if (!files::isDir(d))
        files::mkdir(d);
    return d;
}

std::string ffmpeg::concat(std::vector<std::string> files, const std::string &output, const std::string &ffmpegPath)
{
    return concatenate(files, output, ffmpegPath);
}

std::string ffmpeg::proxy(const std::string& file, std::string output, const std::string &ffmpegPath)
{
    std::string cmd = ffmpegPath + " -loglevel 16 -i \"" + file + "\"";
    cmd += " -c:v mjpeg -y "; 
    cmd += "-threads " + std::to_string(th::maxSystem()) + " ";
    cmd += "-progress - ";
    if (output.empty())
    {
        output = files::parent(file) + files::sep() + files::baseName(file) + "_proxy.avi";
    }
    cmd += "\"" + output + "\"";
    return cmd;
}

std::string ffmpeg::raw(const std::string& file, const std::string& ffmpegPath)
{
    std::string cmd = ffmpegPath + " -i \"" + file + "\"";
    cmd += " -loglevel quiet -f rawvideo -pix_fmt rgb24 pipe:1";

    lg(cmd);
    return cmd;
}

std::string ffmpeg::raw(const std::string& file, const std::string& start, const std::string& duration, const std::string& ffmpegPath)
{
    std::string cmd = ffmpegPath + " -ss " + start + " -t " + duration;
    cmd += " -i \"" + file + "\"";
    cmd += " -loglevel quiet -f rawvideo -pix_fmt rgb24 pipe:1";

    lg(cmd);
    return cmd;
}

std::string ffmpeg::pcm(const std::string& file, const std::string& ffmpegPath)
{
    std::string cmd = ffmpegPath + " -i \"" + file + "\"";
    cmd += " -loglevel quiet -f f32le pipe:1";
    lg(cmd);
    return cmd;
}

std::string ffmpeg::wav(const std::string& file, const std::string output, const std::string& ffmpegPath)
{
    std::string routput = output ;
    if (routput.empty())
        routput = files::parent(file) + files::sep() + files::baseName(file) + ".wav";

    return ffmpegPath + " -i \"" + file + "\" \"" + routput + "\"";
}

std::string ffmpeg::web(const std::string& file, const std::string output, const std::string& ffmpegPath)
{
    std::string routput = output ;
    if (routput.empty())
        routput = file;

    std::string cmd = "";
    cmd += ffmpegPath + " -i \"" + file + "\" -preset slower -crf 25 -movflags faststart -b:a 360k \"" + files::parent(file) + files::sep() + files::baseName(file) + "_ultra.mp4\"\n";
    cmd += ffmpegPath + " -i \"" + file + "\" -preset slower -crf 28 -movflags faststart -b:a 360k \"" + files::parent(file) + files::sep() + files::baseName(file) + "_hd.mp4\"\n";
    cmd += ffmpegPath + " -i \"" + file + "\" -preset slower -crf 30 -movflags faststart -b:a 360k \"" + files::parent(file) + files::sep() + files::baseName(file) + "_med.mp4\"\n";
    cmd += ffmpegPath + " -i \"" + file + "\" -preset slower -crf 45 -movflags faststart -b:a 360k \"" + files::parent(file) + files::sep() + files::baseName(file) + "_low.mp4\"";
    return cmd;
}

std::vector<std::string> ffprobe::data_cmd(const std::string& filename, const std::string& ffprobe)
{
    std::vector<std::string> cmd = {
        ffprobe, "-loglevel", "-8", "-print_format", "json", "-show_format", "-show_streams", filename
    };

    return cmd;
}

// -- FFPROBE --
json ffprobe::data(const std::string& filename, const std::string& ffprobe)
{
    auto cmd = ffprobe::data_cmd(filename, ffprobe);

    std::string res;
    try
    {
        bp::ipstream is;
        bp::child c(cmd, bp::std_out > is);
        std::string line;
        while(c.running() && std::getline(is, line) && !line.empty())
            res += line;
        c.wait();
    }
    catch(const std::exception& e)
    {
        lg(e.what());
        return json::object();
    }

    lg("ffprobe output :");
    lg(res);
    return json::parse(res);
}

std::string ffprobe::format(const json& data)
{
    if (!data.contains("format"))
        return "unknown";
    return data["format"]["format_long_name"];
}
std::string ffprobe::format(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::format(ffprobe::data(filename, ffprobe));
}

double ffprobe::duration(const json& data)
{
    if (!data.contains("format"))
        return -1;
    return stod(data["format"]["duration"].get<std::string>());
}
double ffprobe::duration(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::duration(ffprobe::data(filename, ffprobe));
}

double ffprobe::bitRate(const json& data)
{
    if (!data.contains("format"))
        return -1;
    return stod(data["format"]["bit_rate"].get<std::string>());
}
double ffprobe::bitRate(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::bitRate(ffprobe::data(filename, ffprobe));
}

std::string ffprobe::videoCodec(const json& data)
{
    if (!data.contains("streams"))
        return "unknown";

    for (auto s : data["streams"])
    {
        if (s["codec_type"] == "video")
            return s["codec_name"];
    }

    return "";
}
std::string ffprobe::videoCodec(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::videoCodec(ffprobe::data(filename, ffprobe));
}

std::string ffprobe::audioCodec(const json& data)
{
    if (!data.contains("streams"))
        return "unknown";
    for (auto s : data["streams"])
    {
        if (s["codec_type"] == "audio")
            return s["codec_name"];
    }

    return "";
}
std::string ffprobe::audioCodec(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::audioCodec(ffprobe::data(filename, ffprobe));
}

json ffprobe::videoStream(const json& data)
{
    if (!data.contains("streams"))
    {
        lg("data does not contains stream...");
        json _r;
        return _r;
    }
    for (auto s : data["streams"])
    {
        if (s["codec_type"] == "video")
            return s;
    }

    json _r;
    return _r;
}
json ffprobe::videoStream(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::videoStream(ffprobe::data(filename, ffprobe));
}

json ffprobe::audioStream(const json& data)
{
    if (!data.contains("streams"))
    {
        lg("data does not contains stream...");
        json _r;
        return _r;
    }
    for (auto s : data["streams"])
    {
        if (s["codec_type"] == "audio")
            return s;
    }

    json _r;
    return _r;
}
json ffprobe::audioStream(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::audioStream(ffprobe::data(filename, ffprobe));
}

int ffprobe::width(const json& data)
{
    json v = ffprobe::videoStream(data);
    if (!v.contains("width"))
        return -1;
    return v["width"];
}
int ffprobe::width(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::width(ffprobe::data(filename, ffprobe));
}

int ffprobe::height(const json& data)
{
    json v = ffprobe::videoStream(data);
    if (!v.contains("height"))
        return -1;
    return v["height"];
}
int ffprobe::height(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::height(ffprobe::data(filename, ffprobe));
}

double ffprobe::fps(const json& data)
{
    json v = ffprobe::videoStream(data);
    if (!v.contains("avg_frame_rate"))
        return -1;
    std::string dt = v["avg_frame_rate"].get<std::string>();
    auto dtmp = str::split(dt, "/");
    double n = stod(dtmp[0]);
    double denum = stod(dtmp[1]);

    double res = n/denum;
    lg(n << "/" << denum << " = " << res);

    return res;
}
double ffprobe::fps(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::fps(ffprobe::data(filename, ffprobe));
}

double ffprobe::mTimer(const json& data)
{
    return 1.0/ffprobe::fps(data) * 1000.0;
}
double ffprobe::mTimer(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::mTimer(ffprobe::data(filename, ffprobe));
}

long ffprobe::microTimer(const json& data)
{
    return lround(ffprobe::mTimer(data)) * 1000;
}
long ffprobe::microTimer(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::microTimer(ffprobe::data(filename, ffprobe));
}

int ffprobe::framesNumber(const json& data)
{
    json v = ffprobe::videoStream(data);
    if (!v.contains("nb_frames"))
    {
        auto fps = ffprobe::fps(data);
        auto duration = ffprobe::duration(data);
        return (int)(fps * duration);
    }
    return stoi(v["nb_frames"].get<std::string>());
}
int ffprobe::framesNumber(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::framesNumber(ffprobe::data(filename, ffprobe));
}

int ffprobe::audioSampleRate(const json& data)
{
    json a = ffprobe::audioStream(data);
    if (!a.contains("sample_rate"))
        return -1;
    return stoi(a["sample_rate"].get<std::string>());
}
int ffprobe::audioSampleRate(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::audioSampleRate(ffprobe::data(filename, ffprobe));
}

int ffprobe::audioMaxBitRate(const json& data)
{
    json a = ffprobe::audioStream(data);
    if (!a.contains("max_bit_rate"))
        return -1;
    return stoi(a["max_bit_rate"].get<std::string>());
}
int ffprobe::audioMaxBitRate(const std::string& filename, const std::string& ffprobe)
{
    return ffprobe::audioMaxBitRate(ffprobe::data(filename, ffprobe));
}

json parse_encoder_line(std::string line)
{
    if (line.empty())
        return NULL;

    json r = json::object();
    while (line[0] == ' ')
        line = line.substr(1);

    const char ctype = line[0];
    std::string type = std::string(1, ctype);
    r["type"] = type;
    auto tmp = str::split(line, " ");
    if (tmp.size() < 2)
        return NULL;

    r["code"] = tmp[1];

    tmp = str::split(line, "  ");
    if (tmp.size() < 2)
        return r;

    r["name"] = vc::last(tmp);
    return r;
}

json ffmpeg::encoders(const std::string& ffmpegPath)
{
    std::vector<std::string> cmd = {ffmpegPath, "-encoders"};
    auto out = process::exec(cmd);

    auto lines = str::split(out, "\n");
    json _r = json::array();
    bool started = false;
    for (auto& l : lines)
    {
        if (str::contains(l, "------"))
        {
            started = true;
            continue;
        }

        if (!started)
            continue;

        auto r = parse_encoder_line(l);
        if (r.is_null())
            continue;
        _r.push_back(r);
    }

    return _r;
}

void ffmpeg::encoders_async(const std::function<void(const json&)> cb, const std::string& ffmpegPath)
{
    auto f = [cb, ffmpegPath]
    {
        cb(ffmpeg::encoders(ffmpegPath));
    };
    std::thread(f).detach();
}


namespace ffmpeg
{
    static const unsigned char SOI[] = { 0xFF, 0xD8 };
    static const unsigned char EOI[] = { 0xFF, 0xD9 };

    struct FirstFramePc
    {
        Process p;
        std::vector<unsigned char> buf;
        std::vector<unsigned char> jpg;
    };

    ml::NamedMutex firstFramePcs_mtx("First Frame Processes Mutex");

    std::vector<std::unique_ptr<FirstFramePc>> firstFramePcs;
    void removeFirstFramePc(FirstFramePc* fp)
    {
        for (auto it = firstFramePcs.begin(); it != firstFramePcs.end(); it++)
        {
            if (it->get() == fp)
            {
                firstFramePcs.erase(it);
                return;
            }
        }
    }

    void removeFirstFramePcLater(FirstFramePc* fp)
    {
        auto del = [fp]
        {
            th::msleep(1);
            std::lock_guard lock(firstFramePcs_mtx);
            removeFirstFramePc(fp);
        };

        std::thread(del).detach();
    }

    void firstFrame(const std::string& filepath, const std::function<void(const std::vector<unsigned char>&)> cb, const std::string& ffmpegPath)
    {
        auto fp = std::make_unique<FirstFramePc>();
        FirstFramePc* fp_ptr = nullptr;
        {
            std::lock_guard lock(firstFramePcs_mtx);
            firstFramePcs.push_back(std::move(fp));
            fp_ptr = firstFramePcs.back().get();
        }

        std::vector<std::string> cmd = {ffmpegPath,
            "-nostdin",
            "-ss", "00:00:00",
            "-i", filepath, 
            "-vframes", "2", //changed
            "-f", "image2pipe",  
            "-vcodec", "mjpeg",
            "-qscale:v", "5",
            "-vsync", "0", "-an", "-",
        };
        fp_ptr->p.setCmd(cmd);
        fp_ptr->p.treatOutputAsBinary();

        auto onout = [fp_ptr, cb](const std::vector<unsigned char>& data)
        {
            lg("reading ffmpeg output data : " << data.size());
            fp_ptr->buf.insert(fp_ptr->buf.end(), data.begin(), data.end());

            // Search for complete JPEGs in buffer
            size_t start = 0;
            while (true) 
            {
                // Find SOI
                auto soi = std::search(fp_ptr->buf.begin() + start, fp_ptr->buf.end(),
                        std::begin(SOI), std::end(SOI));

                if (soi == fp_ptr->buf.end())
                {
                    lg("no SOI found. break;");
                    break;
                }

                // Find EOI after SOI
                auto eoi = std::search(soi + 2, fp_ptr->buf.end(),
                        std::begin(EOI), std::end(EOI));

                if (eoi == fp_ptr->buf.end()) 
                {
                    lg("no EOI found. break;");
                    break;
                }

                eoi += 2; // include the EOI marker
                lg("found jpeg : " << (eoi - soi) << " bytes");
                fp_ptr->jpg = std::vector<unsigned char>(soi, eoi);
                lg("executing callback...");
                cb(fp_ptr->jpg);
                break;
            }
        };

        fp_ptr->p.addOnOutputBin(onout);
        fp_ptr->p.addOnEnd([fp_ptr] {removeFirstFramePcLater(fp_ptr); });
        fp_ptr->p.addOnProcessError([fp_ptr] {removeFirstFramePcLater(fp_ptr); });

        lg("pcs size : " << firstFramePcs.size());
        fp_ptr->p.start();
    }
}


