#define NOMINMAX
#include "mp_sdk_audio.h"
#include <algorithm>
  
using namespace gmpi;

class CellulophoneKeyboardDsp : public MpBase2
{
    MidiOutPin pinMidiOut;
    IntInPin pinChannel;
    IntInPin pinNote;
    IntInPin pinVelocity;
    FloatInPin pinPitchBend;
    FloatInPin pinModWheel;
 
    int lastNote = -1;

public:
    CellulophoneKeyboardDsp()
    {
        initializePin(pinMidiOut);
        initializePin(pinChannel);
        initializePin(pinNote);
        initializePin(pinVelocity);
        initializePin(pinPitchBend);
        initializePin(pinModWheel);
    } 
    void onSetPins() override
    {
        unsigned char ch = static_cast<unsigned char>(pinChannel.getValue() & 0x0F);

        if (pinNote.isUpdated() || pinVelocity.isUpdated())
        {
            int note = pinNote.getValue();
            int vel = pinVelocity.getValue();

            if (lastNote != -1 && (note != lastNote || vel == 0))
            {
                const unsigned char offMsg[] = {
                    static_cast<unsigned char>(0x80 | ch),
                    static_cast<unsigned char>(lastNote & 0x7F),
                    0
                };
                pinMidiOut.send(offMsg, 3);
                lastNote = -1;
            }
  
            if (note >= 0 && note <= 127 && vel > 0)
            {
                const unsigned char onMsg[] = {
                    static_cast<unsigned char>(0x90 | ch),
                    static_cast<unsigned char>(note & 0x7F),
                    static_cast<unsigned char>(std::clamp(vel, 1, 127))
                };
                pinMidiOut.send(onMsg, 3);
                lastNote = note;
            }
        }

        if (pinPitchBend.isUpdated())
        {
            int bend = static_cast<int>(std::clamp(pinPitchBend.getValue(), 0.0f, 1.0f) * 16383.0f);
            const unsigned char pbMsg[] = {
                static_cast<unsigned char>(0xE0 | ch),
                static_cast<unsigned char>(bend & 0x7F),
                static_cast<unsigned char>((bend >> 7) & 0x7F)
            };
            pinMidiOut.send(pbMsg, 3);
        }

        if (pinModWheel.isUpdated())
        {
            int cc = static_cast<int>(std::clamp(pinModWheel.getValue(), 0.0f, 1.0f) * 127.0f);
            const unsigned char modMsg[] = {
                static_cast<unsigned char>(0xB0 | ch),
                1,
                static_cast<unsigned char>(cc & 0x7F)
            };
            pinMidiOut.send(modMsg, 3);
        }
    }
};

REGISTER_PLUGIN(CellulophoneKeyboardDsp, L"Cellulophone_Vintage_Keyboard");



