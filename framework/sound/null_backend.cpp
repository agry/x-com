#include "framework/sound_interface.h"
#include "framework/logger.h"

namespace {

using namespace OpenApoc;

class NullSoundBackend : public SoundBackend
{
	AudioFormat preferredFormat;
public:
	NullSoundBackend()
	{
		preferredFormat.channels = 2;
		preferredFormat.format = AudioFormat::SampleFormat::PCM_SINT16;
		preferredFormat.frequency = 22050;
	}
	virtual void playSample(std::shared_ptr<Sample> sample)
	{
		std::ignore = sample;
		LogWarning("Called on NULL backend");
	}

	virtual void playMusic(std::shared_ptr<MusicTrack> track, std::function<void(void*)> finishedCallback, void *callbackData)
	{
		std::ignore = track;
		std::ignore = finishedCallback;
		std::ignore = callbackData;
		LogWarning("Called on NULL backend");
	}

	virtual void stopMusic()
	{
		LogWarning("Called on NULL backend");
	}

	virtual ~NullSoundBackend()
	{
		this->stopMusic();
	}

	virtual const AudioFormat& getPreferredFormat()
	{
		return preferredFormat;
	}
};

class NullSoundBackendFactory : public SoundBackendFactory
{
public:
	virtual SoundBackend *create()
	{
		LogWarning("Creating NULL sound backend (Sound disabled)");
		return new NullSoundBackend();
	};

	virtual ~NullSoundBackendFactory()
	{
	}
};

SoundBackendRegister<NullSoundBackendFactory> load_at_init_null_sound("null");

}; //anonymous namespace
