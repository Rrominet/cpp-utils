#define MINIAUDIO_IMPLEMENTATION
#include "./Sounds.h"
#include "./Sound.h"

namespace ml
{
    Sounds::Sounds() : _sounds(), _engine()
    {

    }

    Sounds::~Sounds()
    {
        if (_initialized)
            ma_engine_uninit(&_engine);
    }

    double SoundData::mslength() const
    {
        if (sampleRate == 0)
            return 0.0;

        return static_cast<double>(pcmLength()) * 1000.0 /
            static_cast<double>(sampleRate);
    }

    long long unsigned SoundData::pcmLength() const
    {
        if (channelsData.empty())
            return 0;

        return static_cast<long long unsigned>(channelsData[0].size());
    }

    ml::Ret<> Sounds::_init()
    {
        if (_initialized)
            return ml::ret::success();

        auto res = ma_engine_init(nullptr, &_engine);
        if (res != MA_SUCCESS)
            return ml::ret::fail("Failed to init miniaudio : " + std::string(ma_result_description(res)));

        _initialized = true;
        return ml::ret::success();
    }

    ml::Ret<ml::Sound*> Sounds::play(const std::string& path)
    {
        auto res = _init();
        if (!res.success)
            return ml::ret::fail<ml::Sound*>(res.message);

        auto s = std::make_unique<ml::Sound>(this, path);        
        res = s->init();
        if (!res.success)
            return ml::ret::fail<ml::Sound*>(res.message);
        s->play();
        _sounds.push_back(std::move(s));
        return ml::ret::success<ml::Sound*>(_sounds.back().get());
    }

    ml::Ret<ml::Sound*> Sounds::at(size_t index)
    {
        if (_sounds.size()<=index)        
            return ml::ret::fail<ml::Sound*>("Sounds::at - Index out of range : " + std::to_string(index));
        return ml::ret::success<ml::Sound*>(_sounds[index].get());
    }

    ml::Ret<SoundData> Sounds::data(size_t index)
    {
        if (_sounds.size()<=index)        
            return {};
        return this->data(_sounds[index].get());
    }

    ml::Ret<SoundData> Sounds::data(ml::Sound* sound)
    {
        if (!sound)
            return ml::ret::fail<SoundData>(
                    "Sounds::data - Sound is null"
                    );

        return this->data(sound->path());
    }

    ml::Ret<SoundData> Sounds::data(const std::string& filepath)
    {
        // ---------------------------------------------------------
        // Init decoder with native PCM format.
        // ---------------------------------------------------------

        ma_decoder decoder;

        ma_result res = ma_decoder_init_file(
                filepath.c_str(),
                nullptr,
                &decoder
                );

        if (res != MA_SUCCESS)
        {
            return ml::ret::fail<SoundData>(
                    "Sounds::data - Failed to init decoder : " +
                    std::string(ma_result_description(res))
                    );
        }


        // ---------------------------------------------------------
        // Get native PCM format.
        // ---------------------------------------------------------

        ma_format format = decoder.outputFormat;

        SoundData result;

        switch (format)
        {
            case ma_format_u8:
                result.format = sound::U8;
                break;

            case ma_format_s16:
                result.format = sound::S16;
                break;

            case ma_format_s24:
                result.format = sound::S24;
                break;

            case ma_format_s32:
                result.format = sound::S32;
                break;

            case ma_format_f32:
                result.format = sound::F32;
                break;

            default:
                ma_decoder_uninit(&decoder);

                return ml::ret::fail<SoundData>(
                        "Sounds::data - Unsupported sound format"
                        );
        }


        const size_t bytesPerSample =
            ma_get_bytes_per_sample(format);

        const ma_uint32 channels =
            decoder.outputChannels;

        const ma_uint32 sampleRate =
            decoder.outputSampleRate;


        // ---------------------------------------------------------
        // Get length.
        // ---------------------------------------------------------

        ma_uint64 frameCount = 0;

        res = ma_decoder_get_length_in_pcm_frames(
                &decoder,
                &frameCount
                );

        if (res != MA_SUCCESS)
        {
            ma_decoder_uninit(&decoder);

            return ml::ret::fail<SoundData>(
                    "Sounds::data - Failed to get sound length : " +
                    std::string(ma_result_description(res))
                    );
        }


        // ---------------------------------------------------------
        // Read native PCM data.
        // ---------------------------------------------------------

        std::vector<unsigned char> rawPCM(
                static_cast<size_t>(frameCount) *
                channels *
                bytesPerSample
                );

        ma_uint64 framesRead = 0;

        res = ma_decoder_read_pcm_frames(
                &decoder,
                rawPCM.data(),
                frameCount,
                &framesRead
                );

        ma_decoder_uninit(&decoder);

        if (res != MA_SUCCESS)
        {
            return ml::ret::fail<SoundData>(
                    "Sounds::data - Failed to read sound : " +
                    std::string(ma_result_description(res))
                    );
        }


        // ---------------------------------------------------------
        // Convert native PCM -> F32.
        // ---------------------------------------------------------

        std::vector<float> pcm(
                static_cast<size_t>(framesRead) * channels
                );

        ma_convert_pcm_frames_format(
                pcm.data(),
                ma_format_f32,

                rawPCM.data(),
                format,

                framesRead,
                channels,

                ma_dither_mode_none
                );


        // ---------------------------------------------------------
        // Store information.
        // ---------------------------------------------------------

        result.channels = channels;
        result.sampleRate = sampleRate;

        result.channelsData.resize(channels);

        for (auto& channel : result.channelsData)
            channel.resize(framesRead);


        // ---------------------------------------------------------
        // Deinterleave:
        //
        // L R L R L R
        //
        // becomes:
        //
        // channel[0] = L L L
        // channel[1] = R R R
        // ---------------------------------------------------------

        for (ma_uint64 frame = 0; frame < framesRead; ++frame)
        {
            for (ma_uint32 channel = 0; channel < channels; ++channel)
            {
                result.channelsData[channel][frame] =
                    pcm[frame * channels + channel];
            }
        }


        return ml::ret::success<SoundData>(std::move(result));

    }
}

